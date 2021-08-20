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

#ifndef PANDA_TIMESWIPE_DRIVER_BOARD
#define PANDA_TIMESWIPE_DRIVER_BOARD

#include "types_fwd.hpp"
#include "bcmspi.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <thread>

// PIN NAMES
static const std::uint8_t DATA0{24};  // BCM 24 - PIN 18
static const std::uint8_t DATA1{25};  // BCM 25 - PIN 22
static const std::uint8_t DATA2{7};   // BCM  7 - PIN 26
static const std::uint8_t DATA3{5};   // BCM  5 - PIN 29
static const std::uint8_t DATA4{6};   // BCM  6 - PIN 31
static const std::uint8_t DATA5{12};  // BCM 12 - PIN 32
static const std::uint8_t DATA6{13};  // BCM 13 - PIN 33
static const std::uint8_t DATA7{16};  // BCM 16 - PIN 36
static const std::uint8_t CLOCK{4};   // BCM  4 - PIN  7
static const std::uint8_t TCO{14};    // BCM 14 - PIN  8
static const std::uint8_t PI_OK{15};  // BCM 15 - PIN 10
static const std::uint8_t FAIL{18};   // BCM 18 - PIN 12
static const std::uint8_t RESET{17};  // BCM 17 - PIN 11
static const std::uint8_t BUTTON{25}; // BCM 25 - PIN 22

static const std::array<std::uint32_t, 8> DATA_POSITION{
  std::uint32_t{1} << DATA0,
  std::uint32_t{1} << DATA1,
  std::uint32_t{1} << DATA2,
  std::uint32_t{1} << DATA3,
  std::uint32_t{1} << DATA4,
  std::uint32_t{1} << DATA5,
  std::uint32_t{1} << DATA6,
  std::uint32_t{1} << DATA7
};

static const std::uint32_t CLOCK_POSITION{std::uint32_t{1} << CLOCK};
static const std::uint32_t TCO_POSITION{std::uint32_t{1} << TCO};
static const std::uint32_t PI_STATUS_POSITION{std::uint32_t{1} << PI_OK};
static const std::uint32_t FAIL_POSITION{std::uint32_t{1} << FAIL};
static const std::uint32_t BUTTON_POSITION{std::uint32_t{1} << BUTTON};

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
   * @see IsBoardInited(), StartMeasurement().
   */
  void Init(bool force = false);

  /**
   * @returns `true` if Init() has been successfully called at least once.
   *
   * @par Thread-safety
   * Thread-safe.
   *
   * @see Init().
   */
  bool IsInited() noexcept;

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
  void StartMeasurement(int mode);

  /**
   * @returns `true` if StartBoard() has been successfully called and
   * StopMeasurement() has not been successfully called yet.
   *
   * @par Thread-safety
   * Thread-safe.
   */
  bool IsMeasurementStarted() noexcept;

  /**
   * Sends the command to a TimeSwipe firmware to stop measurement.
   *
   * @par Effects
   * The reader doesn't receive the data from the board.
   *
   * @see InitBoard(), StartMeasurement().
   */
  void StopMeasurement();

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

  std::list<TimeSwipeEvent> GetEvents();

  std::string GetSettings(const std::string& request, std::string& error);

  std::string SetSettings(const std::string& request, std::string& error);

  /**
   * @param num Zero-based number of PWM.
   */
  bool StartPwm(std::uint8_t num, std::uint32_t frequency, std::uint32_t high,
    std::uint32_t low, std::uint32_t repeats, float duty_cycle);

  /**
   * @param num Zero-based number of PWM.
   */
  bool StopPwm(std::uint8_t num);

  /**
   * @param num Zero-based number of PWM.
   */
  bool GetPwm(std::uint8_t num, bool& active, std::uint32_t& frequency,
    std::uint32_t& high, std::uint32_t& low, std::uint32_t& repeats,
    float& duty_cycle);

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

  friend struct GpioData; // REMOVE ME
  friend class RecordReader; // REMOVE ME
  static void pullGPIO(unsigned pin, unsigned high);
  static void initGPIOInput(unsigned pin);
  static void initGPIOOutput(unsigned pin);
  static void setGPIOHigh(unsigned pin);
  static void setGPIOLow(unsigned pin);
  static void resetAllGPIO();
  static unsigned readAllGPIO();
  static void sleep55ns();
  static void sleep8ns();
};

#endif  // PANDA_TIMESWIPE_DRIVER_BOARD
