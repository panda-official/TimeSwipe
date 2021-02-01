/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief A simple command proccessor structures and classes:
*   CCmdCallDescr, CCmdException, CCmdCallHandler, CCmdSGHandler, CCmdSGHandlerF
*
*/

#pragma once

#include <memory>
#include <string>
#include <map>
#include "frm_stream.h"

/*!
*   @brief An uniform command request descriptor.
*
*   @details The processing of all incoming command requests is released by a CCmdDispatcher
*   object instance. The port object instance that implements the current communication protocol (simple text, binary/specific)
*   transforms an incoming request from a protocol depended form to an uniform request described by CCmdCallDescr class
*   where command name in a string format (if presented) its hash value,  pointers to input/output streams and other service
*   information are stored. Then a port that holds a pointer to the CCmdDispatcher calls  CCmdDispatcher methode "Call" with
*   CCmdCallDescr as a parameter. CCmdDispatcher process the incoming request by finding specific command handler
*   in its internal map and invoking the handler. The call result is stored in the same CCmdCallDescr parameter and
*   hence it is further accesible by a port.
*   By adding number of different port instances several communication protocols can be implemented at once.
*
*/

struct CCmdCallDescr
{
    //!the command in a string format
    std::string     m_strCommand;

    //!a hash value of the command string
    int             m_hashCommand;

    //!a zero based index of the command
    unsigned int    m_nCmdIndex;

    //!input stream: to fetch function/methode input arguments
    CFrmStream    *m_pIn=nullptr;

    //!output stream: to store function/methodes output arguments or return value
    CFrmStream    *m_pOut=nullptr;

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

    //!how to dispatch an invocation? by a command in a string format, its hash value or index?
    enum cmethod :int {

        byCmdName=1,        //!<by a command in a string format (using m_strCommand)
        byCmdHash=2,        //!<by a command's hash value  (using m_hashCommand)
        byCmdIndex=4        //!<by a command's zero-based index (using m_nCmdIndex)
    }m_cmethod=byCmdName;

    //! if true, throw an exception CCmdException instead of returning cres
    bool m_bThrowExcptOnErr=false;


};
typedef  CCmdCallDescr::cres typeCRes;



#include <string.h>

/*!
 * \brief A command execution exception
 */
class CCmdException : public std::exception
{
protected:

    /*!
     * \brief Buffer for holding error text description
     */
    char m_descr[64];

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

/*!
 * \brief A basic class for command handler
 */
struct CCmdCallHandler
{
    /*!
     * \brief A method for handling a concrete command
     * \param d Call descriptor in protocol-independent format
     * \return
     */
    virtual typeCRes Call(CCmdCallDescr &d)=0;
};


typedef std::map<std::string, std::shared_ptr<CCmdCallHandler>> typeDispTable;

/*!
 * \brief A command dispatcher class
 */
class  CCmdDispatcher
{
protected:

    /*!
      * \brief Dispatching table (a map of <command string, pointer to the command handler object> )
      */
     typeDispTable m_DispTable;
     typeCRes __Call(CCmdCallDescr &d);

public:

     /*!
      * \brief Adding a new command handler to the dispatching table
      * \param pCmdName Command in a string format
      * \param pHandler A pointer to the command handler object
      */
     void Add(const char *pCmdName, const std::shared_ptr<CCmdCallHandler> &pHandler)
     {
         m_DispTable.emplace(pCmdName, pHandler);
     }

     /*!
      * \brief Find a corresponding command handler by requested call parameters and call it
      * \param d Call parameters
      * \return call result
      * \exception CCmdException is thrown on error if CCmdCallDescr::m_bThrowExcptOnErr=true;
      */
     typeCRes Call(CCmdCallDescr &d);
};


/*!
 * \brief A command dispatcher handler for handling an access point "get" and "set" requests via binding to the methods
 *  with a corresponding signature of an arbitrary class
 * \tparam typeClass The type of a class
 * \tparam typeArg The type of an access point
 *
 */
template<typename typeClass, typename typeArg>
class CCmdSGHandler : public CCmdCallHandler
{
protected:
    std::shared_ptr<typeClass> m_pObj;
    typeArg (typeClass::*m_pGetter)(void);
    void    (typeClass::*m_pSetter)(typeArg val);

public:

    /*!
     * \brief The class constructor
     * \param pObj A pointer to binding object
     * \param pGetter An obligatory pointer to the class method with "get" signature
     * \param pSetter An optional pointer to the class method with "set" signature
     */
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

/*!
 * \brief A command dispatcher handler for handling an access point "get" and "set" requests via binding to the arbitrary
 *  function with a corresponding signature
 * \tparam typeArg The type of an access point
 *
 */

template<typename typeArg>
class CCmdSGHandlerF : public CCmdCallHandler
{
protected:
    typeArg (*m_pGetter)(void);
    void    (*m_pSetter)(typeArg val);

public:
    /*!
     * \brief A class constructor
     * \param pGetter An obligatory pointer to the function with "get" signature
     * \param pSetter An optional pointer to the function with "set" signature
     */

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




