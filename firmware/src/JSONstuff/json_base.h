/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <nlohmann/json.hpp>

/*!
 * \brief A superclass for all JSON-stuff objects
 */

class CJSONbase
{
protected:
    static int m_nLockCmdSubsysCnt;

public:
    inline void LockCmdSubsys(bool how)
    {
       if(how)
           m_nLockCmdSubsysCnt++;
       else
          m_nLockCmdSubsysCnt--;
    }
    inline bool IsCmdSubsysLocked(){return (m_nLockCmdSubsysCnt>0);}
};

/*!
 * \brief A JSON command sub-sys auto-locker
 */

class CJSONCmdLock
{
protected:
    CJSONbase &m_refCntrObj;

public:
    CJSONCmdLock(CJSONbase &obj) : m_refCntrObj(obj)
    {
        m_refCntrObj.LockCmdSubsys(true);
    }
    ~CJSONCmdLock()
    {
        m_refCntrObj.LockCmdSubsys(false);
    }
};
