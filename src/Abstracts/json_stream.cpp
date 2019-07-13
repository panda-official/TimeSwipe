/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


#include "json_stream.h"

void CJSONStream::get(void *pVar, const std::type_info &ti)
{
    //JSON native type conv:
    const nlohmann::json &jo=*m_pJSON;

    if(ti==typeid(bool))
    {
         *(static_cast<bool*>(pVar))=jo;
    }
    else if(ti==typeid(int))
    {
        *(static_cast<int*>(pVar))=jo;
    }
    else if(ti==typeid(unsigned int))
    {
        *(static_cast<unsigned int*>(pVar))=jo;
    }
    else if(ti==typeid(float))
    {
        *(static_cast<float*>(pVar))=jo;
    }
    /*else if(ti==typeid(const char *))
    {
       strcpy(*((char**)pVar), (const char*)jo);
    }*/
    else if(ti==typeid(std::string))
    {
        *(static_cast<std::string*>(pVar))=jo;
    }
}

void CJSONStream::set(const void *pVar, const std::type_info &ti)
{
    //JSON native type conv:
    nlohmann::json &jo=*m_pJSON;

    if(ti==typeid(bool))
    {
         jo=*(static_cast<const bool*>(pVar));
    }
    else if(ti==typeid(int))
    {
        jo=*(static_cast<const int*>(pVar));
    }
    else if(ti==typeid(unsigned int))
    {
        jo=*(static_cast<const unsigned int*>(pVar));
    }
    else if(ti==typeid(float))
    {
        jo=*(static_cast<const float*>(pVar));
    }
    else if(ti==typeid(const char *))
    {
        jo=*((const char**)pVar);
    }
    else if(ti==typeid(const std::string))
    {
        jo=*(static_cast<const std::string*>(pVar));
    }
}
