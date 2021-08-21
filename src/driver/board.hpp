// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_DRIVER_BOARD_HPP
#define PANDA_TIMESWIPE_DRIVER_BOARD_HPP

#include "bcmspi.hpp"
#include "event.hpp"
#include "gpio.hpp"
#include "sensor_data.hpp"
#include "types_fwd.hpp"

#include "../common/json.hpp"
#include "../3rdparty/dmitigr/assert.hpp"

#include <array>
#include <atomic>
#include <chrono>
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
#include <cmath>
#endif
#include <cstdint>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

class Board final {
public:
  /// The destructor.
  ~Board()
  {
    delete instance_;
  }

  static Board* Instance()
  {
    if (!instance_) instance_ = new Board();
    return instance_;
  }

  /**
   * Initializes GPIO pins.
   *
   * @param force Forces initialization even if IsInited() returns `true`.
   *
   * @par Effects
   * Restarts TimeSwipe firmware on very first run!
   *
   * @see IsInited(), StartMeasurement().
   */
  void Init(const bool force = false)
  {
    if (!force && IsInited()) return;

    setup_io();
    initGPIOInput(DATA0);
    initGPIOInput(DATA1);
    initGPIOInput(DATA2);
    initGPIOInput(DATA3);
    initGPIOInput(DATA4);
    initGPIOInput(DATA5);
    initGPIOInput(DATA6);
    initGPIOInput(DATA7);

    initGPIOInput(TCO);
    initGPIOInput(PI_OK);
    initGPIOInput(FAIL);
    initGPIOInput(BUTTON);

    // initGPIOOutput(PI_OK);
    initGPIOOutput(CLOCK);
    initGPIOOutput(RESET);

    // Initial Reset
    setGPIOLow(CLOCK);
    setGPIOHigh(RESET);

    using std::chrono::milliseconds;
    std::this_thread::sleep_for(milliseconds{1});

    is_board_inited_ = true;
  }

  /**
   * @returns `true` if Init() has been successfully called at least once.
   *
   * @par Thread-safety
   * Thread-safe.
   *
   * @see Init().
   */
  bool IsInited() noexcept
  {
    return is_board_inited_;
  }

  /**
   * Sends the command to a TimeSwipe firmware to start measurement.
   *
   * @param mode Measurement mode.
   *
   * @par Requires
   * `IsInited()`.
   *
   * @par Effects
   * The reader does receive the data from the board.
   *
   * @see Init(), StopMeasurement().
   */
  void StartMeasurement(const int mode)
  {
    DMITIGR_CHECK(IsInited());

    // Set mfactors.
    for (std::size_t i{}; i < mfactors_.size(); ++i)
      mfactors_[i] = gains_[i] * transmissions_[i];

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    emul_point_begin_ = std::chrono::steady_clock::now();
    emul_sent_ = 0;
#endif

    // Select Mode
    SetMode(mode);

    // Start Measurement
    using std::chrono::milliseconds;
    std::this_thread::sleep_for(milliseconds{1});
    SetEnableADmes(true);
    is_measurement_started_ = true;
  }

  /**
   * @returns `true` if StartMeasurement() has been successfully called and
   * StopMeasurement() has not been successfully called yet.
   *
   * @par Thread-safety
   * Thread-safe.
   */
  bool IsMeasurementStarted() noexcept
  {
    return is_measurement_started_;
  }

  /**
   * Sends the command to a TimeSwipe firmware to stop measurement.
   *
   * @par Effects
   * The reader doesn't receive the data from the board.
   *
   * @see InitBoard(), StartMeasurement().
   */
  void StopMeasurement()
  {
    if (!IsMeasurementStarted()) return;

    // Reset Clock
    setGPIOLow(CLOCK);

    // Stop Measurement
    SetEnableADmes(false);

    is_measurement_started_ = false;
    read_skip_count_ = kInitialInvalidDataSetsCount;
  }

  // ---------------------------------------------------------------------------
  // Sensor Data Read
  // ---------------------------------------------------------------------------

  // read records from hardware buffer
  SensorsData ReadSensorData()
  {
    static const auto WaitForPiOk = []
    {
      // for 12MHz Quartz
      std::this_thread::sleep_for(std::chrono::microseconds(700));
    };

#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    // Skip data sets if needed. (First 32 data sets are always invalid.)
    while (read_skip_count_ > 0) {
      WaitForPiOk();
      while (true) {
        const auto [chunk, tco] = GpioData::ReadChunk();
        if (tco != 0x00004000) break;
      }
      --read_skip_count_;
    }

    // Wait the RAM A or RAM B becomes available for reading.
    WaitForPiOk();

    /*
     * Read the data sets. The amount of data depends on the counterstate
     * and can be [1..255]*32 data sets. (The number of data sets are always 32
     * also. Usually, the first data set is of size greater than 1 is followed
     * by 31 data sets of size 1.)
     *
     * TODO: the PIN 12 of Pi-Header is for overflow detection. When it's
     * becomes high it indicates that the RAM is full (failure - data loss).
     * So, check this case.
     */
    SensorsData out;
    out.reserve(8192);
    do {
      const auto [chunk, tco] = GpioData::ReadChunk();
      GpioData::AppendChunk(out, chunk, offsets_, mfactors_);
      if (tco != 0x00004000) break;
    } while (true);

    sleep55ns();
    sleep55ns();

    return out;
#else
    namespace chrono = std::chrono;
    SensorsData out;
    auto& data{out.data()};
    while (true) {
      emul_point_end_ = chrono::steady_clock::now();
      const std::uint64_t diff_us{chrono::duration_cast<chrono::microseconds>
        (emul_point_end_ - emul_point_begin_).count()};
      const std::uint64_t wouldSent{diff_us * emul_rate_ / 1000 / 1000};
      if (wouldSent > emul_sent_) {
        while (emul_sent_++ < wouldSent) {
          const auto val{int(3276 * std::sin(angle_) + 32767)};
          angle_ += (2.0 * M_PI) / emul_rate_;
          data[0].push_back(val);
          data[1].push_back(val);
          data[2].push_back(val);
          data[3].push_back(val);
        }
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return out;
#endif
  }

  const std::array<std::uint16_t, 4>& Offsets() const noexcept
  {
    return offsets_;
  }

  std::array<std::uint16_t, 4>& Offsets() noexcept
  {
    return offsets_;
  }

  const std::array<float, 4>& Gains() const noexcept
  {
    return gains_;
  }

  std::array<float, 4>& Gains() noexcept
  {
    return gains_;
  }

  const std::array<float, 4>& Transmissions() const noexcept
  {
    return transmissions_;
  }

  std::array<float, 4>& Transmissions() noexcept
  {
    return transmissions_;
  }

  // ===========================================================================

  void SetTraceSPI(const bool value)
  {
    trace_spi_ = value;
  }

  void SetMode(const int num)
  {
    sendSetCommand("Mode", std::to_string(num));
    std::string answer;
    receiveAnswer(answer);
  }

  void SetEnableADmes(const bool value)
  {
    sendSetCommand("EnableADmes", std::to_string(value));
    std::string answer;
    receiveAnswer(answer);
  }

  std::list<TimeSwipeEvent> GetEvents()
  {
    std::list<TimeSwipeEvent> events;
    std::lock_guard<std::mutex> lock(mutex_);
#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    std::string data;
    if (getEvents(data) && !data.empty()) {
      if (data[data.length()-1] == 0xa ) data = data.substr(0, data.size()-1);

      if (data.empty()) return events;

      try {
        auto j = nlohmann::json::parse(data);
        auto it_btn = j.find("Button");
        if (it_btn != j.end() && it_btn->is_boolean()) {
          auto it_cnt = j.find("ButtonStateCnt");
          if (it_cnt != j.end() && it_cnt->is_number()) {
            events.push_back(TimeSwipeEvent::Button(it_btn->get<bool>(), it_cnt->get<int>()));
          }
        }

        auto it = j.find("Gain");
        if (it != j.end() && it->is_number()) {
          events.push_back(TimeSwipeEvent::Gain(it->get<int>()));
        }

        it = j.find("SetSecondary");
        if (it != j.end() && it->is_number()) {
          events.push_back(TimeSwipeEvent::SetSecondary(it->get<int>()));
        }

        it = j.find("Bridge");
        if (it != j.end() && it->is_number()) {
          events.push_back(TimeSwipeEvent::Bridge(it->get<int>()));
        }

        it = j.find("Record");
        if (it != j.end() && it->is_number()) {
          events.push_back(TimeSwipeEvent::Record(it->get<int>()));
        }

        it = j.find("Offset");
        if (it != j.end() && it->is_number()) {
          events.push_back(TimeSwipeEvent::Offset(it->get<int>()));
        }

        it = j.find("Mode");
        if (it != j.end() && it->is_number()) {
          events.push_back(TimeSwipeEvent::Mode(it->get<int>()));
        }
      }
      catch (nlohmann::json::parse_error& e)
        {
          // output exception information
          std::cerr << "readBoardEvents: json parse failed data:" << data << "error:" << e.what() << '\n';
        }
    }
#endif
    return events;
  }

  std::string GetSettings(const std::string& request, std::string& error)
  {
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    sendGetSettingsCommand(request);
    std::string answer;
    if (!receiveAnswer(answer, error))
      error = "read SPI failed";
    return answer;
#endif
  }

  std::string SetSettings(const std::string& request, std::string& error)
  {
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    sendSetSettingsCommand(request);
    std::string answer;
    if (!receiveAnswer(answer, error))
      error = "read SPI failed";
    return answer;
#endif
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool StartPwm(const std::uint8_t num, const std::uint32_t frequency,
    const std::uint32_t high, const std::uint32_t low, const std::uint32_t repeats,
    const float duty_cycle)
  {
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(num+1);
    auto obj = nlohmann::json::object({});
    obj.emplace(pwm + ".freq", frequency);
    obj.emplace(pwm + ".high", high);
    obj.emplace(pwm + ".low", low);
    obj.emplace(pwm + ".repeats", repeats);
    obj.emplace(pwm + ".duty", duty_cycle);
    std::string err;

    auto settings = SetSettings(obj.dump(), err);
    if (str2json(settings).empty())
      return false;

    obj.emplace(pwm, true);
    settings = SetSettings(obj.dump(), err);

    return !str2json(settings).empty();
#endif
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool StopPwm(const std::uint8_t num)
  {
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(num + 1);
    /*
      sendGetCommand(pwm);
      std::string answer;
      receiveStripAnswer(answer);
      if (answer == "0") return false; // Already stopped
    */
    return sendSetCommandCheck(pwm, 0);
#endif
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool GetPwm(const std::uint8_t num, bool& active, std::uint32_t& frequency,
    std::uint32_t& high, std::uint32_t& low, std::uint32_t& repeats,
    float& duty_cycle)
  {
    std::lock_guard<std::mutex> lock(mutex_);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    std::string pwm = std::string("PWM") + std::to_string(num+1);
    const auto arr = nlohmann::json::array({pwm, pwm + ".freq", pwm + ".high", pwm + ".low", pwm + ".repeats", pwm + ".duty"});
    std::string err;
    const auto settings = GetSettings(arr.dump(), err);
    const auto s = str2json(settings);
    return !s.empty() &&
      json_get(s, pwm, active) &&
      json_get(s, pwm + ".freq", frequency) &&
      json_get(s, pwm + ".high", high) &&
      json_get(s, pwm + ".low", low) &&
      json_get(s, pwm + ".repeats", repeats) &&
      json_get(s, pwm + ".duty", duty_cycle);
#endif
  }

  bool SetDAC(bool value)
  {
    sendSetCommand("DACsw", value ? "1" : "0");
    std::string answer;
    if (!receiveStripAnswer(answer))
      return false;
    return answer == (value ? "1" : "0");
  }

  /**
   * @param num Zero-based number of PWM.
   */
  bool SetOUT(std::uint8_t num, int val)
  {
    std::string var = std::string("AOUT") + (num ? "4" : "3") + ".raw";
    sendSetCommand(var, std::to_string(val));
    std::string answer;
    if (!receiveStripAnswer(answer)) return false;
    return answer == std::to_string(val);
  }

  bool setChannelMode(unsigned int num, int nMode)
  {
    sendSetCommand(makeChCmd(num, "mode"), std::to_string(nMode));
    std::string answer;
    if (!receiveStripAnswer(answer)) return false;
    return answer == std::to_string(nMode);
  }

  bool getChannelMode(unsigned int num, int &nMode, std::string& error)
  {
    sendGetCommand(makeChCmd(num, "mode"));
    std::string answer;
    if (!receiveAnswer(answer, error)) {
      nMode=0;
      return false;
    }
    nMode=std::stoi(answer);
    return true;

  }

  bool setChannelGain(unsigned int num, float Gain)
  {
    sendSetCommand(makeChCmd(num, "gain"), std::to_string(Gain));
    std::string answer;
    if (!receiveStripAnswer(answer)) return false;
    return true;
  }

  bool getChannelGain(unsigned int num, float &Gain, std::string& error)
  {
    sendGetCommand(makeChCmd(num, "gain"));
    std::string answer;
    if (!receiveAnswer(answer, error)) {
      Gain=0;
      return false;
    }
    Gain=std::stof(answer);
    return true;

  }

  bool setChannelIEPE(unsigned int num, bool bIEPE)
  {
    sendSetCommand(makeChCmd(num, "iepe"), std::to_string(bIEPE));
    std::string answer;
    if (!receiveStripAnswer(answer)) return false;
    return true;
  }

  bool getChannelIEPE(unsigned int num, bool &bIEPE, std::string& error)
  {
    sendGetCommand(makeChCmd(num, "iepe"));
    std::string answer;
    if (!receiveAnswer(answer, error)) {
      bIEPE=0;
      return false;
    }
    bIEPE=std::stoi(answer);
    return true;
  }

private:
  CBcmSPI spi_;
  std::atomic_bool trace_spi_;
  std::atomic_bool is_board_inited_;
  std::atomic_bool is_measurement_started_;
  std::mutex mutex_;

  // ---------------------------------------------------------------------------
  // Sensor Data Read section
  // ---------------------------------------------------------------------------

  // The number of initial invalid data sets.
  static constexpr int kInitialInvalidDataSetsCount{32};
  int read_skip_count_{kInitialInvalidDataSetsCount};
  std::array<std::uint16_t, 4> offsets_{0, 0, 0, 0};
  std::array<float, 4> gains_{1, 1, 1, 1};
  std::array<float, 4> transmissions_{1, 1, 1, 1};
  std::array<float, 4> mfactors_{};

#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
  double angle_{};
  std::chrono::steady_clock::time_point emul_point_begin_;
  std::chrono::steady_clock::time_point emul_point_end_;
  std::uint64_t emul_sent_{};
  static constexpr std::size_t emul_rate_{48000};
#endif

  inline static Board* instance_;

  Board()
    : spi_{CBcmLIB::iSPI::SPI0}
  {}

  static void stripAnswer(std::string& str) noexcept
  {
    if (!str.empty() && str.back() == '\n')
      str.pop_back();
  }

  std::string makeChCmd(const unsigned num, const char* const pSubDomain)
  {
    return std::string{"CH"}.append(std::to_string(num + 1))
      .append(".").append(pSubDomain);
  }

  void sendCommand(const std::string& cmd)
  {
    CFIFO command;
    command += cmd;
    spi_.send(command);
    if (trace_spi_)
      std::clog << "spi: sent: \"" << command << "\"" << std::endl;
  }

  bool receiveAnswer(std::string& ans)
  {
    CFIFO answer;
    if (spi_.receive(answer)) {
      ans = answer;
      if (trace_spi_)
        std::clog << "spi: received: \"" << ans << "\"" << std::endl;
      return true;
    }
    if (trace_spi_)
      std::cerr << "spi: receive failed" << std::endl;
    return false;
  }

  bool receiveAnswer(std::string& ans, std::string& error)
  {
    const auto ret = receiveAnswer(ans);
    if (ret && !ans.empty() && ans[0] == '!') {
      error = ans;
      ans.clear();
      return false;
    }
    return ret;
  }

  bool receiveStripAnswer(std::string& ans, std::string& error)
  {
    if (!receiveAnswer(ans, error))
      return false;
    stripAnswer(ans);
    return true;
  }

  bool receiveStripAnswer(std::string& ans)
  {
    std::string error;
    return receiveStripAnswer(ans, error);
  }

  void sendSetCommand(const std::string& variable, const std::string& value)
  {
    sendCommand(variable + "<" + value + "\n");
  }

  template <class Number>
  bool sendSetCommandCheck(const std::string& variable, const Number value) {
    sendSetCommand(variable, std::to_string(value));
    // // FIXME: if sleep disable and trace_spi_=false spi_.receive fails sometimes
    // std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    std::string answer;
    receiveStripAnswer(answer);
    Number num{};
    std::istringstream s{answer};
    s >> num;
    return num == value;
  }

  void sendGetCommand(const std::string& variable)
  {
    sendCommand(variable + ">\n");
  }

  void sendEventsCommand()
  {
    sendCommand("je>\n");
  }

  void sendSetSettingsCommand(const std::string& request)
  {
    sendCommand("js<" + request + "\n");
  }

  void sendGetSettingsCommand(const std::string& request)
  {
    sendCommand("js>" + request + "\n");
  }

  // ---------------------------------------------------------------------------
  // Events
  // ---------------------------------------------------------------------------

  bool getEvents(std::string& ev)
  {
    sendEventsCommand();
    return receiveAnswer(ev);
  }

  // ---------------------------------------------------------------------------
  // GPIO
  // ---------------------------------------------------------------------------

  // PIN NAMES
  static constexpr std::uint8_t DATA0{24};  // BCM 24 - PIN 18
  static constexpr std::uint8_t DATA1{25};  // BCM 25 - PIN 22
  static constexpr std::uint8_t DATA2{7};   // BCM  7 - PIN 26
  static constexpr std::uint8_t DATA3{5};   // BCM  5 - PIN 29
  static constexpr std::uint8_t DATA4{6};   // BCM  6 - PIN 31
  static constexpr std::uint8_t DATA5{12};  // BCM 12 - PIN 32
  static constexpr std::uint8_t DATA6{13};  // BCM 13 - PIN 33
  static constexpr std::uint8_t DATA7{16};  // BCM 16 - PIN 36
  static constexpr std::uint8_t CLOCK{4};   // BCM  4 - PIN  7
  static constexpr std::uint8_t TCO{14};    // BCM 14 - PIN  8
  static constexpr std::uint8_t PI_OK{15};  // BCM 15 - PIN 10
  static constexpr std::uint8_t FAIL{18};   // BCM 18 - PIN 12
  static constexpr std::uint8_t RESET{17};  // BCM 17 - PIN 11
  static constexpr std::uint8_t BUTTON{25}; // BCM 25 - PIN 22

  static constexpr std::array<std::uint32_t, 8> DATA_POSITION{
    std::uint32_t{1} << DATA0,
    std::uint32_t{1} << DATA1,
    std::uint32_t{1} << DATA2,
    std::uint32_t{1} << DATA3,
    std::uint32_t{1} << DATA4,
    std::uint32_t{1} << DATA5,
    std::uint32_t{1} << DATA6,
    std::uint32_t{1} << DATA7
  };

  static constexpr std::uint32_t CLOCK_POSITION{std::uint32_t{1} << CLOCK};
  static constexpr std::uint32_t TCO_POSITION{std::uint32_t{1} << TCO};
  static constexpr std::uint32_t PI_STATUS_POSITION{std::uint32_t{1} << PI_OK};
  static constexpr std::uint32_t FAIL_POSITION{std::uint32_t{1} << FAIL};
  static constexpr std::uint32_t BUTTON_POSITION{std::uint32_t{1} << BUTTON};

  // (2^32)-1 - ALL BCM_PINS
  static constexpr std::uint32_t ALL_32_BITS_ON{0xFFFFFFFF};

  static void pullGPIO(const unsigned pin, const unsigned high)
  {
    PANDA_TIMESWIPE_GPIO_PULL = high << pin;
  }

  static void initGPIOInput(const unsigned pin)
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
  }

  static void initGPIOOutput(const unsigned pin)
  {
    PANDA_TIMESWIPE_INP_GPIO(pin);
    PANDA_TIMESWIPE_OUT_GPIO(pin);
    pullGPIO(pin, 0);
  }

  static void setGPIOHigh(const unsigned pin)
  {
    PANDA_TIMESWIPE_GPIO_SET = 1 << pin;
  }

  static void setGPIOLow(const unsigned pin)
  {
    PANDA_TIMESWIPE_GPIO_CLR = 1 << pin;
  }

  static void resetAllGPIO()
  {
    PANDA_TIMESWIPE_GPIO_CLR = ALL_32_BITS_ON;
  }

  static unsigned readAllGPIO()
  {
    return (*(gpio + 13) & ALL_32_BITS_ON);
  }

  static void sleep55ns()
  {
    readAllGPIO();
  }

  static void sleep8ns()
  {
    setGPIOHigh(10); // ANY UNUSED PIN!!!
  }

  struct GpioData final {
    std::uint8_t byte{};
    unsigned int tco{};
    bool piOk{};

    // chunk-Layout:
    // ------+----------------------------+---------------------------
    //  Byte | Bit7   Bit6   Bit5   Bit4  | Bit3   Bit2   Bit1   Bit0
    // ------+----------------------------+---------------------------
    //     0 | 1-14   2-14   3-14   4-14  | 1-15   2-15   3-15   4-15
    //     1 | 1-12   2-12   3-12   4-12  | 1-13   2-13   3-13   4-13
    //     2 | 1-10   2-10   3-10   4-10  | 1-11   2-11   3-11   4-11
    //     3 |  1-8    2-8    3-8    4-8  |  1-9    2-9    3-9    4-9
    //     4 |  1-6    2-6    3-6    4-6  |  1-7    2-7    3-7    4-7
    //     5 |  1-4    2-4    3-4    4-4  |  1-5    2-5    3-5    4-5
    //     6 |  1-2    2-2    3-2    4-2  |  1-3    2-3    3-3    4-3
    //     7 |  1-0    2-0    3-0    4-0  |  1-1    2-1    3-1    4-1
    using Chunk = std::array<std::uint8_t, 8>;

    static GpioData Read() noexcept
    {
      setGPIOHigh(CLOCK);
      sleep55ns();
      sleep55ns();

      setGPIOLow(CLOCK);
      sleep55ns();
      sleep55ns();

      const unsigned int allGPIO{readAllGPIO()};
      const std::uint8_t byte =
        ((allGPIO & DATA_POSITION[0]) >> 17) |  // Bit 7
        ((allGPIO & DATA_POSITION[1]) >> 19) |  //     6
        ((allGPIO & DATA_POSITION[2]) >> 2) |   //     5
        ((allGPIO & DATA_POSITION[3]) >> 1) |   //     4
        ((allGPIO & DATA_POSITION[4]) >> 3) |   //     3
        ((allGPIO & DATA_POSITION[5]) >> 10) |  //     2
        ((allGPIO & DATA_POSITION[6]) >> 12) |  //     1
        ((allGPIO & DATA_POSITION[7]) >> 16);   //     0

      sleep55ns();
      sleep55ns();

      return {byte, (allGPIO & TCO_POSITION), (allGPIO & PI_STATUS_POSITION) != 0};
    }

    struct ReadChunkResult final {
      Chunk chunk{};
      unsigned tco{};
    };

    static ReadChunkResult ReadChunk() noexcept
    {
      ReadChunkResult result;
      result.chunk[0] = Read().byte;
      {
        const auto d{Read()};
        result.chunk[1] = d.byte;
        result.tco = d.tco;
      }
      for (unsigned i{2u}; i < result.chunk.size(); ++i)
        result.chunk[i] = Read().byte;
      return result;
    }

    static void AppendChunk(SensorsData& data,
      const Chunk& chunk,
      const std::array<std::uint16_t, 4>& offsets,
      const std::array<float, 4>& mfactors)
    {
      std::array<std::uint16_t, 4> sensors{};
      static_assert(data.SensorsSize() == 4); // KLUDGE
      using OffsetValue = std::decay_t<decltype(offsets)>::value_type;
      using SensorValue = std::decay_t<decltype(sensors)>::value_type;
      static_assert(sizeof(OffsetValue) == sizeof(SensorValue));

      constexpr auto setBit = [](std::uint16_t& word, const std::uint8_t N, const bool bit) noexcept
      {
        word = (word & ~(1UL << N)) | (bit << N);
      };
      constexpr auto getBit = [](const std::uint8_t byte, const std::uint8_t N) noexcept -> bool
      {
        return (byte & (1UL << N));
      };
      for (std::size_t i{}, count{}; i < chunk.size(); ++i) {
        setBit(sensors[0], 15 - count, getBit(chunk[i], 3));
        setBit(sensors[1], 15 - count, getBit(chunk[i], 2));
        setBit(sensors[2], 15 - count, getBit(chunk[i], 1));
        setBit(sensors[3], 15 - count, getBit(chunk[i], 0));
        count++;

        setBit(sensors[0], 15 - count, getBit(chunk[i], 7));
        setBit(sensors[1], 15 - count, getBit(chunk[i], 6));
        setBit(sensors[2], 15 - count, getBit(chunk[i], 5));
        setBit(sensors[3], 15 - count, getBit(chunk[i], 4));
        count++;
      }

      auto& underlying_data{data.data()};
      for (std::size_t i{}; i < 4; ++i)
        underlying_data[i].push_back(static_cast<float>(sensors[i] - offsets[i]) * mfactors[i]);
    }
  };

  // -------------------------------------------------------------------------
  // JSON helpers
  // -------------------------------------------------------------------------

  static nlohmann::json str2json(const std::string& str)
  {
    nlohmann::json j;
    try {
      j = nlohmann::json::parse(str);
    } catch (nlohmann::json::parse_error& e) {
      std::cerr << "Board: json parse failed data:" << str << "error:" << e.what() << '\n';
      return nlohmann::json();
    }
    return j;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, std::string& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_string()) return false;
    value = it->get<std::string>();
    return true;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, std::uint32_t& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_unsigned()) return false;
    value = it->get<uint32_t>();
    return true;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, float& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_float()) return false;
    value = it->get<float>();
    return true;
  }

  static bool json_get(const nlohmann::json& j, const std::string& key, bool& value)
  {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_boolean()) return false;
    value = it->get<bool>();
    return true;
  }
};

#endif  // PANDA_TIMESWIPE_DRIVER_BOARD_HPP
