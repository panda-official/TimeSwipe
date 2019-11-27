/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//a simplest command processor:

#include "cmd.h"

CCmdCallDescr::cres CCmdDispatcher::__Call(CCmdCallDescr &d)
{
    //by call method:
   if(CCmdCallDescr::cmethod::byCmdName==d.m_cmethod)
   {

        typeDispTable::const_iterator pCmd=m_DispTable.find(d.m_strCommand);
        if(pCmd!=m_DispTable.end())
        {
            //call:
            return pCmd->second->Call(d);
        }
        return typeCRes::obj_not_found;
   }
   if(CCmdCallDescr::cmethod::byCmdIndex==d.m_cmethod)
   {
        if(d.m_nCmdIndex < static_cast<unsigned int>(m_DispTable.size()))
        {
            typeDispTable::const_iterator pCmd=m_DispTable.begin();
            std::advance(pCmd, d.m_nCmdIndex);
            d.m_strCommand=pCmd->first;
            return pCmd->second->Call(d);
        }
   }
   return typeCRes::obj_not_found;
}
CCmdCallDescr::cres CCmdDispatcher::Call(CCmdCallDescr &d)
{
    typeCRes cres=__Call(d);
    if(d.m_bThrowExcptOnErr)
    {
        if(typeCRes::obj_not_found==cres)
            throw CCmdException("obj_not_found!");
        if(typeCRes::fget_not_supported==cres)
            throw CCmdException(">_not_supported!");
        if(typeCRes::fset_not_supported==cres)
            throw CCmdException("<_not_supported!");
        if(typeCRes::disabled==cres)
            throw CCmdException("disabled!");
    }
    return cres;
}


