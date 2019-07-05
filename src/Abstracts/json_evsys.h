#pragma once

#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

struct IJSONEvent
{
    virtual void on_event(const char *key, nlohmann::json &val)=0;

protected:
    virtual ~IJSONEvent()=default;
};

class CJSONEvCP
{
protected:
        ~CJSONEvCP(){}  //protect from deletion

        std::vector< std::weak_ptr<IJSONEvent> > m_EvSinks;
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
        void AdviseSink(const std::shared_ptr<IJSONEvent> &sink)
        {
            m_EvSinks.emplace_back(sink);
        }

};

#include "cmd.h"
class CJSONEvDispatcher : public IJSONEvent, public CCmdCallHandler
{
protected:
    nlohmann::json m_event;
    std::shared_ptr<CCmdDispatcher> m_pDisp;

    bool m_EvFlagIsRaised=false;
    unsigned long m_EvFlagRaiseTStamp_mS;

    bool IsEventFlagRaised(){ return m_EvFlagIsRaised; }
    virtual void RaiseEventFlag(bool how); //{m_EvFlagIsRaised=how;}


public:
    virtual void on_event(const char *key, nlohmann::json &val);
    virtual typeCRes Call(CCmdCallDescr &d);

    CJSONEvDispatcher(const std::shared_ptr<CCmdDispatcher> &pDisp)
    {
        m_pDisp=pDisp;
    }
};


/*#ifndef JSON_EVSYS_H
#define JSON_EVSYS_H

#endif // JSON_EVSYS_H*/
