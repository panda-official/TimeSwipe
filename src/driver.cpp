// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH / Dmitry Igrishin

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "bcmlib.hpp"
#include "bcmspi.hpp"
#include "driver.hpp"
#include "error_detail.hpp"
#include "gain.hpp"
#include "hat.hpp"
#include "pidfile.hpp"
#include "resampler.hpp"

#include "3rdparty/dmitigr/filesystem.hpp"
#include "3rdparty/dmitigr/math.hpp"
#include "3rdparty/dmitigr/rajson.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace rajson = dmitigr::rajson;
namespace ts = panda::timeswipe;

namespace panda::timeswipe {
namespace detail {
class iDriver final : public Driver {
public:
  ~iDriver()
  {
    stop_measurement();
    join_threads();
  }

  iDriver()
    : pid_file_{"timeswipe"}
    , spi_{detail::Bcm_spi::Pins::spi0}
    , calibration_slopes_(max_channel_count())
    , translation_offsets_(max_channel_count())
    , translation_slopes_(max_channel_count())
  {
    // Initialize slopes and offsets.
    fill(begin(calibration_slopes_), end(calibration_slopes_), 1);
    fill(begin(translation_offsets_), end(translation_offsets_), 0);
    fill(begin(translation_slopes_), end(translation_slopes_), 1);

    // Lock PID file.
    std::string msg;
    if (!pid_file_.lock(msg))
      // Lock here. Second lock from the same process is allowed.
      throw Generic_exception{Generic_errc::driver_pid_file_lock_failed,
        std::string{"cannot lock by using file "}.append(pid_file_.name())};

    // Initialize GPIO.
    init_gpio();

    // Get the calibration data.
    calibration_map_ = [this]
    {
      using Ct = hat::atom::Calibration::Type;
      hat::Calibration_map result;
      for (const auto ct :
             {Ct::v_in1, Ct::v_in2, Ct::v_in3, Ct::v_in4,
              Ct::c_in1, Ct::c_in2, Ct::c_in3, Ct::c_in4}) {
        const auto settings_request = R"({"cAtom":)" +
          std::to_string(static_cast<int>(ct)) + R"(})";
        const auto json_obj = spi_.execute_get_many(settings_request);
        const auto doc = rajson::to_document(json_obj);
        const rajson::Value_view doc_view{doc};
        const auto doc_cal_entries = doc_view.mandatory("data");
        if (const auto& v = doc_cal_entries.value(); v.IsArray() && !v.Empty()) {
          auto& atom = result.atom(ct);
          const auto cal_entries = v.GetArray();
          const auto cal_entries_size = cal_entries.Size();
          for (std::size_t i{}; i < cal_entries_size; ++i) {
            const rajson::Value_view cal_entry{cal_entries[i]};
            if (!cal_entry.value().IsObject())
              throw Generic_exception{Generic_errc::calib_data_invalid,
                "cannot initialize Timeswipe driver by using invalid calibration atom entry"};
            const auto slope = cal_entry.mandatory<float>("m");
            const auto offset = cal_entry.mandatory<std::int16_t>("b");
            const hat::atom::Calibration::Entry entry{slope, offset};
            atom.set_entry(i, entry);
          }
        } else
          throw Generic_exception{Generic_errc::calib_data_invalid,
            "cannot initialize Timeswipe driver by using invalid calibration data"};
      }
      return result;
    }();
  }

  /**
   * Initializes GPIO pins.
   *
   * @param force Forces initialization even if `is_gpio_inited_ == true`.
   *
   * @par Effects
   * Restarts firmware on very first run!
   *
   * @warning Firmware developers must remember, that restarting the firmware
   * causes reset of all the settings the firmware keeps in the on-board RAM!
   */
  void init_gpio(const bool force = false)
  {
    if (!force && is_gpio_inited_) return;

    detail::setup_io();
    init_gpio_input(gpio_data0);
    init_gpio_input(gpio_data1);
    init_gpio_input(gpio_data2);
    init_gpio_input(gpio_data3);
    init_gpio_input(gpio_data4);
    init_gpio_input(gpio_data5);
    init_gpio_input(gpio_data6);
    init_gpio_input(gpio_data7);

    init_gpio_input(gpio_tco);
    init_gpio_input(gpio_pi_ok);
    init_gpio_input(gpio_fail);
    init_gpio_input(gpio_button);

    // init_gpio_output(gpio_pi_ok);
    init_gpio_output(gpio_clock);
    init_gpio_output(gpio_reset);

    // Initial Reset
    set_gpio_low(gpio_clock);
    set_gpio_high(gpio_reset);

    using std::chrono::milliseconds;
    std::this_thread::sleep_for(milliseconds{1});

    is_gpio_inited_ = true;
  }

  int version() const override
  {
    return ts::version;
  }

  int min_sample_rate() const override
  {
    return 32;
  }

  int max_sample_rate() const override
  {
    return 48000;
  }

  int max_channel_count() const override
  {
    return ts::max_channel_count;
  }

  int max_pwm_count() const override
  {
    return 2;
  }

  void set_board_settings(const Board_settings& settings) override
  {
    // Some settings cannot be applied if the measurement started.
    if (is_measurement_started()) {
      // Check if signal mode setting presents.
      if (settings.signal_mode())
        throw Generic_exception{Generic_errc::board_settings_invalid,
          "cannot set board signal mode when measurement started"};

      // Check if channel measurement modes setting presents.
      if (settings.channel_measurement_modes())
        throw Generic_exception{Generic_errc::board_settings_invalid,
          "cannot set board measurement modes when measurement started"};
    }

    Board_settings new_settings{board_settings_}; // may throw
    new_settings.set(settings); // may throw
    spi_.execute_set_many(settings.to_json_text()); // may throw
    board_settings_.swap(new_settings); // noexcept
  }

  const Board_settings& board_settings() const override
  {
    return board_settings_;
  }

  /// Method is hidden for now.
  Board_settings raw_board_settings() const
  {
    return Board_settings{spi_.execute_get_many("")};
  }

  void set_settings(const Settings& settings) override
  {
    set_settings(settings, {});
  }

  /// @overload
  void set_settings(const Settings& settings,
    std::unique_ptr<detail::Resampler> resampler)
  {
    const auto srate = settings.sample_rate();
    if (srate)
      set_resampler(*srate, std::move(resampler)); // strong guarantee

    const auto bbs = settings.burst_buffer_size();
    const auto freq = settings.frequency();
    PANDA_TIMESWIPE_ASSERT(!(bbs && freq));
    if (bbs)
      burst_buffer_size_ = *bbs;
    if (srate && freq)
      burst_buffer_size_ = *srate / *freq;

    if (const auto values = settings.translation_offsets())
      translation_offsets_ = std::move(*values);
    if (const auto values = settings.translation_slopes())
      translation_slopes_ = std::move(*values);

    settings_.set(settings); // may throw
  }

  const Settings& settings() const override
  {
    return settings_;
  }

  void start_measurement(Data_handler data_handler) override
  {
    const auto gains = board_settings().channel_gains();
    const auto modes = board_settings().channel_measurement_modes();
    if (!data_handler)
      throw Generic_exception{"cannot start measurement by using invalid data handler"};
    else if (is_measurement_started())
      throw Generic_exception{Generic_errc::board_measurement_started,
        "cannot start measurement because it's already started"};
    else if (!gains)
      throw Generic_exception{Generic_errc::board_settings_insufficient,
        "cannot start measurement with unspecified channel gains"};
    else if (!modes)
      throw Generic_exception{Generic_errc::board_settings_insufficient,
        "cannot start measurement with unspecified channel measurement modes"};
    else if (!settings().sample_rate())
      throw Generic_exception{Generic_errc::driver_settings_insufficient,
        "cannot start measurement with unspecified sample rate"};

    join_threads(); // may throw

    // Pick up the calibration slopes depending on both the gain and measurement mode.
    const int mcc = max_channel_count();
    PANDA_TIMESWIPE_ASSERT(gains && modes &&
      (gains->size() == modes->size()) &&
      (gains->size() >= mcc));
    decltype(calibration_slopes_) new_calibration_slopes{calibration_slopes_}; // may throw
    for (int i{}; i < mcc; ++i) {
      const auto gain = gains->at(i);
      const auto mode = modes->at(i);
      using Ct = hat::atom::Calibration::Type;
      using Array = std::array<Ct, ts::max_channel_count>;
      constexpr Array v_types{Ct::v_in1, Ct::v_in2, Ct::v_in3, Ct::v_in4};
      constexpr Array c_types{Ct::c_in1, Ct::c_in2, Ct::c_in3, Ct::c_in4};
      const auto& types = (mode == Measurement_mode::Voltage) ? v_types : c_types;
      const auto& atom = calibration_map_.atom(types[i]);
      const auto ogain_index = gain::ogain_table_index(gain);
      PANDA_TIMESWIPE_ASSERT(ogain_index < atom.entry_count());
      new_calibration_slopes[i] = atom.entry(ogain_index).slope();
    }
    calibration_slopes_.swap(new_calibration_slopes); // noexcept

    /*
     * Send the command to the firmware to start the measurement.
     * Effects: the reader does receive the data from the board.
     */
    {
      PANDA_TIMESWIPE_ASSERT(is_gpio_inited_);
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
      spi_set_enable_ad_mes(true);
    }

    try {
      is_measurement_started_ = true;
      threads_.emplace_back(&iDriver::fetcher_loop, this);
      threads_.emplace_back(&iDriver::poller_loop, this, std::move(data_handler));
    } catch (...) {
      calibration_slopes_.swap(new_calibration_slopes); // noexcept
      throw;
    }
  }

  bool is_measurement_started() const noexcept override
  {
    return is_measurement_started_;
  }

  void stop_measurement() override
  {
    if (!is_measurement_started_) return;

    // Stop threads and reset state they using.
    is_measurement_started_ = false;
    join_threads();
    while (record_queue_.pop());
    read_skip_count_ = initial_invalid_datasets_count;

    // Sends the command to the firmware to stop the measurement.
    {
      // Reset Clock
      set_gpio_low(gpio_clock);

      // Stop Measurement
      spi_set_enable_ad_mes(false); // may throw
    }
  }

  std::vector<float> calculate_drift_references() override
  {
    // Collect the data for calculation.
    auto data = collect_channels_data(drift_samples_count, // 5 ms
      [this]{return Drift_affected_state_guard{*this};}); // strong guarantee

    // Discard the first half.
    data.erase_front(drift_samples_count / 2);

    // Take averages of measured data (references).
    std::vector<float> result(data.channel_count());
    transform(data.cbegin(), data.cend(), result.begin(), [](const auto& vec)
    {
      return static_cast<float>(dmitigr::math::avg(vec));
    });

    // Put references to the tmp_dir/drift_references.
    const auto tmp = tmp_dir();
    std::filesystem::create_directories(tmp);
    constexpr auto open_flags{std::ios_base::out | std::ios_base::trunc};
    std::ofstream refs_file{tmp/"drift_references", open_flags};
    for (std::size_t i{}; i < result.size() - 1; ++i)
      refs_file << result[i] << " ";
    refs_file << result.back() << "\n";

    // Cache references.
    drift_references_ = result;

    return result;
  }

  void clear_drift_references() override
  {
    if (is_measurement_started())
      throw Generic_exception{Generic_errc::board_measurement_started,
        "cannot clear drift compensation references when measurement is started"};

    std::filesystem::remove(tmp_dir()/"drift_references");
    drift_references_.reset();
    drift_deltas_.reset();
  }

  std::vector<float> calculate_drift_deltas() override
  {
    // Throw away if there are no references.
    const auto refs = drift_references();
    if (!refs)
      throw Generic_exception{Generic_errc::drift_comp_refs_not_found,
        "cannot calculate drift compensation deltas because no references found"};

    // Collect the data for calculation.
    auto data = collect_channels_data(drift_samples_count, [this] {
      return Drift_affected_state_guard{*this};
    }); // strong guarantee
    PANDA_TIMESWIPE_ASSERT(refs->size() == data.channel_count());

    // Discard the first half.
    data.erase_front(drift_samples_count / 2);

    // Take averages of measured data (references) and subtract the references.
    std::vector<float> result(data.channel_count());
    transform(data.cbegin(), data.cend(), refs->cbegin(), result.begin(),
      [](const auto& vec, const auto ref)
      {
        return static_cast<float>(dmitigr::math::avg(vec) - ref);
      });

    // Cache deltas.
    drift_deltas_ = result;

    return result;
  }

  void clear_drift_deltas() override
  {
    if (is_measurement_started())
      throw Generic_exception{Generic_errc::board_measurement_started,
        "cannot clear drift compensation deltas when measurement is started"};

    drift_deltas_.reset();
  }

  std::optional<std::vector<float>> drift_references(const bool force = {}) const override
  {
    if (!force && drift_references_)
      return drift_references_;

    const auto drift_references = tmp_dir() / "drift_references";
    if (!std::filesystem::exists(drift_references))
      return std::nullopt;

    std::ifstream in{drift_references};
    if (!in)
      throw Generic_exception{Generic_errc::drift_comp_refs_not_available,
        std::string{"drift compensation references are not available from "}
          .append(drift_references.string())};

    std::vector<float> refs;
    while (in && refs.size() < max_channel_count()) {
      float val;
      if (in >> val)
        refs.push_back(val);
    }
    if (!in.eof()) {
      float val;
      if (in >> val)
        throw Generic_exception{Generic_errc::drift_comp_refs_invalid,
          std::string{"too many floating point numbers found in "}
            .append(drift_references.string())};
    }
    if (refs.size() < max_channel_count())
      throw Generic_exception{Generic_errc::drift_comp_refs_invalid,
        std::string{"too few floating point numbers found in "}
          .append(drift_references.string())};

    PANDA_TIMESWIPE_ASSERT(refs.size() == max_channel_count());

    // Cache and return references.
    return drift_references_ = refs;
  }

  std::optional<std::vector<float>> drift_deltas() const override
  {
    return drift_deltas_;
  }

private:
  // ---------------------------------------------------------------------------
  // Constants
  // ---------------------------------------------------------------------------

  // "Switching oscillation" completely (according to PSpice) decays after 1.5ms.
  static constexpr std::chrono::microseconds switching_oscillation_period{1500};

  // Only 5ms of raw data is needed. (5ms * 48kHz = 240 values.)
  static constexpr std::size_t drift_samples_count{5*48000/1000};
  static_assert(!(drift_samples_count % 2));

  // ---------------------------------------------------------------------------
  // Basic data
  // ---------------------------------------------------------------------------

  detail::Pid_file pid_file_;
  mutable detail::Bcm_spi spi_;
  std::atomic_bool is_gpio_inited_{};
  std::atomic_bool is_measurement_started_{};

  // ---------------------------------------------------------------------------
  // Measurement data
  // ---------------------------------------------------------------------------

  // The number of initial invalid data sets.
  static constexpr int initial_invalid_datasets_count{32};
  static constexpr std::uint16_t channel_offset{32768};
  int read_skip_count_{initial_invalid_datasets_count};
  std::vector<float> calibration_slopes_;
  std::vector<int> translation_offsets_;
  std::vector<float> translation_slopes_;
  hat::Calibration_map calibration_map_;
  Board_settings board_settings_;
  Settings settings_;
  std::unique_ptr<detail::Resampler> resampler_;

  // Record queue capacity must be enough to store records for 1s.
  boost::lockfree::spsc_queue<Data_vector, boost::lockfree::capacity<48000/32*2>> record_queue_;
  std::atomic_int record_error_count_{};
  std::size_t burst_buffer_size_{};
  Data_vector burst_buffer_;
  std::vector<std::thread> threads_;

  // ---------------------------------------------------------------------------
  // Drift compensation data
  // ---------------------------------------------------------------------------

  mutable std::optional<std::vector<float>> drift_references_;
  std::optional<std::vector<float>> drift_deltas_;

  // ---------------------------------------------------------------------------
  // RAII protectors
  // ---------------------------------------------------------------------------

  /*
   * An automatic restorer of state affected by drift calculation stuff. Stashed
   * state will be restored upon destruction of the instance of this class.
   */
  class Drift_affected_state_guard final {
    friend iDriver;

    Drift_affected_state_guard(const Drift_affected_state_guard&) = delete;
    Drift_affected_state_guard& operator=(const Drift_affected_state_guard&) = delete;
    Drift_affected_state_guard(Drift_affected_state_guard&&) = delete;
    Drift_affected_state_guard& operator=(Drift_affected_state_guard&&) = delete;

    // Restores the driver state.
    void restore() noexcept
    {
      try {
        // Restore driver settings.
        driver_.set_settings(std::move(settings_),
          std::move(resampler_));

        // Restore board settings (input modes).
        driver_.set_board_settings(Board_settings{}
          .set_channel_measurement_modes(chmm_));
      } catch (...) {}
    }

    ~Drift_affected_state_guard()
    {
      restore();
    }

    /**
     * Stores the rep state and driver settings, and prepares the board
     * for measurement.
     */
    Drift_affected_state_guard(iDriver& driver) try
      : driver_{driver}
      , resampler_{std::move(driver_.resampler_)} // store
      , settings_{std::move(driver_.settings_)} // settings
      , chmm_(driver.max_channel_count())
    {
      // Store board settings (input modes).
      if (const auto modes = driver_.board_settings().channel_measurement_modes())
        chmm_ = std::move(*modes);
      else
        throw Generic_exception{Generic_errc::board_settings_invalid,
          "channel measurement modes are not available"};

      /*
       * Change input modes to `current`.
       * This will cause a "switching oscillation" appears at the output of
       * the measured value, which completely (according to PSpice) decays
       * after 1.5 ms.
       */
      {
        auto chmm = chmm_;
        for (auto& mm : chmm) mm = Measurement_mode::Current;
        driver_.set_board_settings(Board_settings{}
          .set_channel_measurement_modes(chmm));
      }

      std::this_thread::sleep_for(driver_.switching_oscillation_period);

      // Set specific driver settings.
      driver_.set_settings(Settings{}.set_sample_rate(48000)
        .set_burst_buffer_size(driver_.drift_samples_count));
    } catch (...) {
      restore();
    }

    iDriver& driver_;
    decltype(driver_.resampler_) resampler_;
    decltype(driver_.settings_) settings_;
    std::vector<Measurement_mode> chmm_;
  };

  // ---------------------------------------------------------------------------
  // GPIO stuff
  // ---------------------------------------------------------------------------

  // PIN NAMES
  static constexpr std::uint8_t gpio_data0{24};  // BCM 24 - PIN 18
  static constexpr std::uint8_t gpio_data1{25};  // BCM 25 - PIN 22
  static constexpr std::uint8_t gpio_data2{7};   // BCM  7 - PIN 26
  static constexpr std::uint8_t gpio_data3{5};   // BCM  5 - PIN 29
  static constexpr std::uint8_t gpio_data4{6};   // BCM  6 - PIN 31
  static constexpr std::uint8_t gpio_data5{12};  // BCM 12 - PIN 32
  static constexpr std::uint8_t gpio_data6{13};  // BCM 13 - PIN 33
  static constexpr std::uint8_t gpio_data7{16};  // BCM 16 - PIN 36
  static constexpr std::uint8_t gpio_clock{4};   // BCM  4 - PIN  7
  static constexpr std::uint8_t gpio_tco{14};    // BCM 14 - PIN  8
  static constexpr std::uint8_t gpio_pi_ok{15};  // BCM 15 - PIN 10
  static constexpr std::uint8_t gpio_fail{18};   // BCM 18 - PIN 12
  static constexpr std::uint8_t gpio_reset{17};  // BCM 17 - PIN 11
  static constexpr std::uint8_t gpio_button{25}; // BCM 25 - PIN 22

  static constexpr std::array<std::uint32_t, 8> gpio_data_position{
    std::uint32_t{1} << gpio_data0,
    std::uint32_t{1} << gpio_data1,
    std::uint32_t{1} << gpio_data2,
    std::uint32_t{1} << gpio_data3,
    std::uint32_t{1} << gpio_data4,
    std::uint32_t{1} << gpio_data5,
    std::uint32_t{1} << gpio_data6,
    std::uint32_t{1} << gpio_data7
  };

  static constexpr std::uint32_t gpio_clock_position{std::uint32_t{1} << gpio_clock};
  static constexpr std::uint32_t gpio_tco_position{std::uint32_t{1} << gpio_tco};
  static constexpr std::uint32_t gpio_pi_status_position{std::uint32_t{1} << gpio_pi_ok};
  static constexpr std::uint32_t gpio_fail_position{std::uint32_t{1} << gpio_fail};
  static constexpr std::uint32_t gpio_button_position{std::uint32_t{1} << gpio_button};

  // (2^32)-1 - ALL BCM pins
  static constexpr std::uint32_t gpio_all_32_bits_on{0xFFFFFFFF};

  static void pull_gpio(const unsigned pin, const unsigned high) noexcept
  {
    PANDA_TIMESWIPE_GPIO_PULL = high << pin;
  }

  static void init_gpio_input(const unsigned pin) noexcept
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
  }

  static void init_gpio_output(const unsigned pin) noexcept
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
    PANDA_TIMESWIPE_OUT_GPIO(pin);
    pull_gpio(pin, 0);
  }

  static void set_gpio_high(const unsigned pin) noexcept
  {
    PANDA_TIMESWIPE_GPIO_SET = 1 << pin;
  }

  static void set_gpio_low(const unsigned pin) noexcept
  {
    PANDA_TIMESWIPE_GPIO_CLR = 1 << pin;
  }

  static void reset_all_gpio() noexcept
  {
    PANDA_TIMESWIPE_GPIO_CLR = gpio_all_32_bits_on;
  }

  static unsigned read_all_gpio() noexcept
  {
    return (*(ts::detail::bcm_gpio + 13) & gpio_all_32_bits_on);
  }

  static void sleep_for_55ns() noexcept
  {
    read_all_gpio();
  }

  static void sleep_for_8ns() noexcept
  {
    set_gpio_high(10); // ANY UNUSED PIN!!!
  }

  struct Gpio_data final {
    std::uint8_t byte{};
    unsigned int tco{};
    bool pi_ok{};

    // chunk-Layout:
    // ------+----------------------------+---------------------------
    //  Byte | Bit7   Bit6   Bit5   Bit4  | Bit3   Bit2   Bit1   Bit0
    // ------+----------------------------+---------------------------
    //     0 | 1-14   2-14   3-14   4-14  | 1-15   2-15   3-15   4-15
    //     1 | 1-12   2-12   3-12   4-12  | 1-13   2-13   3-13   4-13
    //     2 | 1-10   2-10   3-10   4-10  | 1-11   2-11   3-11   4-11
    //     3 |  1-8    2-8    3-8    4-8  |  1-9    2-9    3-9    4-9
    //     4 |  1-6    2-6    3-6    4-6  |  1-7    2-7    3-7    4-7
    //     5 |  1-4    2-4    3-4    4-4  |  1-5    2-5    3-5    4-5
    //     6 |  1-2    2-2    3-2    4-2  |  1-3    2-3    3-3    4-3
    //     7 |  1-0    2-0    3-0    4-0  |  1-1    2-1    3-1    4-1
    using Chunk = std::array<std::uint8_t, 8>;

    struct Read_chunk_result final {
      Chunk chunk{};
      unsigned tco{};
    };

    static Read_chunk_result read_chunk() noexcept
    {
      Read_chunk_result result;
      result.chunk[0] = read().byte;
      {
        const auto d = read();
        result.chunk[1] = d.byte;
        result.tco = d.tco;
      }
      for (unsigned i{2u}; i < result.chunk.size(); ++i)
        result.chunk[i] = read().byte;
      return result;
    }

    static void append_chunk(Data_vector& data,
      const Chunk& chunk,
      const std::vector<float>& slopes,
      const std::vector<int>& translation_offsets,
      const std::vector<float>& translation_slopes)
    {
      PANDA_TIMESWIPE_ASSERT((slopes.size() == translation_offsets.size())
        && (slopes.size() == translation_slopes.size()));

      std::array<std::uint16_t, ts::max_channel_count> digits{};
      static_assert(sizeof(channel_offset) == sizeof(digits[0]));

      const auto channel_count = data.channel_count();
      PANDA_TIMESWIPE_ASSERT(channel_count <= digits.size());

      constexpr auto set_bit = [](std::uint16_t& word, const std::uint8_t N, const bool bit) noexcept
      {
        word = (word & ~(1UL << N)) | (bit << N);
      };
      constexpr auto bit = [](const std::uint8_t byte, const std::uint8_t N) noexcept -> bool
      {
        return (byte & (1UL << N));
      };
      for (std::size_t i{}, count{}; i < chunk.size(); ++i) {
        set_bit(digits[0], 15 - count, bit(chunk[i], 3));
        set_bit(digits[1], 15 - count, bit(chunk[i], 2));
        set_bit(digits[2], 15 - count, bit(chunk[i], 1));
        set_bit(digits[3], 15 - count, bit(chunk[i], 0));
        count++;

        set_bit(digits[0], 15 - count, bit(chunk[i], 7));
        set_bit(digits[1], 15 - count, bit(chunk[i], 6));
        set_bit(digits[2], 15 - count, bit(chunk[i], 5));
        set_bit(digits[3], 15 - count, bit(chunk[i], 4));
        count++;
      }

      for (std::size_t i{}; i < channel_count; ++i) {
        const auto mv = (digits[i] - channel_offset) * slopes[i];
        const auto unit = (mv - translation_offsets[i]) * translation_slopes[i];
        data[i].push_back(unit);
      }
    }

  private:
    static Gpio_data read() noexcept
    {
      set_gpio_high(gpio_clock);
      sleep_for_55ns();
      sleep_for_55ns();

      set_gpio_low(gpio_clock);
      sleep_for_55ns();
      sleep_for_55ns();

      const unsigned int all_gpio{read_all_gpio()};
      const std::uint8_t byte =
        ((all_gpio & gpio_data_position[0]) >> 17) |  // Bit 7
        ((all_gpio & gpio_data_position[1]) >> 19) |  //     6
        ((all_gpio & gpio_data_position[2]) >> 2) |   //     5
        ((all_gpio & gpio_data_position[3]) >> 1) |   //     4
        ((all_gpio & gpio_data_position[4]) >> 3) |   //     3
        ((all_gpio & gpio_data_position[5]) >> 10) |  //     2
        ((all_gpio & gpio_data_position[6]) >> 12) |  //     1
        ((all_gpio & gpio_data_position[7]) >> 16);   //     0

      sleep_for_55ns();
      sleep_for_55ns();

      return {byte, (all_gpio & gpio_tco_position), (all_gpio & gpio_pi_status_position) != 0};
    }
  };

  // ---------------------------------------------------------------------------
  // SPI stuff
  // ---------------------------------------------------------------------------

  void spi_set_enable_ad_mes(const bool value)
  {
    spi_.execute_set_one("EnableADmes", std::to_string(value));
  }

  // -----------------------------------------------------------------------------
  // Channels data reading, queueing and pushing stuff
  // -----------------------------------------------------------------------------

  /// Read records from hardware buffer.
  Data_vector read_channels_data()
  {
    static const auto wait_for_pi_ok = []
    {
      /// Matches 12MHz Quartz.
      std::this_thread::sleep_for(std::chrono::microseconds{700});
    };

    // Skip data sets if needed. (First 32 data sets are always invalid.)
    while (read_skip_count_ > 0) {
      wait_for_pi_ok();
      while (true) {
        const auto [chunk, tco] = Gpio_data::read_chunk();
        if (tco != 0x00004000) break;
      }
      --read_skip_count_;
    }

    // Wait the RAM A or RAM B becomes available for reading.
    wait_for_pi_ok();

    /*
     * Read the data sets. The amount of data depends on the counterstate
     * and can be [1..255]*32 data sets. (The number of data sets are always 32
     * also. Usually, the first data set is of size greater than 1 is followed
     * by 31 data sets of size 1.)
     *
     * TODO: the PIN 12 of Pi-Header is for overflow detection. When it's
     * becomes high it indicates that the RAM is full (failure - data loss).
     * So, check this case.
     */
    Data_vector result(max_channel_count());
    result.reserve(8192);
    do {
      const auto [chunk, tco] = Gpio_data::read_chunk();
      Gpio_data::append_chunk(result, chunk, calibration_slopes_,
        translation_offsets_, translation_slopes_);
      if (tco != 0x00004000) break;
    } while (true);

    sleep_for_55ns();
    sleep_for_55ns();

    return result;
  }

  // ---------------------------------------------------------------------------
  // Thread loops
  // ---------------------------------------------------------------------------

  void fetcher_loop()
  {
    while (is_measurement_started_) {
      if (const auto data = read_channels_data(); !record_queue_.push(data))
        ++record_error_count_;
    }
  }

  void poller_loop(Data_handler&& handler)
  {
    PANDA_TIMESWIPE_ASSERT(handler);
    while (is_measurement_started_) {
      Data_vector records[10];
      const auto num = record_queue_.pop(records);
      const auto errors = record_error_count_.fetch_and(0);

      if (!num) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        continue;
      }

      // If there are drift deltas substract them.
      if (drift_deltas_) {
        const auto& deltas = *drift_deltas_;
        for (std::size_t i{}; i < num; ++i) {
          const auto channel_count = records[i].channel_count();
          PANDA_TIMESWIPE_ASSERT(deltas.size() == channel_count);
          for (std::size_t j{}; j < channel_count; ++j) {
            auto& values = records[i][j];
            const auto delta = deltas[j];
            transform(cbegin(values), cend(values), begin(values),
              [delta](const auto& value) { return value - delta; });
          }
        }
      }

      Data_vector* records_ptr{};
      Data_vector samples;
      if (resampler_) {
        for (std::size_t i{}; i < num; i++) {
          auto s = resampler_->apply(std::move(records[i]));
          samples.append(std::move(s));
        }
        records_ptr = &samples;
      } else {
        for (std::size_t i{1}; i < num; i++) {
          records[0].append(std::move(records[i]));
        }
        records_ptr = records;
      }

      if (burst_buffer_.empty() && burst_buffer_size_ <= records_ptr->size()) {
        // Optimization: if burst buffer not used or smaller than first buffer.
        handler(std::move(*records_ptr), errors);
        records_ptr->clear();
      } else {
        // Utilize burst buffer.
        burst_buffer_.append(std::move(*records_ptr));
        records_ptr->clear();
        if (burst_buffer_.size() >= burst_buffer_size_) {
          handler(std::move(burst_buffer_), errors);
          burst_buffer_.clear();
        }
      }
    }

    // Flush the resampler instance into the burst buffer.
    if (resampler_)
      burst_buffer_.append(resampler_->flush());

    // Flush the remaining values from the burst buffer.
    if (!burst_buffer_.empty()) {
      handler(std::move(burst_buffer_), 0);
      burst_buffer_.clear();
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  /**
   * @returns Previous resampler if any.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  std::unique_ptr<detail::Resampler> set_resampler(const int rate,
    std::unique_ptr<detail::Resampler> resampler = {})
  {
    PANDA_TIMESWIPE_ASSERT(!is_measurement_started());

    const auto max_rate = max_sample_rate();
    if (!(1 <= rate && rate <= max_rate))
      throw Generic_exception{"cannot use invalid sample rate"};

    auto result = std::move(resampler_);
    if (rate != max_rate) {
      const auto rates_gcd = std::gcd(rate, max_rate);
      const auto up = rate / rates_gcd;
      const auto down = max_rate / rates_gcd;
      if (resampler) {
        PANDA_TIMESWIPE_ASSERT(up == resampler->options().up_factor());
        PANDA_TIMESWIPE_ASSERT(down == resampler->options().down_factor());
        resampler_ = std::move(resampler);
      } else
        resampler_ = std::make_unique<detail::Resampler>
          (detail::Resampler_options{up, down});
    } else {
      PANDA_TIMESWIPE_ASSERT(!resampler);
      resampler_.reset();
    }

    return result;
  }

  static std::filesystem::path tmp_dir()
  {
    const auto cwd = std::filesystem::current_path();
    return cwd/".panda"/"timeswipe";
  }

  void join_threads()
  {
    for (auto it = threads_.begin(); it != threads_.end();) {
      if (it->get_id() == std::this_thread::get_id()) {
        ++it;
        continue;
      }
      if (it->joinable())
        it->join();
      it = threads_.erase(it);
    }
  }

  /**
   * Collects the specified samples count.
   *
   * @returns The collected channel data.
   *
   * @param samples_count A number of samples to collect.
   * @param state_guard A function without arguments which returns an object
   * for automatic resourse cleanup (e.g. RAII state keeper and restorer).
   */
  template<typename F>
  Data_vector collect_channels_data(const std::size_t samples_count, F&& state_guard)
  {
    if (is_measurement_started())
      throw Generic_exception{Generic_errc::board_measurement_started,
        "cannot collect channels data because measurment is started"};

    const auto guard{state_guard()};

    std::error_condition errc;
    std::atomic_bool done{};
    Data_vector data;
    std::condition_variable update;
    start_measurement([this, samples_count, &errc, &done, &data, &update]
      (const Data_vector sd, const int)
    {
      if (errc || done)
        return;

      try {
        if (data.size() < samples_count)
          data.append(sd, samples_count - data.size());
      } catch (...) {
        errc = Generic_errc::generic;
      }

      if (errc || (!done && data.size() == samples_count)) {
        done = true;
        update.notify_one();
      }
    });

    // Await for notification from the handler.
    {
      static std::mutex mutex; // Dummy mutex actually: `done` is atomic already.
      std::unique_lock lock{mutex};
      update.wait(lock, [&done]{ return done.load(); });
    }
    PANDA_TIMESWIPE_ASSERT(done);
    stop_measurement();

    // Throw away if the data collection failed.
    if (errc)
      throw Generic_exception{errc, "cannot collect channels data"};

    return data;
  }
};
} // namespace detail

Driver& Driver::instance()
{
  if (!instance_)
    instance_ = std::make_unique<detail::iDriver>();
  return *instance_;
}

} // namespace panda::timeswipe
