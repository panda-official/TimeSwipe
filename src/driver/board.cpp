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

#include "board.hpp"
#include "event.hpp"
#include "gpio/gpio.h"
#include "../common/json.hpp"

#include "../3rdparty/dmitigr/assert.hpp"

#include <mutex>

// RPI GPIO FUNCTIONS
void pullGPIO(unsigned pin, unsigned high)
{
    GPIO_PULL = high << pin;
}

void initGPIOInput(unsigned pin)
{
    INP_GPIO(pin);
}

void initGPIOOutput(unsigned pin)
{
    INP_GPIO(pin);
    OUT_GPIO(pin);
    pullGPIO(pin, 0);
}

void setGPIOHigh(unsigned pin)
{
    GPIO_SET = 1 << pin;
}

void setGPIOLow(unsigned pin)
{
    GPIO_CLR = 1 << pin;
}

static const uint32_t ALL_32_BITS_ON = 0xFFFFFFFF; // (2^32)-1 - ALL BCM_PINS
void resetAllGPIO()
{
    GPIO_CLR = ALL_32_BITS_ON;
}

void sleep55ns()
{
    readAllGPIO();
}

void sleep8ns()
{
    setGPIOHigh(10); // ANY UNUSED PIN!!!
}

unsigned int readAllGPIO()
{
    return (*(gpio + 13) & ALL_32_BITS_ON);
}

static std::mutex boardMtx;

std::list<TimeSwipeEvent> Board::GetEvents()
{
  std::list<TimeSwipeEvent> events;
  std::lock_guard<std::mutex> lock(boardMtx);
#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
  std::string data;
  if (Board::Instance()->getEvents(data) && !data.empty()) {
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

std::string Board::GetSettings(const std::string& request, std::string& error)
{
  std::lock_guard<std::mutex> lock(boardMtx);
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

std::string Board::SetSettings(const std::string& request, std::string& error) {
    std::lock_guard<std::mutex> lock(boardMtx);
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

bool BoardStartPWM(uint8_t num, uint32_t frequency, uint32_t high, uint32_t low, uint32_t repeats, float duty_cycle) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    return Board::Instance()->startPWM(num, frequency, high, low, repeats, duty_cycle);
#endif
}

bool BoardStopPWM(uint8_t num) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    return Board::Instance()->stopPWM(num);
#endif
}

bool BoardGetPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    return Board::Instance()->getPWM(num, active, frequency, high, low, repeats, duty_cycle);
#endif
}

// =============================================================================

nlohmann::json str2json(const std::string& str) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(str);
    } catch (nlohmann::json::parse_error& e) {
        std::cerr << "Board: json parse failed data:" << str << "error:" << e.what() << '\n';
        return nlohmann::json();
    }
    return j;
}

template <class OUT>
bool json_get(const nlohmann::json& j, const std::string& key, OUT& value);

template <>
bool json_get(const nlohmann::json& j, const std::string& key, std::string& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_string()) return false;
    value = it->get<std::string>();
    return true;
}

template <>
bool json_get(const nlohmann::json& j, const std::string& key, uint32_t& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_unsigned()) return false;
    value = it->get<uint32_t>();
    return true;
}

template <>
bool json_get(const nlohmann::json& j, const std::string& key, float& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_number_float()) return false;
    value = it->get<float>();
    return true;
}

template <>
bool json_get(const nlohmann::json& j, const std::string& key, bool& value) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    if (!it->is_boolean()) return false;
    value = it->get<bool>();
    return true;
}

void Board::Init(const bool force)
{
  if (!force && Board::Instance()->IsInited()) return;

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

  Instance()->is_board_inited_ = true;
}

bool Board::IsInited() noexcept
{
  return Instance()->is_board_inited_;
}

void Board::StartMeasurement(const int mode)
{
  DMITIGR_CHECK(Board::Instance()->IsInited());

  // Select Mode
  Instance()->SetMode(mode);

  // Start Measurement
  using std::chrono::milliseconds;
  std::this_thread::sleep_for(milliseconds{1});
  Instance()->SetEnableADmes(true);
  Instance()->is_measurement_started_ = true;
}

bool Board::IsMeasurementStarted() noexcept
{
  return Instance()->is_measurement_started_;
}

void Board::StopMeasurement()
{
  if (!IsMeasurementStarted()) return;

  // Reset Clock
  setGPIOLow(CLOCK);

  // Stop Measurement
  Instance()->SetEnableADmes(false);

  Instance()->is_measurement_started_ = false;
}

bool Board::getPWM(uint8_t num, bool& active, uint32_t& frequency,
  uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle)
{
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
}

bool Board::startPWM(uint8_t num, uint32_t frequency, uint32_t high,
  uint32_t low, uint32_t repeats, float duty_cycle)
{
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
}
