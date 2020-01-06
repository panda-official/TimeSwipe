/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   IJSONEvent, CJSONEvCP and CJSONEvDispatcher
*/

#pragma once

#include <vector>
#include <memory>
#include "json_base.h"


/*!
 * \brief A callback interface used to notify the derived class that a JSON event happened
 */
struct IJSONEvent
{
    /*!
     * \brief A callback methode for a JSON event
     * \param key The event key (a string name)
     * \param val The event value (a JSON object containig the value)
     */
    virtual void on_event(const char *key, nlohmann::json &val)=0;

    //! default constructor
    IJSONEvent()=default;

    /*!
     * \brief remove copy constructor
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    IJSONEvent(const IJSONEvent&) = delete;

    /*!
     * \brief remove copy operator
     * \return
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    IJSONEvent& operator=(const IJSONEvent&) = delete;

protected:

    //! virtual destructor
    virtual ~IJSONEvent()=default;
};


/*!
 * \brief   A basic class for all JSON event system classes
 * \details This is a template for deriving all JSON event system classes.
 *  It implements a connection point for IJSONEvent inside. All objects that realize IJSONEvent can be advised
 *  to this class by AdviseSink and receive corresponding notifications
 *
 */
class CJSONEvCP
{
protected:
        ~CJSONEvCP(){}

        /*!
        * \brief A list of connection points for IJSONEvent
        */
        std::vector< std::weak_ptr<IJSONEvent> > m_EvSinks;

        /*!
         * \brief Notify all connected objects with
         *  a JSON event
         * \param key The event key (a string name)
         * \param val The event value (a JSON object containig the value)
         */
        void Fire_on_event(const char *key, nlohmann::json &val)
        {
            for(std::vector< std::weak_ptr<IJSONEvent> >::const_iterator i=m_EvSinks.begin(); i!=m_EvSinks.end(); i++)
            {
                if(i->expired())
                {
                   m_EvSinks.erase(i);
                }
                else
                {
                    i->lock()->on_event(key, val);
                }
            }
        }
public:
        /*!
         * \brief Subscribe a new listener to the JSON events of derived class (if any)
         * \param A sink to an object to subscribe
         */
        void AdviseSink(const std::shared_ptr<IJSONEvent> &sink)
        {
            m_EvSinks.emplace_back(sink);
        }

};

#include "cmd.h"
/*!
 * \brief The "je" command dispatcher and holder of the last JSON events
 * \details Please, see CommunicationProtocol.md and EventSysytem.md for details.
 * All JSON events to which the class object is subscribed falls to on_event and stored
 * in m_event until they being readout by "je" command. The m_event is cleared after handling "je" command.
 *
 */
class CJSONEvDispatcher : public IJSONEvent, public CJSONbase, public CCmdCallHandler
{
protected:

    /*!
     * \brief A holder for JSON events
     */
    nlohmann::json m_event;

    /*!
     * \brief A pointer to a command dispatcher
     */
    std::shared_ptr<CCmdDispatcher> m_pDisp;

public:
    /*!
     * \brief JSON event notification
     * \param key The event key (a string name)
     * \param val The event value (a JSON object containing the value)
     */
    virtual void on_event(const char *key, nlohmann::json &val);

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
    CJSONEvDispatcher(const std::shared_ptr<CCmdDispatcher> &pDisp)
    {
        m_pDisp=pDisp;
    }
};
