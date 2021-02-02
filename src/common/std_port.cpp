/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


//iostream based port:

#include "std_port.h"
#include "frm_stream.h"

void CStdPort::parser(typeSChar ch)
{
    if(m_bTrimming)
    {
        if(' '==ch)
            return;

        m_bTrimming=false;
    }

    if(TERM_CHAR==ch)
    {
        typeCRes cres;

        //preparing streams:
        CFrmStream in(&m_In);
        CFrmStream out(&m_Out);

        try{

            if(proc_args!=m_PState)
                throw CCmdException("protocol_error!");

            //call:
            m_CallDescr.m_pIn=&in;
            m_CallDescr.m_pOut=&out;
            m_CallDescr.m_bThrowExcptOnErr=true;
            cres=m_pDisp->Call(m_CallDescr);

        }
        catch(const std::exception& ex)
        {
           out<<"!"<<ex.what();
        }

        //send data:
     /*if(typeCRes::OK!=cres) switch(cres){

            case typeCRes::parse_err:           out<<"!parse_err!";       break;
            case typeCRes::obj_not_found:       out<<"!obj_not_found!";   break;
            case typeCRes::fget_not_supported:  out<<"!>_not_supported!";  break;
            case typeCRes::fset_not_supported:  out<<"!<_not_supported!";  break;
        }*/
        m_Out<<TERM_CHAR;
        m_pBus->send(m_Out);

        //reset:
        reset();
        return; //!!!!
    }

    switch(m_PState)
    {
        case proc_cmd:
            if(' '==ch || '<'==ch || '>'==ch)
            {
                m_PState=proc_function;
                m_bTrimming=true;
                parser(ch);
                return;
            }
            m_CallDescr.m_strCommand+=ch;
        return;

        case proc_function:
            if('>'==ch) //get
            {
                m_CallDescr.m_ctype=CCmdCallDescr::ctype::ctGet;
                m_PState=proc_args;
                m_bTrimming=true;
            }
            else if('<'==ch) //set
            {
                m_CallDescr.m_ctype=CCmdCallDescr::ctype::ctSet;
                m_PState=proc_args;
                m_bTrimming=true;
            }
            else {

                //format error: no function
                //parser(TERM_CHAR);
                m_PState=err_protocol;

            }
        return;

        case proc_args:
            m_In<<ch;
        return;
    }
}
