/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "json_stream.h"
#include "../error.hpp"

void CJSONStream::get(void *pVar, const std::type_info &ti)
{
    if(ti==typeid(bool))
    {
      *(static_cast<bool*>(pVar))=value_.GetBool();
    }
    else if(ti==typeid(int))
    {
      *(static_cast<int*>(pVar))=value_.GetInt();
    }
    else if(ti==typeid(unsigned int))
    {
      *(static_cast<unsigned int*>(pVar))=value_.GetUint();
    }
    else if(ti==typeid(float))
    {
      *(static_cast<float*>(pVar))=value_.GetFloat();
    }
    else if(ti==typeid(std::string))
    {
      *(static_cast<std::string*>(pVar))=value_.GetString();
    }
}

void CJSONStream::set(const void *pVar, const std::type_info &ti)
{
    PANDA_TIMESWIPE_FIRMWARE_ASSERT(alloc_); // read-only stream

    if(ti==typeid(bool))
    {
      value_.SetBool(*(static_cast<const bool*>(pVar)));
    }
    else if(ti==typeid(int))
    {
      value_.SetInt(*(static_cast<const int*>(pVar)));
    }
    else if(ti==typeid(unsigned int))
    {
      value_.SetUint(*(static_cast<const unsigned int*>(pVar)));
    }
    else if(ti==typeid(float))
    {
      value_.SetFloat(*(static_cast<const float*>(pVar)));
    }
    else if(ti==typeid(const char *))
    {
      value_.SetString(static_cast<const char*>(pVar), *alloc_);
    }
    else if(ti==typeid(const std::string))
    {
      value_.SetString(*static_cast<const std::string*>(pVar), *alloc_);
    }
}
