/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

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
