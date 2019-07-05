#pragma once

#include <nlohmann/json.hpp>
#include "cmd.h"

class CJSONDispatcher : public CCmdCallHandler //just connect handler here
{
protected:
    std::shared_ptr<CCmdDispatcher> m_pDisp;



public:
    void Call(nlohmann::json &jObj, nlohmann::json &jResp, CCmdCallDescr::ctype ct);    //recursive

    virtual typeCRes Call(CCmdCallDescr &d);

    CJSONDispatcher(const std::shared_ptr<CCmdDispatcher> &pDisp)
    {
        m_pDisp=pDisp;
    }
};


/*#ifndef JSONDISP_H
#define JSONDISP_H

#endif // JSONDISP_H*/
