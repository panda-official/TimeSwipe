/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//a simplest command processor:

#pragma once

#include <memory>
#include <string>
#include <map>
#include "frm_stream.h"

struct CCmdCallDescr
{
    std::string     m_strCommand;
    int             m_hashCommand;

    CFrmStream    *m_pIn=nullptr;
    CFrmStream    *m_pOut=nullptr;

    //call result:
    enum cres :int {

        OK=0,
        obj_not_found,
        fget_not_supported,
        fset_not_supported,
        parse_err
    };

    //call type: get,set,methode, etc:
    enum ctype :int {

        ctGet=1,
        ctSet=2

    }m_ctype=ctGet;

    //09.06.2019: flags
    bool m_bThrowExcptOnErr=false;


};
typedef  CCmdCallDescr::cres typeCRes;


//cmd proc exception: 09.06.2019
#include <string.h>
//#include <exception>
class CCmdException : public std::exception
{
protected:
    char m_descr[64]; //light exception

public:
    explicit CCmdException(const char *pDescr)
    {
        strcpy(m_descr, pDescr);
    }
    virtual const char* what() const noexcept
    {
        return m_descr;
    }

};


struct CCmdCallHandler
{
    virtual typeCRes Call(CCmdCallDescr &d)=0;
};

typedef std::map<std::string, std::shared_ptr<CCmdCallHandler>> typeDispTable;
class  CCmdDispatcher
{
protected:
     typeDispTable m_DispTable;

public:

     void Add(const char *pCmdName, const std::shared_ptr<CCmdCallHandler> &pHandler)
     {
         m_DispTable.emplace(pCmdName, pHandler);
     }
     typeCRes Call(CCmdCallDescr &d);
};


//set/get handler:
template<typename typeClass, typename typeArg>
class CCmdSGHandler : public CCmdCallHandler
{
protected:
    //typeClass *m_pObj;
    std::shared_ptr<typeClass> m_pObj;
    typeArg (typeClass::*m_pGetter)(void);
    void    (typeClass::*m_pSetter)(typeArg val);

public:
    CCmdSGHandler(const std::shared_ptr<typeClass> &pObj, typeArg (typeClass::*pGetter)(void), void (typeClass::*pSetter)(typeArg val)=nullptr)
    {
        m_pObj=pObj;
        m_pGetter=pGetter;
        m_pSetter=pSetter;
    }
    virtual typeCRes Call(CCmdCallDescr &d)
    {
        if(d.m_ctype & CCmdCallDescr::ctype::ctSet) //set
        {
            if(m_pSetter)
            {
                typeArg val;
                *(d.m_pIn)>>val;
                if( d.m_pIn->bad())
                    return typeCRes::parse_err;

                (m_pObj.get()->*m_pSetter)(val);
            }
            else {
                //error:
                return typeCRes::fset_not_supported;
            }
        }
        if(d.m_ctype & CCmdCallDescr::ctype::ctGet)
        {
            if(m_pGetter)
            {
                 *(d.m_pOut)<<(m_pObj.get()->*m_pGetter)();
            }
            else {
                //error
                return typeCRes::fget_not_supported;
            }
        }
        return typeCRes::OK;
    }
};

template<typename typeArg>
class CCmdSGHandlerF : public CCmdCallHandler
{
protected:
    typeArg (*m_pGetter)(void);
    void    (*m_pSetter)(typeArg val);

public:
    CCmdSGHandlerF(typeArg (*pGetter)(void), void (*pSetter)(typeArg val)=nullptr)
    {
        m_pGetter=pGetter;
        m_pSetter=pSetter;
    }
    virtual typeCRes Call(CCmdCallDescr &d)
    {
        if(d.m_ctype & CCmdCallDescr::ctype::ctSet) //set
        {
            if(m_pSetter)
            {
                typeArg val;
                *(d.m_pIn)>>val;
                if( d.m_pIn->bad()){
                    return typeCRes::parse_err; }

                    m_pSetter(val);
            }
            else {
                //error:
                return typeCRes::fset_not_supported;
            }
        }
        if(d.m_ctype & CCmdCallDescr::ctype::ctGet)
        {
            if(m_pGetter)
            {
                *(d.m_pOut)<<m_pGetter();
            }
            else {
                //error
                return typeCRes::fget_not_supported;
            }
        }
        return typeCRes::OK;
    }
};




