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

    /** @enum TimeSwipe::Mode
     *
     * \brief Input mode
     *
     */
    enum class Mode {
        Primary,
        Norm,
        Digital
    };

    /**
     * \brief Setup hardware mode
     *
     * @param number - one of @ref Mode
     */
    void SetMode(Mode number);

    /**
     * \brief Get current hardware mode
     *
     * @return current mode
     */
    Mode GetMode();

    /**
     * \brief Setup Sensor offsets
     *
     * Default offsets are all 0
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
    void SetSensorGains(float gain1, float gain2, float gain3, float gain4);

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
    void SetSensorTransmissions(float trans1, float trans2, float trans3, float trans4);

    /**
     * \brief Setup Burst buffer size
     *
     * This method notifies the driver to return at least burstNum records to the cb of @ref Start function per each call
     *
     * @param burstNum - number of records in burst buffer
     */
    void SetBurstSize(size_t burstNum);

    /**
     * \brief Set sample rate. Default value is 48000
     *
     * @param rate - new sample rate
     * @return false on wrong rate value requested
     */
    bool SetSampleRate(int rate);

    /**
     * \brief Read sensors callback function pointer
     */
    using ReadCallback = std::function<void(std::vector<Record>, uint64_t errors)>;

    /**
     * \brief Start reading Sensor loop
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
     * onEvent must be called before @ref Start called, otherwise register fails
     *
     * @param cb callback called with true when button pressed and with false when button released, button counter (odd - pressed, even - released)
     * @return false if register callback failed, true otherwise
     */
    bool onEvent(OnButtonCallback cb);

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

    static bool resample_log;
private:
    std::unique_ptr<TimeSwipeImpl> _impl;

};
