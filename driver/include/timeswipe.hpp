/**
 * \file
 * \brief Timeswipe cpp library
 */
#pragma once
#include <memory>
#include <functional>

/**
 * \brief Sensor read primitive
 */
struct Record
{
    std::array<float, 4> Sensors{0};
};

class TimeSwipeImpl;

/**
 * TimeSwipe interface for Sensor
 */
class TimeSwipe {
public:
    TimeSwipe();
    ~TimeSwipe();
    /**
     * \brief Setup bridge number
     *
     * It is mandatory to setup the bridge before @ref Start
     *
     * @param bridge bridge number
     */
    void SetBridge(int bridge);

    /**
     * \brief Setup Sensor offsets
     *
     * It is mandatory to setup offsets before @ref Start
     *
     * @param offset1
     * @param offset2
     * @param offset3
     * @param offset4
     */
    void SetSensorOffsets(int offset1, int offset2, int offset3, int offset4);

    /**
     * \brief Setup Sensor gains
     *
     * It is mandatory to setup gains before @ref Start
     *
     * @param gain1
     * @param gain2
     * @param gain3
     * @param gain4
     */
    void SetSensorGains(int gain1, int gain2, int gain3, int gain4);

    /**
     * \brief Setup Sensor transmissions
     *
     * It is mandatory to setup transmissions before @ref Start
     *
     * @param trans1
     * @param trans2
     * @param trans3
     * @param trans4
     */
    void SetSensorTransmissions(double trans1, double trans2, double trans3, double trans4);
    /**
     * \brief Start reading Sensor loop
     *
     * It is mandatory to setup @ref SetBridge @ref SetSensorOffsets @ref SetSensorGains and @ref SetSensorTransmissions before start.
     *
     * Only one instance of @ref TimeSwipe can be running each moment of the time
     *
     * After each sensor read complete cb called with vector of @ref Record
     *
     * @param cb
     * @return false if reading procedure start failed, otherwise it blocks current execution thread and returns true after reading finished
     */
    bool Start(std::function<void(std::vector<Record>)> cb);

    /**
     * \brief Stop reading Sensor loop
     *
     * @return true is stop succeeded, false otherwise
     */
    bool Stop();

    /**
      \brief Minimal interval between sensor reading in milliseconds
      */
    static const unsigned READ_INTERVAL_MS = 100;

private:
    std::unique_ptr<TimeSwipeImpl> _impl;

};
