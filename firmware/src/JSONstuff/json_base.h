/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CJSONbase and CJSONCmdLock
*/

#pragma once

#include <nlohmann/json.hpp>

/*!
 * \brief A superclass for all JSON-stuff objects
 * \details The class used for controlling the entire JSON system. For exampe all JSON command handlers can be switched on/off
 * by calling a CJSONbase::LockCmdSubsys(...)
 */

class CJSONbase
{
protected:

    /*!
     * \brief Locks counter
     */
    static int m_nLockCmdSubsysCnt;

public:

    /*!
     * \brief Locks entire JSON command handlers (makes them "disabled" )
     * \param how true=lock, false=unlock
     */
    inline void LockCmdSubsys(bool how)
    {
       if(how)
           m_nLockCmdSubsysCnt++;
       else
          m_nLockCmdSubsysCnt--;
    }

    /*!
     * \brief Is command system locked
     * \return true=locked, false=unlocked
     */
    inline bool IsCmdSubsysLocked(){return (m_nLockCmdSubsysCnt>0);}
};

/*!
 * \brief A JSON command sub-sys auto-locker (lock helper)
 * \details Used to automatically unlock the command handlers when leaving the scope.
 *  Must be used inside try{} catch(..) blocks to prevent "locked forever" state
 */

class CJSONCmdLock
{
protected:

    /*!
     * \brief A reference to the monitoring object
     */
    CJSONbase &m_refCntrObj;

public:
    /*!
     * \brief The class constructor. Automatically locks monitoring object
     * \param obj A monitoring object
     */
    CJSONCmdLock(CJSONbase &obj) : m_refCntrObj(obj)
    {
        m_refCntrObj.LockCmdSubsys(true);
    }

    /*!
     * \brief The class destructor. Automatically unlocks monitoring object
     */
    ~CJSONCmdLock()
    {
        m_refCntrObj.LockCmdSubsys(false);
    }
};
