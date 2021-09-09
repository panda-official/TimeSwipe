// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

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
#include "error.hpp"
#include "gain.hpp"
#include "hat.hpp"
#include "pidfile.hpp"
#include "resampler.hpp"
#include "version.hpp"

#include "3rdparty/dmitigr/assert.hpp"
#include "3rdparty/dmitigr/filesystem.hpp"
#include "3rdparty/dmitigr/math.hpp"
#include "3rdparty/dmitigr/rajson.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#include <array>
#include <atomic>
#include <chrono>
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
#include <cmath>
#endif
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

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// class Driver::Rep
// -----------------------------------------------------------------------------

class Driver::Rep final {
public:
  ~Rep()
  {
    stop();
    join_threads();
  }

  Rep()
    : pid_file_{"timeswipe"}
    , spi_{detail::Bcm_spi::Pins::spi0}
  {
    // Lock PID file.
    std::string msg;
    if (!pid_file_.lock(msg))
      // Lock here. Second lock from the same process is allowed.
      throw Runtime_exception{Errc::pid_file_lock_failed};

    // Initialize GPIO.
    init_gpio();
  }

  /**
   * Initializes GPIO pins.
   *
   * @param force Forces initialization even if `is_gpio_inited_ == true`.
   *
   * @par Effects
   * Restarts firmware on very first run!
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

  void set_board_settings(const Board_settings& settings)
  {
    spi_.execute_set_many(settings.to_stringified_json());
    board_settings_.reset(); // invalidate cache (this could be optimized)
  }

  const Board_settings& get_board_settings() const
  {
    if (!board_settings_)
      board_settings_.emplace(spi_.execute_get_many(""));
    return *board_settings_;
  }

  constexpr int get_version() const
  {
    return version;
  }

  int get_min_sample_rate() const
  {
    return 32;
  }

  int get_max_sample_rate() const
  {
    return 48000;
  }

  int get_max_data_channel_count() const
  {
    return max_data_channel_count;
  }

  void set_settings(Settings settings,
    std::unique_ptr<detail::Resampler> resampler = {})
  {
    set_resampler(settings.get_sample_rate(), std::move(resampler));
    burst_buffer_size_ = settings.get_burst_buffer_size();
    for (std::size_t i{}; i < sensor_translation_offsets_.size(); ++i)
      sensor_translation_offsets_[i] = settings.get_data_translation_offset(i);
    for (std::size_t i{}; i < sensor_translation_slopes_.size(); ++i)
      sensor_translation_slopes_[i] = settings.get_data_translation_slope(i);
    settings_ = std::move(settings);
  }

  const Settings& get_settings() const
  {
    return settings_;
  }

  void start(Data_handler&& data_handler, Event_handler&& event_handler = {})
  {
    if (!data_handler)
      throw Runtime_exception{Errc::invalid_data_handler};

    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    join_threads();

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
          auto& atom = result.get_atom(ct);
          const auto cal_entries = v.GetArray();
          const auto cal_entries_size = cal_entries.Size();
          for (std::size_t i{}; i < cal_entries_size; ++i) {
            const rajson::Value_view cal_entry{cal_entries[i]};
            if (!cal_entry.value().IsObject())
              throw Runtime_exception{"calibration entry not an object"};
            const auto slope = cal_entry.mandatory<float>("m");
            const auto offset = cal_entry.mandatory<std::int16_t>("b");
            const hat::atom::Calibration::Entry entry{slope, offset};
            std::string err;
            atom.set_entry(i, entry, err);
          }
        } else
          throw Runtime_exception{"invalid calibration data"};
      }
      return result;
    }();

    /*
     * Send the command to the firmware to start the measurement.
     * Effects: the reader does receive the data from the board.
     */
    {
      DMITIGR_ASSERT(is_gpio_inited_);

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
      emul_point_begin_ = std::chrono::steady_clock::now();
      emul_sent_ = 0;
#endif

      // Start measurement.
      using std::chrono::milliseconds;
      std::this_thread::sleep_for(milliseconds{1});
      spi_set_enable_ad_mes(true);
      is_measurement_started_ = true;
    }

    threads_.emplace_back(&Rep::fetcher_loop, this);
    threads_.emplace_back(&Rep::poller_loop, this, std::move(data_handler));
    if (event_handler)
      threads_.emplace_back(&Rep::events_loop, this, std::move(event_handler));
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    threads_.emplace_back(&Rep::emul_loop, this);
#endif
  }

  bool is_busy() const noexcept
  {
    return is_measurement_started_ || in_handler_;
  }

  void stop()
  {
    if (!is_measurement_started_) return;

    is_measurement_started_ = false;
    join_threads();

    while (record_queue_.pop());

    /*
     * Sends the command to the firmware to stop the measurement.
     * Effects: the reader doesn't receive the data from the board.
     */
    {
      // Reset Clock
      set_gpio_low(gpio_clock);

      // Stop Measurement
      spi_set_enable_ad_mes(false);

      // Reset state.
      read_skip_count_ = initial_invalid_datasets_count;
    }
  }

  std::vector<float> calculate_drift_references()
  {
    // Collect the data for calculation.
    auto data = collect_sensors_data(drift_samples_count, // 5 ms
      [this]{return Drift_affected_state_guard{*this};});

    // Discard the first half.
    data.erase_front(drift_samples_count / 2);

    // Take averages of measured data (references).
    std::vector<float> result(data.get_channel_count());
    transform(data.cbegin(), data.cend(), result.begin(), [](const auto& dat)
    {
      return static_cast<float>(dmitigr::math::avg(dat));
    });

    // Put references to the tmp_dir/drift_references.
    const auto tmp = tmp_dir();
    std::filesystem::create_directories(tmp);
    constexpr auto open_flags{std::ios_base::out | std::ios_base::trunc};
    std::ofstream refs_file{tmp/"drift_references", open_flags};
    for (auto i = 0*result.size(); i < result.size() - 1; ++i)
      refs_file << result[i] << " ";
    refs_file << result.back() << "\n";

    // Cache references.
    drift_references_ = result;

    return result;
  }

  void clear_drift_references()
  {
    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    std::filesystem::remove(tmp_dir()/"drift_references");
    drift_references_.reset();
    drift_deltas_.reset();
  }

  std::vector<float> calculate_drift_deltas()
  {
    // Throw away if there are no references.
    const auto refs{get_drift_references()};
    if (!refs)
      throw Runtime_exception{Errc::no_drift_references};

    // Collect the data for calculation.
    auto data{collect_sensors_data(drift_samples_count,
      [this]{return Drift_affected_state_guard{*this};})};
    DMITIGR_ASSERT(refs->size() == data.get_channel_count());

    // Discard the first half.
    data.erase_front(drift_samples_count / 2);

    // Take averages of measured data (references) and subtract the references.
    std::vector<float> result(data.get_channel_count());
    transform(data.cbegin(), data.cend(), refs->cbegin(), result.begin(),
      [](const auto& dat, const auto ref)
      {
        return static_cast<float>(dmitigr::math::avg(dat) - ref);
      });

    // Cache deltas.
    drift_deltas_ = result;

    return result;
  }

  void clear_drift_deltas()
  {
    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    drift_deltas_.reset();
  }

  std::optional<std::vector<float>> get_drift_references(const bool force = {}) const
  {
    if (!force && drift_references_)
      return drift_references_;

    const auto drift_references{tmp_dir()/"drift_references"};
    if (!std::filesystem::exists(drift_references))
      return std::nullopt;

    std::ifstream in{drift_references};
    if (!in)
      throw Runtime_exception{Errc::invalid_drift_reference};

    std::vector<float> refs;
    while (in && refs.size() < get_max_data_channel_count()) {
      float val;
      if (in >> val)
        refs.push_back(val);
    }
    if (!in.eof()) {
      float val;
      if (in >> val)
        throw Runtime_exception{Errc::excessive_drift_references};
    }
    if (refs.empty())
      throw Runtime_exception{Errc::insufficient_drift_references};

    DMITIGR_ASSERT(refs.size() <= get_max_data_channel_count());

    // Cache and return references.
    return drift_references_ = refs;
  }

  std::optional<std::vector<float>> get_drift_deltas() const
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
  static constexpr std::uint16_t sensor_offset{32768};
  int read_skip_count_{initial_invalid_datasets_count};
  std::array<float, max_data_channel_count> sensor_slopes_{1, 1, 1, 1};
  std::array<int, max_data_channel_count> sensor_translation_offsets_{};
  std::array<float, max_data_channel_count> sensor_translation_slopes_{1, 1, 1, 1};
  hat::Calibration_map calibration_map_;
  mutable std::optional<Board_settings> board_settings_;
  Settings settings_;
  std::unique_ptr<detail::Resampler> resampler_;

  // Record queue capacity must be enough to store records for 1s.
  boost::lockfree::spsc_queue<Data_vector, boost::lockfree::capacity<48000/32*2>> record_queue_;
  std::atomic_int record_error_count_{};
  std::size_t burst_buffer_size_{};
  Data_vector burst_buffer_;
  std::vector<std::thread> threads_;
  std::atomic_bool in_handler_{};

  // ---------------------------------------------------------------------------
  // Drift compensation data
  // ---------------------------------------------------------------------------

  mutable std::optional<std::vector<float>> drift_references_;
  std::optional<std::vector<float>> drift_deltas_;

  // ---------------------------------------------------------------------------
  // Firmware emulation data
  // ---------------------------------------------------------------------------

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
  double angle_{};
  std::chrono::steady_clock::time_point emul_point_begin_;
  std::chrono::steady_clock::time_point emul_point_end_;
  std::uint64_t emul_sent_{};
  static constexpr std::size_t emul_rate_{48000};
#endif

  // ---------------------------------------------------------------------------
  // RAII protectors
  // ---------------------------------------------------------------------------

  /*
   * An automatic resetter of value of in_handler_. `false` will
   * be assigned upon destruction of the instance of this class.
   */
  struct Callbacker final {
    Callbacker(const Callbacker&) = delete;
    Callbacker& operator=(const Callbacker&) = delete;
    Callbacker(Callbacker&&) = delete;
    Callbacker& operator=(Callbacker&&) = delete;

    ~Callbacker()
    {
      self_.in_handler_ = false;
    }

    Callbacker(Rep& self) noexcept
      : self_{self}
    {}

    template<typename F, typename ... Types>
    auto operator()(const F& handler, Types&& ... args)
    {
      self_.in_handler_ = true;
      return handler(std::forward<Types>(args)...);
    }

  private:
    Rep& self_;
  };

  /*
   * An automatic restorer of state affected by drift calculation stuff. Stashed
   * state will be restored upon destruction of the instance of this class.
   */
  class Drift_affected_state_guard final {
    friend Rep;

    Drift_affected_state_guard(const Drift_affected_state_guard&) = delete;
    Drift_affected_state_guard& operator=(const Drift_affected_state_guard&) = delete;
    Drift_affected_state_guard(Drift_affected_state_guard&&) = delete;
    Drift_affected_state_guard& operator=(Drift_affected_state_guard&&) = delete;

    // Restores the driver state.
    void restore() noexcept
    {
      try {
        // Restore driver settings.
        rep_.set_settings(std::move(settings_),
          std::move(resampler_));

        // Restore board settings (input modes).
        Board_settings settings;
        for (std::size_t i{}; i < chmm_.size(); ++i)
          settings.set_channel_measurement_mode(i, chmm_[i]);
        rep_.set_board_settings(settings);
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
    Drift_affected_state_guard(Rep& rep)
      : rep_{rep}
      , resampler_{std::move(rep_.resampler_)} // store
      , settings_{std::move(rep_.settings_)} // settings
    {
      try {
        // Store board settings (input modes).
        const auto channel_mode = [this](const int index)
        {
          if (const auto mm = rep_.get_board_settings().get_channel_measurement_mode(index))
            return *mm;
          else
            throw Runtime_exception{Errc::invalid_board_state};
        };
        for (int i{}; i < chmm_.size(); ++i)
          chmm_[i] = channel_mode(i);

        /*
         * Change input modes to `current`.
         * This will cause a "switching oscillation" appears at the output of
         * the measured value, which completely (according to PSpice) decays
         * after 1.5 ms.
         */
        {
          Board_settings settings;
          for (int i{}; i < chmm_.size(); ++i)
            settings.set_channel_measurement_mode(i, Measurement_mode::Current);
          rep_.set_board_settings(settings);
        }

        std::this_thread::sleep_for(rep_.switching_oscillation_period);

        // Set specific driver settings.
        Settings settings;
        settings.set_sample_rate(48000)
          .set_burst_buffer_size(rep_.drift_samples_count);
        rep_.set_settings(std::move(settings));
      } catch (...) {
        restore();
      }
    }

    Rep& rep_;
    decltype(rep_.resampler_) resampler_;
    decltype(rep_.settings_) settings_;
    std::array<Measurement_mode, max_data_channel_count> chmm_;
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

  static void pull_gpio(const unsigned pin, const unsigned high)
  {
    PANDA_TIMESWIPE_GPIO_PULL = high << pin;
  }

  static void init_gpio_input(const unsigned pin)
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
  }

  static void init_gpio_output(const unsigned pin)
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
    PANDA_TIMESWIPE_OUT_GPIO(pin);
    pull_gpio(pin, 0);
  }

  static void set_gpio_high(const unsigned pin)
  {
    PANDA_TIMESWIPE_GPIO_SET = 1 << pin;
  }

  static void set_gpio_low(const unsigned pin)
  {
    PANDA_TIMESWIPE_GPIO_CLR = 1 << pin;
  }

  static void reset_all_gpio()
  {
    PANDA_TIMESWIPE_GPIO_CLR = gpio_all_32_bits_on;
  }

  static unsigned read_all_gpio()
  {
    return (*(panda::timeswipe::detail::bcm_gpio + 13) & gpio_all_32_bits_on);
  }

  static void sleep_for_55ns()
  {
    read_all_gpio();
  }

  static void sleep_for_8ns()
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
      const std::array<float, max_data_channel_count>& slopes,
      const std::array<int, max_data_channel_count>& translation_offsets,
      const std::array<float, max_data_channel_count>& translation_slopes)
    {
      std::array<std::uint16_t, max_data_channel_count> sensors{};
      static_assert(sizeof(sensor_offset) == sizeof(sensors[0]));
      static_assert(slopes.size() == sensors.size());
      static_assert(translation_offsets.size() == sensors.size());
      static_assert(translation_slopes.size() == sensors.size());

      const auto channel_count = data.get_channel_count();
      DMITIGR_ASSERT(channel_count <= sensors.size());

      constexpr auto set_bit = [](std::uint16_t& word, const std::uint8_t N, const bool bit) noexcept
      {
        word = (word & ~(1UL << N)) | (bit << N);
      };
      constexpr auto get_bit = [](const std::uint8_t byte, const std::uint8_t N) noexcept -> bool
      {
        return (byte & (1UL << N));
      };
      for (std::size_t i{}, count{}; i < chunk.size(); ++i) {
        set_bit(sensors[0], 15 - count, get_bit(chunk[i], 3));
        set_bit(sensors[1], 15 - count, get_bit(chunk[i], 2));
        set_bit(sensors[2], 15 - count, get_bit(chunk[i], 1));
        set_bit(sensors[3], 15 - count, get_bit(chunk[i], 0));
        count++;

        set_bit(sensors[0], 15 - count, get_bit(chunk[i], 7));
        set_bit(sensors[1], 15 - count, get_bit(chunk[i], 6));
        set_bit(sensors[2], 15 - count, get_bit(chunk[i], 5));
        set_bit(sensors[3], 15 - count, get_bit(chunk[i], 4));
        count++;
      }

      for (std::size_t i{}; i < channel_count; ++i) {
        const auto mv = (sensors[i] - sensor_offset) * slopes[i];
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

  std::vector<Event> spi_get_events() noexcept
  {
    std::vector<Event> result;
#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    if (const auto json = spi_.execute_get_events(); !json.empty()) {
      try {
        result.reserve(7);
        const auto doc = rajson::to_document(json);
        const rajson::Value_view view{doc};

        // Get Button event.
        if (const auto button = view.optional<bool>("Button")) {
          if (const auto count = view.optional<int>("ButtonStateCnt"))
            result.emplace_back(Event::Button(*button, *count));
        }

        // Get Gain event.
        if (const auto value = view.optional<int>("Gain"))
          result.emplace_back(Event::Gain(*value));

        // Get SetSecondary event.
        if (const auto value = view.optional<int>("SetSecondary"))
          result.emplace_back(Event::Set_secondary(*value));

        // Get Bridge event.
        if (const auto value = view.optional<int>("Bridge"))
          result.emplace_back(Event::Bridge(*value));

        // Get Record event.
        if (const auto value = view.optional<int>("Record"))
          result.emplace_back(Event::Record(*value));

        // Get Offset event.
        if (const auto value = view.optional<int>("Offset"))
          result.emplace_back(Event::Offset(*value));

        // Get Mode event.
        if (const auto value = view.optional<int>("Mode"))
          result.emplace_back(Event::Mode(*value));
      } catch (const std::exception& e) {
        std::cerr << "spi_get_events(): " << e.what() << "\n";
      } catch (...) {
        std::cerr << "spi_get_events(): unknown error\n";
      }
    }
#endif
    return result;
  }

  // -----------------------------------------------------------------------------
  // Sensor data reading, queueing and pushing stuff
  // -----------------------------------------------------------------------------

  /// Read records from hardware buffer.
  Data_vector read_sensors_data()
  {
    static const auto wait_for_pi_ok = []
    {
      /// Matches 12MHz Quartz.
      std::this_thread::sleep_for(std::chrono::microseconds{700});
    };

#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
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
    Data_vector result(get_max_data_channel_count());
    result.reserve(8192);
    do {
      const auto [chunk, tco] = Gpio_data::read_chunk();
      Gpio_data::append_chunk(result, chunk, sensor_slopes_,
        sensor_translation_offsets_, sensor_translation_slopes_);
      if (tco != 0x00004000) break;
    } while (true);

    sleep_for_55ns();
    sleep_for_55ns();

    return result;
#else
    namespace chrono = std::chrono;
    Data_vector result;
    auto& data{result.data()};
    while (true) {
      emul_point_end_ = chrono::steady_clock::now();
      const std::uint64_t diff_us{chrono::duration_cast<chrono::microseconds>
        (emul_point_end_ - emul_point_begin_).count()};
      const std::uint64_t would_sent{diff_us * emul_rate_ / 1000 / 1000};
      if (would_sent > emul_sent_) {
        while (emul_sent_++ < would_sent) {
          const auto val{int(3276 * std::sin(angle_) + 32767)};
          angle_ += (2.0 * M_PI) / emul_rate_;
          data[0].push_back(val);
          data[1].push_back(val);
          data[2].push_back(val);
          data[3].push_back(val);
        }
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return result;
#endif
  }

  // ---------------------------------------------------------------------------
  // Thread loops
  // ---------------------------------------------------------------------------

  void fetcher_loop()
  {
    while (is_measurement_started_) {
      if (const auto data = read_sensors_data(); !record_queue_.push(data))
        ++record_error_count_;
    }
  }

  void poller_loop(Data_handler&& handler)
  {
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
          const auto channel_count = records[i].get_channel_count();
          DMITIGR_ASSERT(deltas.size() == channel_count);
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

      if (burst_buffer_.empty() && burst_buffer_size_ <= records_ptr->get_size()) {
        // optimization if burst buffer not used or smaller than first buffer
        Callbacker{*this}(handler, std::move(*records_ptr), errors);
        records_ptr->clear();
      } else {
        // burst buffer mode
        burst_buffer_.append(std::move(*records_ptr));
        records_ptr->clear();
        if (burst_buffer_.get_size() >= burst_buffer_size_) {
          Callbacker{*this}(handler, std::move(burst_buffer_), errors);
          burst_buffer_.clear();
        }
      }
    }

    // Flush the resampler instance into the burst buffer.
    if (resampler_)
      burst_buffer_.append(resampler_->flush());

    // Flush the remaining values from the burst buffer.
    if (!in_handler_ && burst_buffer_.get_size()) {
      Callbacker{*this}(handler, std::move(burst_buffer_), 0);
      burst_buffer_.clear();
    }
  }

  void events_loop(Event_handler&& handler)
  {
    while (is_measurement_started_) {
      // Receive events
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
      if (emul_button_sent_ < emul_button_pressed_) {
        Event::Button btn{true, emul_button_pressed_};
        emul_button_sent_ = emul_button_pressed_;
        events_.push(btn);
      }
#else
      for (auto&& event : spi_get_events()) {
        if (handler)
          Callbacker{*this}(handler, std::move(event));
      }
#endif
      std::this_thread::sleep_for(std::chrono::milliseconds{20});
    }
  }

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
  int emul_button_pressed_{};
  int emul_button_sent_{};

  void emul_loop()
  {
    emul_button_pressed_ = 0;
    emul_button_sent_ = 0;
    while (is_measurement_started_) {
      timeval tv;
      tv.tv_sec = 1;
      tv.tv_usec = 0;
      fd_set read_fds;
      FD_ZERO(&read_fds);
      FD_SET(0, &read_fds);

      auto result = select(1, &read_fds, NULL, NULL, &tv);
      if (result == -1 && errno != EINTR) {
        std::cerr << "emul_loop: error select" << std::endl;
        return;
      } else if (result == -1 && errno == EINTR) {
        std::cerr << "emul_loop: EINTR select" << std::endl;
        return;
      } else {
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
          emul_button_pressed_ += 2; // press and release
          std::string buf;
          std::getline(std::cin, buf);
        }
      }
    }
  }
#endif  // PANDA_TIMESWIPE_FIRMWARE_EMU

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  /// @returns Previous resampler if any.
  std::unique_ptr<detail::Resampler> set_resampler(const int rate,
    std::unique_ptr<detail::Resampler> resampler = {})
  {
    if (is_busy()) return {};

    const auto max_rate = get_max_sample_rate();
    if (!(1 <= rate && rate <= max_rate))
      throw Runtime_exception{Errc::out_of_range, "invalid sample rate"};

    auto result = std::move(resampler_);
    if (rate != max_rate) {
      const auto rates_gcd = std::gcd(rate, max_rate);
      const auto up = rate / rates_gcd;
      const auto down = max_rate / rates_gcd;
      if (resampler) {
        DMITIGR_ASSERT(up == resampler->get_options().get_up_factor());
        DMITIGR_ASSERT(down == resampler->get_options().get_down_factor());
        resampler_ = std::move(resampler);
      } else
        resampler_ = std::make_unique<detail::Resampler>
          (detail::Resampler_options{up, down});
    } else {
      DMITIGR_ASSERT(!resampler);
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
   * @brief Collects the specified samples count.
   *
   * @returns The collected sensor data.
   *
   * @param samples_count A number of samples to collect.
   * @param state_guard A function without arguments which returns an object
   * for automatic resourse cleanup (e.g. RAII state keeper and restorer).
   */
  template<typename F>
  Data_vector collect_sensors_data(const std::size_t samples_count, F&& state_guard)
  {
    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    const auto guard{state_guard()};

    Errc errc{};
    std::atomic_bool done{};
    Data_vector data;
    std::condition_variable update;
    start([this, samples_count, &errc, &done, &data, &update]
      (const Data_vector sd, const int)
    {
      if (is_error(errc) || done)
        return;

      try {
        if (data.get_size() < samples_count)
          data.append(sd, samples_count - data.get_size());
      } catch (...) {
        errc = Errc::generic;
      }

      if (is_error(errc) || (!done && data.get_size() == samples_count)) {
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
    DMITIGR_ASSERT(done);
    stop();

    // Throw away if the data collection failed.
    if (is_error(errc))
      throw Runtime_exception{errc};

    return data;
  }
};

// -----------------------------------------------------------------------------
// class Driver
// -----------------------------------------------------------------------------

Driver::~Driver() = default;

Driver::Driver()
  : rep_{std::make_unique<Rep>()}
{}

Driver& Driver::get_instance()
{
  if (!instance_) instance_.reset(new Driver);
  return *instance_;
}

int Driver::get_version() const
{
  return rep_->get_version();
}

int Driver::get_min_sample_rate() const
{
  return rep_->get_min_sample_rate();
}

int Driver::get_max_sample_rate() const
{
  return rep_->get_max_sample_rate();
}

int Driver::get_max_data_channel_count() const
{
  return rep_->get_max_data_channel_count();
}

void Driver::set_board_settings(const Board_settings& settings)
{
  return rep_->set_board_settings(settings);
}

const Board_settings& Driver::get_board_settings() const
{
  return rep_->get_board_settings();
}

void Driver::set_settings(Settings settings)
{
  return rep_->set_settings(std::move(settings));
}

auto Driver::get_settings() const -> const Settings&
{
  return rep_->get_settings();
}

void Driver::start(Data_handler data_handler, Event_handler event_handler)
{
  rep_->start(std::move(data_handler), std::move(event_handler));
}

bool Driver::is_busy() const noexcept
{
  return rep_->is_busy();
}

void Driver::stop()
{
  return rep_->stop();
}

std::vector<float> Driver::calculate_drift_references()
{
  return rep_->calculate_drift_references();
}

void Driver::clear_drift_references()
{
  rep_->clear_drift_references();
}

std::vector<float> Driver::calculate_drift_deltas()
{
  return rep_->calculate_drift_deltas();
}

void Driver::clear_drift_deltas()
{
  rep_->clear_drift_deltas();
}

std::optional<std::vector<float>> Driver::get_drift_references(const bool force) const
{
  return rep_->get_drift_references(force);
}

std::optional<std::vector<float>> Driver::get_drift_deltas() const
{
  return rep_->get_drift_deltas();
}

} // namespace panda::timeswipe
