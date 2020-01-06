/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "jsondisp.h"
#include "json_stream.h"

void CJSONDispatcher::DumpAllSettings(const CCmdCallDescr &d, nlohmann::json &jResp)
{
    //! here we use cmethod::byCmdIndex to enumerate all possible "get" handlers:

    CCmdCallDescr CallDescr;    //! the exception mode is set to false
    CallDescr.m_pIn=d.m_pIn; //-!!!
    CallDescr.m_ctype=CCmdCallDescr::ctype::ctGet;
    CallDescr.m_cmethod=CCmdCallDescr::cmethod::byCmdIndex;

    for(CallDescr.m_nCmdIndex=0;; CallDescr.m_nCmdIndex++)
    {
        nlohmann::json jval;
        CJSONStream out(&jval);
        CallDescr.m_pOut=&out;
        typeCRes cres=m_pDisp->Call(CallDescr);
        if(CCmdCallDescr::cres::obj_not_found==cres)
            break;  //!reached the end of the command table
        if(CCmdCallDescr::cres::OK==cres)
        {
            //!filling the jresp obj: command name will be returned in a CallDescr.m_strCommand
            jResp[CallDescr.m_strCommand]=jval;

        }
    }
}

void CJSONDispatcher::CallPrimitive(const std::string &strKey, nlohmann::json &jReq, nlohmann::json &jResp, const CCmdCallDescr::ctype ct)
{
    //prepare:
    CJSONStream in(&jReq);
    CJSONStream out(&jResp);
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
                jResp=jReq;
            }
        }
    }
    catch(const std::exception& ex)
    {
        cres=typeCRes::parse_err;

        //form an error:
        nlohmann::json &jerr=jResp["error"];
        jerr["val"]=jReq;
        jerr["edescr"]=ex.what();
    }
}

void CJSONDispatcher::Call(nlohmann::json &jObj, nlohmann::json &jResp, const CCmdCallDescr::ctype ct, bool bArrayMode)
{
    nlohmann::json stub=""; //stub
    for (auto& el : jObj.items()) {

        nlohmann::json &val=el.value();
        const std::string &strKey=el.key();
        if(val.is_primitive())          //can resolve a call
        {
            if(bArrayMode)
            {
                if(!val.is_string())
                {
                    nlohmann::json &jerr=jResp[strKey]["error"];
                    jerr["edescr"]="cannot resolve this key!";
                    continue;
                }
                const std::string &strValKey=static_cast<const std::string>(val);
                if(CCmdCallDescr::ctype::ctGet!=ct)
                {
                    nlohmann::json &jerr=jResp[strValKey]["error"];
                    jerr["edescr"]="cannot resolve single key in non-get call!";
                    continue;
                }
                CallPrimitive(strValKey, stub, jResp[strValKey], ct);
            }
            else
            {
                CallPrimitive(strKey, val, jResp[strKey], ct);
            }
        }
        else                            //recursy
        {
            Call(val, jResp[strKey], ct, val.is_array());
        }
    }
}

typeCRes CJSONDispatcher::Call(CCmdCallDescr &d)
{
    if(IsCmdSubsysLocked())
        return typeCRes::disabled;

    //! an exception can be thrown wile working with a JSON!

    CJSONCmdLock CmdLock(*this); //lock the command sys against recursive calls

    std::string str;
    nlohmann::json jresp;

    *(d.m_pIn)>>str;
     if(str.empty() && CCmdCallDescr::ctype::ctGet==d.m_ctype)
     {
        DumpAllSettings(d, jresp);
     }
     else
     {
        if( d.m_pIn->bad())
            return typeCRes::parse_err;

        auto cmd=nlohmann::json::parse(str);
        Call(cmd, jresp, d.m_ctype, cmd.is_array());
     }

     *(d.m_pOut)<<jresp.dump();
     return typeCRes::OK;
}
