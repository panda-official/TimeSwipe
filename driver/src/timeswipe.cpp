#include <memory>
#include <mutex>
#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include "timeswipe.hpp"
#include "reader.hpp"


class TimeSwipeImpl {
    static std::mutex startStopMtx;
    static TimeSwipeImpl* startedInstance;
public:
    void SetBridge(int bridge);
    void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);
    void SetSensorGains(int gain1, int gain2, int gain3, int gain4);
    void SetSensorTransmissions(double trans1, double trans2, double trans3, double trans4);
    bool Start(TimeSwipe::ReadCallback);
    bool Stop();
private:
    RecordReader Rec;
    // 32 - minimal sample 48K maximal rate, next buffer is enough too keep records for 1 sec
    static const unsigned constexpr BUFFER_SIZE = 48000/32*2;
    boost::lockfree::spsc_queue<std::vector<Record>, boost::lockfree::capacity<BUFFER_SIZE>> recordBuffer;
    std::atomic_uint64_t recordErrors = 0;

    void _startFetcher();
    void _stopFetcher();
    void _fetcherLoop();
    bool _fetcherWork = false;
    std::thread _fetcherThread;
};

std::mutex TimeSwipeImpl::startStopMtx;
TimeSwipeImpl* TimeSwipeImpl::startedInstance = nullptr;

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
        if (startedInstance) {
            return false;
        }
        startedInstance = this;
    }

    Rec.setup();
    Rec.start();

    _startFetcher();

    while (true)
    {
        std::vector<Record> empty;
        std::vector<Record> records[4096];
        auto num = recordBuffer.pop(&records[0], 4096);
        uint64_t errors = recordErrors.fetch_and(0UL);
        for (size_t i = 1; i < num; i++) {
            records[0].insert(std::end(records[0]), std::begin(records[i]), std::end(records[i]));
        }
        cb(std::move(records[0]), errors);
    }

    return true;
}

bool TimeSwipeImpl::Stop() {
    {
        std::lock_guard<std::mutex> lock(startStopMtx);
        if (startedInstance != this) {
            return false;
        }
        startedInstance = nullptr;
    }
    _stopFetcher();
    Rec.stop();
    return true;
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

bool TimeSwipe::Start(TimeSwipe::ReadCallback cb) {
    return _impl->Start(cb);
}

void TimeSwipeImpl::_startFetcher() {
    _fetcherWork = true;
    _fetcherThread = std::thread(std::bind(&TimeSwipeImpl::_fetcherLoop,this));
}

void TimeSwipeImpl::_stopFetcher() {
    _fetcherWork = false;
    if(_fetcherThread.joinable())
        _fetcherThread.join();
}

void TimeSwipeImpl::_fetcherLoop() {
    while (_fetcherWork) {
        auto data = Rec.read();
        if (!recordBuffer.push(data))
            ++recordErrors;
    }
}

bool TimeSwipe::Stop() {
    return _impl->Stop();
}

