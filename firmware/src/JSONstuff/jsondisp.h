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
#include "cmd.h"

/*!
 * \brief The "js" command dispatcher
 * \details Please, see CommunicationProtocol.md and EventSysytem.md for details.
 */

class CJSONDispatcher : public CJSONbase, public CCmdCallHandler
{
protected:
    std::shared_ptr<CCmdDispatcher> m_pDisp;

    /*!
     *  \brief: Called for "js>". Returns all possible settings (enumerates all "get" handlers)
     *
     *  \param jResp - a JSON object to fill with the settings
     *
     */
    void DumpAllSettings(const CCmdCallDescr &d, nlohmann::json &jResp);

    /*!
     * \brief Handles an elementary JSON object that represents a primitive type - this is an endpoint in recursive CJSONDispatcher::Call(...)
     * \param strKey An input object key
     * \param ReqVal An input object value
     * \param jResp An output object (responce)
     * \param ct A call type: "get" or "set"
     */
    void CallPrimitive(const std::string &strKey, nlohmann::json &ReqVal, nlohmann::json &jResp, const CCmdCallDescr::ctype ct);

public:

    /*!
     * \brief An recursive handler for incoming JSON object.
     * It traverses the object tree recursively finding finite primitive types to be handled by CJSONDispatcher::CallPrimitive(...)
     * \param jObj An incoming JSON object
     * \param jResp A responce object
     * \param ct A call type: "get" or "set"
     * \param bArrayMode Is incoming object an array type? (Used with js>[...])
     */
    void Call(nlohmann::json &jObj, nlohmann::json &jResp, const CCmdCallDescr::ctype ct, const bool bArrayMode);

    /*!
     * \brief A command dispatcher handler override for this class
     * \param d An uniform command request descriptor.
     * \return The operation result
     */
    virtual typeCRes Call(CCmdCallDescr &d);

    /*!
     * \brief The class constructor
     * \param pDisp A pointer to a command dispatcher
     */
    CJSONDispatcher(const std::shared_ptr<CCmdDispatcher> &pDisp)
    {
        m_pDisp=pDisp;
    }
};
