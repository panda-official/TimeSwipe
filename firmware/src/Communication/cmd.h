/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief A simple command proccessor structures and classes.
*
*   @details The processing of all incoming command requests is realized by a CCmdDispatcher
*   object instance. The port object instance that implements the current communication protocol (simple text, binary/specific)
*   transforms an incoming request from a protocol depended form to an uniform request described by CCmdCallDescr class
*   where command name in a string format (if presented) its hash value,  pointers to input/output streams and other service
*   information are stored. Then a port that holds a pointer to the CCmdDispatcher calls  CCmdDispatcher methode "Call" with
*   CCmdCallDescr as a parameter. CCmdDispatcher process the incoming request by finding specific command handler
*   in its internal map and invoking the handler. The call result is stored in the same CCmdCallDescr parameter and
*   hence it is further accesible by a port.
*   By adding number of different port instances several communication protocols can be implemented at once.
*/

#pragma once

#include <memory>
#include <string>
#include <map>
#include "frm_stream.h"

/*!
*   @brief A uniform command request descriptor.
*
*/

struct CCmdCallDescr
{
    std::string     m_strCommand;   //!the command string name
    int             m_hashCommand;  //!the command hash value
    unsigned int    m_nCmdIndex;    //!the command index (zero-based)

    CFrmStream    *m_pIn=nullptr;   //!input stream: to fetch function/methode input arguments
    CFrmStream    *m_pOut=nullptr;  //!output stream: to store function/methodes output arguments or return value

    //!command handler invocation result ("call result"=cres)
    enum cres :int {

        OK=0,               //!<successful invocation
        obj_not_found,      //!<requested command(object) was not found
        fget_not_supported, //!<"get" property is not supported by a handler
        fset_not_supported, //!<"set" property is not supported by a handler
        parse_err,          //!<an error occurred while parsing arguments from the input stream
        disabled            //!<handler is disabled for some reasons
    };

    //!invocation type ("call type"=ctype)
    enum ctype :int {

        ctGet=1,            //!<"get" property
        ctSet=2             //!<"set" property

    }m_ctype=ctGet;

    //!how to dispatch an invocation? by command string name, hash value or index?
    enum cmethod :int {

        byCmdName=1,        //!<by a command string name (using m_strCommand)
        byCmdHash=2,        //!<by a command hash value  (using m_hashCommand)
        byCmdIndex=4        //!<by a command zero-based index (using m_nCmdIndex)
    }m_cmethod=byCmdName;

    bool m_bThrowExcptOnErr=false; //! if true, throw an exception CCmdException instead of returning cres


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
     typeCRes __Call(CCmdCallDescr &d);

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

                //14.08.2019: feedback
                if(m_pGetter)
                {
                     *(d.m_pOut)<<(m_pObj.get()->*m_pGetter)();
                }

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

                    //14.08.2019: feedback
                    if(m_pGetter)
                    {
                        *(d.m_pOut)<<m_pGetter();
                    }
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




