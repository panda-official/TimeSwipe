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

#ifndef PANDA_TIMESWIPE_COMMON_BUTTON_HPP
#define PANDA_TIMESWIPE_COMMON_BUTTON_HPP

/*!
*   @file
*   @brief A definition file for basic button event interface CButtonEvent
*
*/

/*!
 * \brief The enumeration of possible button states
 */
enum class typeButtonState
{
    pressed,     //!<button is pressed
    released,     //!<button is released

    short_click,
    long_click,
    double_click,
    very_long_click
};

/*!
 * \brief A callback interface used to notify the derived class that a button state is changed
 */
class CButtonEvent{
public:

    /*!
     * \brief The button state has been changed
     * \param nState the current button state (pressed or released)
     */
    virtual void OnButtonState(typeButtonState nState)=0;

    //! default constructor
    CButtonEvent()=default;

    /*!
     * \brief remove copy constructor
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
        that is unacceptable)
     */
    CButtonEvent(const CButtonEvent&) = delete;

    /*!
     * \brief remove copy operator
     * \return
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
        that is unacceptable)
     */
    CButtonEvent& operator=(const CButtonEvent&) = delete;
protected:
    //! virtual destructor
    virtual ~CButtonEvent()=default;
};

#endif  // PANDA_TIMESWIPE_COMMON_BUTTON_HPP
