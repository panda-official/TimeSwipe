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

Error CJSONEvDispatcher::handle(Setting_descriptor& d)
{
  if (IsCmdSubsysLocked()) // FIXME: REMOVEME
    return Errc::generic;
  else if (d.access_type == Setting_access_type::write)
    return Errc::board_settings_write_forbidden;

  if (!m_event.ObjectEmpty()) {
    set(d.out_value, to_text(m_event));
    m_event.RemoveAllMembers();
  }

  return {};
}
