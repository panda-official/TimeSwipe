// -*- C++ -*-

// PANDA TimeSwipe Project
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
#include "eeprom.hpp"
#include "pidfile.hpp"
#include "resampler.hpp"
#include "spi.hpp"
#include "timeswipe.hpp"

#include "../common/error.hpp"
#include "../common/json.hpp"
#include "../common/version.hpp"

#include "../3rdparty/dmitigr/assert.hpp"
#include "../3rdparty/dmitigr/filesystem.hpp"
#include "../3rdparty/dmitigr/math.hpp"

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
#include <list>
#include <mutex>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace panda::timeswipe::driver {

Version version() noexcept
{
  namespace ver = version;
  return {ver::major, ver::minor, ver::patch};
}

// -----------------------------------------------------------------------------
// class TimeSwipe::Rep
// -----------------------------------------------------------------------------

class TimeSwipe::Rep final {
public:
  ~Rep()
  {
    Stop();
    joinThreads();
  }

  Rep()
    : pid_file_{"timeswipe"}
    , spi_{detail::Bcm_spi::Spi_pins::spi0}
  {
    std::string msg;
    if (!pid_file_.lock(msg))
      // Lock here. Second lock from the same process is allowed.
      throw RuntimeException{Errc::kPidFileLockFailed};

    InitGpio();
  }

  /**
   * Initializes GPIO pins.
   *
   * @param force Forces initialization even if IsInited() returns `true`.
   *
   * @par Effects
   * Restarts TimeSwipe firmware on very first run!
   */
  void InitGpio(const bool force = false)
  {
    if (!force && is_gpio_inited_) return;

    detail::setup_io();
    initGPIOInput(DATA0);
    initGPIOInput(DATA1);
    initGPIOInput(DATA2);
    initGPIOInput(DATA3);
    initGPIOInput(DATA4);
    initGPIOInput(DATA5);
    initGPIOInput(DATA6);
    initGPIOInput(DATA7);

    initGPIOInput(TCO);
    initGPIOInput(PI_OK);
    initGPIOInput(FAIL);
    initGPIOInput(BUTTON);

    // initGPIOOutput(PI_OK);
    initGPIOOutput(CLOCK);
    initGPIOOutput(RESET);

    // Initial Reset
    setGPIOLow(CLOCK);
    setGPIOHigh(RESET);

    using std::chrono::milliseconds;
    std::this_thread::sleep_for(milliseconds{1});

    is_gpio_inited_ = true;
  }

  bool Start(ReadCallback&& callback)
  {
    if (IsBusy()) {
      std::cerr << "TimeSwipe drift calculation/compensation or reading in progress,"
                << " or other instance started, or called from callback function."
                << std::endl;
      return false;
    }

    std::string err;
    if (!detail::Eeprom::read(err)) {
      std::cerr << "EEPROM read failed: \"" << err << "\"" << std::endl;
      //TODO: uncomment once parsing implemented
      //return false;
    }

    joinThreads();

    /*
     * Sends the command to a TimeSwipe firmware to start measurement.
     * Effects: the reader does receive the data from the board.
     */
    {
      DMITIGR_ASSERT(is_gpio_inited_);

      // Set mfactors.
      for (std::size_t i{}; i < mfactors_.size(); ++i)
        mfactors_[i] = gains_[i] * transmissions_[i];

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
      emul_point_begin_ = std::chrono::steady_clock::now();
      emul_sent_ = 0;
#endif

      // Set mode.
      SpiSetMode(static_cast<int>(read_mode_));

      // Start measurement.
      using std::chrono::milliseconds;
      std::this_thread::sleep_for(milliseconds{1});
      SpiSetEnableADmes(true);
      is_measurement_started_ = true;
    }

    threads_.emplace_back(&Rep::fetcherLoop, this);
    threads_.emplace_back(&Rep::pollerLoop, this, std::move(callback));
    threads_.emplace_back(&Rep::spiLoop, this);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    threads_.emplace_back(&Rep::emulLoop, this);
#endif
    return true;
  }

  bool IsBusy() const noexcept
  {
    return is_measurement_started_ || in_callback_;
  }

  bool OnEvent(OnEventCallback cb)
  {
    if (is_measurement_started_)
      return false;
    on_event_cb_ = std::move(cb);
    return true;
  }

  bool OnError(OnErrorCallback cb)
  {
    if (is_measurement_started_)
      return false;
    on_error_cb_ = std::move(cb);
    return true;
  }

  std::string Settings(const std::uint8_t set_or_get, const std::string& request, std::string& error)
  {
    in_spi_.push(std::make_pair(set_or_get, request));
    std::pair<std::string,std::string> resp;

    if (!is_measurement_started_)
      processSPIRequests();

    while (!out_spi_.pop(resp))
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    error = resp.second;

    return resp.first;
  }

  bool Stop()
  {
    if (!is_measurement_started_)
      return false;

    joinThreads();

    while (record_queue_.pop());
    while (in_spi_.pop());
    while (out_spi_.pop());

    /*
     * Sends the command to a TimeSwipe firmware to stop measurement.
     * Effects: the reader doesn't receive the data from the board.
     */
    {
      if (!is_measurement_started_) return false;

      // Reset Clock
      setGPIOLow(CLOCK);

      // Stop Measurement
      SpiSetEnableADmes(false);

      is_measurement_started_ = false;
      read_skip_count_ = kInitialInvalidDataSetsCount;
    }

    return true;
  }

  bool start_pwm(const int index, const Pwm_state& state)
  {
    DMITIGR_CHECK_ARG(0 <= index && index <= 1);
    return SpiStartPwm(index, state);
  }

  bool stop_pwm(const int index)
  {
    DMITIGR_CHECK_ARG(0 <= index && index <= 1);
    return SpiStopPwm(index);
  }

  std::optional<Pwm_state> pwm_state(const int index)
  {
    DMITIGR_CHECK_ARG(0 <= index && index <= 1);
    return SpiGetPwm(index);
  }

  void TraceSPI(const bool value)
  {
    SpiSetTrace(value);
  }

  bool SetChannelMode(const int channel, const Measurement_mode mode)
  {
    return SpiSetChannelMode(channel, mode);
  }

  bool GetChannelMode(const int channel, Measurement_mode& mode)
  {
    std::string err;
    return SpiGetChannelMode(channel, mode, err);
  }

  bool SetChannelGain(const int channel, const float gain)
  {
    return SpiSetChannelGain(channel, gain);
  }

  bool GetChannelGain(const int channel, float& gain)
  {
    std::string err;
    return SpiGetChannelGain(channel, gain, err);
  }

  bool SetChannelIEPE(const int channel, const bool iepe)
  {
    return SpiSetiepe(channel, iepe);
  }

  bool GetChannelIEPE(const int channel, bool& iepe)
  {
    std::string err;
    return SpiGetiepe(channel, iepe, err);
  }

  void set_burst_size(const std::size_t size)
  {
    burst_size_ = size;
  }

  void set_mode(const Mode mode)
  {
    read_mode_ = mode;
  }

  Mode mode() const noexcept
  {
    return read_mode_;
  }

  void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4)
  {
    offsets_[0] = offset1;
    offsets_[1] = offset2;
    offsets_[2] = offset3;
    offsets_[3] = offset4;
  }

  void SetSensorGains(float gain1, float gain2, float gain3, float gain4)
  {
    gains_[0] = 1.0 / gain1;
    gains_[1] = 1.0 / gain2;
    gains_[2] = 1.0 / gain3;
    gains_[3] = 1.0 / gain4;
  }

  void SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4)
  {
    transmissions_[0] = 1.0 / trans1;
    transmissions_[1] = 1.0 / trans2;
    transmissions_[2] = 1.0 / trans3;
    transmissions_[3] = 1.0 / trans4;
  }

  /// @returns Previous resampler if any.
  std::unique_ptr<detail::Resampler> set_sample_rate(const int rate)
  {
    if (IsBusy()) return {};

    PANDA_TIMESWIPE_CHECK(1 <= rate && rate <= max_sample_rate());

    auto result{std::move(resampler_)};
    if (rate != max_sample_rate()) {
      const auto rates_gcd = std::gcd(rate, max_sample_rate());
      const auto up = rate / rates_gcd;
      const auto down = max_sample_rate() / rates_gcd;
      resampler_ = std::make_unique<detail::Resampler>
        (detail::Resampler_options{up, down});
    } else
      resampler_.reset();

    sample_rate_ = rate;
    return result;
  }

  int max_sample_rate() const noexcept
  {
    return max_sample_rate_;
  }

  // ---------------------------------------------------------------------------
  // Drift Compensation
  // ---------------------------------------------------------------------------

  std::vector<float> calculate_drift_references()
  {
    // Collect the data for calculation.
    auto data{CollectSensorsData(kDriftSamplesCount_, // 5 ms
      [this]{return DriftAffectedStateGuard{*this};})};

    // Discard the first half.
    data.erase_front(kDriftSamplesCount_ / 2);

    // Take averages of measured data (references).
    std::vector<float> result(data.sensor_count());
    transform(data.cbegin(), data.cend(), result.begin(), [](const auto& dat)
    {
      return static_cast<float>(dmitigr::math::avg(dat));
    });

    // Put references to the TmpDir/drift_references.
    const auto tmp_dir{TmpDir()};
    std::filesystem::create_directories(tmp_dir);
    constexpr auto open_flags{std::ios_base::out | std::ios_base::trunc};
    std::ofstream refs_file{tmp_dir/"drift_references", open_flags};
    for (auto i = 0*result.size(); i < result.size() - 1; ++i)
      refs_file << result[i] << " ";
    refs_file << result.back() << "\n";

    // Cache references.
    drift_references_ = result;

    return result;
  }

  void clear_drift_references()
  {
    if (IsBusy())
      throw RuntimeException{Errc::kBoardIsBusy};

    std::filesystem::remove(TmpDir()/"drift_references");
    drift_references_.reset();
    drift_deltas_.reset();
  }

  std::vector<float> CalculateDriftDeltas()
  {
    // Throw away if there are no references.
    const auto refs{DriftReferences()};
    if (!refs)
      throw RuntimeException{Errc::kNoDriftReferences};

    // Collect the data for calculation.
    auto data{CollectSensorsData(kDriftSamplesCount_,
      [this]{return DriftAffectedStateGuard{*this};})};
    DMITIGR_ASSERT(refs->size() == data.sensor_count());

    // Discard the first half.
    data.erase_front(kDriftSamplesCount_ / 2);

    // Take averages of measured data (references) and subtract the references.
    std::vector<float> result(data.sensor_count());
    transform(data.cbegin(), data.cend(), refs->cbegin(), result.begin(),
      [](const auto& dat, const auto ref)
      {
        return static_cast<float>(dmitigr::math::avg(dat) - ref);
      });

    // Cache deltas.
    drift_deltas_ = result;

    return result;
  }

  void ClearDriftDeltas()
  {
    if (IsBusy())
      throw RuntimeException{Errc::kBoardIsBusy};

    drift_deltas_.reset();
  }

  std::optional<std::vector<float>> DriftReferences(const bool force = {}) const
  {
    if (!force && drift_references_)
      return drift_references_;

    const auto drift_references{TmpDir()/"drift_references"};
    if (!std::filesystem::exists(drift_references))
      return std::nullopt;

    std::ifstream in{drift_references};
    if (!in)
      throw RuntimeException{Errc::kInvalidDriftReference};

    std::vector<float> refs;
    while (in && refs.size() < Sensors_data::sensor_count()) {
      float val;
      if (in >> val)
        refs.push_back(val);
    }
    if (!in.eof()) {
      float val;
      if (in >> val)
        throw RuntimeException{Errc::kExcessiveDriftReferences};
    }
    if (refs.size() < Sensors_data::sensor_count())
      throw RuntimeException{Errc::kInsufficientDriftReferences};

    DMITIGR_ASSERT(refs.size() == Sensors_data::sensor_count());

    // Cache and return references.
    return drift_references_ = refs;
  }

  std::optional<std::vector<float>> DriftDeltas() const
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
  int read_skip_count_{kInitialInvalidDataSetsCount};
  std::array<std::uint16_t, 4> offsets_{0, 0, 0, 0};
  std::array<float, 4> gains_{1, 1, 1, 1};
  std::array<float, 4> transmissions_{1, 1, 1, 1};
  std::array<float, 4> mfactors_{};
  Mode read_mode_{};

  // ---------------------------------------------------------------------------
  // Queues data
  // ---------------------------------------------------------------------------

  // Next buffer must be enough to keep records for 1 s
  constexpr static unsigned queue_size_{max_sample_rate_/min_sample_rate_*2};
  boost::lockfree::spsc_queue<Sensors_data, boost::lockfree::capacity<queue_size_>> record_queue_;
  std::atomic_uint64_t record_error_count_{};
  std::size_t burst_size_{};
  Sensors_data burst_buffer_;

  boost::lockfree::spsc_queue<std::pair<std::uint8_t, std::string>, boost::lockfree::capacity<1024>> in_spi_;
  boost::lockfree::spsc_queue<std::pair<std::string, std::string>, boost::lockfree::capacity<1024>> out_spi_;
  boost::lockfree::spsc_queue<Event, boost::lockfree::capacity<128>> events_;

  std::list<std::thread> threads_;

  // ---------------------------------------------------------------------------
  // Callbacks data
  // ---------------------------------------------------------------------------

  OnEventCallback on_event_cb_;
  OnErrorCallback on_error_cb_;
  std::atomic_bool in_callback_{};

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
  detail::Bcm_spi spi_;
  std::atomic_bool is_gpio_inited_{};
  std::atomic_bool is_measurement_started_{};

  // ---------------------------------------------------------------------------
  // RAII protectors
  // ---------------------------------------------------------------------------

  /*
   * An automatic resetter of value of in_callback_. `false` will
   * be assigned upon destruction of the instance of this class.
   */
  struct Callbacker final {
    Callbacker(const Callbacker&) = delete;
    Callbacker& operator=(const Callbacker&) = delete;
    Callbacker(Callbacker&&) = delete;
    Callbacker& operator=(Callbacker&&) = delete;

    ~Callbacker()
    {
      self_.in_callback_ = false;
    }

    Callbacker(Rep& self) noexcept
      : self_{self}
    {}

    template<typename F, typename ... Types>
    auto operator()(const F& callback, Types&& ... args)
    {
      self_.in_callback_ = true;
      return callback(std::forward<Types>(args)...);
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

    // Restores the state of TimeSwipe instance.
    ~DriftAffectedStateGuard()
    {
      rep_.burst_size_ = burst_size_;
      rep_.sample_rate_ = sample_rate_;
      rep_.resampler_ = std::move(resampler_);

      // Restore input modes.
      rep_.SetChannelMode(4, chmm4_);
      rep_.SetChannelMode(3, chmm3_);
      rep_.SetChannelMode(2, chmm2_);
      rep_.SetChannelMode(1, chmm1_);
    }

    // Stores the state and prepares TimeSwipe instance for measurement.
    DriftAffectedStateGuard(Rep& impl)
      : rep_{impl}
      , sample_rate_{rep_.sample_rate_}
      , burst_size_{rep_.burst_size_}
    {
      // Store current input modes.
      if (!(rep_.GetChannelMode(1, chmm1_) &&
          rep_.GetChannelMode(2, chmm2_) &&
          rep_.GetChannelMode(3, chmm3_) &&
          rep_.GetChannelMode(4, chmm4_)))
        throw RuntimeException{Errc::kGeneric};

      /*
       * Change input modes to 1.
       * This will cause a "switching oscillation" appears at the output of
       * the measured value, which completely (according to PSpice) decays
       * after 1.5 ms.
       */
      for (const auto m : {1, 2, 3, 4}) {
        if (!rep_.SetChannelMode(m, Measurement_mode::Current))
          throw RuntimeException{Errc::kGeneric};
      }
      std::this_thread::sleep_for(rep_.kSwitchingOscillationPeriod_);

      // Store the current state of self.
      resampler_ = rep_.set_sample_rate(rep_.max_sample_rate());
      rep_.set_burst_size(rep_.kDriftSamplesCount_);
    }

    Rep& rep_;
    const decltype(rep_.sample_rate_) sample_rate_;
    const decltype(rep_.burst_size_) burst_size_;
    Measurement_mode chmm1_, chmm2_, chmm3_, chmm4_;
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

  static void pullGPIO(const unsigned pin, const unsigned high)
  {
    PANDA_TIMESWIPE_GPIO_PULL = high << pin;
  }

  static void initGPIOInput(const unsigned pin)
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
  }

  static void initGPIOOutput(const unsigned pin)
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
    PANDA_TIMESWIPE_OUT_GPIO(pin);
    pullGPIO(pin, 0);
  }

  static void setGPIOHigh(const unsigned pin)
  {
    PANDA_TIMESWIPE_GPIO_SET = 1 << pin;
  }

  static void setGPIOLow(const unsigned pin)
  {
    PANDA_TIMESWIPE_GPIO_CLR = 1 << pin;
  }

  static void resetAllGPIO()
  {
    PANDA_TIMESWIPE_GPIO_CLR = ALL_32_BITS_ON;
  }

  static unsigned readAllGPIO()
  {
    return (*(panda::timeswipe::driver::detail::bcm_gpio + 13) & ALL_32_BITS_ON);
  }

  static void sleep55ns()
  {
    readAllGPIO();
  }

  static void sleep8ns()
  {
    setGPIOHigh(10); // ANY UNUSED PIN!!!
  }

  struct GpioData final {
    std::uint8_t byte{};
    unsigned int tco{};
    bool piOk{};

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

    struct ReadChunkResult final {
      Chunk chunk{};
      unsigned tco{};
    };

    static ReadChunkResult ReadChunk() noexcept
    {
      ReadChunkResult result;
      result.chunk[0] = Read().byte;
      {
        const auto d{Read()};
        result.chunk[1] = d.byte;
        result.tco = d.tco;
      }
      for (unsigned i{2u}; i < result.chunk.size(); ++i)
        result.chunk[i] = Read().byte;
      return result;
    }

    static void AppendChunk(Sensors_data& data,
      const Chunk& chunk,
      const std::array<std::uint16_t, 4>& offsets,
      const std::array<float, 4>& mfactors)
    {
      std::array<std::uint16_t, 4> sensors{};
      static_assert(data.sensor_count() == 4); // KLUDGE
      using OffsetValue = std::decay_t<decltype(offsets)>::value_type;
      using SensorValue = std::decay_t<decltype(sensors)>::value_type;
      static_assert(sizeof(OffsetValue) == sizeof(SensorValue));

      constexpr auto setBit = [](std::uint16_t& word, const std::uint8_t N, const bool bit) noexcept
      {
        word = (word & ~(1UL << N)) | (bit << N);
      };
      constexpr auto getBit = [](const std::uint8_t byte, const std::uint8_t N) noexcept -> bool
      {
        return (byte & (1UL << N));
      };
      for (std::size_t i{}, count{}; i < chunk.size(); ++i) {
        setBit(sensors[0], 15 - count, getBit(chunk[i], 3));
        setBit(sensors[1], 15 - count, getBit(chunk[i], 2));
        setBit(sensors[2], 15 - count, getBit(chunk[i], 1));
        setBit(sensors[3], 15 - count, getBit(chunk[i], 0));
        count++;

        setBit(sensors[0], 15 - count, getBit(chunk[i], 7));
        setBit(sensors[1], 15 - count, getBit(chunk[i], 6));
        setBit(sensors[2], 15 - count, getBit(chunk[i], 5));
        setBit(sensors[3], 15 - count, getBit(chunk[i], 4));
        count++;
      }

      for (std::size_t i{}; i < 4; ++i)
        data[i].push_back(static_cast<float>(sensors[i] - offsets[i]) * mfactors[i]);
    }

  private:
    static GpioData Read() noexcept
    {
      setGPIOHigh(CLOCK);
      sleep55ns();
      sleep55ns();

      setGPIOLow(CLOCK);
      sleep55ns();
      sleep55ns();

      const unsigned int allGPIO{readAllGPIO()};
      const std::uint8_t byte =
        ((allGPIO & DATA_POSITION[0]) >> 17) |  // Bit 7
        ((allGPIO & DATA_POSITION[1]) >> 19) |  //     6
        ((allGPIO & DATA_POSITION[2]) >> 2) |   //     5
        ((allGPIO & DATA_POSITION[3]) >> 1) |   //     4
        ((allGPIO & DATA_POSITION[4]) >> 3) |   //     3
        ((allGPIO & DATA_POSITION[5]) >> 10) |  //     2
        ((allGPIO & DATA_POSITION[6]) >> 12) |  //     1
        ((allGPIO & DATA_POSITION[7]) >> 16);   //     0

      sleep55ns();
      sleep55ns();

      return {byte, (allGPIO & TCO_POSITION), (allGPIO & PI_STATUS_POSITION) != 0};
    }
  };

  // ---------------------------------------------------------------------------
  // SPI stuff
  // ---------------------------------------------------------------------------

  void SpiSetTrace(const bool value)
  {
    spi_.enable_tracing(value);
  }

  void SpiSetMode(const int num)
  {
    spi_.send_set_command("Mode", std::to_string(num));
    std::string answer;
    spi_.receive_answer(answer);
  }

  void SpiSetEnableADmes(const bool value)
  {
    spi_.send_set_command("EnableADmes", std::to_string(value));
    std::string answer;
    spi_.receive_answer(answer);
  }

  std::list<Event> SpiGetEvents()
  {
    const auto get_events = [this](std::string& ev)
    {
      spi_.send_events_command();
      return spi_.receive_answer(ev);
    };

    std::list<Event> result;
#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    std::string data;
    if (get_events(data) && !data.empty()) {
      if (data[data.length()-1] == 0xa ) data = data.substr(0, data.size()-1);

      if (data.empty()) return result;

      try {
        auto j = nlohmann::json::parse(data);
        auto it_btn = j.find("Button");
        if (it_btn != j.end() && it_btn->is_boolean()) {
          auto it_cnt = j.find("ButtonStateCnt");
          if (it_cnt != j.end() && it_cnt->is_number()) {
            result.push_back(Event::Button(it_btn->get<bool>(), it_cnt->get<int>()));
          }
        }

        auto it = j.find("Gain");
        if (it != j.end() && it->is_number()) {
          result.push_back(Event::Gain(it->get<int>()));
        }

        it = j.find("Set_secondary");
        if (it != j.end() && it->is_number()) {
          result.push_back(Event::Set_secondary(it->get<int>()));
        }

        it = j.find("Bridge");
        if (it != j.end() && it->is_number()) {
          result.push_back(Event::Bridge(it->get<int>()));
        }

        it = j.find("Record");
        if (it != j.end() && it->is_number()) {
          result.push_back(Event::Record(it->get<int>()));
        }

        it = j.find("Offset");
        if (it != j.end() && it->is_number()) {
          result.push_back(Event::Offset(it->get<int>()));
        }

        it = j.find("Mode");
        if (it != j.end() && it->is_number()) {
          result.push_back(Event::Mode(it->get<int>()));
        }
      }
      catch (nlohmann::json::parse_error& e)
        {
          // output exception information
          std::cerr << "readBoardEvents: json parse failed data:" << data << "error:" << e.what() << '\n';
        }
    }
#endif
    return result;
  }

  std::string SpiGetSettings(const std::string& request, std::string& error)
  {
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    spi_.send_get_settings_command(request);
    std::string answer;
    if (!spi_.receive_answer(answer, error))
      error = "read SPI failed";
    return answer;
#endif
  }

  std::string SpiSetSettings(const std::string& request, std::string& error)
  {
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    spi_.send_set_settings_command(request);
    std::string answer;
    if (!spi_.receive_answer(answer, error))
      error = "read SPI failed";
    return answer;
#endif
  }

  bool SpiStartPwm(const int index, const Pwm_state& state)
  {
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string err;
    std::string pwm = std::string("PWM") + std::to_string(index + 1);
    auto obj = nlohmann::json::object({});
    obj.emplace(pwm + ".freq", state.frequency());
    obj.emplace(pwm + ".low", state.low());
    obj.emplace(pwm + ".high", state.high());
    obj.emplace(pwm + ".repeats", state.repeat_count());
    obj.emplace(pwm + ".duty", state.duty_cycle());
    auto settings = SpiSetSettings(obj.dump(), err);
    if (str2json(settings).empty())
      return false;

    obj.emplace(pwm, true);
    settings = SpiSetSettings(obj.dump(), err);

    return !str2json(settings).empty();
#endif
  }

  bool SpiStopPwm(const int index)
  {
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(index + 1);
    /*
      sendGetCommand(pwm);
      std::string answer;
      receiveStripAnswer(answer);
      if (answer == "0") return false; // Already stopped
    */
    return spi_.send_set_command_check(pwm, 0);
#endif
  }

  std::optional<Pwm_state> SpiGetPwm(const int index)
  {
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(index + 1);
    const auto arr = nlohmann::json::array({pwm, pwm + ".freq", pwm + ".high", pwm + ".low", pwm + ".repeats", pwm + ".duty"});
    std::string err;
    const auto settings = SpiGetSettings(arr.dump(), err);
    const auto json = str2json(settings);
    if (!json.empty() && json_get_bool(json, pwm)) {
      return Pwm_state{}
        .frequency(json_get_unsigned(json, pwm + ".freq"))
        .high(json_get_unsigned(json, pwm + ".high"))
        .low(json_get_unsigned(json, pwm + ".low"))
        .repeat_count(json_get_unsigned(json, pwm + ".repeats"))
        .duty_cycle(json_get_float(json, pwm + ".duty"));
    }
    return {};
#endif
  }

  bool SpiSetDACsw(const bool value)
  {
    spi_.send_set_command("DACsw", value ? "1" : "0");
    std::string answer;
    if (!spi_.receive_strip_answer(answer))
      return false;
    return answer == (value ? "1" : "0");
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool SpiSetAOUT(const std::uint8_t num, const int val)
  {
    std::string var = std::string("AOUT") + (num ? "4" : "3") + ".raw";
    spi_.send_set_command(var, std::to_string(val));
    std::string answer;
    if (!spi_.receive_strip_answer(answer)) return false;
    return answer == std::to_string(val);
  }

  bool SpiSetChannelMode(const int channel, const Measurement_mode mode)
  {
    spi_.send_set_command(detail::Bcm_spi::make_channel_command(channel, "mode"),
      std::to_string(static_cast<int>(mode)));
    std::string answer;
    if (!spi_.receive_strip_answer(answer)) return false;
    return answer == std::to_string(static_cast<int>(mode));
  }

  bool SpiGetChannelMode(const int channel, Measurement_mode& mode, std::string& err)
  {
    spi_.send_get_command(detail::Bcm_spi::make_channel_command(channel, "mode"));
    std::string answer;
    if (!spi_.receive_answer(answer, err)) {
      mode = static_cast<Measurement_mode>(0);
      return false;
    }
    mode = static_cast<Measurement_mode>(std::stoi(answer));
    return true;
  }

  bool SpiSetChannelGain(const int channel, const float gain)
  {
    spi_.send_set_command(detail::Bcm_spi::make_channel_command(channel, "gain"),
      std::to_string(gain));
    std::string answer;
    return spi_.receive_strip_answer(answer);
  }

  bool SpiGetChannelGain(const int channel, float& gain, std::string& err)
  {
    spi_.send_get_command(detail::Bcm_spi::make_channel_command(channel, "gain"));
    std::string answer;
    if (!spi_.receive_answer(answer, err)) {
      gain = 0;
      return false;
    }
    gain = std::stof(answer);
    return true;
  }

  bool SpiSetiepe(const int channel, const bool iepe)
  {
    spi_.send_set_command(detail::Bcm_spi::make_channel_command(channel, "iepe"),
      std::to_string(iepe));
    std::string answer;
    return spi_.receive_strip_answer(answer);
  }

  bool SpiGetiepe(const int channel, bool& iepe, std::string& err)
  {
    spi_.send_get_command(detail::Bcm_spi::make_channel_command(channel, "iepe"));
    std::string answer;
    if (!spi_.receive_answer(answer, err))
      return iepe = false;
    iepe = std::stoi(answer);
    return true;
  }

  // -----------------------------------------------------------------------------
  // Sensor data reading, queueing and pushing stuff
  // -----------------------------------------------------------------------------

  /// Read records from hardware buffer.
  Sensors_data ReadSensorsData()
  {
    static const auto WaitForPiOk = []
    {
      /// Matches 12MHz Quartz.
      std::this_thread::sleep_for(std::chrono::microseconds{700});
    };

#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    // Skip data sets if needed. (First 32 data sets are always invalid.)
    while (read_skip_count_ > 0) {
      WaitForPiOk();
      while (true) {
        const auto [chunk, tco] = GpioData::ReadChunk();
        if (tco != 0x00004000) break;
      }
      --read_skip_count_;
    }

    // Wait the RAM A or RAM B becomes available for reading.
    WaitForPiOk();

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
      const auto [chunk, tco] = GpioData::ReadChunk();
      GpioData::AppendChunk(out, chunk, offsets_, mfactors_);
      if (tco != 0x00004000) break;
    } while (true);

    sleep55ns();
    sleep55ns();

    return out;
#else
    namespace chrono = std::chrono;
    Sensors_data out;
    auto& data{out.data()};
    while (true) {
      emul_point_end_ = chrono::steady_clock::now();
      const std::uint64_t diff_us{chrono::duration_cast<chrono::microseconds>
        (emul_point_end_ - emul_point_begin_).count()};
      const std::uint64_t wouldSent{diff_us * emul_rate_ / 1000 / 1000};
      if (wouldSent > emul_sent_) {
        while (emul_sent_++ < wouldSent) {
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

  void fetcherLoop()
  {
    while (is_measurement_started_) {
      if (const auto data{ReadSensorsData()}; !record_queue_.push(data))
        ++record_error_count_;

      Event event;
      while (events_.pop(event)) {
        if (on_event_cb_)
          Callbacker{*this}(on_event_cb_, std::move(event));
      }
    }
  }

  void pollerLoop(ReadCallback callback)
  {
    while (is_measurement_started_) {
      Sensors_data records[10];
      const auto num = record_queue_.pop(records);
      const std::uint64_t errors = record_error_count_.fetch_and(0UL);

      if (errors && on_error_cb_)
        Callbacker{*this}(on_error_cb_, errors);

      if (!num) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      // If there are drift deltas substract them.
      if (drift_deltas_) {
        const auto& deltas = *drift_deltas_;
        for (auto i = 0*num; i < num; ++i) {
          const auto sz = records[i].sensor_count();
          DMITIGR_ASSERT(deltas.size() == sz);
          for (auto j = 0*sz; j < sz; ++j) {
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

      if (burst_buffer_.empty() && burst_size_ <= records_ptr->size()) {
        // optimization if burst buffer not used or smaller than first buffer
        {
          Callbacker{*this}(callback, std::move(*records_ptr), errors);
        }
        records_ptr->clear();
      } else {
        // burst buffer mode
        burst_buffer_.append(std::move(*records_ptr));
        records_ptr->clear();
        if (burst_buffer_.size() >= burst_size_) {
          {
            Callbacker{*this}(callback, std::move(burst_buffer_), errors);
          }
          burst_buffer_.clear();
        }
      }
    }

    // Flush the resampler instance into the burst buffer.
    if (resampler_)
      burst_buffer_.append(resampler_->flush());

    // Flush the remaining values from the burst buffer.
    if (!in_callback_ && burst_buffer_.size()) {
      {
        Callbacker{*this}(callback, std::move(burst_buffer_), 0);
      }
      burst_buffer_.clear();
    }
  }

  // ---------------------------------------------------------------------------
  // SPI processing
  // ---------------------------------------------------------------------------

  void spiLoop()
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
      for (auto&& event: SpiGetEvents())
        events_.push(event);
#endif

      processSPIRequests();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  }

  void processSPIRequests()
  {
    std::pair<std::uint8_t, std::string> request;
    while (in_spi_.pop(request)) {
      std::string error;
      auto response = request.first ?
        SpiSetSettings(request.second, error) :
        SpiGetSettings(request.second, error);
      out_spi_.push(std::make_pair(response, error));
    }
  }

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
  int emul_button_pressed_{};
  int emul_button_sent_{};

  void emulLoop()
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
        std::cerr << "emulLoop: error select" << std::endl;
        return;
      } else if (result == -1 && errno == EINTR) {
        std::cerr << "emulLoop: EINTR select" << std::endl;
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

  static std::filesystem::path TmpDir()
  {
    const auto cwd = std::filesystem::current_path();
    return cwd/".panda"/"timeswipe";
  }

  void joinThreads()
  {
    for (auto it = threads_.begin(); it != threads_.end();) {
      if (it->get_id() == std::this_thread::get_id()) {
        ++it;
        continue;
      }
      if(it->joinable()) {
        it->join();
      }
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
  Sensors_data CollectSensorsData(const std::size_t samples_count, F&& state_guard)
  {
    if (IsBusy())
      throw RuntimeException{Errc::kBoardIsBusy};

    const auto guard{state_guard()};

    Errc errc{};
    std::atomic_bool done{};
    Sensors_data data;
    std::condition_variable update;
    const bool is_started = Start([this,
        samples_count, &errc, &done, &data, &update]
      (const Sensors_data sd, const std::uint64_t errors)
    {
      if (IsError(errc) || done)
        return;

      try {
        if (data.size() < samples_count)
          data.append(sd, samples_count - data.size());
      } catch (...) {
        errc = Errc::kGeneric;
      }

      if (IsError(errc) || (!done && data.size() == samples_count)) {
        done = true;
        update.notify_one();
      }
    });
    if (!is_started)
      throw RuntimeException{Errc::kGeneric}; // FIXME: Start() throw a more detailed error instead

    // Await for notification from the callback.
    {
      static std::mutex mutex; // Dummy mutex actually: `done` is atomic already.
      std::unique_lock lock{mutex};
      update.wait(lock, [&done]{ return done.load(); });
    }
    DMITIGR_ASSERT(done);
    Stop();

    // Throw away if the data collection failed.
    if (IsError(errc))
      throw RuntimeException{errc};

    return data;
  }

  // -------------------------------------------------------------------------
  // JSON helpers
  // -------------------------------------------------------------------------

  static nlohmann::json str2json(const std::string& str)
  {
    nlohmann::json j;
    try {
      j = nlohmann::json::parse(str);
    } catch (nlohmann::json::parse_error& e) {
      std::cerr << "Board: json parse failed data:" << str << "error:" << e.what() << '\n';
      return nlohmann::json();
    }
    return j;
  }

  static std::string json_get_string(const nlohmann::json& j, const std::string& key)
  {
    auto it = j.find(key);
    if (it == j.end())
      throw std::runtime_error{"no JSON member \"" + key + "\" found"};
    if (!it->is_string())
      throw std::runtime_error{"invalid string in JSON"};
    return it->get<std::string>();
  }

  static std::uint32_t json_get_unsigned(const nlohmann::json& j, const std::string& key)
  {
    auto it = j.find(key);
    if (it == j.end())
      throw std::runtime_error{"no JSON member \"" + key + "\" found"};
    if (!it->is_number_unsigned())
      throw std::runtime_error{"invalid unsigned in JSON"};
    return it->get<std::uint32_t>();
  }

  static float json_get_float(const nlohmann::json& j, const std::string& key)
  {
    auto it = j.find(key);
    if (it == j.end())
      throw std::runtime_error{"no JSON member \"" + key + "\" found"};
    if (!it->is_number_float())
      throw std::runtime_error{"invalid float in JSON"};
    return it->get<float>();
  }

  static bool json_get_bool(const nlohmann::json& j, const std::string& key)
  {
    auto it = j.find(key);
    if (it == j.end())
      throw std::runtime_error{"no JSON member \"" + key + "\" found"};
    if (!it->is_boolean())
      throw std::runtime_error{"invalid boolean in JSON"};
    return it->get<bool>();
  }
};

// -----------------------------------------------------------------------------
// class TimeSwipe
// -----------------------------------------------------------------------------

TimeSwipe& TimeSwipe::instance()
{
  if (!instance_) instance_.reset(new TimeSwipe);
  return *instance_;
}

TimeSwipe::TimeSwipe()
  : rep_{std::make_unique<Rep>()}
{}

TimeSwipe::~TimeSwipe() = default;

auto TimeSwipe::to_mode(const std::string_view value) -> Mode
{
  if (value == "iepe") return TimeSwipe::Mode::iepe;
  else if (value == "normal") return TimeSwipe::Mode::normal;
  else if (value == "digital") return TimeSwipe::Mode::digital;
  else throw std::invalid_argument{"invalid text representation of TimeSwipe::Mode"};
}

std::string_view TimeSwipe::to_string_view(const Mode value)
{
  switch (value) {
  case Mode::iepe: return "iepe";
  case Mode::normal: return "normal";
  case Mode::digital: return "digital";
  }
  throw std::invalid_argument{"invalid value of TimeSwipe::Mode"};
}

void TimeSwipe::SetSensorOffsets(int offset1, int offset2, int offset3, int offset4)
{
  return rep_->SetSensorOffsets(offset1, offset2, offset3, offset4);
}

void TimeSwipe::SetSensorGains(float gain1, float gain2, float gain3, float gain4)
{
  return rep_->SetSensorGains(gain1, gain2, gain3, gain4);
}

void TimeSwipe::SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4)
{
  return rep_->SetSensorTransmissions(trans1, trans2, trans3, trans4);
}

void TimeSwipe::set_mode(const Mode mode)
{
  return rep_->set_mode(mode);
}

auto TimeSwipe::mode() const noexcept -> Mode
{
  return rep_->mode();
}

int TimeSwipe::max_sample_rate() const noexcept
{
  return rep_->max_sample_rate();
}

void TimeSwipe::set_sample_rate(const int rate)
{
  rep_->set_sample_rate(rate);
}

void TimeSwipe::set_burst_size(const std::size_t size)
{
  return rep_->set_burst_size(size);
}

std::vector<float> TimeSwipe::calculate_drift_references()
{
  return rep_->calculate_drift_references();
}

void TimeSwipe::clear_drift_references()
{
  rep_->clear_drift_references();
}

std::vector<float> TimeSwipe::CalculateDriftDeltas()
{
  return rep_->CalculateDriftDeltas();
}

void TimeSwipe::ClearDriftDeltas()
{
  rep_->ClearDriftDeltas();
}

std::optional<std::vector<float>> TimeSwipe::DriftReferences(const bool force) const
{
  return rep_->DriftReferences(force);
}

std::optional<std::vector<float>> TimeSwipe::DriftDeltas() const
{
  return rep_->DriftDeltas();
}

bool TimeSwipe::Start(ReadCallback cb)
{
  return rep_->Start(std::move(cb));
}

bool TimeSwipe::IsBusy() const noexcept
{
  return rep_->IsBusy();
}

bool TimeSwipe::OnError(OnErrorCallback cb)
{
  return rep_->OnError(std::move(cb));
}

bool TimeSwipe::OnEvent(OnEventCallback cb)
{
  return rep_->OnEvent(std::move(cb));
}

std::string TimeSwipe::SetSettings(const std::string& request, std::string& error)
{
  return rep_->Settings(1, request, error);
}

std::string TimeSwipe::GetSettings(const std::string& request, std::string& error)
{
  return rep_->Settings(0, request, error);
}

bool TimeSwipe::Stop()
{
  return rep_->Stop();
}

bool TimeSwipe::start_pwm(const int index, const Pwm_state& state)
{
  return rep_->start_pwm(index, state);
}

bool TimeSwipe::stop_pwm(const int index)
{
  return rep_->stop_pwm(index);
}

std::optional<Pwm_state> TimeSwipe::pwm_state(const int index)
{
  return rep_->pwm_state(index);
}

void TimeSwipe::TraceSPI(const bool value)
{
  rep_->TraceSPI(value);
}

bool TimeSwipe::SetChannelMode(const int channel, const Measurement_mode mode)
{
  return rep_->SetChannelMode(channel, mode);
}

bool TimeSwipe::GetChannelMode(const int channel, Measurement_mode& mode)
{
  return rep_->GetChannelMode(channel, mode);
}

bool TimeSwipe::SetChannelGain(const int channel, const float gain)
{
  return rep_->SetChannelGain(channel, gain);
}

bool TimeSwipe::GetChannelGain(const int channel, float& gain)
{
  return rep_->GetChannelGain(channel, gain);
}

bool TimeSwipe::SetChannelIEPE(const int channel, const bool iepe)
{
  return rep_->SetChannelIEPE(channel, iepe);
}

bool TimeSwipe::GetChannelIEPE(const int channel, bool& iepe)
{
  return rep_->GetChannelIEPE(channel, iepe);
}

} // namespace panda::timeswipe::driver
