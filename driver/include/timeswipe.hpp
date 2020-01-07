/**
 * \file
 * \brief Timeswipe cpp library
 */
#pragma once
#include <memory>
#include <functional>
#include <array>
#include <vector>
#include <string>

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
    TimeSwipe(const TimeSwipe&) = delete;
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
     * \brief Setup secondary number
     *
     * @param number - secondary number
     */
    void SetSecondary(int number);

    /**
     * \brief Initialize sensors
     *
     * This method is all-in-one replacement for @ref SetBridge @ref SetSensorOffsets @ref SetSensorGains @ref SetSensorTransmissions
     *
     * @param bridge - bridge number
     * @param offsets - sensor offsets @ref SetSensorOffsets
     * @param gains - sensor gains @ref SetSensorGains
     * @param transmissions - sensor transmissions - @ref SetSensorTransmissions
     */
    void Init(int bridge, int offsets[4], int gains[4], double transmissions[4]);


    /**
     * \brief Read sensors callback function pointer
     */
    using ReadCallback = std::function<void(std::vector<Record>, uint64_t errors)>;

    /**
     * \brief Start reading Sensor loop
     *
     * It is mandatory to setup @ref SetBridge @ref SetSensorOffsets @ref SetSensorGains and @ref SetSensorTransmissions before start.
     *
     * Only one instance of @ref TimeSwipe can be running each moment of the time
     *
     * After each sensor read complete cb called with vector of @ref Record
     *
     * Buffer is for 1 second data if \p cb works longer than 1 second, next data can be loosed and next callback called with non-zero errors
     *
     * Function starts two threads: one thread reads sensor values to the ring buffer, second thread polls ring buffer and calls @ref cb
     *
     * @param cb
     * @return false if reading procedure start failed, otherwise true
     */
    bool Start(ReadCallback cb);

    /**
     * \brief Send SPI SetSettings request and receive the answer
     *
     * @param request - request json string
     * @param error - output error
     * @return json-formatted answer, set error if error occured
     */
    std::string SetSettings(const std::string& request, std::string& error);

    /**
     * \brief Send SPI GetSettings request and receive the answer
     *
     * @param request - request json string
     * @param error - output error
     * @return json-formatted answer, set error if error occured
     */
    std::string GetSettings(const std::string& request, std::string& error);

    using OnButtonCallback = std::function<void(bool, unsigned)>;
    /**
     * \brief Register callback for button pressed/released
     *
     * onButton must be called before @ref Start called, otherwise register fails
     *
     * @param cb callback called with true when button pressed and with false when button released, button counter (odd - pressed, even - released)
     * @return false if register callback failed, true otherwise
     */
    bool onButton(OnButtonCallback cb);

    using OnErrorCallback = std::function<void(uint64_t)>;
    /**
     * \brief Register error callback
     *
     * onError must be called before @ref Start called, otherwise register fails
     *
     * @param cb callback called once read error occurred
     * @return false if register callback failed, true otherwise
     */
    bool onError(OnErrorCallback cb);

    /**
     * \brief Stop reading Sensor loop
     *
     * @return true is stop succeeded, false otherwise
     */
    bool Stop();

private:
    std::unique_ptr<TimeSwipeImpl> _impl;

};
