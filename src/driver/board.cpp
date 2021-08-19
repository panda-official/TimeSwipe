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
#include "board_iface.hpp"
#include "event.hpp"
#include "gpio/gpio.h"
#include "../common/json.hpp"

#include "../3rdparty/dmitigr/assert.hpp"

#include <atomic>
#include <mutex>

std::atomic_bool is_board_inited;
std::atomic_bool is_measurement_started;

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

void InitBoard(const bool force)
{
  if (!force && IsBoardInited()) return;

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

  is_board_inited = true;
}

bool IsBoardInited() noexcept
{
  return is_board_inited;
}

void StartMeasurement(const int mode)
{
  DMITIGR_CHECK(IsBoardInited());

  // Select Mode
  BoardInterface::get()->setMode(mode);

  // Start Measurement
  using std::chrono::milliseconds;
  std::this_thread::sleep_for(milliseconds{1});
  BoardInterface::get()->setEnableADmes(1);

  is_measurement_started = true;
}

bool IsMeasurementStarted() noexcept
{
  return is_measurement_started;
}

void StopMeasurement()
{
  if (!IsMeasurementStarted()) return;

  // Reset Clock
  setGPIOLow(CLOCK);

  // Stop Measurement
  BoardInterface::get()->setEnableADmes(0);

  is_measurement_started = false;
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

std::list<TimeSwipeEvent> readBoardEvents()
{
    std::list<TimeSwipeEvent> events;
    std::lock_guard<std::mutex> lock(boardMtx);
#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    std::string data;
    if (BoardInterface::get()->getEvents(data) && !data.empty()) {
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

std::string readBoardGetSettings(const std::string& request, std::string& error) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    return BoardInterface::get()->getGetSettings(request, error);
#endif
}

std::string readBoardSetSettings(const std::string& request, std::string& error) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return request;
#else
    return BoardInterface::get()->getSetSettings(request, error);
#endif
}

bool BoardStartPWM(uint8_t num, uint32_t frequency, uint32_t high, uint32_t low, uint32_t repeats, float duty_cycle) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    return BoardInterface::get()->startPWM(num, frequency, high, low, repeats, duty_cycle);
#endif
}

bool BoardStopPWM(uint8_t num) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    return BoardInterface::get()->stopPWM(num);
#endif
}

bool BoardGetPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle) {
    std::lock_guard<std::mutex> lock(boardMtx);
#ifdef PANDA_TIMESWIPE_FIRMWARE_EMU
    return false;
#else
    return BoardInterface::get()->getPWM(num, active, frequency, high, low, repeats, duty_cycle);
#endif
}

void BoardTraceSPI(bool val)
{
    BoardInterface::trace_spi = val;
}
