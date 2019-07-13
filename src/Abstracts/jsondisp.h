/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

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
