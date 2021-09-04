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
#include "pidfile.hpp"
#include "resampler.hpp"
#include "spi.hpp"
#include "timeswipe.hpp"

#include "../common/error.hpp"
#include "../common/gain.hpp"
#include "../common/hat.hpp"
#include "../common/version.hpp"

#include "../3rdparty/dmitigr/assert.hpp"
#include "../3rdparty/dmitigr/filesystem.hpp"
#include "../3rdparty/dmitigr/math.hpp"
#include "../3rdparty/dmitigr/rajson.hpp"

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

namespace panda::timeswipe::driver {

Version version() noexcept
{
  namespace ver = version;
  return {ver::major, ver::minor, ver::patch};
}

// -----------------------------------------------------------------------------
// class Timeswipe::Rep
// -----------------------------------------------------------------------------

class Timeswipe::Rep final {
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
   * @param force Forces initialization even if IsInited() returns `true`.
   *
   * @par Effects
   * Restarts Timeswipe firmware on very first run!
   */
  void init_gpio(const bool force = false)
  {
    if (!force && is_gpio_inited_) return;

    detail::setup_io();
    init_gpio_input(DATA0);
    init_gpio_input(DATA1);
    init_gpio_input(DATA2);
    init_gpio_input(DATA3);
    init_gpio_input(DATA4);
    init_gpio_input(DATA5);
    init_gpio_input(DATA6);
    init_gpio_input(DATA7);

    init_gpio_input(TCO);
    init_gpio_input(PI_OK);
    init_gpio_input(FAIL);
    init_gpio_input(BUTTON);

    // init_gpio_output(PI_OK);
    init_gpio_output(CLOCK);
    init_gpio_output(RESET);

    // Initial Reset
    set_gpio_low(CLOCK);
    set_gpio_high(RESET);

    using std::chrono::milliseconds;
    std::this_thread::sleep_for(milliseconds{1});

    is_gpio_inited_ = true;
  }

  void start(Sensor_data_handler&& handler)
  {
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
        const auto state_request = R"({"cAtom":)" +
          std::to_string(static_cast<int>(ct)) + R"(})";
        const auto json_obj = spi_.execute_get_many(state_request);
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
     * Send the command to a Timeswipe firmware to start measurement.
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
    threads_.emplace_back(&Rep::poller_loop, this, std::move(handler));
    threads_.emplace_back(&Rep::events_loop, this);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    threads_.emplace_back(&Rep::emul_loop, this);
#endif
  }

  bool is_busy() const noexcept
  {
    return is_measurement_started_ || in_handler_;
  }

  void set_event_handler(Event_handler&& handler)
  {
    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    event_handler_ = std::move(handler);
  }

  void set_error_handler(Error_handler&& handler)
  {
    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    error_handler_ = std::move(handler);
  }

  void set_state(const State& state)
  {
    spi_.execute_set_many(state.to_stringified_json());
    state_.reset(); // invalidate cache (this could be optimized)
  }

  const State& state() const
  {
    if (!state_)
      state_.emplace(spi_.execute_get_many(""));
    return *state_;
  }

  void stop()
  {
    if (!is_measurement_started_) return;

    is_measurement_started_ = false;
    join_threads();

    while (record_queue_.pop());

    /*
     * Sends the command to a Timeswipe firmware to stop measurement.
     * Effects: the reader doesn't receive the data from the board.
     */
    {
      // Reset Clock
      set_gpio_low(CLOCK);

      // Stop Measurement
      spi_set_enable_ad_mes(false);

      // Reset state.
      read_skip_count_ = kInitialInvalidDataSetsCount;
    }
  }

  void set_burst_size(const std::size_t size)
  {
    burst_size_ = size;
  }

  /// @returns Previous resampler if any.
  std::unique_ptr<detail::Resampler> set_sample_rate(const int rate)
  {
    if (is_busy()) return {};

    const auto max_rate = get_max_sample_rate();
    PANDA_TIMESWIPE_CHECK(1 <= rate && rate <= max_rate);
    auto result{std::move(resampler_)};
    if (rate != max_rate) {
      const auto rates_gcd = std::gcd(rate, max_rate);
      const auto up = rate / rates_gcd;
      const auto down = max_rate / rates_gcd;
      resampler_ = std::make_unique<detail::Resampler>
        (detail::Resampler_options{up, down});
    } else
      resampler_.reset();

    sample_rate_ = rate;
    return result;
  }

  int get_max_sample_rate() const noexcept
  {
    return max_sample_rate_;
  }

  // ---------------------------------------------------------------------------
  // Drift Compensation
  // ---------------------------------------------------------------------------

  std::vector<float> calculate_drift_references()
  {
    // Collect the data for calculation.
    auto data{collect_sensors_data(kDriftSamplesCount_, // 5 ms
      [this]{return DriftAffectedStateGuard{*this};})};

    // Discard the first half.
    data.erase_front(kDriftSamplesCount_ / 2);

    // Take averages of measured data (references).
    std::vector<float> result(data.get_sensor_count());
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
    auto data{collect_sensors_data(kDriftSamplesCount_,
      [this]{return DriftAffectedStateGuard{*this};})};
    DMITIGR_ASSERT(refs->size() == data.get_sensor_count());

    // Discard the first half.
    data.erase_front(kDriftSamplesCount_ / 2);

    // Take averages of measured data (references) and subtract the references.
    std::vector<float> result(data.get_sensor_count());
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
    while (in && refs.size() < Sensors_data::get_sensor_count()) {
      float val;
      if (in >> val)
        refs.push_back(val);
    }
    if (!in.eof()) {
      float val;
      if (in >> val)
        throw Runtime_exception{Errc::excessive_drift_references};
    }
    if (refs.size() < Sensors_data::get_sensor_count())
      throw Runtime_exception{Errc::insufficient_drift_references};

    DMITIGR_ASSERT(refs.size() == Sensors_data::get_sensor_count());

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

  // Min sample rate per second.
  constexpr static int min_sample_rate_{32};
  // Max sample rate per second.
  constexpr static int max_sample_rate_{48000};

  // "Switching oscillation" completely (according to PSpice) decays after 1.5ms.
  constexpr static std::chrono::microseconds kSwitchingOscillationPeriod_{1500};

  // Only 5ms of raw data is needed. (5ms * 48kHz = 240 values.)
  constexpr static std::size_t kDriftSamplesCount_{5*max_sample_rate_/1000};
  static_assert(!(kDriftSamplesCount_ % 2));

  // ---------------------------------------------------------------------------
  // Resampling data
  // ---------------------------------------------------------------------------

  int sample_rate_{max_sample_rate_};
  std::unique_ptr<detail::Resampler> resampler_;

  // ---------------------------------------------------------------------------
  // Drift compensation data
  // ---------------------------------------------------------------------------

  mutable std::optional<std::vector<float>> drift_references_;
  std::optional<std::vector<float>> drift_deltas_;

  // ---------------------------------------------------------------------------
  // Read data
  // ---------------------------------------------------------------------------

  // The number of initial invalid data sets.
  static constexpr int kInitialInvalidDataSetsCount{32};
  static constexpr std::uint16_t k_sensor_offset{32768};
  int read_skip_count_{kInitialInvalidDataSetsCount};
  std::array<float, 4> sensor_slopes_{1, 1, 1, 1};
  std::array<float, 4> sensor_translation_offsets_{};
  std::array<float, 4> sensor_translation_slopes_{1, 1, 1, 1};
  hat::Calibration_map calibration_map_;
  mutable std::optional<State> state_;

  // ---------------------------------------------------------------------------
  // Queues data
  // ---------------------------------------------------------------------------

  // Next buffer must be enough to keep records for 1 s
  constexpr static unsigned queue_size_{max_sample_rate_/min_sample_rate_*2};
  boost::lockfree::spsc_queue<Sensors_data, boost::lockfree::capacity<queue_size_>> record_queue_;
  std::atomic_uint64_t record_error_count_{};
  std::size_t burst_size_{};
  Sensors_data burst_buffer_;
  boost::lockfree::spsc_queue<Event, boost::lockfree::capacity<128>> events_;
  std::vector<std::thread> threads_;

  // ---------------------------------------------------------------------------
  // Callbacks data
  // ---------------------------------------------------------------------------

  Event_handler event_handler_;
  Error_handler error_handler_;
  std::atomic_bool in_handler_{};

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
  // Other data
  // ---------------------------------------------------------------------------

  detail::Pid_file pid_file_;
  mutable detail::Bcm_spi spi_;
  std::atomic_bool is_gpio_inited_{};
  std::atomic_bool is_measurement_started_{};

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
  class DriftAffectedStateGuard final {
    friend Rep;

    DriftAffectedStateGuard(const DriftAffectedStateGuard&) = delete;
    DriftAffectedStateGuard& operator=(const DriftAffectedStateGuard&) = delete;
    DriftAffectedStateGuard(DriftAffectedStateGuard&&) = delete;
    DriftAffectedStateGuard& operator=(DriftAffectedStateGuard&&) = delete;

    // Restores the state of Timeswipe instance.
    ~DriftAffectedStateGuard()
    {
      rep_.burst_size_ = burst_size_;
      rep_.sample_rate_ = sample_rate_;
      rep_.resampler_ = std::move(resampler_);

      // Restore input modes.
      State state;
      for (int i{}; i < chmm_.size(); ++i)
        state.set_channel_measurement_mode(i, chmm_[i]);
      rep_.set_state(state);
    }

    // Stores the state and prepares Timeswipe instance for measurement.
    DriftAffectedStateGuard(Rep& impl)
      : rep_{impl}
      , sample_rate_{rep_.sample_rate_}
      , burst_size_{rep_.burst_size_}
    {
      // Store current input modes.
      const auto channel_mode = [this](const int index)
      {
        if (const auto mm = rep_.state().channel_measurement_mode(index))
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
        State state;
        for (int i{}; i < chmm_.size(); ++i)
          state.set_channel_measurement_mode(i, Measurement_mode::Current);
        rep_.set_state(state);
      }

      std::this_thread::sleep_for(rep_.kSwitchingOscillationPeriod_);

      // Store the other current state of rep_.
      resampler_ = rep_.set_sample_rate(rep_.get_max_sample_rate());
      rep_.set_burst_size(rep_.kDriftSamplesCount_);
    }

    Rep& rep_;
    const decltype(rep_.sample_rate_) sample_rate_;
    const decltype(rep_.burst_size_) burst_size_;
    std::array<Measurement_mode, 4> chmm_;
    decltype(rep_.resampler_) resampler_;
  };

  // ---------------------------------------------------------------------------
  // GPIO stuff
  // ---------------------------------------------------------------------------

  // PIN NAMES
  static constexpr std::uint8_t DATA0{24};  // BCM 24 - PIN 18
  static constexpr std::uint8_t DATA1{25};  // BCM 25 - PIN 22
  static constexpr std::uint8_t DATA2{7};   // BCM  7 - PIN 26
  static constexpr std::uint8_t DATA3{5};   // BCM  5 - PIN 29
  static constexpr std::uint8_t DATA4{6};   // BCM  6 - PIN 31
  static constexpr std::uint8_t DATA5{12};  // BCM 12 - PIN 32
  static constexpr std::uint8_t DATA6{13};  // BCM 13 - PIN 33
  static constexpr std::uint8_t DATA7{16};  // BCM 16 - PIN 36
  static constexpr std::uint8_t CLOCK{4};   // BCM  4 - PIN  7
  static constexpr std::uint8_t TCO{14};    // BCM 14 - PIN  8
  static constexpr std::uint8_t PI_OK{15};  // BCM 15 - PIN 10
  static constexpr std::uint8_t FAIL{18};   // BCM 18 - PIN 12
  static constexpr std::uint8_t RESET{17};  // BCM 17 - PIN 11
  static constexpr std::uint8_t BUTTON{25}; // BCM 25 - PIN 22

  static constexpr std::array<std::uint32_t, 8> DATA_POSITION{
    std::uint32_t{1} << DATA0,
    std::uint32_t{1} << DATA1,
    std::uint32_t{1} << DATA2,
    std::uint32_t{1} << DATA3,
    std::uint32_t{1} << DATA4,
    std::uint32_t{1} << DATA5,
    std::uint32_t{1} << DATA6,
    std::uint32_t{1} << DATA7
  };

  static constexpr std::uint32_t CLOCK_POSITION{std::uint32_t{1} << CLOCK};
  static constexpr std::uint32_t TCO_POSITION{std::uint32_t{1} << TCO};
  static constexpr std::uint32_t PI_STATUS_POSITION{std::uint32_t{1} << PI_OK};
  static constexpr std::uint32_t FAIL_POSITION{std::uint32_t{1} << FAIL};
  static constexpr std::uint32_t BUTTON_POSITION{std::uint32_t{1} << BUTTON};

  // (2^32)-1 - ALL BCM_PINS
  static constexpr std::uint32_t ALL_32_BITS_ON{0xFFFFFFFF};

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
    PANDA_TIMESWIPE_GPIO_CLR = ALL_32_BITS_ON;
  }

  static unsigned read_all_gpio()
  {
    return (*(panda::timeswipe::driver::detail::bcm_gpio + 13) & ALL_32_BITS_ON);
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

    static void append_chunk(Sensors_data& data,
      const Chunk& chunk,
      const std::array<float, 4>& slopes,
      const std::array<float, 4>& translation_offsets,
      const std::array<float, 4>& translation_slopes)
    {
      std::array<std::uint16_t, 4> sensors{};
      static_assert(sizeof(k_sensor_offset) == sizeof(sensors[0]));
      static_assert(slopes.size() == sensors.size());
      static_assert(translation_offsets.size() == sensors.size());
      static_assert(translation_slopes.size() == sensors.size());

      const auto data_size = data.size();
      DMITIGR_ASSERT(data_size <= sensors.size());

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

      for (std::size_t i{}; i < data_size; ++i) {
        const auto mv = (sensors[i] - k_sensor_offset) * slopes[i];
        const auto unit = (mv - translation_offsets[i]) * translation_slopes[i];
        data[i].push_back(unit);
      }
    }

  private:
    static Gpio_data read() noexcept
    {
      set_gpio_high(CLOCK);
      sleep_for_55ns();
      sleep_for_55ns();

      set_gpio_low(CLOCK);
      sleep_for_55ns();
      sleep_for_55ns();

      const unsigned int all_gpio{read_all_gpio()};
      const std::uint8_t byte =
        ((all_gpio & DATA_POSITION[0]) >> 17) |  // Bit 7
        ((all_gpio & DATA_POSITION[1]) >> 19) |  //     6
        ((all_gpio & DATA_POSITION[2]) >> 2) |   //     5
        ((all_gpio & DATA_POSITION[3]) >> 1) |   //     4
        ((all_gpio & DATA_POSITION[4]) >> 3) |   //     3
        ((all_gpio & DATA_POSITION[5]) >> 10) |  //     2
        ((all_gpio & DATA_POSITION[6]) >> 12) |  //     1
        ((all_gpio & DATA_POSITION[7]) >> 16);   //     0

      sleep_for_55ns();
      sleep_for_55ns();

      return {byte, (all_gpio & TCO_POSITION), (all_gpio & PI_STATUS_POSITION) != 0};
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
  Sensors_data read_sensors_data()
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
    Sensors_data out;
    out.reserve(8192);
    do {
      const auto [chunk, tco] = Gpio_data::read_chunk();
      Gpio_data::append_chunk(out, chunk, sensor_slopes_,
        sensor_translation_offsets_, sensor_translation_slopes_);
      if (tco != 0x00004000) break;
    } while (true);

    sleep_for_55ns();
    sleep_for_55ns();

    return out;
#else
    namespace chrono = std::chrono;
    Sensors_data out;
    auto& data{out.data()};
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
    return out;
#endif
  }

  void fetcher_loop()
  {
    while (is_measurement_started_) {
      if (const auto data = read_sensors_data(); !record_queue_.push(data))
        ++record_error_count_;

      Event event;
      while (events_.pop(event)) {
        if (event_handler_)
          Callbacker{*this}(event_handler_, std::move(event));
      }
    }
  }

  void poller_loop(Sensor_data_handler handler)
  {
    while (is_measurement_started_) {
      Sensors_data records[10];
      const auto num = record_queue_.pop(records);
      const std::uint64_t errors = record_error_count_.fetch_and(0UL);

      if (errors && error_handler_)
        Callbacker{*this}(error_handler_, errors);

      if (!num) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        continue;
      }

      // If there are drift deltas substract them.
      if (drift_deltas_) {
        const auto& deltas = *drift_deltas_;
        for (auto i = 0*num; i < num; ++i) {
          const auto sc = records[i].get_sensor_count();
          DMITIGR_ASSERT(deltas.size() == sc);
          for (std::size_t j{}; j < sc; ++j) {
            auto& values{records[i][j]};
            const auto delta{deltas[j]};
            transform(cbegin(values), cend(values), begin(values),
              [delta](const auto& value) { return value - delta; });
          }
        }
      }

      Sensors_data* records_ptr{};
      Sensors_data samples;
      if (resampler_) {
        for (std::size_t i = 0; i < num; i++) {
          auto s = resampler_->apply(std::move(records[i]));
          samples.append(std::move(s));
        }
        records_ptr = &samples;
      } else {
        for (std::size_t i = 1; i < num; i++) {
          records[0].append(std::move(records[i]));
        }
        records_ptr = records;
      }

      if (burst_buffer_.empty() && burst_size_ <= records_ptr->get_size()) {
        // optimization if burst buffer not used or smaller than first buffer
        {
          Callbacker{*this}(handler, std::move(*records_ptr), errors);
        }
        records_ptr->clear();
      } else {
        // burst buffer mode
        burst_buffer_.append(std::move(*records_ptr));
        records_ptr->clear();
        if (burst_buffer_.get_size() >= burst_size_) {
          {
            Callbacker{*this}(handler, std::move(burst_buffer_), errors);
          }
          burst_buffer_.clear();
        }
      }
    }

    // Flush the resampler instance into the burst buffer.
    if (resampler_)
      burst_buffer_.append(resampler_->flush());

    // Flush the remaining values from the burst buffer.
    if (!in_handler_ && burst_buffer_.get_size()) {
      {
        Callbacker{*this}(handler, std::move(burst_buffer_), 0);
      }
      burst_buffer_.clear();
    }
  }

  // ---------------------------------------------------------------------------
  // Events processing
  // ---------------------------------------------------------------------------

  void events_loop()
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
      for (auto&& event : spi_get_events())
        events_.push(std::move(event));
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
  Sensors_data collect_sensors_data(const std::size_t samples_count, F&& state_guard)
  {
    if (is_busy())
      throw Runtime_exception{Errc::board_is_busy};

    const auto guard{state_guard()};

    Errc errc{};
    std::atomic_bool done{};
    Sensors_data data;
    std::condition_variable update;
    start([this, samples_count, &errc, &done, &data, &update]
      (const Sensors_data sd, const std::uint64_t errors)
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
// class Timeswipe
// -----------------------------------------------------------------------------

Timeswipe& Timeswipe::instance()
{
  if (!instance_) instance_.reset(new Timeswipe);
  return *instance_;
}

Timeswipe::Timeswipe()
  : rep_{std::make_unique<Rep>()}
{}

Timeswipe::~Timeswipe() = default;

int Timeswipe::get_max_sample_rate() const noexcept
{
  return rep_->get_max_sample_rate();
}

void Timeswipe::set_sample_rate(const int rate)
{
  rep_->set_sample_rate(rate);
}

void Timeswipe::set_burst_size(const std::size_t size)
{
  return rep_->set_burst_size(size);
}

std::vector<float> Timeswipe::calculate_drift_references()
{
  return rep_->calculate_drift_references();
}

void Timeswipe::clear_drift_references()
{
  rep_->clear_drift_references();
}

std::vector<float> Timeswipe::calculate_drift_deltas()
{
  return rep_->calculate_drift_deltas();
}

void Timeswipe::clear_drift_deltas()
{
  rep_->clear_drift_deltas();
}

std::optional<std::vector<float>> Timeswipe::get_drift_references(const bool force) const
{
  return rep_->get_drift_references(force);
}

std::optional<std::vector<float>> Timeswipe::get_drift_deltas() const
{
  return rep_->get_drift_deltas();
}

void Timeswipe::start(Sensor_data_handler handler)
{
  rep_->start(std::move(handler));
}

bool Timeswipe::is_busy() const noexcept
{
  return rep_->is_busy();
}

void Timeswipe::stop()
{
  return rep_->stop();
}

void Timeswipe::set_event_handler(Event_handler&& handler)
{
  return rep_->set_event_handler(std::move(handler));
}

void Timeswipe::set_error_handler(Error_handler&& handler)
{
  return rep_->set_error_handler(std::move(handler));
}

void Timeswipe::set_state(const State& state)
{
  return rep_->set_state(state);
}

auto Timeswipe::state() const -> const State&
{
  return rep_->state();
}

} // namespace panda::timeswipe::driver
