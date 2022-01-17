/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "../os.h"
#include "json_evsys.h"

void CJSONEvDispatcher::on_event(const char* key, rapidjson::Value& val)
{
  using Value = rapidjson::Value;
  auto& alloc = m_event.GetAllocator();
  if (m_event.FindMember(key) == m_event.MemberEnd())
    m_event.AddMember(Value{std::string{key}, alloc}, Value{}, alloc);
  m_event[key].CopyFrom(val, alloc, true);
}
typeCRes CJSONEvDispatcher::handle(Setting_descriptor &d)
{
    if(IsCmdSubsysLocked())
        return typeCRes::disabled;

    if(d.m_ctype == Setting_descriptor::ctype::ctSet)
    {
       return typeCRes::fset_not_supported;
    }

    //don't send if there is nothing to send
    if (!m_event.ObjectEmpty())
    {
        *(d.m_pOut) << to_text(m_event);
        m_event.RemoveAllMembers();
    }
    return typeCRes::OK;
}
