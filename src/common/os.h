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
 * An operating system interface.
 */

#ifndef PANDA_TIMESWIPE_COMMON_OS_HPP
#define PANDA_TIMESWIPE_COMMON_OS_HPP

/*!
 *  \brief an operating system interface namespace
 */
namespace os {

/*!
 * \brief A time elapsed since system start in milliseconds
 * \return timer value in milliseconds
 */
unsigned long get_tick_mS() noexcept;

/*!
 * \brief Sleep for defined time
 * \param time_mS sleep time in milliseconds
 * \details in case of the cooperative multitasking this call should be used to return the control to the OS scheduler
 */
void wait(unsigned long time_mS);

/*!
 * \brief Sleep for defined microseconds time
 * \param time_mS sleep time in microseconds
 * \details in case of the cooperative multitasking this call should be used to return the control to the OS scheduler
 */
void uwait(unsigned long time_uS);

/*!
 * \brief Set an error for this thread
 * \param perrtxt a pointer to the error string
 */
void set_err(const char *perrtxt);

/*!
 * \brief Clear the current error for the thread
 */
void clear_err();

}

#endif  // PANDA_TIMESWIPE_COMMON_OS_HPP
