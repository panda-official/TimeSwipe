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

/**
 * @file
 * CPin stuff.
 */

#ifndef PANDA_TIMESWIPE_FIRMWARE_PIN_HPP
#define PANDA_TIMESWIPE_FIRMWARE_PIN_HPP

#include "os.h"

/**
 * Pin abstraction.
 *
 * @details There are two possible Pin behaviors:
 *   - normal behavior: logical `true` denotes high output level (1), logical
 *   `false` denotes low output level (0);
 *   - inverted behavior: logical `true` denotes low output level (0), logical
 *   `false` denotes high output level (1).
 */
class Pin {
public:
  /// The destructor.
  virtual ~Pin() = default;

  /// The default-constructible.
  Pin() = default;

  /// Non copy-constructible.
  Pin(const Pin&) = delete;

  /// Non copy-assignable.
  Pin& operator=(const Pin&) = delete;

  /// Non move-constructible.
  Pin(Pin&&) = delete;

  /// Non move-assignable.
  Pin& operator=(Pin&&) = delete;

  /**
   * @brief Sets logic state of the pin.
   *
   * @param how The logical state to be set.
   *
   * @remarks May differ from actual output level.
   *
   * @see SetInvertedBehaviour().
   */
  void Set(const bool how)
  {
    impl_Set(m_bInvertedBehaviour ? !how:how);

    if(m_SetupTime_uS)
      os::uwait(m_SetupTime_uS);
  }

  /**
   * @brief Reads back set logical state of the pin.
   *
   * @returns The pin value that was set.
   */
  bool RbSet()
  {
    bool rv=impl_RbSet();
    return m_bInvertedBehaviour ? !rv:rv;
  }

  /**
   * @returns Measured logic state when pin acts as an input.
   *
   * @remarks May differ from actual output level.
   *
   * @see SetInvertedBehaviour().
   */
  bool Get()
  {
    bool rv=impl_Get();
    return m_bInvertedBehaviour ? !rv:rv;
  }

  /// Inverts logic behaviour of the pin.
  void SetInvertedBehaviour(bool how)
  {
    m_bInvertedBehaviour=how;
  }

  /**
   * @brief Sets output level setup time.
   *
   * @details In general, pin output level doesn't changes instantly. It takes
   * a while to wait for the level to rise or fall.
   *
   * @param nSetupTime_uS - the setup time in uS
   */
  void SetPinSetupTime(unsigned long nSetupTime_uS)
  {
    m_SetupTime_uS=nSetupTime_uS;
  }

private:
  /// Called by Set().
  virtual void impl_Set(bool bHow) = 0;

  /// Called by Rbset().
  virtual bool impl_RbSet() = 0;

  /// Called by Get().
  virtual bool impl_Get()=0;

protected:
  bool m_bInvertedBehaviour{};
  unsigned long m_SetupTime_uS{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_PIN_HPP
