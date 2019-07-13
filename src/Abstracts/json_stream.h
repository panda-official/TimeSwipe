/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <nlohmann/json.hpp>
#include "frm_stream.h"

class CJSONStream : public CFrmStream
{
protected:
    nlohmann::json *m_pJSON=nullptr;

public:
    virtual void get(void *pVar, const std::type_info &ti);
    virtual void set(const void *pVar, const std::type_info &ti);

    CJSONStream(nlohmann::json *pJSON) : CFrmStream(nullptr)
    {
        m_pJSON=pJSON;
    }
};


/*#ifndef JSON_STREAM_H
#define JSON_STREAM_H

#endif // JSON_STREAM_H*/
