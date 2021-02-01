/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

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
