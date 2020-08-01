/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   @file
*   @brief A definition file for operating system interface ::os
*
*/


/*!
 *  \brief an operating system interface namespace
 */
namespace os {

/*!
 * \brief A time elapsed since system start in milliseconds
 * \return timer value in milliseconds
 */
unsigned long get_tick_mS(void);

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


/*#ifndef OS_H
#define OS_H

#endif // OS_H*/
