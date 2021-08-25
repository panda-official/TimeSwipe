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

namespace {
std::mutex global_mutex;
} // namespace

namespace panda::timeswipe::driver {
Version version() noexcept
{
  namespace ver = version;
  return {ver::major, ver::minor, ver::patch};
}
} // namespace panda::timeswipe::driver

// FIXME!!!
using namespace panda::timeswipe::driver;

// -----------------------------------------------------------------------------
// TimeSwipe::Rep
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
    , spi_{BcmSpi::SpiPins::kSpi0}
  {
    const std::lock_guard lock{global_mutex};
    std::string msg;
    if (!pid_file_.Lock(msg))
      // Lock here. Second lock from the same process is allowed.
      throw RuntimeException{Errc::kPidFileLockFailed};

    Init();
  }

  /**
   * Initializes GPIO pins.
   *
   * @param force Forces initialization even if IsInited() returns `true`.
   *
   * @par Effects
   * Restarts TimeSwipe firmware on very first run!
   *
   * @see IsInited(), StartMeasurement().
   */
  void Init(const bool force = false)
  {
    if (!force && IsInited()) return;

    setup_io();
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

    is_board_inited_ = true;
  }

  /**
   * @returns `true` if Init() has been successfully called at least once.
   *
   * @par Thread-safety
   * Thread-safe.
   *
   * @see Init().
   */
  bool IsInited() noexcept
  {
    return is_board_inited_;
  }

  bool Start(TimeSwipe::ReadCallback callback)
  {
    const std::unique_lock lk{global_mutex};
    return Start__(lk, std::move(callback));
  }

  bool IsBusy() const noexcept
  {
    const std::unique_lock lk{global_mutex};
    return IsBusy__(lk);
  }

  bool OnEvent(TimeSwipe::OnEventCallback cb)
  {
    if (isStarted())
      return false;
    on_event_cb_ = std::move(cb);
    return true;
  }

  bool OnError(TimeSwipe::OnErrorCallback cb)
  {
    if (isStarted())
      return false;
    on_error_cb_ = std::move(cb);
    return true;
  }

  std::string Settings(const std::uint8_t set_or_get, const std::string& request, std::string& error)
  {
    in_spi_.push(std::make_pair(set_or_get, request));
    std::pair<std::string,std::string> resp;

    if (!work_)
      processSPIRequests();

    while (!out_spi_.pop(resp))
      std::this_thread::sleep_for(std::chrono::milliseconds{100});
    error = resp.second;

    return resp.first;
  }

  bool Stop()
  {
    const std::unique_lock lk{global_mutex};
    return Stop__(lk);
  }

  bool StartPWM(const std::uint8_t num,
    const std::uint32_t frequency,
    const std::uint32_t high,
    const std::uint32_t low,
    const std::uint32_t repeats,
    const float duty_cycle)
  {
    if (num > 1) return false;
    else if (frequency < 1 || frequency > 1000) return false;
    else if (high > 4096) return false;
    else if (low > 4096) return false;
    else if (low > high) return false;
    else if (duty_cycle < 0.001 || duty_cycle > 0.999) return false;
    return SpiStartPwm(num, frequency, high, low, repeats, duty_cycle);
  }

  bool StopPWM(const std::uint8_t num)
  {
    if (num > 1) return false;
    return SpiStopPwm(num);
  }

  bool GetPWM(const std::uint8_t num,
    bool& active,
    std::uint32_t& frequency,
    std::uint32_t& high,
    std::uint32_t& low,
    std::uint32_t& repeats,
    float& duty_cycle)
  {
    if (num > 1) return false;
    return SpiGetPwm(num, active, frequency, high, low, repeats, duty_cycle);
  }

  void TraceSPI(const bool value)
  {
    SpiSetTrace(value);
  }

  bool SetChannelMode(const Channel nCh, const ChannelMesMode nMode)
  {
    return SpiSetChannelMode(static_cast<unsigned int>(nCh), static_cast<int>(nMode));
  }

  bool GetChannelMode(const Channel nCh, ChannelMesMode& nMode)
  {
    std::string strErrMsg;
    int rMode;
    const bool rv = SpiGetChannelMode(static_cast<unsigned int>(nCh), rMode, strErrMsg);
    nMode = static_cast<ChannelMesMode>(rMode);
    return rv;
  }

  bool SetChannelGain(const Channel nCh, const float gain)
  {
    return SpiSetChannelGain(static_cast<unsigned int>(nCh), gain);
  }

  bool GetChannelGain(const Channel nCh, float& gain)
  {
    std::string err;
    return SpiGetChannelGain(static_cast<unsigned int>(nCh), gain, err);
  }

  bool SetChannelIEPE(const Channel nCh, const bool bIEPEon)
  {
    return SpiSetiepe(static_cast<unsigned>(nCh), bIEPEon);
  }

  bool GetChannelIEPE(const Channel nCh, bool& bIEPEon)
  {
    std::string err;
    return SpiGetiepe(static_cast<unsigned int>(nCh), bIEPEon, err);
  }

  void SetBurstSize(const std::size_t burst)
  {
    burst_size_ = burst;
  }

  void SetMode(const Mode mode)
  {
    read_mode_ = mode;
  }

  Mode GetMode() const noexcept
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

  bool SetSampleRate(const int rate)
  {
    const std::unique_lock lk{global_mutex};
    if (!IsBusy__(lk)) {
      SetSampleRate__(rate);
      return true;
    } else
      return false;
  }

  int MaxSampleRate() const noexcept
  {
    return kMaxSampleRate_;
  }

  // ---------------------------------------------------------------------------
  // Drift Compensation
  // ---------------------------------------------------------------------------

  std::vector<float> CalculateDriftReferences()
  {
    // Collect the data for calculation.
    auto data{CollectSensorsData(kDriftSamplesCount_, // 5 ms
      [this](const auto&){return DriftAffectedStateGuard{*this};})};

    // Discard the first half.
    data.erase_front(kDriftSamplesCount_ / 2);

    // Take averages of measured data (references).
    std::vector<float> result(data.SensorsSize());
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

  void ClearDriftReferences()
  {
    const std::unique_lock lk{global_mutex};
    if (IsBusy__(lk))
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
      [this](const auto&){ return DriftAffectedStateGuard{*this}; })};
    assert(refs->size() == data.SensorsSize());

    // Discard the first half.
    data.erase_front(kDriftSamplesCount_ / 2);

    // Take averages of measured data (references) and subtract the references.
    std::vector<float> result(data.SensorsSize());
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
    const std::unique_lock lk{global_mutex};
    if (IsBusy__(lk))
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
    while (in && refs.size() < SensorsData::SensorsSize()) {
      float val;
      if (in >> val)
        refs.push_back(val);
    }
    if (!in.eof()) {
      float val;
      if (in >> val)
        throw RuntimeException{Errc::kExcessiveDriftReferences};
    }
    if (refs.size() < SensorsData::SensorsSize())
      throw RuntimeException{Errc::kInsufficientDriftReferences};

    assert(refs.size() == SensorsData::SensorsSize());

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
  constexpr static int kMinSampleRate_{32};
  // Max sample rate per second.
  constexpr static int kMaxSampleRate_{48000};

  // "Switching oscillation" completely (according to PSpice) decays after 1.5ms.
  constexpr static std::chrono::microseconds kSwitchingOscillationPeriod_{1500};

  // Only 5ms of raw data is needed. (5ms * 48kHz = 240 values.)
  constexpr static std::size_t kDriftSamplesCount_{5*kMaxSampleRate_/1000};
  static_assert(!(kDriftSamplesCount_ % 2));

  // ---------------------------------------------------------------------------
  // Resampling data
  // ---------------------------------------------------------------------------

  int sample_rate_{kMaxSampleRate_};
  std::unique_ptr<TimeSwipeResampler> resampler_;

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
  constexpr static unsigned queue_size_{kMaxSampleRate_/kMinSampleRate_*2};
  boost::lockfree::spsc_queue<SensorsData, boost::lockfree::capacity<queue_size_>> record_queue_;
  std::atomic_uint64_t record_error_count_{0};
  std::size_t burst_size_{};
  SensorsData burst_buffer_;

  boost::lockfree::spsc_queue<std::pair<std::uint8_t, std::string>, boost::lockfree::capacity<1024>> in_spi_;
  boost::lockfree::spsc_queue<std::pair<std::string, std::string>, boost::lockfree::capacity<1024>> out_spi_;
  boost::lockfree::spsc_queue<TimeSwipeEvent, boost::lockfree::capacity<128>> events_;

  std::list<std::thread> threads_;

  // ---------------------------------------------------------------------------
  // Callbacks data
  // ---------------------------------------------------------------------------

  TimeSwipe::OnEventCallback on_event_cb_;
  TimeSwipe::OnErrorCallback on_error_cb_;
  bool in_callback_{};

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

  bool work_{}; // FIXME: remove
  PidFile pid_file_;
  BcmSpi spi_;
  std::atomic_bool is_board_inited_;
  std::atomic_bool is_measurement_started_;

  inline static Rep* started_instance_;

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

    using Ch = TimeSwipe::Channel;
    using Chmm = TimeSwipe::ChannelMesMode;

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
      rep_.SetChannelMode(Ch::CH4, chmm4_);
      rep_.SetChannelMode(Ch::CH3, chmm3_);
      rep_.SetChannelMode(Ch::CH2, chmm2_);
      rep_.SetChannelMode(Ch::CH1, chmm1_);
    }

    // Stores the state and prepares TimeSwipe instance for measurement.
    DriftAffectedStateGuard(Rep& impl)
      : rep_{impl}
      , sample_rate_{rep_.sample_rate_}
      , burst_size_{rep_.burst_size_}
    {
      // Store current input modes.
      if (!(rep_.GetChannelMode(Ch::CH1, chmm1_) &&
          rep_.GetChannelMode(Ch::CH2, chmm2_) &&
          rep_.GetChannelMode(Ch::CH3, chmm3_) &&
          rep_.GetChannelMode(Ch::CH4, chmm4_)))
        throw RuntimeException{Errc::kGeneric};

      /*
       * Change input modes to 1.
       * This will cause a "switching oscillation" appears at the output of
       * the measured value, which completely (according to PSpice) decays
       * after 1.5 ms.
       */
      for (const auto m : {Ch::CH1, Ch::CH2, Ch::CH3, Ch::CH4}) {
        if (!rep_.SetChannelMode(m, TimeSwipe::ChannelMesMode::Current))
          throw RuntimeException{Errc::kGeneric};
      }
      std::this_thread::sleep_for(rep_.kSwitchingOscillationPeriod_);

      // Store the current state of self.
      resampler_ = rep_.SetSampleRate__(rep_.MaxSampleRate());
      rep_.SetBurstSize(rep_.kDriftSamplesCount_);
    }

    Rep& rep_;
    const decltype(rep_.sample_rate_) sample_rate_;
    const decltype(rep_.burst_size_) burst_size_;
    Chmm chmm1_, chmm2_, chmm3_, chmm4_;
    decltype(rep_.resampler_) resampler_;
  };

  // ---------------------------------------------------------------------------
  // GPIO
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

    static void AppendChunk(SensorsData& data,
      const Chunk& chunk,
      const std::array<std::uint16_t, 4>& offsets,
      const std::array<float, 4>& mfactors)
    {
      std::array<std::uint16_t, 4> sensors{};
      static_assert(data.SensorsSize() == 4); // KLUDGE
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

      auto& underlying_data{data.data()};
      for (std::size_t i{}; i < 4; ++i)
        underlying_data[i].push_back(static_cast<float>(sensors[i] - offsets[i]) * mfactors[i]);
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
  // SPI
  // ---------------------------------------------------------------------------

  void SpiSetTrace(const bool value)
  {
    spi_.SetTrace(value);
  }

  void SpiSetMode(const int num)
  {
    spi_.sendSetCommand("Mode", std::to_string(num));
    std::string answer;
    spi_.receiveAnswer(answer);
  }

  void SpiSetEnableADmes(const bool value)
  {
    spi_.sendSetCommand("EnableADmes", std::to_string(value));
    std::string answer;
    spi_.receiveAnswer(answer);
  }

  std::list<TimeSwipeEvent> SpiGetEvents()
  {
    const auto get_events = [this](std::string& ev)
    {
      spi_.sendEventsCommand();
      return spi_.receiveAnswer(ev);
    };

    std::list<TimeSwipeEvent> result;
    const std::lock_guard<std::mutex> lock{global_mutex};
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
            result.push_back(TimeSwipeEvent::Button(it_btn->get<bool>(), it_cnt->get<int>()));
          }
        }

        auto it = j.find("Gain");
        if (it != j.end() && it->is_number()) {
          result.push_back(TimeSwipeEvent::Gain(it->get<int>()));
        }

        it = j.find("SetSecondary");
        if (it != j.end() && it->is_number()) {
          result.push_back(TimeSwipeEvent::SetSecondary(it->get<int>()));
        }

        it = j.find("Bridge");
        if (it != j.end() && it->is_number()) {
          result.push_back(TimeSwipeEvent::Bridge(it->get<int>()));
        }

        it = j.find("Record");
        if (it != j.end() && it->is_number()) {
          result.push_back(TimeSwipeEvent::Record(it->get<int>()));
        }

        it = j.find("Offset");
        if (it != j.end() && it->is_number()) {
          result.push_back(TimeSwipeEvent::Offset(it->get<int>()));
        }

        it = j.find("Mode");
        if (it != j.end() && it->is_number()) {
          result.push_back(TimeSwipeEvent::Mode(it->get<int>()));
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
    // const std::lock_guard<std::mutex> lock{global_mutex};
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    spi_.sendGetSettingsCommand(request);
    std::string answer;
    if (!spi_.receiveAnswer(answer, error))
      error = "read SPI failed";
    return answer;
#endif
  }

  std::string SpiSetSettings(const std::string& request, std::string& error)
  {
    // const std::lock_guard<std::mutex> lock{global_mutex};
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    spi_.sendSetSettingsCommand(request);
    std::string answer;
    if (!spi_.receiveAnswer(answer, error))
      error = "read SPI failed";
    return answer;
#endif
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool SpiStartPwm(const std::uint8_t num, const std::uint32_t frequency,
    const std::uint32_t high, const std::uint32_t low, const std::uint32_t repeats,
    const float duty_cycle)
  {
    // const std::lock_guard<std::mutex> lock{global_mutex};
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(num+1);
    auto obj = nlohmann::json::object({});
    obj.emplace(pwm + ".freq", frequency);
    obj.emplace(pwm + ".high", high);
    obj.emplace(pwm + ".low", low);
    obj.emplace(pwm + ".repeats", repeats);
    obj.emplace(pwm + ".duty", duty_cycle);
    std::string err;

    auto settings = SpiSetSettings(obj.dump(), err);
    if (str2json(settings).empty())
      return false;

    obj.emplace(pwm, true);
    settings = SpiSetSettings(obj.dump(), err);

    return !str2json(settings).empty();
#endif
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool SpiStopPwm(const std::uint8_t num)
  {
    // const std::lock_guard<std::mutex> lock{global_mutex};
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(num + 1);
    /*
      sendGetCommand(pwm);
      std::string answer;
      receiveStripAnswer(answer);
      if (answer == "0") return false; // Already stopped
    */
    return spi_.sendSetCommandCheck(pwm, 0);
#endif
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool SpiGetPwm(const std::uint8_t num, bool& active, std::uint32_t& frequency,
    std::uint32_t& high, std::uint32_t& low, std::uint32_t& repeats,
    float& duty_cycle)
  {
    // const std::lock_guard<std::mutex> lock{global_mutex};
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(num+1);
    const auto arr = nlohmann::json::array({pwm, pwm + ".freq", pwm + ".high", pwm + ".low", pwm + ".repeats", pwm + ".duty"});
    std::string err;
    const auto settings = SpiGetSettings(arr.dump(), err);
    const auto s = str2json(settings);
    return !s.empty() &&
      json_get(s, pwm, active) &&
      json_get(s, pwm + ".freq", frequency) &&
      json_get(s, pwm + ".high", high) &&
      json_get(s, pwm + ".low", low) &&
      json_get(s, pwm + ".repeats", repeats) &&
      json_get(s, pwm + ".duty", duty_cycle);
#endif
  }

  bool SpiSetDACsw(const bool value)
  {
    spi_.sendSetCommand("DACsw", value ? "1" : "0");
    std::string answer;
    if (!spi_.receiveStripAnswer(answer))
      return false;
    return answer == (value ? "1" : "0");
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool SpiSetAOUT(const std::uint8_t num, const int val)
  {
    std::string var = std::string("AOUT") + (num ? "4" : "3") + ".raw";
    spi_.sendSetCommand(var, std::to_string(val));
    std::string answer;
    if (!spi_.receiveStripAnswer(answer)) return false;
    return answer == std::to_string(val);
  }

  bool SpiSetChannelMode(const unsigned num, const int nMode)
  {
    spi_.sendSetCommand(BcmSpi::makeChCmd(num, "mode"), std::to_string(nMode));
    std::string answer;
    if (!spi_.receiveStripAnswer(answer)) return false;
    return answer == std::to_string(nMode);
  }

  bool SpiGetChannelMode(const unsigned num, int& nMode, std::string& error)
  {
    spi_.sendGetCommand(BcmSpi::makeChCmd(num, "mode"));
    std::string answer;
    if (!spi_.receiveAnswer(answer, error)) {
      nMode = 0;
      return false;
    }
    nMode = std::stoi(answer);
    return true;
  }

  bool SpiSetChannelGain(const unsigned num, const float Gain)
  {
    spi_.sendSetCommand(BcmSpi::makeChCmd(num, "gain"), std::to_string(Gain));
    std::string answer;
    if (!spi_.receiveStripAnswer(answer)) return false;
    return true;
  }

  bool SpiGetChannelGain(const unsigned num, float& Gain, std::string& error)
  {
    spi_.sendGetCommand(BcmSpi::makeChCmd(num, "gain"));
    std::string answer;
    if (!spi_.receiveAnswer(answer, error)) {
      Gain = 0;
      return false;
    }
    Gain = std::stof(answer);
    return true;
  }

  bool SpiSetiepe(const unsigned num, const bool bIEPE)
  {
    spi_.sendSetCommand(BcmSpi::makeChCmd(num, "iepe"), std::to_string(bIEPE));
    std::string answer;
    if (!spi_.receiveStripAnswer(answer)) return false;
    return true;
  }

  bool SpiGetiepe(const unsigned num, bool& bIEPE, std::string& error)
  {
    spi_.sendGetCommand(BcmSpi::makeChCmd(num, "iepe"));
    std::string answer;
    if (!spi_.receiveAnswer(answer, error)) {
      bIEPE = 0;
      return false;
    }
    bIEPE = std::stoi(answer);
    return true;
  }

  // -----------------------------------------------------------------------------
  // SensorData queueing and pushing
  // -----------------------------------------------------------------------------

  /// Read records from hardware buffer.
  SensorsData ReadSensorData()
  {
    static const auto WaitForPiOk = []
    {
      // for 12MHz Quartz
      std::this_thread::sleep_for(std::chrono::microseconds(700));
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
    SensorsData out;
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
    SensorsData out;
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
    while (work_) {
      if (const auto data{ReadSensorData()}; !record_queue_.push(data))
        ++record_error_count_;

      TimeSwipeEvent event;
      while (events_.pop(event)) {
        if (on_event_cb_)
          Callbacker{*this}(on_event_cb_, std::move(event));
      }
    }
  }

  void pollerLoop(TimeSwipe::ReadCallback callback)
  {
    while (work_) {
      SensorsData records[10];
      auto num = record_queue_.pop(records);
      std::uint64_t errors = record_error_count_.fetch_and(0UL);

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
          const auto sz = records[i].SensorsSize();
          assert(deltas.size() == sz);
          for (auto j = 0*sz; j < sz; ++j) {
            auto& values{records[i][j]};
            const auto delta{deltas[j]};
            transform(cbegin(values), cend(values), begin(values),
              [delta](const auto& value) { return value - delta; });
          }
        }
      }

      SensorsData* records_ptr{};
      SensorsData samples;
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

      if (burst_buffer_.empty() && burst_size_ <= records_ptr->DataSize()) {
        // optimization if burst buffer not used or smaller than first buffer
        {
          Callbacker{*this}(callback, std::move(*records_ptr), errors);
        }
        records_ptr->clear();
      } else {
        // burst buffer mode
        burst_buffer_.append(std::move(*records_ptr));
        records_ptr->clear();
        if (burst_buffer_.DataSize() >= burst_size_) {
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
    if (!in_callback_ && burst_buffer_.DataSize()) {
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
    while (work_) {
      // Receive events
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
      if (emul_button_sent_ < emul_button_pressed_) {
        TimeSwipeEvent::Button btn(true, emul_button_pressed_);
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
    while (work_) {
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
    return cwd/".pandagmbh"/"timeswipe";
  }

  bool isStarted() const noexcept
  {
    const std::lock_guard<std::mutex> lock{global_mutex};
    return started_instance_ != nullptr;
  }

  void joinThreads()
  {
    auto it = threads_.begin();
    while (it != threads_.end()) {
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

  /// @warning Not thread-safe!
  bool IsBusy__(const std::unique_lock<std::mutex>&) const noexcept
  {
    return work_ || started_instance_ || in_callback_;
  }

  /// @warning Not thread-safe!
  bool Start__(const std::unique_lock<std::mutex>& lk, TimeSwipe::ReadCallback&& cb)
  {
    if (IsBusy__(lk)) {
      std::cerr << "TimeSwipe drift calculation/compensation or reading in progress,"
                << " or other instance started, or called from callback function."
                << std::endl;
      return false;
    }

    std::string err;
    if (!TimeSwipeEEPROM::Read(err)) {
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
      DMITIGR_CHECK(IsInited());

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

    started_instance_ = this;
    work_ = true;
    threads_.push_back(std::thread(std::bind(&Rep::fetcherLoop, this)));
    threads_.push_back(std::thread(std::bind(&Rep::pollerLoop, this, std::move(cb))));
    threads_.push_back(std::thread(std::bind(&Rep::spiLoop, this)));
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    threads_.push_back(std::thread(std::bind(&Rep::emulLoop, this)));
#endif
    return true;
  }

  /// @warning Not thread-safe!
  bool Stop__(const std::unique_lock<std::mutex>&)
  {
    if (!work_ || started_instance_ != this)
      return false;
    started_instance_ = nullptr;

    work_ = false;

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

    /// @returns Previous resampler if any.
  std::unique_ptr<TimeSwipeResampler> SetSampleRate__(const int rate)
  {
    PANDA_TIMESWIPE_CHECK(1 <= rate && rate <= MaxSampleRate());

    auto result{std::move(resampler_)};
    if (rate != MaxSampleRate()) {
      const auto rates_gcd = std::gcd(rate, MaxSampleRate());
      const auto up = rate / rates_gcd;
      const auto down = MaxSampleRate() / rates_gcd;
      resampler_ = std::make_unique<TimeSwipeResampler>(TimeSwipeResamplerOptions{up, down});
    } else
      resampler_.reset();

    sample_rate_ = rate;
    return result;
  }

  /**
   * @brief Collects the specified samples count.
   *
   * @returns The collected sensor data.
   *
   * @param samples_count A number of samples to collect.
   * @param state_guard A function which takes an argument of type
   * `std::unique_lock` and returns an object which can be used for
   * automatic resourse cleanup (e.g. RAII state keeper and restorer).
   */
  template<typename F>
  SensorsData CollectSensorsData(const std::size_t samples_count, F&& state_guard)
  {
    std::unique_lock lk{global_mutex};

    if (IsBusy__(lk))
      throw RuntimeException{Errc::kBoardIsBusy};

    const auto guard{state_guard(lk)};

    Errc errc{};
    bool done{};
    SensorsData data;
    std::condition_variable update;
    const bool is_started = Start__(lk, [this,
        samples_count, &errc, &done, &data, &update]
      (const SensorsData sd, const std::uint64_t errors)
    {
      if (IsError(errc) || done)
        return;

      try {
        if (data.DataSize() < samples_count)
          data.append(sd, samples_count - data.DataSize());
      } catch (...) {
        errc = Errc::kGeneric;
      }

      if (IsError(errc) || (!done && data.DataSize() == samples_count)) {
        done = true;
        update.notify_one();
      }
    });
    if (!is_started)
      throw RuntimeException{Errc::kGeneric}; // FIXME: Start__() throw a more detailed error instead

    // Await for notification from the callback.
    update.wait(lk, [&done]{ return done; });
    assert(done);
    Stop__(lk);

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

  static bool json_get(const nlohmann::json& j, const std::string& key, std::string& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_string()) return false;
    value = it->get<std::string>();
    return true;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, std::uint32_t& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_unsigned()) return false;
    value = it->get<uint32_t>();
    return true;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, float& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_float()) return false;
    value = it->get<float>();
    return true;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, bool& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_boolean()) return false;
    value = it->get<bool>();
    return true;
  }
};

// -----------------------------------------------------------------------------
// TimeSwipe
// -----------------------------------------------------------------------------

TimeSwipe& TimeSwipe::GetInstance()
{
  const std::lock_guard lock{global_mutex};
  if (!instance_) instance_.reset(new TimeSwipe);
  return *instance_;
}

TimeSwipe::TimeSwipe()
  : rep_{std::make_unique<Rep>()}
{}

TimeSwipe::~TimeSwipe() = default;

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

void TimeSwipe::SetMode(const Mode mode)
{
  return rep_->SetMode(mode);
}

auto TimeSwipe::GetMode() const noexcept -> Mode
{
  return rep_->GetMode();
}

int TimeSwipe::MaxSampleRate() const noexcept
{
  return rep_->MaxSampleRate();
}

void TimeSwipe::SetBurstSize(const std::size_t burst)
{
  return rep_->SetBurstSize(burst);
}

bool TimeSwipe::SetSampleRate(const int rate)
{
  return rep_->SetSampleRate(rate);
}

std::vector<float> TimeSwipe::CalculateDriftReferences()
{
  return rep_->CalculateDriftReferences();
}

void TimeSwipe::ClearDriftReferences()
{
  rep_->ClearDriftReferences();
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

bool TimeSwipe::StartPWM(const std::uint8_t num,
  const std::uint32_t frequency,
  const std::uint32_t high,
  const std::uint32_t low,
  const std::uint32_t repeats,
  const float duty_cycle)
{
  return rep_->StartPWM(num, frequency, high, low, repeats, duty_cycle);
}

bool TimeSwipe::StopPWM(const std::uint8_t num)
{
  return rep_->StopPWM(num);
}

bool TimeSwipe::GetPWM(std::uint8_t num, bool& active,
  std::uint32_t& frequency, std::uint32_t& high,
  std::uint32_t& low, std::uint32_t& repeats, float& duty_cycle)
{
  return rep_->GetPWM(num, active, frequency, high, low, repeats, duty_cycle);
}

void TimeSwipe::TraceSPI(const bool value)
{
  rep_->TraceSPI(value);
}

bool TimeSwipe::SetChannelMode(const Channel nCh, const ChannelMesMode nMode)
{
  return rep_->SetChannelMode(nCh, nMode);
}

bool TimeSwipe::GetChannelMode(Channel nCh, ChannelMesMode& nMode)
{
  return rep_->GetChannelMode(nCh, nMode);
}

bool TimeSwipe::SetChannelGain(const Channel nCh, const float gain)
{
  return rep_->SetChannelGain(nCh, gain);
}

bool TimeSwipe::GetChannelGain(const Channel nCh, float& gain)
{
  return rep_->GetChannelGain(nCh, gain);
}

bool TimeSwipe::SetChannelIEPE(const Channel nCh, const bool bIEPEon)
{
  return rep_->SetChannelIEPE(nCh, bIEPEon);
}

bool TimeSwipe::GetChannelIEPE(const Channel nCh, bool& bIEPEon)
{
  return rep_->GetChannelIEPE(nCh, bIEPEon);
}
