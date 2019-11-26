/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <nlohmann/json.hpp>

/*!
 * \brief The CJSONbase a superclass for all JSON-stuff objects
 */

class CJSONbase
{
protected:
    static bool m_bLockCmdSubsys;

public:
    inline void LockCmdSubsys(bool how){m_bLockCmdSubsys=how;}
    inline bool IsCmdSubsysLocked(){return m_bLockCmdSubsys;}
};
