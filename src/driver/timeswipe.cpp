#include "pidfile.hpp"
#include "reader.hpp"
#include "resampler.hpp"
#include "timeswipe.hpp"
#include "timeswipe_eeprom.hpp"

#include <boost/lockfree/spsc_queue.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <stdexcept>

bool TimeSwipe::resample_log = false;

std::vector<float>& SensorsData::operator[](std::size_t num)
{
  return data_[num];
}

std::size_t SensorsData::DataSize()
{
  return data_[0].size();
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

void SensorsData::clear()
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].clear();
}

bool SensorsData::empty()
{
  return DataSize() == 0;
}

void SensorsData::append(SensorsData&& other)
{
  for (std::size_t i = 0; i < SENSORS; i++)
    std::move(other.data_[i].begin(), other.data_[i].end(), std::back_inserter(data_[i]));
  other.clear();
}

void SensorsData::erase_front(std::size_t num)
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].erase(data_[i].begin(), data_[i].begin() + num);
}

void SensorsData::erase_back(std::size_t num)
{
  for (std::size_t i = 0; i < SENSORS; i++)
    data_[i].resize(data_[i].size()-num);
}

class TimeSwipeImpl final {
private:
  inline static std::mutex mutex_;
  inline static TimeSwipeImpl* started_instance_;
  inline static constexpr int max_sample_rate_{48000};
public:
  TimeSwipeImpl();
  ~TimeSwipeImpl();
  void SetMode(int number);
  int GetMode();

  void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);
  void SetSensorGains(float gain1, float gain2, float gain3, float gain4);
  void SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4);

  bool SetSampleRate(int rate);
  bool Start(TimeSwipe::ReadCallback);
  bool onEvent(TimeSwipe::OnEventCallback cb);
  bool onError(TimeSwipe::OnErrorCallback cb);
  std::string Settings(std::uint8_t set_or_get, const std::string& request, std::string& error);
  bool Stop();

  void SetBurstSize(std::size_t burst);

private:
  struct Callbacker final {
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

  RecordReader record_reader_;
  // 32 - minimal sample 48K maximal rate, next buffer is enough too keep records for 1 sec
  static constexpr unsigned queue_size_ = 48000/32*2;
  boost::lockfree::spsc_queue<SensorsData, boost::lockfree::capacity<queue_size_>> record_queue_;
  std::atomic_uint64_t record_error_count_{0};

  SensorsData burst_buffer_;
  std::size_t burst_size_{};

  boost::lockfree::spsc_queue<std::pair<std::uint8_t, std::string>, boost::lockfree::capacity<1024>> in_spi_;
  boost::lockfree::spsc_queue<std::pair<std::string, std::string>, boost::lockfree::capacity<1024>> out_spi_;
  boost::lockfree::spsc_queue<TimeSwipeEvent, boost::lockfree::capacity<128>> events_;
  void processSPIRequests();

  void clearThreads();

  TimeSwipe::OnEventCallback on_event_cb_;
  TimeSwipe::OnErrorCallback on_error_cb_;

  bool work_{};
  bool in_callback_{};
  std::list<std::thread> threads_;

  std::unique_ptr<TimeSwipeResampler> resampler_;

  PidFile pid_file_;
};

TimeSwipeImpl::TimeSwipeImpl()
  : pid_file_{"timeswipe"}
{
  std::lock_guard<std::mutex> lock(mutex_);
  std::string err;
  if (!pid_file_.Lock(err)) {
    // Lock here. Second lock from the same process is allowed.
    std::cerr << "pid file lock failed: \"" << err << "\"" << std::endl;
    throw std::runtime_error("pid file lock failed");
  }
}

TimeSwipeImpl::~TimeSwipeImpl()
{
  Stop();
  clearThreads();
}

void TimeSwipeImpl::SetMode(int number)
{
  record_reader_.mode = number;
}

int TimeSwipeImpl::GetMode()
{
  return record_reader_.mode;
}

void TimeSwipeImpl::SetSensorOffsets(int offset1, int offset2, int offset3, int offset4)
{
  record_reader_.offset[0] = offset1;
  record_reader_.offset[1] = offset2;
  record_reader_.offset[2] = offset3;
  record_reader_.offset[3] = offset4;
}

void TimeSwipeImpl::SetSensorGains(float gain1, float gain2, float gain3, float gain4)
{
  record_reader_.gain[0] = 1.0 / gain1;
  record_reader_.gain[1] = 1.0 / gain2;
  record_reader_.gain[2] = 1.0 / gain3;
  record_reader_.gain[3] = 1.0 / gain4;
}

void TimeSwipeImpl::SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4)
{
  record_reader_.transmission[0] = 1.0 / trans1;
  record_reader_.transmission[1] = 1.0 / trans2;
  record_reader_.transmission[2] = 1.0 / trans3;
  record_reader_.transmission[3] = 1.0 / trans4;
}

bool TimeSwipeImpl::SetSampleRate(int rate)
{
  if (rate < 1 || rate > max_sample_rate_)
    return false;
  else if (rate != max_sample_rate_)
    resampler_ = std::make_unique<TimeSwipeResampler>(TimeSwipeResamplerOptions{static_cast<unsigned>(rate), max_sample_rate_});
  else
    resampler_.reset();
  return true;
}

bool TimeSwipeImpl::Start(TimeSwipe::ReadCallback cb)
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (work_ || started_instance_ || in_callback_) {
      std::cerr << "TimeSwipe already started or other instance started or called from callback function" << std::endl;
      return false;
    }
    started_instance_ = this;

    std::string err;
    if (!TimeSwipeEEPROM::Read(err)) {
      std::cerr << "EEPROM read failed: \"" << err << "\"" << std::endl;
      //TODO: uncomment once parsing implemented
      //return false;
    }
  }
  clearThreads();

  record_reader_.setup();
  record_reader_.start();

  work_ = true;
  threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::fetcherLoop, this)));
  threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::pollerLoop, this, cb)));
  threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::spiLoop, this)));
#ifdef PANDA_BUILD_FIRMWARE_EMU
  threads_.push_back(std::thread(std::bind(&TimeSwipeImpl::emulLoop, this)));
#endif
  return true;
}

bool TimeSwipeImpl::Stop()
{
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!work_ || started_instance_ != this) {
      return false;
    }
    started_instance_ = nullptr;
  }

  work_ = false;

  clearThreads();

  while (record_queue_.pop());
  while (in_spi_.pop());
  while (out_spi_.pop());
  record_reader_.stop();
  return true;
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
  impl_ = std::make_unique<TimeSwipeImpl>();
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

TimeSwipe::Mode TimeSwipe::GetMode()
{
  return TimeSwipe::Mode(impl_->GetMode());
}

void TimeSwipe::SetBurstSize(std::size_t burst)
{
  return impl_->SetBurstSize(burst);
}

bool TimeSwipe::SetSampleRate(int rate)
{
  return impl_->SetSampleRate(rate);
}

bool TimeSwipe::Start(TimeSwipe::ReadCallback cb)
{
  return impl_->Start(cb);
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
    auto data = record_reader_.read();
    if (!record_queue_.push(data))
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
    auto num = record_queue_.pop(&records[0], 10);
    std::uint64_t errors = record_error_count_.fetch_and(0UL);
    if (num == 0 && errors == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      continue;
    }

    if (errors && on_error_cb_)
      Callbacker{*this}(on_error_cb_, errors);

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
      records_ptr = &records[0];
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
