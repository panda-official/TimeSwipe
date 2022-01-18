/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "jsondisp.h"
#include "stream.hpp"

void CJSONDispatcher::DumpAllSettings(const Setting_descriptor& d,
  rapidjson::Document& jResp)
{
    //! here we use cmethod::byCmdIndex to enumerate all possible "get" handlers:
    auto& alloc = jResp.GetAllocator();
    jResp.SetObject();
    for (int i{} ; ; ++i) {
      using Value = rapidjson::Value;
      Value result;
      Json_stream out{result, &alloc};
      Setting_descriptor descriptor;
      descriptor.in_value_stream = d.in_value_stream;
      descriptor.access_type = Setting_access_type::read;
      descriptor.index = i;
      descriptor.out_value_stream = &out;
      if (const auto err = m_pDisp->handle(descriptor)) {
        if (err == Errc::board_settings_unknown)
          return; // end of command table
      } else
        jResp.AddMember(Value{descriptor.name, alloc}, std::move(result), alloc);
    }
}

void CJSONDispatcher::CallPrimitive(const std::string& strKey,
  rapidjson::Value& jReq,
  rapidjson::Document& jResp,
  rapidjson::Value& resp_root,
  Setting_access_type access_type)
{
  using Value = rapidjson::Value;
  auto& alloc = jResp.GetAllocator();
  /*
   * Add the result member only if it is not added already. Thus, for example,
   * the result of ["temperature", "temperature"] request shall contains only 1
   * "temperature" value.
   */
  if (resp_root.FindMember(strKey) == resp_root.MemberEnd())
    resp_root.AddMember(Value{strKey, alloc}, Value{}, alloc);
  auto& result = resp_root[strKey];
  Json_stream in{jReq, nullptr};
  Json_stream out{result, &alloc};
  Setting_descriptor descriptor;
  descriptor.in_value_stream = &in;
  descriptor.out_value_stream = &out;
  descriptor.name = strKey;
  descriptor.access_type = access_type;
  if (const auto err = m_pDisp->handle(descriptor))
    set_error(jResp, result, err);
}

void CJSONDispatcher::Call(rapidjson::Value& jObj,
  rapidjson::Document& jResp,
  rapidjson::Value& resp_root,
  const Setting_access_type ct)
{
  auto& alloc = jResp.GetAllocator();
  const auto process = [&](const rapidjson::Value& key, rapidjson::Value& val)
  {
    using Value = rapidjson::Value;

    // Get key string.
    if (!key.IsString()) {
      resp_root.AddMember("unresolved", Value{}, alloc);
      set_error(jResp, resp_root["unresolved"],
        Error{Errc::board_settings_unknown, "unresolved reference"});
      return true; // continue
    }
    const std::string key_str = key.GetString();

    // If the key references a subhandler, then call it.
    const auto pSubHandler = m_SubHandlersMap.find(key_str);
    if (pSubHandler != m_SubHandlersMap.end()) {
      typeSubHandler pHandler = pSubHandler->second;
      pHandler(jObj, jResp, ct);
      return false; // done
    }

    // Check the end of possible recursion.
    if (!val.IsObject()) {
      if (jObj.IsArray()) {
        if (ct != Setting_access_type::read) {
          resp_root.AddMember(Value{key_str, alloc}, Value{}, alloc);
          set_error(jResp, resp_root[key_str],
            Error{Errc::board_settings_invalid, "not a read access requested"});
          return true; // continue
        }
      }
      CallPrimitive(key_str, val, jResp, resp_root, ct);
    } else {
      // Recursive call.
      resp_root.AddMember(Value{key_str, alloc}, Value{rapidjson::kObjectType}, alloc);
      Call(val, jResp, resp_root[key_str], ct);
    }

    return true; // continue
  };

  if (jObj.IsArray()) {
    for (auto& val : jObj.GetArray())
      if (!process(val, val)) return;
  } else if (jObj.IsObject()) {
    for (auto& el : jObj.GetObject())
      if (!process(el.name, el.value)) return;
  }
}

Error CJSONDispatcher::handle(Setting_descriptor& d)
{
    if (IsCmdSubsysLocked())
      return Errc::generic;

    CJSONCmdLock CmdLock(*this); //lock the command sys against recursive calls

    std::string str;
    rapidjson::Document jresp{rapidjson::kObjectType};
    *(d.in_value_stream) >> str;
     if (str.empty() && (d.access_type == Setting_access_type::read))
     {
       DumpAllSettings(d, jresp);
     }
     else
     {
        if (!d.in_value_stream->is_good())
          return Errc::board_settings_invalid;

        rapidjson::Document cmd;
        const rapidjson::ParseResult pr{cmd.Parse(str.data(), str.size())};
        if (pr)
          Call(cmd, jresp, jresp, d.access_type);
        else
          return Errc::board_settings_invalid;
     }
     *d.out_value_stream << to_text(jresp);
     return Errc::ok;
}
