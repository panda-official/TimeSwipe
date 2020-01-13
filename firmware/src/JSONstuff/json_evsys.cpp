/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "os.h"
#include "json_evsys.h"

void CJSONEvDispatcher::on_event(const char *key, nlohmann::json &val)
{
    m_event[key]=val;
}
typeCRes CJSONEvDispatcher::Call(CCmdCallDescr &d)
{
    if(IsCmdSubsysLocked())
        return typeCRes::disabled;

    if(d.m_ctype & CCmdCallDescr::ctype::ctSet)
    {
       return typeCRes::fset_not_supported;
    }

    //don't send if there is nothing to send
    if(!m_event.empty())
    {
        *(d.m_pOut)<<m_event.dump();
        m_event.clear();
    }
    return typeCRes::OK;
}
