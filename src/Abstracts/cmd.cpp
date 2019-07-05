//a simplest command processor:

#include "cmd.h"

CCmdCallDescr::cres CCmdDispatcher::Call(CCmdCallDescr &d)
{
   typeDispTable::const_iterator pCmd=m_DispTable.find(d.m_strCommand);
   if(pCmd!=m_DispTable.end())
   {
       //call:
       typeCRes cres=pCmd->second->Call(d);
       if(d.m_bThrowExcptOnErr)
       {
           if(typeCRes::fget_not_supported==cres)
               throw CCmdException(">_not_supported!");
           if(typeCRes::fset_not_supported==cres)
               throw CCmdException("<_not_supported!");
       }
       return cres;
   }

   //error: cannot found 09.06.2019
   if(d.m_bThrowExcptOnErr)
       throw CCmdException("obj_not_found!");
   return typeCRes::obj_not_found;
}

