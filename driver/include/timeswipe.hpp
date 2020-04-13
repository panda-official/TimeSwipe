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
 * \brief Sensors container
 */
class SensorsData {
    static constexpr size_t SENSORS = 4;
    using CONTAINER = std::array<std::vector<float>, SENSORS>;
public:
    /**
     * \brief Get number of sensors
     *
     *
     * @return number of sensors
     */
    size_t SensorsSize();

    /**
     * \brief Get number of data entries
     *
     *
     * @return number of data entries each sensor has
     */
    size_t DataSize();

    /**
     * \brief Access sensor data
     *
     * @param num - sensor number. Valid values from 0 to @ref SensorsSize-1
     *
     * @return number of data entries each sensor has
     */

    std::vector<float>& operator[](size_t num);

    CONTAINER& data();
    void reserve(size_t num);
    void clear();
    bool empty();
    void append(SensorsData&& other);
    void erase_front(size_t num);
    void erase_back(size_t num);

private:
    CONTAINER _data;
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
    void Init(int bridge, int offsets[4], float gains[4], float transmissions[4]);

    /**
     * \brief Start PWM generator
     * Method can be called in any time.
     *
     * @param num - output number - possible values are 0 or 1
     * @param frequency - periods per second - possible values between 1 and 1000
     * @param high - PWM signal high value - possible values are 0..4095 high >= low, default is 4095
     * @param low - PWM signal low value - possible values are 0..4095 low <= high, default is 0
     * @param repeats - number of periods to repeat. PWM generator will work (repeats/frequency) seconds
     *                  after repeats number get exhausted PWM goes to stop state and StartPWM can be called again
     *                  0 is for unlimited repeats (default)
     * @param duty_cycle - part of PWM period when signal is in high state. 0.001 duty_cycle <= 0.999. default value is 0.5
     *
     * @return false if at least one wrong parameter given or generator already in start state
     */
    bool StartPWM(uint8_t num, uint32_t frequency, uint32_t high = 4095, uint32_t low = 0, uint32_t repeats = 0, float duty_cycle = 0.5);

    /**
     * \brief Stop PWM generator
     * Method can be called in any time.
     *
     * @param numb - output number - possible values 0 or 1
     *
     * @return false if at least wrong parameter given or generator already in stop state
     */
    bool StopPWM(uint8_t num);

    /**
     * \brief Get PWM generator state if it is in a Start state
     * Method can be called in any time.
     *
     * @param[in] num - output number - possible values 0 or 1
     * @param[out] active - pwm active flag, if active is false other parameters can not be considered valid
     * other parameters are output references to paramteres with same names in @ref StartPWM
     * they are valid only if true returned
     *
     * @return false if num parameter is wrong or generator is in stop state
     */

    bool GetPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle);

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
    using ReadCallback = std::function<void(SensorsData, uint64_t errors)>;

    /**
     * \brief Start reading Sensor loop
     *
     * Only one instance of @ref TimeSwipe can be running each moment of the time
     *
     * After each sensor read complete cb called with vector of @ref SensorsData
     *
     * Buffer is for 1 second data if \p cb works longer than 1 second, next data can be loosed and next callback called with non-zero errors
     *
     * Function starts two threads: one thread reads sensor values to the ring buffer, second thread polls ring buffer and calls @ref cb
     *
     * Function can not be called from callback
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

    /*!
     * \brief TraceSPI
     * \param val true=on
     */
    void TraceSPI(bool val);

    static bool resample_log;
private:
    std::unique_ptr<TimeSwipeImpl> _impl;

};
