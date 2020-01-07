#include <memory>
#include <mutex>
#include <atomic>
#include <chrono>
#include <boost/lockfree/spsc_queue.hpp>
#include <future>
#include "timeswipe.hpp"
#include "reader.hpp"


class TimeSwipeImpl {
    static std::mutex startStopMtx;
    static TimeSwipeImpl* startedInstance;
public:
    ~TimeSwipeImpl();
    void SetBridge(int bridge);
    void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);
    void SetSensorGains(int gain1, int gain2, int gain3, int gain4);
    void SetSensorTransmissions(double trans1, double trans2, double trans3, double trans4);
    bool Start(TimeSwipe::ReadCallback);
    bool onButton(TimeSwipe::OnButtonCallback cb);
    bool onError(TimeSwipe::OnErrorCallback cb);
    std::string Settings(uint8_t set_or_get, const std::string& request, std::string& error);
    bool Stop();

private:
    bool _isStarted();
    void _fetcherLoop();
    void _pollerLoop(TimeSwipe::ReadCallback cb);
    void _receiveEvents(const std::chrono::steady_clock::time_point& now);
    void _waitThreads();

    RecordReader Rec;
    // 32 - minimal sample 48K maximal rate, next buffer is enough too keep records for 1 sec
    static const unsigned constexpr BUFFER_SIZE = 48000/32*2;
    boost::lockfree::spsc_queue<std::vector<Record>, boost::lockfree::capacity<BUFFER_SIZE>> recordBuffer;
    std::atomic_uint64_t recordErrors = 0;

    boost::lockfree::spsc_queue<std::pair<uint8_t,std::string>, boost::lockfree::capacity<1024>> _inSPI;
    boost::lockfree::spsc_queue<std::pair<std::string,std::string>, boost::lockfree::capacity<1024>> _outSPI;
    void _processSPIRequests();

    TimeSwipe::OnButtonCallback onButtonCb;
    TimeSwipe::OnErrorCallback onErrorCb;

    std::chrono::steady_clock::time_point _lastButtonCheck;
    bool _work = false;
    std::thread _fetcherThread;
    std::thread _pollerThread;
};

std::mutex TimeSwipeImpl::startStopMtx;
TimeSwipeImpl* TimeSwipeImpl::startedInstance = nullptr;

TimeSwipeImpl::~TimeSwipeImpl() {
    Stop();
}

void TimeSwipeImpl::SetBridge(int bridge) {
    Rec.sensorType = bridge;
}

void TimeSwipeImpl::SetSensorOffsets(int offset1, int offset2, int offset3, int offset4) {
    Rec.offset[0] = offset1;
    Rec.offset[1] = offset2;
    Rec.offset[2] = offset3;
    Rec.offset[3] = offset4;
}

void TimeSwipeImpl::SetSensorGains(int gain1, int gain2, int gain3, int gain4) {
    Rec.gain[0] = gain1;
    Rec.gain[1] = gain2;
    Rec.gain[2] = gain3;
    Rec.gain[3] = gain4;
}

void TimeSwipeImpl::SetSensorTransmissions(double trans1, double trans2, double trans3, double trans4) {
    Rec.transmission[0] = trans1;
    Rec.transmission[1] = trans2;
    Rec.transmission[2] = trans3;
    Rec.transmission[3] = trans4;
}


bool TimeSwipeImpl::Start(TimeSwipe::ReadCallback cb) {
    {
        std::lock_guard<std::mutex> lock(startStopMtx);
        if (_work || startedInstance) {
            return false;
        }
        startedInstance = this;
    }

    Rec.setup();
    Rec.start();

    _work = true;
    _fetcherThread = std::thread(std::bind(&TimeSwipeImpl::_fetcherLoop, this));
    _pollerThread = std::thread(std::bind(&TimeSwipeImpl::_pollerLoop, this, cb));
    std::thread(std::bind(&TimeSwipeImpl::_waitThreads, this)).detach();

    return true;
}

bool TimeSwipeImpl::Stop() {
    {
        std::lock_guard<std::mutex> lock(startStopMtx);
        if (!_work || startedInstance != this) {
            return false;
        }
        startedInstance = nullptr;
    }

    _work = false;
    return true;
}

void TimeSwipeImpl::_waitThreads() {
    if(_fetcherThread.joinable())
        _fetcherThread.join();
    if(_pollerThread.joinable())
        _pollerThread.join();

    while (recordBuffer.pop());
    while (_inSPI.pop());
    while (_outSPI.pop());

    Rec.stop();
}

bool TimeSwipeImpl::onButton(TimeSwipe::OnButtonCallback cb) {
    if (_isStarted()) return false;
    onButtonCb = cb;
    return true;
}

bool TimeSwipeImpl::onError(TimeSwipe::OnErrorCallback cb) {
    if (_isStarted()) return false;
    onErrorCb = cb;
    return true;
}

std::string TimeSwipeImpl::Settings(uint8_t set_or_get, const std::string& request, std::string& error) {
    _inSPI.push(std::make_pair(set_or_get, request));
    std::pair<std::string,std::string> resp;

    if (!_work) {
        _processSPIRequests();
    }

    while (!_outSPI.pop(resp)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    error = resp.second;

    return resp.first;
}

bool TimeSwipeImpl::_isStarted() {
    std::lock_guard<std::mutex> lock(startStopMtx);
    return startedInstance != nullptr;
}

void TimeSwipeImpl::_receiveEvents(const std::chrono::steady_clock::time_point& now) {
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - _lastButtonCheck).count() > 100) {
        _lastButtonCheck = now;
        auto event = readBoardEvents();
        if (event.button && onButtonCb) {
            onButtonCb(event.buttonCounter % 2, event.buttonCounter);
        }
    }
}

void TimeSwipeImpl::_processSPIRequests() {
    std::pair<uint8_t,std::string> request;
    while (_inSPI.pop(request)) {
        std::string error;
        auto response = request.first ? readBoardSetSettings(request.second, error) : readBoardGetSettings(request.second, error);
        _outSPI.push(std::make_pair(response, error));
    }
}

TimeSwipe::TimeSwipe() {
    _impl = std::make_unique<TimeSwipeImpl>();
}

TimeSwipe::~TimeSwipe() {}

void TimeSwipe::SetBridge(int bridge) {
    return _impl->SetBridge(bridge);
}

void TimeSwipe::SetSensorOffsets(int offset1, int offset2, int offset3, int offset4) {
    return _impl->SetSensorOffsets(offset1, offset2, offset3, offset4);
}

void TimeSwipe::SetSensorGains(int gain1, int gain2, int gain3, int gain4) {
    return _impl->SetSensorGains(gain1, gain2, gain3, gain4);
}

void TimeSwipe::SetSensorTransmissions(double trans1, double trans2, double trans3, double trans4) {
    return _impl->SetSensorTransmissions(trans1, trans2, trans3, trans4);
}

void TimeSwipe::Init(int bridge, int offsets[4], int gains[4], double transmissions[4]) {
    SetBridge(bridge);
    SetSensorOffsets(offsets[0], offsets[1], offsets[2], offsets[3]);
    SetSensorGains(gains[0], gains[1], gains[2], gains[3]);
    SetSensorTransmissions(transmissions[0], transmissions[1], transmissions[2], transmissions[3]);
}

void TimeSwipe::SetSecondary(int number) {
    //TODO: complete on ontegration
    SetBridge(number);
}

bool TimeSwipe::Start(TimeSwipe::ReadCallback cb) {
    return _impl->Start(cb);
}

bool TimeSwipe::onError(TimeSwipe::OnErrorCallback cb) {
    return _impl->onError(cb);
}

bool TimeSwipe::onButton(TimeSwipe::OnButtonCallback cb) {
    return _impl->onButton(cb);
}

std::string TimeSwipe::SetSettings(const std::string& request, std::string& error) {
    return _impl->Settings(1, request, error);
}

std::string TimeSwipe::GetSettings(const std::string& request, std::string& error) {
    return _impl->Settings(0, request, error);
}

void TimeSwipeImpl::_fetcherLoop() {
    while (_work) {
        auto now = std::chrono::steady_clock::now();
        auto data = Rec.read();
        if (!recordBuffer.push(data))
            ++recordErrors;
        _receiveEvents(now);
        _processSPIRequests();
    }
}

void TimeSwipeImpl::_pollerLoop(TimeSwipe::ReadCallback cb) {
    while (_work)
    {
        std::vector<Record> empty;
        std::vector<Record> records[4096];
        auto num = recordBuffer.pop(&records[0], 4096);
        uint64_t errors = recordErrors.fetch_and(0UL);
        if (num == 0 && errors == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        for (size_t i = 1; i < num; i++) {
            records[0].insert(std::end(records[0]), std::begin(records[i]), std::end(records[i]));
        }
        if (errors && onErrorCb) onErrorCb(errors);
        cb(std::move(records[0]), errors);
    }
}

bool TimeSwipe::Stop() {
    return _impl->Stop();
}

