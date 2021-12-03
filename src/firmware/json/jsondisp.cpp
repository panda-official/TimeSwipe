/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "jsondisp.h"
#include "json_stream.h"
//#include "nodeControl.h"

void CJSONDispatcher::DumpAllSettings(const CCmdCallDescr& d, rapidjson::Document& jResp)
{
    //! here we use cmethod::byCmdIndex to enumerate all possible "get" handlers:
    CCmdCallDescr CallDescr;    //! the exception mode is set to false
    CallDescr.m_pIn=d.m_pIn; //-!!!
    CallDescr.m_ctype=CCmdCallDescr::ctype::ctGet;
    CallDescr.m_cmethod=CCmdCallDescr::cmethod::byCmdIndex;

    auto& alloc = jResp.GetAllocator();
    jResp.SetObject();
    for (CallDescr.m_nCmdIndex=0;; CallDescr.m_nCmdIndex++) {
      rapidjson::Value jval;
      CJSONStream out(jval, &alloc);
      CallDescr.m_pOut=&out;
      typeCRes cres=m_pDisp->Call(CallDescr);
      if(CCmdCallDescr::cres::obj_not_found==cres)
        break;  //!reached the end of the command table
      if (CCmdCallDescr::cres::OK==cres) {
        //!filling the jresp obj: command name will be returned in a CallDescr.m_strCommand
        using Value = rapidjson::Value;
        jResp.AddMember(Value{CallDescr.m_strCommand, alloc}, std::move(jval), alloc);
        // jResp[CallDescr.m_strCommand]=jval;
      }
    }
}

void CJSONDispatcher::CallPrimitive(const std::string& strKey, rapidjson::Value& jReq, rapidjson::Document& jResp, rapidjson::Value& resp_root, const CCmdCallDescr::ctype ct)
{
  //prepare:
    using Value = rapidjson::Value;
    auto& alloc = jResp.GetAllocator();
    resp_root.AddMember(Value{strKey, alloc}, Value{}, alloc);
    resp_root = resp_root[strKey];
    CJSONStream in(jReq, nullptr);
    CJSONStream out(resp_root, &alloc);
    CCmdCallDescr CallDescr;
    CallDescr.m_pIn=&in;
    CallDescr.m_pOut=&out;
    CallDescr.m_strCommand=strKey;
    CallDescr.m_ctype=ct;
    CallDescr.m_bThrowExcptOnErr=true;
    typeCRes cres;

    try{

        if(CCmdCallDescr::ctype::ctSet==ct)
        {
            cres=m_pDisp->Call(CallDescr); //set
            CallDescr.m_bThrowExcptOnErr=false; //test can we read back a value...
        }
        //and get back:
        CallDescr.m_ctype=CCmdCallDescr::ctype::ctGet;
        cres=m_pDisp->Call(CallDescr);
        if(CCmdCallDescr::ctype::ctSet==ct)
        {
            if(typeCRes::fget_not_supported==cres)
            {
              resp_root.CopyFrom(jReq, alloc, true);
            }
        }
    }
    catch(const std::exception& ex)
    {
      set_error(jResp, resp_root, ex.what(), jReq);
    }
}

void CJSONDispatcher::Call(rapidjson::Value& jObj,
  rapidjson::Document& jResp,
  rapidjson::Value& resp_root,
  const CCmdCallDescr::ctype ct)
{
  auto& alloc = jResp.GetAllocator();
  const auto process = [&](const rapidjson::Value& key, rapidjson::Value& val)
  {
    using Value = rapidjson::Value;

    if (!key.IsString()) {
      resp_root.AddMember("unresolved", Value{}, alloc);
      set_error(jResp, resp_root["unresolved"], "unresolved reference", key);
      return true; // continue
    }
    const std::string key_str = key.GetString();

    //is it a protocol extension?
    const auto pSubHandler = m_SubHandlersMap.find(key_str);
    if (pSubHandler != m_SubHandlersMap.end()) {
      //exec handler:
      typeSubHandler pHandler = pSubHandler->second;
      //(this->*pHandler)(jObj, jResp, ct);
      pHandler(jObj, jResp, ct);
      return false; // exit Call()
    }

    // Check the end of possible recursion.
    if (!val.IsObject()) {
      if (jObj.IsArray()) {
        if (ct != CCmdCallDescr::ctype::ctGet) {
          resp_root.AddMember(Value{key_str, alloc}, Value{}, alloc);
          set_error(jResp, resp_root[key_str], "not a get-call");
          return true; // continue
        }
        Value stub{""};
        CallPrimitive(key_str, stub, jResp, resp_root, ct);
      } else
        CallPrimitive(key_str, val, jResp, resp_root, ct);
    } else {
      // Recursive call.
      resp_root.AddMember(Value{key_str, alloc}, Value{rapidjson::kObjectType}, alloc);
      Call(val, jResp, resp_root[key_str], ct);
    }

    return true; // continue
  };

  if (jObj.IsArray())
    for (auto& val : jObj.GetArray())
      if (!process(val, val)) return;
  else if (jObj.IsObject())
    for (auto& el : jObj.GetObject())
      if (!process(el.name, el.value)) return;
}

typeCRes CJSONDispatcher::Call(CCmdCallDescr &d)
{
    if (IsCmdSubsysLocked()) return typeCRes::disabled;

    CJSONCmdLock CmdLock(*this); //lock the command sys against recursive calls

    std::string str;
    rapidjson::Document jresp{rapidjson::kObjectType};
    *(d.m_pIn)>>str;
     if (str.empty() && (d.m_ctype == CCmdCallDescr::ctype::ctGet))
     {
       DumpAllSettings(d, jresp);
     }
     else
     {
        if (d.m_pIn->bad())
          return typeCRes::parse_err;

        rapidjson::Document cmd;
        const rapidjson::ParseResult pr{cmd.Parse(str.data(), str.size())};
        if (pr)
          Call(cmd, jresp, jresp, d.m_ctype);
        else
          return typeCRes::parse_err;
     }
     *(d.m_pOut)<<to_text(jresp);
     return typeCRes::OK;
}
