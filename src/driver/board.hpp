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

#include <array>
#include <chrono>
#include <cstdint>
#include <list>
#include <string>
#include <thread>

#include "types_fwd.hpp"

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

/**
 * Initializes GPIO pins.
 *
 * @param force Forces initialization even if IsBoardInited() returns `true`.
 *
 * @par Effects
 * Restarts TimeSwipe firmware on very first run!
 *
 * @see IsBoardInited(), StartMeasurement().
 */
void InitBoard(bool force = false);

/**
 * @returns `true` if InitBoard() has been successfully called at least once.
 *
 * @par Thread-safety
 * Thread-safe.
 *
 * @see InitBoard().
 */
bool IsBoardInited() noexcept;

/**
 * Sends the command to a TimeSwipe firmware to start measurement.
 *
 * @param mode Measurement mode.
 *
 * @par Requires
 * `IsBoardInited()`.
 *
 * @par Effects
 * The reader does receive the data from the board.
 *
 * @see InitBoard(), StopMeasurement().
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

void pullGPIO(unsigned pin, unsigned high);
void initGPIOInput(unsigned pin);
void initGPIOOutput(unsigned pin);
void setGPIOHigh(unsigned pin);
void setGPIOLow(unsigned pin);
void resetAllGPIO();
void sleep55ns();
void sleep8ns();
unsigned int readAllGPIO();
std::list<TimeSwipeEvent> readBoardEvents();
std::string readBoardGetSettings(const std::string& request, std::string& error);
std::string readBoardSetSettings(const std::string& request, std::string& error);
bool BoardStartPWM(uint8_t num, uint32_t frequency, uint32_t high, uint32_t low, uint32_t repeats, float duty_cycle);
bool BoardStopPWM(uint8_t num);
bool BoardGetPWM(uint8_t num, bool& active, uint32_t& frequency, uint32_t& high, uint32_t& low, uint32_t& repeats, float& duty_cycle);
void BoardTraceSPI(bool val);

#endif  // PANDA_TIMESWIPE_DRIVER_BOARD
