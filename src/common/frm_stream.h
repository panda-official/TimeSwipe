// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/**
 * @file
 * Formatted stream stuff.
 */

#ifndef PANDA_TIMESWIPE_COMMON_FRM_STREAM_HPP
#define PANDA_TIMESWIPE_COMMON_FRM_STREAM_HPP

#include "Serial.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

/*!
 * \brief A formatted stream class.
 * \details Provides a mechanism for retrieving/storing
 *  primitive data types (int, float, std::string, e.t.c) from/to the stream.
 *  similar to ios::ios, but standard library is too heavy
 */
class CFrmStream {
protected:
  /// A pointer to FIFO buffer used as stream-buffer.
  CFIFO *m_pBuf=nullptr;

  /// Actual parsing error status (true=active).
  bool m_bErr=false;

  /**
   * @brief Extraction operator helper.
   * @param pVar void pointer to an extracted variable.
   * @param ti variable type.
   */
  virtual void get(void* pVar, const std::type_info& ti)
  {
    std::string str;
    if(!fetch_string(str)) {
      m_bErr=true;
      return;
    }

    bool bHexMode=false;
    int HexVal=0;
    if(str.length()>=2) {
      if('0'==str[0] && 'x'==str[1]) {
        bHexMode=true;
        sscanf(str.c_str()+2, "%X", &HexVal );
      }
    }

    if(ti==typeid(bool)) {
      /*bool bres=std::stoi(str) ? true:false;
       *(static_cast<bool*>(pVar))=bres;*/

      bool bres=false;
      if(str.length()>0) {
        if(std::isdigit(str[0])) {
          bres=str[0]-0x30;
        } else {
          bres=(str=="True" || str=="true");
        }
      }
      *(static_cast<bool*>(pVar))=bres;
    } else if(ti==typeid(int)) {
      *(static_cast<int*>(pVar))=bHexMode ? HexVal : std::stoi(str);
    }
    else if(ti==typeid(unsigned int)) {
      *(static_cast<unsigned int*>(pVar))=bHexMode ? HexVal : std::stoul(str);
    }
    else if(ti==typeid(float)) {
      *(static_cast<float*>(pVar))=std::stof(str);
    }
    else if(ti==typeid(const char *)) {
      strcpy(*((char**)pVar), str.c_str());
    }
    else if(ti==typeid(std::string)) {
      (static_cast<std::string*>(pVar))->swap(str);
    }
  }

  /**
   * @brief Insertion operator helper.
   * @param pVar void pointer to an inserted variable.
   * @param ti variable type.
   */
  virtual void set(const void* pVar, const std::type_info& ti)
  {
    char tbuf[64];

    const char* pStr=tbuf;
    if(ti==typeid(bool)) {
      sprintf(tbuf, "%d", *(static_cast<const bool*>(pVar)) ? 1:0  );
    } else if(ti==typeid(int)) {
      sprintf(tbuf, "%d", *(static_cast<const int*>(pVar))  );
    } else if(ti==typeid(unsigned int)) {
      sprintf(tbuf, "%d", *(static_cast<const unsigned int*>(pVar))  );
    } else if(ti==typeid(float)) {
      sprintf(tbuf, "%g", *(static_cast<const float*>(pVar))  ); //+++not working with NanoLib, use NewLib
    } else if(ti==typeid(const char *)) {
      pStr=*((char**)pVar);
    } else if(ti==typeid(const std::string)) {
      pStr=(static_cast<const std::string*>(pVar))->c_str();
    }
    *m_pBuf+=pStr;
    /*while(*pStr!='\0')
      {
      *m_pBuf<<*pStr;
      pStr++;
      }*/
  }

  /// Start token used for string extraction.
  Character m_chStartTocken=' ';

  /// End token used for string extraction.
  Character m_chEndTocken='\0';

  /**
   * @brief Extract a string from the stream.
   * @param str Filled with extracted string (IN parameter).
   * @returns `true` on successfull extraction.
   */
  bool fetch_string(std::string& str)
  {
    bool bProcStr=false;
    Character Tocken=m_chStartTocken;

    str.clear();
    while(m_pBuf->in_avail()) {
      Character ch;
      *m_pBuf>>ch;

      if(ch == Tocken) {
        if(bProcStr)
          return true;
      } else {
        Tocken=m_chEndTocken;
        bProcStr=true;
        str+=(char)ch;
      }
    }
    return bProcStr;
  }

  template<typename type>
  void frm_vget(type &var)
  {
    get(&var, typeid(var));
  }

  template<typename type>
  void frm_vset(type &var) // FIXME: ref????
  {
    set(&var, typeid(var));
  }

public:
  /**
   * @brief Returns the status of the last parsing operation
   * FIXME: @returns
   */
  bool bad()
  {
    return m_bErr;
  }

  CFrmStream(CFIFO* pBuf)
  {
    m_pBuf=pBuf;
  }

  CFrmStream& operator<<(Character ch)
  {
    frm_vset<Character>(ch);
    return *this;
  }

  CFrmStream& operator>>(Character& ch)
  {
    frm_vget<Character>(ch);
    return *this;
  }

  CFrmStream& operator<<(const char* ch)
  {
    frm_vset<const char*>(ch);
    return *this;
  }

  CFrmStream& operator>>(char* ch)
  {
    frm_vget<char*>(ch);
    return *this;
  }

  //03.06.2019 - string support:
  CFrmStream& operator<<(const std::string& str)
  {
    frm_vset<const std::string>(str);
    return *this;
  }

  CFrmStream& operator>>(std::string& str)
  {
    frm_vget<std::string>(str);
    return *this;
  }

  CFrmStream& operator<<(unsigned int val)
  {
    frm_vset<unsigned int>(val);
    return *this;
  }

  CFrmStream& operator>>(unsigned int& val)
  {
    frm_vget<unsigned int>(val);
    return *this;
  }

  CFrmStream& operator<<(float val)
  {
    frm_vset<float>(val);
    return *this;
  }

  CFrmStream& operator>>(float& val)
  {
    frm_vget<float>(val);
    return *this;
  }

  CFrmStream& operator<<(bool val)
  {
    frm_vset<bool>(val);
    return *this;
  }

  CFrmStream& operator>>(bool& val)
  {
    frm_vget<bool>(val);
    return *this;
  }
};

#endif  // PANDA_TIMESWIPE_COMMON_FRM_STREAM_HPP
