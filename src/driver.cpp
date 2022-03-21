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
#include "debug.hpp"
#include "driver.hpp"
#include "exceptions.hpp"
#include "gain.hpp"
#include "hat.hpp"
#include "limits.hpp"
#include "pidfile.hpp"
#include "resampler.hpp"
#include "version.hpp"
#include "board_settings.cpp"
#include "driver_settings.cpp"

#include "3rdparty/dmitigr/fs/filesystem.hpp"
#include "3rdparty/dmitigr/math/statistic.hpp"
#include "3rdparty/dmitigr/rajson/rajson.hpp"
#include "3rdparty/dmitigr/str/transform.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <numeric>
#include <thread>
#include <type_traits>

namespace chrono = std::chrono;
namespace rajson = dmitigr::rajson;
namespace ts = panda::timeswipe;
using chrono::milliseconds;
using chrono::microseconds;

namespace panda::timeswipe {
namespace detail {
class iDriver final : public Driver {
public:
  using Resampler = detail::Resampler<Data::Value>;

  ~iDriver()
  {
    try {
      stop_measurement();
    } catch (...){}
  }

  iDriver()
    : pid_file_{"panda_timeswipe"}
    , calibration_slopes_(max_channel_count())
    , translation_offsets_(max_channel_count())
    , translation_slopes_(max_channel_count())
    , burst_buffer_(max_channel_count())
  {}

  iDriver& initialize() override
  {
    if (is_initialized_) return *this;

    // Initialize slopes and offsets.
    fill(begin(calibration_slopes_), end(calibration_slopes_), 1);
    fill(begin(translation_offsets_), end(translation_offsets_), 0);
    fill(begin(translation_slopes_), end(translation_slopes_), 1);

    // Initialize default driver settings.
    driver_settings_.set_translation_offsets(translation_offsets_);
    driver_settings_.set_translation_slopes(translation_slopes_);

    // Lock PID file.
    pid_file_.lock();

    // Initialize BCM and SPI.
    spi_.initialize(detail::Bcm_spi::Pins::spi0);

    /*
     * Initialize GPIO pins.
     *
     * @par Effects
     * Restarts firmware on very first run!
     *
     * @warning Firmware developers must remember, that restarting the firmware
     * causes reset of all the settings the firmware keeps in the on-board RAM!
     */
    if (!is_gpio_inited_) {
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

      /*
       * On some devices delay is required to get SPI communication work.
       * There is also delay in Bcm_spi::initialize().
       */
      std::this_thread::sleep_for(milliseconds{100});

      is_gpio_inited_ = true;
    }

    is_initialized_ = true;
    return *this;
  }

  bool is_initialized() const override
  {
    return is_initialized_;
  }

  int version() const override
  {
    return detail::driver_version;
  }

  int min_sample_rate() const override
  {
    return 32;
  }

  int max_sample_rate() const override
  {
    return 48000;
  }

  unsigned max_channel_count() const override
  {
    return detail::max_channel_count;
  }

  float min_channel_gain() const override
  {
    return gain::ogain_min;
  }

  float max_channel_gain() const override
  {
    return gain::ogain_max;
  }

  /*
   * @remarks Some settings cannot be applied if the measurement is started.
   * It's up to firmware to check this.
   */
  iDriver& set_board_settings(const Board_settings& settings) override
  {
    if (!is_initialized())
      throw Exception{Errc::driver_not_initialized,
        "cannot set board settings while driver is not initialized"};

    // Define helper.
    static const auto throw_exception = [](const std::string& msg)
    {
      throw Exception{Errc::board_settings_invalid, msg};
    };

    // Get the document.
    const auto& doc = settings.rep_->doc();

    // Direct application of some settings are prohibited.
    const auto prohibited = settings.inapplicable_names();
    for (const auto& setting : doc.GetObject()) {
      const std::string name{setting.name.GetString(),
        setting.name.GetStringLength()};
      if (any_of(cbegin(prohibited), cend(prohibited),
          [name](const auto& prohibited_name)
          {
            return name == prohibited_name;
          }))
        throw_exception(std::string{"direct application of "}.append(name)
          .append(" board setting is prohibited"));
    }

    // Attempt to apply.
    spi_.execute_set("all", rajson::to_text(doc));
    return *this;
  }

  Board_settings board_settings(std::string_view criteria={}) const override
  {
    if (criteria.empty())
      criteria = "all";
    auto doc = spi_.execute_get(criteria);
    return Board_settings{std::make_unique<Board_settings::Rep>(std::move(doc))};
  }

  iDriver& set_driver_settings(const Driver_settings& settings) override
  {
    set_driver_settings(settings, {});
    return *this;
  }

  /// @overload
  void set_driver_settings(const Driver_settings& settings,
    std::unique_ptr<Resampler> resampler)
  {
    if (is_measurement_started(true))
      /*
       * Currently, there are no driver settings which can be applied when
       * measurement in progress.
       */
      throw Exception{Errc::board_measurement_started,
        "cannot set driver settings when measurement started"};

    const auto bbs = settings.burst_buffer_size();
    const auto freq = settings.frequency();
    const auto srate = settings.sample_rate();
    PANDA_TIMESWIPE_ASSERT(!(bbs && freq));
    if (bbs)
      burst_buffer_size_ = *bbs;
    if (srate && freq)
      burst_buffer_size_ = *srate / *freq;

    if (const auto values = settings.translation_offsets())
      translation_offsets_ = std::move(*values);
    if (const auto values = settings.translation_slopes())
      translation_slopes_ = std::move(*values);

    driver_settings_.set(settings); // may throw
  }

  const Driver_settings& driver_settings() const override
  {
    return driver_settings_;
  }

  void start_measurement(Data_handler handler) override
  {
    if (!is_initialized())
      throw Exception{Errc::driver_not_initialized,
        "cannot start measurement while driver isn't initialized"};

    /// @returns The calibration map from board settings.
    static const auto calibration_map = [](const Board_settings& bs)
    {
      using hat::atom::Calibration;
      hat::Calibration_map result;
      const auto& doc = bs.rep_->doc();

      // Use default slopes and offsets if the calibration data is disabled.
      if (const auto calib_enabled = rajson::Value_view{doc}.optional("calibrationDataEnabled")) {
        PANDA_TIMESWIPE_ASSERT(calib_enabled->value().IsBool());
        if (!rajson::to<bool>(calib_enabled->value()))
          return result;
      }

      // Otherwise, use slopes and offset provided by firmware.
      const auto calib = rajson::Value_view{doc}.mandatory("calibrationData");
      PANDA_TIMESWIPE_ASSERT(calib.value().IsArray());
      try {
        // "calibrationData":[{"type":%, "data":[{"slope":%, "offset":%},...]},...]
        for (const auto& catom_v : calib.value().GetArray()) {
          rajson::Value_view catom_view{catom_v};

          // Get the calibration atom type.
          const auto type = catom_view.mandatory<Calibration::Type>("type");

          // Check that data member is array.
          const auto& catom_data_view = catom_view.mandatory("data");
          if (!catom_data_view.value().IsArray())
            throw Exception{"data member is not array"};

          // Get the data array and ensure it has the proper size.
          const auto& catom_data = catom_data_view.value().GetArray();
          const auto catom_data_size = catom_data.Size();
          if (catom_data_size != result.atom(type).entry_count())
            throw Exception{"invalid data member size"};

          // Extract each entry with slope and offset from the data array.
          for (std::decay_t<decltype(catom_data_size)> i{}; i < catom_data_size; ++i) {
            const auto& catom_data_entry = catom_data[i];
            if (!catom_data_entry.IsObject())
              throw Exception{"data entry is not object"};

            rajson::Value_view catom_data_entry_view{catom_data_entry};
            const auto slope = catom_data_entry_view.mandatory<float>("slope");
            const auto offset = catom_data_entry_view.mandatory<std::int16_t>("offset");
            const Calibration::Entry entry{slope, offset};
            result.atom(type).set_entry(i, entry);
          }
        }
      } catch (const std::exception& e) {
        throw Exception{Errc::board_settings_invalid,
          std::string{"cannot use calibration data: "}.append(e.what())};
      } catch (...) {
        throw Exception{Errc::board_settings_invalid,
          "cannot use calibration data: unknown error"};
      }

      return result;
    };

    const auto bs = board_settings();
    const auto calib = calibration_map(bs);
    const auto gains = channel_settings<float>(bs, "Gain");
    const auto modes = channel_settings<Measurement_mode>(bs, "Mode");
    const auto srate = driver_settings().sample_rate();
    if (!handler)
      throw Exception{"cannot start measurement with invalid data handler"};
    else if (is_measurement_started(true))
      throw Exception{Errc::board_measurement_started,
        "cannot start measurement because it's already started"};
    else if (!gains)
      throw Exception{Errc::board_settings_insufficient,
        "cannot start measurement with unspecified channel gains"};
    else if (!modes)
      throw Exception{Errc::board_settings_insufficient,
        "cannot start measurement with unspecified channel measurement modes"};
    else if (!srate)
      throw Exception{Errc::driver_settings_insufficient,
        "cannot start measurement with unspecified sample rate"};

    // Pick up the calibration slopes depending on both the gain and measurement mode.
    const auto mcc = max_channel_count();
    PANDA_TIMESWIPE_ASSERT(gains && modes &&
      (gains->size() == modes->size()) &&
      (gains->size() >= mcc));
    // may throw
    decltype(calibration_slopes_) new_calibration_slopes{calibration_slopes_};
    for (std::decay_t<decltype(mcc)> i{}; i < mcc; ++i) {
      const auto gain = gains->at(i);
      const auto mode = modes->at(i);
      using Ct = hat::atom::Calibration::Type;
      using Array = std::array<Ct, detail::max_channel_count>;
      constexpr Array v_types{Ct::v_in1, Ct::v_in2, Ct::v_in3, Ct::v_in4};
      constexpr Array c_types{Ct::c_in1, Ct::c_in2, Ct::c_in3, Ct::c_in4};
      const auto& types = (mode == Measurement_mode::voltage) ? v_types : c_types;
      const auto& atom = calib.atom(types[i]);
      const auto ogain_index = gain::ogain_table_index(gain);
      PANDA_TIMESWIPE_ASSERT(ogain_index < atom.entry_count());
      new_calibration_slopes[i] = atom.entry(ogain_index).slope();
    }
    calibration_slopes_.swap(new_calibration_slopes); // noexcept

    // Reset resampler.
    set_resampler(*srate, {}); // strong guarantee

    /*
     * Send the command to the firmware to start the measurement.
     * Effects: the reader does receive the data from the board.
     */
    {
      PANDA_TIMESWIPE_ASSERT(is_gpio_inited_);
      std::this_thread::sleep_for(milliseconds{1});
      spi_set_channels_adc_enabled(true);
    }

    try {
      is_threads_running_ = true;
      is_measurement_started_ = true;
      threads_.emplace_back(&iDriver::data_reading, this);
      threads_.emplace_back(&iDriver::data_processing, this, std::move(handler));
    } catch (...) {
      is_measurement_started_ = false;
      join_threads();
      calibration_slopes_.swap(new_calibration_slopes); // noexcept
      throw;
    }

    // Done.
    PANDA_TIMESWIPE_ASSERT(is_measurement_started());
  }

  bool is_measurement_started(const bool ask_board = {}) const override
  {
    if (ask_board) {
      if (!is_initialized())
        throw Exception{Errc::driver_not_initialized,
          "cannot ask the board for measurement status while driver isn't initialized"};

      return is_measurement_started_ = spi_is_channels_adc_enabled();
    } else
      return is_measurement_started_;
  }

  void stop_measurement() override
  {
    if (!is_initialized())
      throw Exception{Errc::driver_not_initialized,
        "cannot stop measurement while driver isn't initialized"};

    if (!is_measurement_started(true)) return;

    // Wait threads and reset state they are using.
    join_threads();
    while (record_queue_.pop());
    read_skip_count_ = initial_invalid_datasets_count;

    // Send the command to the firmware to stop the measurement.
    {
      // Reset clock.
      set_gpio_low(gpio_clock);

      // Stop measurement.
      spi_set_channels_adc_enabled(false); // may throw
    }

    // Done.
    is_measurement_started_ = false;
    PANDA_TIMESWIPE_ASSERT(!is_measurement_started());
  }

  std::vector<float> calculate_drift_references() override
  {
    // Collect the data for calculation.
    auto data = collect_channels_data(drift_samples_count, // 5 ms
      [this]{return Drift_affected_state_guard{*this};}); // strong guarantee

    // Discard the first half.
    data.remove_begin_rows(drift_samples_count / 2);

    // Take averages of measured data (references).
    std::vector<float> result(data.column_count());
    transform(data.columns_cbegin(), data.columns_cend(), result.begin(),
      [](const auto& column)
      {
        return static_cast<float>(dmitigr::math::avg(column));
      });

    // Put references to the tmp_dir/drift_references.
    const auto tmp = tmp_dir();
    std::filesystem::create_directories(tmp);
    constexpr auto open_flags{std::ios_base::out | std::ios_base::trunc};
    std::ofstream refs_file{tmp/"drift_references", open_flags};
    for (decltype(result)::size_type i{}; i < result.size() - 1; ++i)
      refs_file << result[i] << " ";
    refs_file << result.back() << "\n";

    // Cache references.
    drift_references_ = result;

    return result;
  }

  void clear_drift_references() override
  {
    if (is_measurement_started())
      throw Exception{Errc::board_measurement_started,
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
      throw Exception{Errc::drift_comp_refs_not_found,
        "cannot calculate drift compensation deltas because no references found"};

    // Collect the data for calculation.
    auto data = collect_channels_data(drift_samples_count, [this] {
      return Drift_affected_state_guard{*this};
    }); // strong guarantee
    PANDA_TIMESWIPE_ASSERT(refs->size() == data.column_count());

    // Discard the first half.
    data.remove_begin_rows(drift_samples_count / 2);

    // Take averages of measured data (references) and subtract the references.
    std::vector<float> result(data.column_count());
    transform(data.columns_cbegin(), data.columns_cend(),
      refs->cbegin(), result.begin(),
      [](const auto& column, const auto ref)
      {
        return static_cast<float>(dmitigr::math::avg(column) - ref);
      });

    // Cache deltas.
    drift_deltas_ = result;

    return result;
  }

  void clear_drift_deltas() override
  {
    if (is_measurement_started())
      throw Exception{Errc::board_measurement_started,
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
      throw Exception{Errc::drift_comp_refs_not_available,
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
        throw Exception{Errc::drift_comp_refs_invalid,
          std::string{"too many floating point numbers found in "}
            .append(drift_references.string())};
    }
    if (refs.size() < max_channel_count())
      throw Exception{Errc::drift_comp_refs_invalid,
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
  static constexpr microseconds switching_oscillation_period{1500};

  // Only 5ms of raw data is needed. (5ms * 48kHz = 240 values.)
  static constexpr std::size_t drift_samples_count{5*48000/1000};
  static_assert(!(drift_samples_count % 2));

  // ---------------------------------------------------------------------------
  // Basic data
  // ---------------------------------------------------------------------------

  detail::Pid_file pid_file_;
  mutable detail::Bcm_spi spi_;
  std::atomic_bool is_initialized_{};
  std::atomic_bool is_gpio_inited_{};
  std::atomic_bool is_threads_running_{};
  mutable std::atomic_bool is_measurement_started_{};

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
  Driver_settings driver_settings_;
  std::unique_ptr<Resampler> resampler_;

  // Record queue capacity must be enough to store records for 1s.
  boost::lockfree::spsc_queue<Data, boost::lockfree::capacity<48000/32*2>> record_queue_;
  std::atomic_int record_error_count_{};
  std::size_t burst_buffer_size_{};
  Data burst_buffer_;
  std::vector<std::thread> threads_;

  // ---------------------------------------------------------------------------
  // Drift compensation data
  // ---------------------------------------------------------------------------

  mutable std::optional<std::vector<float>> drift_references_;
  std::optional<std::vector<float>> drift_deltas_;

  // ---------------------------------------------------------------------------
  // RAII protectors
  // ---------------------------------------------------------------------------

  /**
   * @brief An automatic restorer of state affected by drift calculation stuff.
   *
   * @details Stashed state will be restored upon destruction of the instance of
   * this class.
   */
  class Drift_affected_state_guard final {
    friend iDriver;

    Drift_affected_state_guard(const Drift_affected_state_guard&) = delete;
    Drift_affected_state_guard& operator=(const Drift_affected_state_guard&) = delete;
    Drift_affected_state_guard(Drift_affected_state_guard&&) = delete;
    Drift_affected_state_guard& operator=(Drift_affected_state_guard&&) = delete;

    /// Calls restore().
    ~Drift_affected_state_guard()
    {
      restore();
    }

    /**
     * @brief Stores the rep state and driver settings, and prepares the board
     * for measurement.
     */
    Drift_affected_state_guard(iDriver& driver) try
      : driver_{driver}
      , resampler_{std::move(driver_.resampler_)} // store
      , driver_settings_{std::move(driver_.driver_settings_)} // settings
      , board_settings_{driver_.board_settings("basic")} // store
    {
      /*
       * Change input modes to `current`.
       * This will cause a "switching oscillation" appears at the output of
       * the measured value, which completely (according to PSpice) decays
       * after 1.5 ms.
       */
      driver_.set_settings([this]
      {
        Board_settings bs;
        const unsigned mcc = driver_.max_channel_count();
        for (unsigned i{}; i < mcc; ++i)
          bs.set_value("channel"+std::to_string(i+1)+"Mode", Measurement_mode::current);
        return bs;
      }());

      std::this_thread::sleep_for(driver_.switching_oscillation_period);

      // Set specific driver settings.
      driver_.set_settings(Driver_settings{}.set_sample_rate(48000)
        .set_burst_buffer_size(driver_.drift_samples_count));
    } catch (...) {
      restore();
    }

    /// Restores the driver state.
    void restore() noexcept
    {
      try {
        // Restore driver settings.
        driver_.set_driver_settings(std::move(driver_settings_), std::move(resampler_));

        // Restore board settings.
        driver_.set_board_settings(board_settings_);
      } catch (...) {}
    }

    iDriver& driver_;
    decltype(driver_.resampler_) resampler_;
    decltype(driver_.driver_settings_) driver_settings_;
    Board_settings board_settings_;
    std::optional<std::vector<Measurement_mode>> chmm_;
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
    unsigned tco{};
    bool pi_ok{};

    // Chunk Layout
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
      bool pi_ok{};
    };

    static Read_chunk_result read_chunk() noexcept
    {
      Read_chunk_result result;
      result.chunk[0] = read().byte;
      {
        const auto d = read();
        result.chunk[1] = d.byte;
        result.tco = d.tco;
        result.pi_ok = d.pi_ok;
      }
      for (unsigned i{2}; i < result.chunk.size(); ++i)
        result.chunk[i] = read().byte;
      return result;
    }

    static void append_chunk(Data& data,
      const Chunk& chunk,
      const std::vector<float>& slopes,
      const std::vector<int>& translation_offsets,
      const std::vector<float>& translation_slopes)
    {
      PANDA_TIMESWIPE_ASSERT((slopes.size() == translation_offsets.size())
        && (slopes.size() == translation_slopes.size()));

      std::array<std::uint16_t, detail::max_channel_count> digits{};
      static_assert(sizeof(channel_offset) == sizeof(digits[0]));
      PANDA_TIMESWIPE_ASSERT(data.column_count() <= digits.size());

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

      data.append_generated_row([&](const auto i)
      {
        const auto mv = (digits[i] - channel_offset) / slopes[i];
        return (mv - translation_offsets[i]) / translation_slopes[i];
      });
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

      const unsigned all_gpio{read_all_gpio()};
      const std::uint8_t byte =
        ((all_gpio & gpio_data_position[0]) >> 17) |  // Bit 7
        ((all_gpio & gpio_data_position[1]) >> 19) |  //     6
        ((all_gpio & gpio_data_position[2]) >>  2) |  //     5
        ((all_gpio & gpio_data_position[3]) >>  1) |  //     4
        ((all_gpio & gpio_data_position[4]) >>  3) |  //     3
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

  void spi_set_channels_adc_enabled(const bool value)
  {
    spi_.execute_set("channelsAdcEnabled", value ? "true" : "false");
  }

  bool spi_is_channels_adc_enabled() const
  {
    PANDA_TIMESWIPE_ASSERT(is_initialized());
    const auto doc = spi_.execute_get("channelsAdcEnabled");
    return rajson::Value_view{doc}.mandatory<bool>("channelsAdcEnabled");
  }

  // -----------------------------------------------------------------------------
  // Channels data reading, queueing and pushing stuff
  // -----------------------------------------------------------------------------

  /// Read records from hardware buffer.
  Data read_data()
  {
    static const auto wait_for_pi_ok = []
    {
      // Matches 12 MHz Quartz.
      std::this_thread::sleep_for(microseconds{700});
    };

    // Skip data sets if needed. (First 32 data sets are always invalid.)
    while (read_skip_count_ > 0) {
      wait_for_pi_ok();
      while (true) {
        const auto [chunk, tco, pi_ok] = Gpio_data::read_chunk();
        if (!pi_ok || tco != 0x00004000) break;
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
    Data result(max_channel_count());
    result.reserve_rows(8192);
    do {
      const auto [chunk, tco, pi_ok] = Gpio_data::read_chunk();
      Gpio_data::append_chunk(result, chunk, calibration_slopes_,
        translation_offsets_, translation_slopes_);
      if (!pi_ok || tco != 0x00004000) break;
    } while (true);

    sleep_for_55ns();
    sleep_for_55ns();

    return result;
  }

  // ---------------------------------------------------------------------------
  // Threads
  // ---------------------------------------------------------------------------

  void data_reading()
  {
    while (is_threads_running_) {
      if (const auto data = read_data(); !record_queue_.push(data))
        ++record_error_count_;
    }
  }

  void data_processing(Data_handler&& handler)
  {
    PANDA_TIMESWIPE_ASSERT(handler);
    while (is_threads_running_) {
      Data records[10];
      const auto num = record_queue_.pop(records);
      const auto errors = record_error_count_.fetch_and(0);

      if (!num) {
        std::this_thread::sleep_for(milliseconds{1});
        continue;
      }

      // If there are drift deltas substract them.
      if (drift_deltas_) {
        const auto& deltas = *drift_deltas_;
        for (std::decay_t<decltype(num)> i{}; i < num; ++i) {
          const auto channel_count = records[i].column_count();
          PANDA_TIMESWIPE_ASSERT(deltas.size() == channel_count);
          for (std::decay_t<decltype(channel_count)> j{}; j < channel_count; ++j)
            records[i].transform_column(j,
              [delta = deltas[j]](const auto value){return value - delta;});
        }
      }

      Data* records_ptr{};
      Data samples;
      if (resampler_) {
        for (std::decay_t<decltype(num)> i{}; i < num; ++i) {
          auto s = resampler_->apply(std::move(records[i]));
          samples.append_rows(std::move(s));
        }
        records_ptr = &samples;
      } else {
        for (std::decay_t<decltype(num)> i{1}; i < num; ++i)
          records[0].append_rows(std::move(records[i]));
        records_ptr = records;
      }

      // std::clog << "burst_buffer_.row_count() = " << burst_buffer_.row_count()
      //           << "burst_buffer_size_ = " << burst_buffer_size_ << std::endl;
      if (burst_buffer_.row_count() || records_ptr->row_count() < burst_buffer_size_) {
        // Go through burst buffer.
        burst_buffer_.append_rows(std::move(*records_ptr));
        if (burst_buffer_.row_count() >= burst_buffer_size_) {
          handler(std::move(burst_buffer_), errors);
          burst_buffer_ = Data(max_channel_count());
        }
      } else
        // Go directly (burst buffer not used or smaller than data).
        handler(std::move(*records_ptr), errors);
    }

    // Flush the resampler instance into the burst buffer.
    if (resampler_)
      burst_buffer_.append_rows(resampler_->flush());

    // Flush the remaining values from the burst buffer.
    if (burst_buffer_.row_count()) {
      handler(std::move(burst_buffer_), 0);
      burst_buffer_ = Data(max_channel_count());
    }
  }

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  /// @returns The channel setting from the specified board settings.
  template<typename T>
  std::optional<std::vector<T>> channel_settings(const Board_settings& bs,
    const std::string_view name)
  {
    const unsigned mcc = max_channel_count();
    std::vector<T> result(mcc);
    for (unsigned i{}; i < mcc; ++i) {
      const auto val = bs.value("channel"+std::to_string(i+1).append(name));
      if (val.has_value())
        result[i] = std::any_cast<T>(val);
      else
        return {};
    }
    return result;
  };

  /**
   * @returns Previous resampler if any.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  std::unique_ptr<Resampler> set_resampler(const int rate,
    std::unique_ptr<Resampler> resampler = {})
  {
    const auto max_rate = max_sample_rate();
    PANDA_TIMESWIPE_ASSERT(1 <= rate && rate <= max_rate);
    PANDA_TIMESWIPE_ASSERT(!is_measurement_started());
    auto result = std::move(resampler_);
    try {
      if (rate != max_rate) {
        const auto rates_gcd = std::gcd(rate, max_rate);
        const auto up = rate / rates_gcd;
        const auto down = max_rate / rates_gcd;
        if (resampler) {
          PANDA_TIMESWIPE_ASSERT(up == resampler->options().up_factor());
          PANDA_TIMESWIPE_ASSERT(down == resampler->options().down_factor());
          resampler_ = std::move(resampler);
        } else
          resampler_ = std::make_unique<Resampler>(detail::Resampler_options{}
            .set_channel_count(max_channel_count()).set_up_down(up, down));
      } else {
        PANDA_TIMESWIPE_ASSERT(!resampler);
        resampler_.reset();
      }
    } catch (...) {
      resampler_.swap(result);
      throw;
    }

    return result;
  }

  void join_threads()
  {
    is_threads_running_ = false;
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
   * @returns The collected channel data.
   *
   * @param samples_count A number of samples to collect.
   * @param state_guard A function without arguments which returns an object
   * for automatic resourse cleanup (e.g. RAII state keeper and restorer).
   */
  template<typename F>
  Data collect_channels_data(const std::size_t samples_count, const F& state_guard)
  {
    if (is_measurement_started(true))
      throw Exception{Errc::board_measurement_started,
        "cannot collect channels data because measurement is started"};

    const auto guard{state_guard()};

    std::error_condition errc;
    std::atomic_bool done{};
    Data result;
    std::condition_variable update;
    start_measurement([this, samples_count,
      &errc, &done, &result, &update](const Data data, const int)
    {
      if (errc || done)
        return;

      try {
        if (result.row_count() < samples_count)
          result.append_rows(data, samples_count - result.row_count());
      } catch (...) {
        errc = Errc::generic;
      }

      if (errc || (!done && (result.row_count() == samples_count))) {
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
      throw Exception{errc, "cannot collect channels data"};

    return result;
  }

  /// @returns Path to directory for temporary files.
  static std::filesystem::path tmp_dir()
  {
    const auto cwd = std::filesystem::current_path();
    return cwd/".panda"/"timeswipe";
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
