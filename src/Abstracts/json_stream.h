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
