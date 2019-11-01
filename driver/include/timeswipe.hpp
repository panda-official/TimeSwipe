#pragma once
#include <memory>
#include <functional>

struct Record
{
    std::array<float, 4> Sensors{0};
};

class TimeSwipeImpl;

class TimeSwipe {
public:
    TimeSwipe();
    ~TimeSwipe();
    void SetBridge(int bridge);
    void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);
    void SetSensorGains(int gain1, int gain2, int gain3, int gain4);
    void SetSensorTransmissions(double trans1, double trans2, double trans3, double trans4);
    /**
      Start reading cb called after each bundle of records received
      returns true if start succeeded, false if not
      */
    bool Start(std::function<void(std::vector<Record>)> cb);

    /**
      Stop reading
      returns true if stop succeeded, false if not
      */
    bool Stop();

private:
    std::unique_ptr<TimeSwipeImpl> _impl;

};
