#include "pidfile.hpp"
#include "reader.hpp"
#include "resampler.hpp"
#include "timeswipe.hpp"
#include "timeswipe_eeprom.hpp"

#include <boost/lockfree/spsc_queue.hpp>
#include <dmitigr/misc/filesystem.hpp>
#include <dmitigr/misc/math.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <list>
#include <mutex>
#include <numeric>
#include <stdexcept>
#include <thread>

// FIXME!!!
using namespace panda::timeswipe::driver;

std::vector<float>& SensorsData::operator[](std::size_t num) noexcept
{
  return data_[num];
}

std::size_t SensorsData::DataSize() const noexcept
{
  return !data_.empty() ? data_[0].size() : 0;
}

SensorsData::CONTAINER& SensorsData::data()
{
  return data_;
}

void SensorsData::reserve(std::size_t num)
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].reserve(num);
}

void SensorsData::clear() noexcept
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].clear();
}

bool SensorsData::empty() const noexcept
{
  return DataSize() == 0;
}

void SensorsData::append(const SensorsData& other)
{
  append(other, other.DataSize());
}

void SensorsData::append(const SensorsData& other, const std::size_t count)
{
  for (std::size_t i = 0; i < SENSORS; ++i) {
    const auto in_size = std::min<std::size_t>(other.data_[i].size(), count);
    const auto out_offset = data_[i].size();
    data_[i].resize(data_[i].size() + in_size);
    const auto b = other.data_[i].begin();
    std::copy(b, b + in_size, data_[i].begin() + out_offset);
  }
}

void SensorsData::erase_front(const std::size_t count) noexcept
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].erase(data_[i].begin(), data_[i].begin() + count);
}

void SensorsData::erase_back(const std::size_t count) noexcept
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].resize(data_[i].size() - count);
}

// =============================================================================

class TimeSwipeImpl final {
public:
  ~TimeSwipeImpl()
  {
    Stop();
    clearThreads();
  }

  TimeSwipeImpl(TimeSwipe& self)
    : self_{self}
    , pid_file_{"timeswipe"}
  {
    const std::lock_guard lock{mutex_};
    std::string msg;
    if (!pid_file_.Lock(msg))
      // Lock here. Second lock from the same process is allowed.
      throw Exception{Errc::kPidFileLockFailed};
  }

  void SetMode(const int number)
  {
    record_reader_.SetMode(number);
  }

  int GetMode() const noexcept
  {
    return record_reader_.Mode();
  }

  void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);
  void SetSensorGains(float gain1, float gain2, float gain3, float gain4);
  void SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4);

  bool SetSampleRate(const int rate)
  {
    const std::unique_lock lk{mutex_};
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
    const std::unique_lock lk{mutex_};
    if (IsBusy__(lk))
      throw Exception{Errc::kBoardIsBusy};

    std::filesystem::remove(TmpDir()/"drift_references");
    drift_references_.reset();
    drift_deltas_.reset();
  }

  std::vector<float> CalculateDriftDeltas()
  {
    // Throw away if there are no references.
    const auto refs{DriftReferences()};
    if (!refs)
      throw Exception{Errc::kNoDriftReferences};

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
    const std::unique_lock lk{mutex_};
    if (IsBusy__(lk))
      throw Exception{Errc::kBoardIsBusy};

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
      throw Exception{Errc::kInvalidDriftReference};

    std::vector<float> refs;
    while (in && refs.size() < SensorsData::SensorsSize()) {
      float val;
      if (in >> val)
        refs.push_back(val);
    }
    if (!in.eof()) {
      float val;
      if (in >> val)
        throw Exception{Errc::kExcessiveDriftReferences};
    }
    if (refs.size() < SensorsData::SensorsSize())
      throw Exception{Errc::kInsufficientDriftReferences};

    assert(refs.size() == SensorsData::SensorsSize());

    // Cache and return references.
    return drift_references_ = refs;
  }

  std::optional<std::vector<float>> DriftDeltas() const
  {
    return drift_deltas_;
  }

  bool Start(TimeSwipe::ReadCallback callback)
  {
    const std::unique_lock lk{mutex_};
    return Start__(lk, std::move(callback));
  }

  bool IsBusy() const noexcept
  {
    const std::unique_lock lk{mutex_};
    return IsBusy__(lk);
  }

  bool onEvent(TimeSwipe::OnEventCallback cb);
  bool onError(TimeSwipe::OnErrorCallback cb);
  std::string Settings(std::uint8_t set_or_get, const std::string& request, std::string& error);

  bool Stop()
  {
    const std::unique_lock lk{mutex_};
    return Stop__(lk);
  }

  void SetBurstSize(std::size_t burst);

private:
  // Min sample rate per second.
  constexpr static int kMinSampleRate_{32};
  // Max sample rate per second.
  constexpr static int kMaxSampleRate_{48000};

  // "Switching oscillation" completely (according to PSpice) decays after 1.5 ms.
  constexpr static std::chrono::microseconds kSwitchingOscillationPeriod_{1500};

  // We need only 5 ms of raw data. (5 ms * 48 kHz = 240 values.)
  constexpr static std::size_t kDriftSamplesCount_{5 * kMaxSampleRate_/1000};
  static_assert(!(kDriftSamplesCount_ % 2));

  inline static std::mutex mutex_;
  inline static TimeSwipeImpl* started_instance_;

  TimeSwipe& self_;

  std::size_t burst_size_{};
  int sample_rate_{kMaxSampleRate_};
  std::unique_ptr<TimeSwipeResampler> resampler_;

  mutable std::optional<std::vector<float>> drift_references_;
  std::optional<std::vector<float>> drift_deltas_;

  RecordReader record_reader_;
  // Next buffer must be enough to keep records for 1 s
  constexpr static unsigned queue_size_{kMaxSampleRate_/kMinSampleRate_*2};
  boost::lockfree::spsc_queue<SensorsData, boost::lockfree::capacity<queue_size_>> record_queue_;
  std::atomic_uint64_t record_error_count_{0};
  SensorsData burst_buffer_;

  boost::lockfree::spsc_queue<std::pair<std::uint8_t, std::string>, boost::lockfree::capacity<1024>> in_spi_;
  boost::lockfree::spsc_queue<std::pair<std::string, std::string>, boost::lockfree::capacity<1024>> out_spi_;
  boost::lockfree::spsc_queue<TimeSwipeEvent, boost::lockfree::capacity<128>> events_;

  TimeSwipe::OnEventCallback on_event_cb_;
  TimeSwipe::OnErrorCallback on_error_cb_;

  bool work_{}; // FIXME: remove
  bool in_callback_{};
  std::list<std::thread> threads_;

  PidFile pid_file_;

  // -----------------------------------------------------------------------------
  // API
  // -----------------------------------------------------------------------------

  bool isStarted();
  void fetcherLoop();
  void pollerLoop(TimeSwipe::ReadCallback cb);
  void spiLoop();
  void receiveEvents();
#ifdef PANDA_BUILD_FIRMWARE_EMU
  int emul_button_pressed_{};
  int emul_button_sent_{};
  void emulLoop();
#endif

  void processSPIRequests();
  void clearThreads();

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  struct Callbacker final {
    Callbacker(const Callbacker&) = delete;
    Callbacker& operator=(const Callbacker&) = delete;
    Callbacker(Callbacker&&) = delete;
    Callbacker& operator=(Callbacker&&) = delete;

    ~Callbacker()
    {
      self_.in_callback_ = false;
    }

    Callbacker(TimeSwipeImpl& self) noexcept
      : self_{self}
    {}

    template<typename F, typename ... Types>
    auto operator()(const F& callback, Types&& ... args)
    {
      self_.in_callback_ = true;
      return callback(std::forward<Types>(args)...);
    }

  private:
    TimeSwipeImpl& self_;
  };

  /*
   * An automatic restorer of state affected by drift calculation stuff. Stashed
   * state will be restored upon destruction of the instance of this class.
   */
  struct DriftAffectedStateGuard final {
  private:
    friend TimeSwipeImpl;
    using Ch = TimeSwipe::Channel;
    using Chmm = TimeSwipe::ChannelMesMode;

    DriftAffectedStateGuard(const DriftAffectedStateGuard&) = delete;
    DriftAffectedStateGuard& operator=(const DriftAffectedStateGuard&) = delete;
    DriftAffectedStateGuard(DriftAffectedStateGuard&&) = delete;
    DriftAffectedStateGuard& operator=(DriftAffectedStateGuard&&) = delete;

    // Restores the state of TimeSwipe instance.
    ~DriftAffectedStateGuard()
    {
      impl_.burst_size_ = burst_size_;
      impl_.sample_rate_ = sample_rate_;
      impl_.resampler_ = std::move(resampler_);

      // Restore input modes.
      impl_.self_.SetChannelMode(Ch::CH4, chmm4_);
      impl_.self_.SetChannelMode(Ch::CH3, chmm3_);
      impl_.self_.SetChannelMode(Ch::CH2, chmm2_);
      impl_.self_.SetChannelMode(Ch::CH1, chmm1_);
    }

    // Stores the state and prepares TimeSwipe instance for measurement.
    DriftAffectedStateGuard(TimeSwipeImpl& impl)
      : impl_{impl}
      , sample_rate_{impl_.sample_rate_}
      , burst_size_{impl_.burst_size_}
    {
      // Store current input modes.
      if (!(impl_.self_.GetChannelMode(Ch::CH1, chmm1_) &&
          impl_.self_.GetChannelMode(Ch::CH2, chmm2_) &&
          impl_.self_.GetChannelMode(Ch::CH3, chmm3_) &&
          impl_.self_.GetChannelMode(Ch::CH4, chmm4_)))
        throw Exception{Errc::kGeneric};

      /*
       * Change input modes to 1.
       * This will cause a "switching oscillation" appears at the output of
       * the measured value, which completely (according to PSpice) decays
       * after 1.5 ms.
       */
      for (const auto m : {Ch::CH1, Ch::CH2, Ch::CH3, Ch::CH4}) {
        if (!impl_.self_.SetChannelMode(m, TimeSwipe::ChannelMesMode::Current))
          throw Exception{Errc::kGeneric};
      }
      std::this_thread::sleep_for(impl_.kSwitchingOscillationPeriod_);

      // Store the current state of self.
      resampler_ = impl_.SetSampleRate__(impl_.MaxSampleRate());
      impl_.SetBurstSize(impl_.kDriftSamplesCount_);
    }

    TimeSwipeImpl& impl_;
    const decltype(impl_.sample_rate_) sample_rate_;
    const decltype(impl_.burst_size_) burst_size_;
    Chmm chmm1_, chmm2_, chmm3_, chmm4_;
    decltype(impl_.resampler_) resampler_;
  };

  static std::filesystem::path TmpDir()
  {
    const auto cwd = std::filesystem::current_path();
    return cwd/".pandagmbh"/"timeswipe";
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

    clearThreads();

    record_reader_.Setup();
    record_reader_.Start();

    started_instance_ = this;
    work_ = true;
    threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::fetcherLoop, this)));
    threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::pollerLoop, this, std::move(cb))));
    threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::spiLoop, this)));
#ifdef PANDA_BUILD_FIRMWARE_EMU
    threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::emulLoop, this)));
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

    clearThreads();

    while (record_queue_.pop());
    while (in_spi_.pop());
    while (out_spi_.pop());
    record_reader_.Stop();
    return true;
  }

    /// @returns Previous resampler if any.
  std::unique_ptr<TimeSwipeResampler> SetSampleRate__(const int rate)
  {
    if (!(1 <= rate && rate <= MaxSampleRate()))
      throw std::invalid_argument{"rate"};

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
    std::unique_lock lk{mutex_};

    if (IsBusy__(lk))
      throw Exception{Errc::kBoardIsBusy};

    const auto guard{state_guard(lk)};

    Errc errc{};
    bool done{};
    SensorsData data;
    std::condition_variable update;
    const bool is_started = Start__(lk, [this,
        samples_count, &errc, &done, &data, &update]
      (const SensorsData sd, const std::uint64_t errors)
    {
      if (is_error(errc) || done)
        return;

      try {
        if (data.DataSize() < samples_count)
          data.append(sd, samples_count - data.DataSize());
      } catch (...) {
        errc = Errc::kGeneric;
      }

      if (is_error(errc) || (!done && data.DataSize() == samples_count)) {
        done = true;
        update.notify_one();
      }
    });
    if (!is_started)
      throw Exception{Errc::kGeneric}; // FIXME: Start__() throw a more detailed error instead

    // Await for notification from the callback.
    update.wait(lk, [&done]{ return done; });
    assert(done);
    Stop__(lk);

    // Throw away if the data collection failed.
    if (is_error(errc))
      throw Exception{errc};

    return data;
  }
};

void TimeSwipeImpl::SetSensorOffsets(int offset1, int offset2, int offset3, int offset4)
{
  record_reader_.Offsets()[0] = offset1;
  record_reader_.Offsets()[1] = offset2;
  record_reader_.Offsets()[2] = offset3;
  record_reader_.Offsets()[3] = offset4;
}

void TimeSwipeImpl::SetSensorGains(float gain1, float gain2, float gain3, float gain4)
{
  record_reader_.Gains()[0] = 1.0 / gain1;
  record_reader_.Gains()[1] = 1.0 / gain2;
  record_reader_.Gains()[2] = 1.0 / gain3;
  record_reader_.Gains()[3] = 1.0 / gain4;
}

void TimeSwipeImpl::SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4)
{
  record_reader_.Transmissions()[0] = 1.0 / trans1;
  record_reader_.Transmissions()[1] = 1.0 / trans2;
  record_reader_.Transmissions()[2] = 1.0 / trans3;
  record_reader_.Transmissions()[3] = 1.0 / trans4;
}

bool TimeSwipeImpl::onEvent(TimeSwipe::OnEventCallback cb)
{
  if (isStarted())
    return false;
  on_event_cb_ = cb;
  return true;
}

bool TimeSwipeImpl::onError(TimeSwipe::OnErrorCallback cb)
{
  if (isStarted())
    return false;
  on_error_cb_ = cb;
  return true;
}

std::string TimeSwipeImpl::Settings(std::uint8_t set_or_get, const std::string& request, std::string& error)
{
  in_spi_.push(std::make_pair(set_or_get, request));
  std::pair<std::string,std::string> resp;

  if (!work_) {
    processSPIRequests();
  }

  while (!out_spi_.pop(resp)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  error = resp.second;

  return resp.first;
}

bool TimeSwipeImpl::isStarted()
{
  std::lock_guard<std::mutex> lock(mutex_);
  return started_instance_ != nullptr;
}

void TimeSwipeImpl::receiveEvents()
{
#ifdef PANDA_BUILD_FIRMWARE_EMU
  if (emul_button_sent_ < emul_button_pressed_) {
    TimeSwipeEvent::Button btn(true, emul_button_pressed_);
    emul_button_sent_ = emul_button_pressed_;
    events_.push(btn);
  }
#else
  for (auto&& event: readBoardEvents())
    events_.push(event);
#endif
}

void TimeSwipeImpl::processSPIRequests()
{
  std::pair<std::uint8_t,std::string> request;
  while (in_spi_.pop(request)) {
    std::string error;
    auto response = request.first ? readBoardSetSettings(request.second, error) : readBoardGetSettings(request.second, error);
    out_spi_.push(std::make_pair(response, error));
  }
}

void TimeSwipeImpl::clearThreads()
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

TimeSwipe::TimeSwipe()
{
  impl_ = std::make_unique<TimeSwipeImpl>(*this);
}

TimeSwipe::~TimeSwipe() = default;

void TimeSwipe::SetSensorOffsets(int offset1, int offset2, int offset3, int offset4)
{
  return impl_->SetSensorOffsets(offset1, offset2, offset3, offset4);
}

void TimeSwipe::SetSensorGains(float gain1, float gain2, float gain3, float gain4)
{
  return impl_->SetSensorGains(gain1, gain2, gain3, gain4);
}

void TimeSwipe::SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4)
{
  return impl_->SetSensorTransmissions(trans1, trans2, trans3, trans4);
}

void TimeSwipe::SetMode(Mode number)
{
  return impl_->SetMode(int(number));
}

TimeSwipe::Mode TimeSwipe::GetMode() const noexcept
{
  return TimeSwipe::Mode(impl_->GetMode());
}

int TimeSwipe::MaxSampleRate() const noexcept
{
  return impl_->MaxSampleRate();
}

void TimeSwipe::SetBurstSize(std::size_t burst)
{
  return impl_->SetBurstSize(burst);
}

bool TimeSwipe::SetSampleRate(const int rate)
{
  return impl_->SetSampleRate(rate);
}

std::vector<float> TimeSwipe::CalculateDriftReferences()
{
  return impl_->CalculateDriftReferences();
}

void TimeSwipe::ClearDriftReferences()
{
  impl_->ClearDriftReferences();
}

std::vector<float> TimeSwipe::CalculateDriftDeltas()
{
  return impl_->CalculateDriftDeltas();
}

void TimeSwipe::ClearDriftDeltas()
{
  impl_->ClearDriftDeltas();
}

std::optional<std::vector<float>> TimeSwipe::DriftReferences(const bool force) const
{
  return impl_->DriftReferences(force);
}

std::optional<std::vector<float>> TimeSwipe::DriftDeltas() const
{
  return impl_->DriftDeltas();
}

bool TimeSwipe::Start(TimeSwipe::ReadCallback cb)
{
  return impl_->Start(cb);
}

bool TimeSwipe::IsBusy() const noexcept
{
  return impl_->IsBusy();
}

bool TimeSwipe::onError(TimeSwipe::OnErrorCallback cb)
{
  return impl_->onError(cb);
}

bool TimeSwipe::onEvent(TimeSwipe::OnEventCallback cb)
{
  return impl_->onEvent(cb);
}

std::string TimeSwipe::SetSettings(const std::string& request, std::string& error)
{
  return impl_->Settings(1, request, error);
}

std::string TimeSwipe::GetSettings(const std::string& request, std::string& error)
{
  return impl_->Settings(0, request, error);
}

void TimeSwipeImpl::fetcherLoop()
{
  while (work_) {
    if (const auto data{record_reader_.Read()}; !record_queue_.push(data))
      ++record_error_count_;

    TimeSwipeEvent event;
    while (events_.pop(event)) {
      if (on_event_cb_)
        Callbacker{*this}(on_event_cb_, std::move(event));
    }
  }
}

void TimeSwipeImpl::spiLoop()
{
  while (work_) {
    receiveEvents();
    processSPIRequests();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
}

void TimeSwipeImpl::pollerLoop(TimeSwipe::ReadCallback callback)
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

#ifdef PANDA_BUILD_FIRMWARE_EMU
void TimeSwipeImpl::emulLoop()
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
#endif

void TimeSwipeImpl::SetBurstSize(std::size_t burst)
{
  burst_size_ = burst;
}

bool TimeSwipe::Stop()
{
  return impl_->Stop();
}

bool TimeSwipe::StartPWM(std::uint8_t num, std::uint32_t frequency,
  std::uint32_t high, std::uint32_t low,
  std::uint32_t repeats, float duty_cycle)
{
  if (num > 1) return false;
  else if (frequency < 1 || frequency > 1000) return false;
  else if (high > 4096) return false;
  else if (low > 4096) return false;
  else if (low > high) return false;
  else if (duty_cycle < 0.001 || duty_cycle > 0.999) return false;
  return BoardStartPWM(num, frequency, high, low, repeats, duty_cycle);
}

bool TimeSwipe::StopPWM(std::uint8_t num)
{
  if (num > 1) return false;
  return BoardStopPWM(num);
}

bool TimeSwipe::GetPWM(std::uint8_t num, bool& active,
  std::uint32_t& frequency, std::uint32_t& high,
  std::uint32_t& low, std::uint32_t& repeats, float& duty_cycle)
{
  if (num > 1) return false;
  return BoardGetPWM(num, active, frequency, high, low, repeats, duty_cycle);
}

void TimeSwipe::TraceSPI(bool val)
{
  BoardTraceSPI(val);
}

bool TimeSwipe::SetChannelMode(Channel nCh, ChannelMesMode nMode)
{
  return BoardInterface::get()->setChannelMode(static_cast<unsigned int>(nCh), static_cast<int>(nMode));
}

bool TimeSwipe::GetChannelMode(Channel nCh, ChannelMesMode &nMode)
{
  std::string strErrMsg;
  int rMode;
  bool rv=BoardInterface::get()->getChannelMode(static_cast<unsigned int>(nCh), rMode, strErrMsg);
  nMode=static_cast<ChannelMesMode>(rMode);
  return rv;
}

bool TimeSwipe::SetChannelGain(Channel nCh, float Gain)
{
  return BoardInterface::get()->setChannelGain(static_cast<unsigned int>(nCh), Gain);
}

bool TimeSwipe::GetChannelGain(Channel nCh, float &Gain)
{
  std::string strErrMsg;
  return BoardInterface::get()->getChannelGain(static_cast<unsigned int>(nCh), Gain, strErrMsg);
}

bool TimeSwipe::SetChannelIEPE(Channel nCh, bool bIEPEon)
{
  return BoardInterface::get()->setChannelIEPE(static_cast<unsigned int>(nCh), bIEPEon);
}

bool TimeSwipe::GetChannelIEPE(Channel nCh, bool &bIEPEon)
{
  std::string strErrMsg;
  return BoardInterface::get()->getChannelIEPE(static_cast<unsigned int>(nCh), bIEPEon, strErrMsg);
}
