#include <memory>
#include <mutex>
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
    bool Start(std::function<void(std::vector<Record>)>);
    bool Stop();
private:
    RecordReader Rec;
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


bool TimeSwipeImpl::Start(std::function<void(std::vector<Record>)> cb) {
    {
        std::lock_guard<std::mutex> lock(startStopMtx);
        if (startedInstance) {
            return false;
        }
        startedInstance = this;
    }

    Rec.setup();
    Rec.start();

    while (true)
    {
        auto data = Rec.read();
        cb(std::move(data));

        // if (data.size())
        // {
        //     std::cout << data[0].Sensors[0] << "\t" << data[0].Sensors[1] << "\t" << data[0].Sensors[2] << "\t" << data[0].Sensors[3] << "\n";
        //     for (size_t i = 1; i < data.size(); ++i)
        //     {
        //         data_log << data[i].Sensors[0] << "\t" << data[i].Sensors[1] << "\t" << data[i].Sensors[2] << "\t" << data[i].Sensors[3] << "\n";
        //     }
        // }
        // std::this_thread::sleep_for(std::chrono::microseconds(100000));
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

bool TimeSwipe::Start(std::function<void(std::vector<Record>)> cb) {
    return _impl->Start(cb);
}

bool TimeSwipe::Stop() {
    return _impl->Stop();
}

