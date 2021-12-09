// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/**
 * @file
 * Command proccessor stuff.
 */

#ifndef PANDA_TIMESWIPE_FIRMWARE_CMD_HPP
#define PANDA_TIMESWIPE_FIRMWARE_CMD_HPP

#include "frm_stream.h"

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @brief An uniform command request descriptor.
 *
 * The processing of all incoming commands is handled by an instance of class
 * CCmdDispatcher. The implementation of the current communication protocol
 * (simple text, binary/specific) transforms an incoming request from a protocol
 * depended form to an uniform request described by the class CCmdCallDescr,
 * where command name in a string format (if presents) denotes hash value,
 * pointers to input/output streams and other service information stored.
 * Implementation then calls method CCmdDispatcher::Call with an object of type
 * CCmdCallDescr as a parameter. CCmdDispatcher::Call handles it by finding
 * specific command handler in its internal map and invokes that handler. The
 * result of call is stored to the passed parameter and hence its further
 * accesible by a port. By adding number of different port classes communication
 * protocols can be implemented at once.
 */
struct CCmdCallDescr final {
  /// The command in a string format.
  std::string m_strCommand;

  /// A hash value of the command string.
  int m_hashCommand;

  /// A zero based index of the command.
  unsigned int m_nCmdIndex;

  /// Input stream: to fetch function/methode input arguments.
  CFrmStream    *m_pIn=nullptr;

  /// Output stream: to store function/methodes output arguments or return value.
  CFrmStream    *m_pOut=nullptr;

  /// Command handler invocation result ("call result"=cres).
  enum cres {
    OK=0,               //!<successful invocation
    obj_not_found,      //!<requested command(object) was not found
    fget_not_supported, //!<"get" property is not supported by a handler
    fset_not_supported, //!<"set" property is not supported by a handler
    parse_err,          //!<an error occurred while parsing arguments from the input stream
    disabled            //!<handler is disabled for some reasons
  };

  /// Invocation type ("call type"=ctype).
  enum ctype {
    /// "get" property
    ctGet = 1,
    /// "set" property
    ctSet = 2
  } m_ctype{ctGet};

  /// How to dispatch an invocation? by a command in a string format, its hash value or index?
  enum cmethod {
    byCmdName=1,        //!<by a command in a string format (using m_strCommand)
    byCmdHash=2,        //!<by a command's hash value  (using m_hashCommand)
    byCmdIndex=4        //!<by a command's zero-based index (using m_nCmdIndex)
  } m_cmethod{byCmdName};

  /// If true, throw an exception CCmdException instead of returning cres
  bool m_bThrowExcptOnErr=false;
};
typedef  CCmdCallDescr::cres typeCRes;

/// A command execution exception
class CCmdException final : public std::runtime_error {
  using runtime_error::runtime_error;
};

/// A basic class for command handler.
struct CCmdCallHandler {
  /**
   * @brief A method for handling a concrete command.
   *
   * @param d Call descriptor in protocol-independent format.
   */
  virtual typeCRes Call(CCmdCallDescr& d) = 0;
};

/// A command dispatcher class.
class  CCmdDispatcher final {
protected:
  /// Dispatching table.
  using typeDispTable = std::map<std::string, std::shared_ptr<CCmdCallHandler>>;

  typeDispTable m_DispTable;
  typeCRes __Call(CCmdCallDescr &d)
  {
    //by call method:
    if(CCmdCallDescr::cmethod::byCmdName==d.m_cmethod) {

      typeDispTable::const_iterator pCmd=m_DispTable.find(d.m_strCommand);
      if(pCmd!=m_DispTable.end()) //call:
        return pCmd->second->Call(d);
      return typeCRes::obj_not_found;
    }
    if(CCmdCallDescr::cmethod::byCmdIndex==d.m_cmethod) {
      if(d.m_nCmdIndex < static_cast<unsigned int>(m_DispTable.size())) {
        typeDispTable::const_iterator pCmd=m_DispTable.begin();
        std::advance(pCmd, d.m_nCmdIndex);
        d.m_strCommand=pCmd->first;
        return pCmd->second->Call(d);
      }
    }
    return typeCRes::obj_not_found;
  }

public:
  /**
   * @brief Adding a new command handler to the dispatching table
   *
   * @param pCmdName Command in a string format.
   * @param pHandler A pointer to the command handler object.
   */
  void Add(const char* pCmdName, const std::shared_ptr<CCmdCallHandler> pHandler)
  {
    m_DispTable[pCmdName] = pHandler;
  }

  /**
   * @brief Finds a corresponding command handler by given call parameters
   * and calls it.
   *
   * @param d Call parameters
   * @returns The result of call.
   * @throws CCmdException on error if `CCmdCallDescr::m_bThrowExcptOnErr == true`.
   */
  typeCRes Call(CCmdCallDescr &d)
  {
    typeCRes cres=__Call(d);
    if(d.m_bThrowExcptOnErr) {
      if(typeCRes::obj_not_found == cres)
        throw CCmdException{"obj_not_found!"};
      if(typeCRes::fget_not_supported == cres)
        throw CCmdException{">_not_supported!"};
      if(typeCRes::fset_not_supported == cres)
        throw CCmdException{"<_not_supported!"};
      if(typeCRes::disabled == cres)
        throw CCmdException{"disabled!"};
    }
    return cres;
  }
};

/**
 * @brief A command dispatcher handler for handling an access point `get` and
 * `set` requests via binding to the methods with a corresponding signature of
 * an arbitrary class.
 *
 * @tparam typeClass The type of a class.
 * @tparam typeArg The type of an access point.
 */
template<typename typeClass, typename typeArg>
class CCmdSGHandler final : public CCmdCallHandler {
protected:
  using Getter = typeArg(typeClass::*)()const;
  using Setter = void(typeClass::*)(typeArg);

  std::shared_ptr<typeClass> m_pObj;
  Getter m_pGetter{};
  Setter m_pSetter{};
public:
  /**
   * @brief The class constructor.
   * @param pObj A pointer to binding object.
   * @param pGetter An obligatory pointer to the class method with "get" signature.
   * @param pSetter An optional pointer to the class method with "set" signature.
   */
  CCmdSGHandler(const std::shared_ptr<typeClass>& pObj,
    Getter pGetter, Setter pSetter = {})
    : m_pObj{pObj}
    , m_pGetter{pGetter}
    , m_pSetter{pSetter}
  {}

  typeCRes Call(CCmdCallDescr& d) override
  {
    if (d.m_ctype & CCmdCallDescr::ctype::ctSet) { //set
      if (m_pSetter) {
        typeArg val;
        *(d.m_pIn)>>val;
        if (d.m_pIn->bad())
          return typeCRes::parse_err;

        (m_pObj.get()->*m_pSetter)(val);

        //14.08.2019: feedback
        if (m_pGetter)
          *(d.m_pOut)<<(m_pObj.get()->*m_pGetter)();
      } else // error
        return typeCRes::fset_not_supported;
    }

    if (d.m_ctype & CCmdCallDescr::ctype::ctGet) {
      if (m_pGetter)
        *(d.m_pOut)<<(m_pObj.get()->*m_pGetter)();
      else // error
        return typeCRes::fget_not_supported;
    }
    return typeCRes::OK;
  }
};

/**
 * @brief A command dispatcher handler for handling an access point "get" and
 * "set" requests via binding to the arbitrary function with a corresponding
 * signature.
 *
 * @tparam typeArg The type of an access point.
 */
template<typename typeArg>
class CCmdSGHandlerF final : public CCmdCallHandler {
protected:
    typeArg (*m_pGetter)(void);
    void (*m_pSetter)(typeArg val);

public:
  /**
   * @brief A class constructor
   * @param pGetter An obligatory pointer to the function with "get" signature
   * @param pSetter An optional pointer to the function with "set" signature
   */
  CCmdSGHandlerF(typeArg (*pGetter)(void), void (*pSetter)(typeArg val) = nullptr)
  {
    m_pGetter=pGetter;
    m_pSetter=pSetter;
  }

  typeCRes Call(CCmdCallDescr &d) override
  {
    if(d.m_ctype & CCmdCallDescr::ctype::ctSet) { //set
      if(m_pSetter) {
        typeArg val;
        *(d.m_pIn)>>val;
        if(d.m_pIn->bad())
          return typeCRes::parse_err;

        m_pSetter(val);

        //14.08.2019: feedback
        if(m_pGetter)
          *(d.m_pOut)<<m_pGetter();
      } else // error
        return typeCRes::fset_not_supported;
    }

    if(d.m_ctype & CCmdCallDescr::ctype::ctGet) {
      if(m_pGetter)
        *(d.m_pOut)<<m_pGetter();
      else //error
        return typeCRes::fget_not_supported;
    }
    return typeCRes::OK;
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_CMD_HPP
