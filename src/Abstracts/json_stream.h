/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
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
