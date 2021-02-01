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

#ifndef PANDA_TIMESWIPE_COMMON_TIMER_HPP
#define PANDA_TIMESWIPE_COMMON_TIMER_HPP

/*!
*   @file
*   @brief A definition file for basic timer event interface CTimerEvent
*
*/


/*!
 * \brief A callback interface used to notify the derived class that a timer event happened
 */
class CTimerEvent{
public:

    /*!
     * \brief a new timer event triggered
     * \param nId timer ID value
     */
    virtual void OnTimer(int nId)=0;

    //! default constructor
    CTimerEvent()=default;

    /*!
     * \brief remove copy constructor
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    CTimerEvent(const CTimerEvent&) = delete;

    /*!
     * \brief remove copy operator
     * \return
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    CTimerEvent& operator=(const CTimerEvent&) = delete;
protected:

     //! virtual destructor
     virtual ~CTimerEvent()=default;
};

#endif  // PANDA_TIMESWIPE_COMMON_TIMER_HPP
