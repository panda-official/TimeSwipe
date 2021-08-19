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
#include <list>
#include <string>
#include <thread>

#include "board_iface.hpp"
#include "gpio/gpio.h"
#include "timeswipe.hpp"

// PIN NAMES
static const unsigned char DATA0 = 24; //BCM 24 - PIN 18
static const unsigned char DATA1 = 25; //BCM 25 - PIN 22
static const unsigned char DATA2 = 7; //BCM 7 - PIN 26
static const unsigned char DATA3 = 5; //BCM 5 - PIN 29
static const unsigned char DATA4 = 6; //BCM 6 - PIN 31
static const unsigned char DATA5 = 12; //BCM 12 - PIN 32
static const unsigned char DATA6 = 13; //BCM 13 - PIN 33
static const unsigned char DATA7 = 16; //BCM 16 - PIN 36
static const unsigned char CLOCK = 4; //BCM 4 - PIN 7
static const unsigned char TCO = 14;   //BCM 14 - PIN 8
static const unsigned char PI_OK = 15; //BCM 15 - PIN 10
static const unsigned char FAIL = 18; //BCM 18 - PIN 12
static const unsigned char RESET = 17; //BCM 17 - PIN 11
static const unsigned char BUTTON = 25; //BCM 25 - PIN 22

static const std::array<uint32_t, 8> DATA_POSITION{
  1UL << DATA0,
  1UL << DATA1,
  1UL << DATA2,
  1UL << DATA3,
  1UL << DATA4,
  1UL << DATA5,
  1UL << DATA6,
  1UL << DATA7
};

static const uint32_t CLOCK_POSITION = 1UL << CLOCK;
static const uint32_t TCO_POSITION = 1UL << TCO;
static const uint32_t PI_STATUS_POSITION = 1UL << PI_OK;
static const uint32_t FAIL_POSITION = 1UL << FAIL;
static const uint32_t BUTTON_POSITION = 1UL << BUTTON;

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
