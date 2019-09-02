/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "json_evsys.h"
unsigned long get_tick_mS(void);
void CJSONEvDispatcher::RaiseEventFlag(bool how)
{
    m_EvFlagIsRaised=how;
    m_EvFlagRaiseTStamp_mS=get_tick_mS();
}

void CJSONEvDispatcher::on_event(const char *key, nlohmann::json &val)
{
    if(!IsEventFlagRaised())
    {
        //reserve place for a time? -> 18.06.2019: doesn't matter
       // m_event["telapsed_mS"]=0;
        RaiseEventFlag(true);
    }
    m_event[key]=val;
}
typeCRes CJSONEvDispatcher::Call(CCmdCallDescr &d)
{
    if(d.m_ctype & CCmdCallDescr::ctype::ctSet) //set
    {
       return typeCRes::fset_not_supported;
    }

   // m_event["telapsed_mS"]=get_tick_mS()-m_EvFlagRaiseTStamp_mS; 14.08.2019 - empty message by default
    RaiseEventFlag(false);

    //don't send if there is nothing to send 14.08.2019
    if(!m_event.empty())
    {
        *(d.m_pOut)<<m_event.dump();
        m_event.clear();
    }

    return typeCRes::OK;
}
