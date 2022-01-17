/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CJSONDispatcher
*/

#pragma once

#include "json_base.h"
#include "../settings.hpp"

#include <functional>
#include <map>

/*!
 * \brief The "js" command dispatcher
 * \details Please, see CommunicationProtocol.md and EventSysytem.md for details.
 */
class CJSONDispatcher : public CJSONbase, public CCmdCallHandler
{
protected:
    std::shared_ptr<CCmdDispatcher> m_pDisp;

    using typeSubHandler = std::function<void(rapidjson::Value& jObj,
      rapidjson::Document& jResp, const Setting_descriptor::ctype ct)>;

    using typeSubMap = std::map<std::string, typeSubHandler>;

    typeSubMap m_SubHandlersMap;

    /*!
     *  \brief: Called for "js>". Returns all possible settings (enumerates all "get" handlers)
     *
     *  \param jResp - a JSON object to fill with the settings
     *
     */
  void DumpAllSettings(const Setting_descriptor &d, rapidjson::Document& jResp);

    /*!
     * \brief Handles an elementary JSON object that represents a primitive type - this is an endpoint in recursive CJSONDispatcher::Call(...)
     * \param strKey An input object key
     * \param ReqVal An input object value
     * \param jResp An output object (responce)
     * \param ct A call type: "get" or "set"
     */
  void CallPrimitive(const std::string &strKey, rapidjson::Value& ReqVal, rapidjson::Document& jResp, rapidjson::Value& resp_root, const Setting_descriptor::ctype ct);

public:

    /*!
     * \brief An recursive handler for incoming JSON object.
     * It traverses the object tree recursively finding finite primitive types to be handled by CJSONDispatcher::CallPrimitive(...)
     * \param jObj An incoming JSON object
     * \param jResp A responce object
     * \param ct A call type: "get" or "set"
     */
  void Call(rapidjson::Value &jObj, rapidjson::Document& jResp, rapidjson::Value& resp_root, const Setting_descriptor::ctype ct);

    /*!
     * \brief A command dispatcher handler override for this class
     * \param d An uniform command request descriptor.
     * \return The operation result
     */
    typeCRes Call(Setting_descriptor &d) override;

    /*!
     * \brief The class constructor
     * \param pDisp A pointer to a command dispatcher
     */
    CJSONDispatcher(const std::shared_ptr<CCmdDispatcher>& pDisp)
    {
      m_pDisp=pDisp;
    }

    void AddSubHandler(std::string strName, typeSubHandler SubHandler)
    {
      m_SubHandlersMap.emplace(strName, SubHandler);
    }
};
