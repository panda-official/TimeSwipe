/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "jsondisp.h"
#include "json_stream.h"

void CJSONDispatcher::Call(nlohmann::json &jObj, nlohmann::json &jResp, CCmdCallDescr::ctype ct)
{
    for (auto& el : jObj.items()) {

        nlohmann::json &val=el.value();
        nlohmann::json &rval=jResp[el.key()];
        if(val.is_primitive())          //can resolve a call
        {
            //prepare:
            CJSONStream in(&val);
            CJSONStream out(&rval);
            CCmdCallDescr CallDescr;
            CallDescr.m_pIn=&in;
            CallDescr.m_pOut=&out;
            CallDescr.m_strCommand=el.key();
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
                        rval=val;
                    }
                }
            }
            catch(const std::exception& ex)
            {
                cres=typeCRes::parse_err;

                //form an error:
                nlohmann::json &jerr=rval["error"];
                jerr["val"]=val;
                jerr["edescr"]=ex.what();
            }
        }
        else                            //recursy
        {
            Call(val, rval, ct);
        }
    }
}

typeCRes CJSONDispatcher::Call(CCmdCallDescr &d)
{
    std::string str;

    *(d.m_pIn)>>str;
    if( d.m_pIn->bad()){
        return typeCRes::parse_err; }

     nlohmann::json jresp;
     auto cmd=nlohmann::json::parse(str);
     Call(cmd, jresp, d.m_ctype);
     *(d.m_pOut)<<jresp.dump();

     return typeCRes::OK;
}
