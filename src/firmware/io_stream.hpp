// -*- C++ -*-

// PANDA Timeswipe Project
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

#ifndef PANDA_TIMESWIPE_FIRMWARE_IO_STREAM_HPP
#define PANDA_TIMESWIPE_FIRMWARE_IO_STREAM_HPP

#include "../serial.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

/**
 * @brief An IO stream.
 *
 * @details Provides an API for reading and writing the data of various types.
 *
 * @remarks Designed as a lightweight alternative of the standard IO streams.
 */
class Io_stream {
public:
  /// @returns `true` if the last operation was successful.
  bool is_good() const noexcept
  {
    return !m_bErr;
  }

  Io_stream(CFIFO* pBuf)
  {
    m_pBuf=pBuf;
  }

  Io_stream& operator<<(Character ch)
  {
    frm_vset<Character>(ch);
    return *this;
  }

  Io_stream& operator>>(Character& ch)
  {
    frm_vget<Character>(ch);
    return *this;
  }

  Io_stream& operator<<(const char* ch)
  {
    frm_vset<const char*>(ch);
    return *this;
  }

  Io_stream& operator>>(char* ch)
  {
    frm_vget<char*>(ch);
    return *this;
  }

  //03.06.2019 - string support:
  Io_stream& operator<<(const std::string& str)
  {
    frm_vset<const std::string>(str);
    return *this;
  }

  Io_stream& operator>>(std::string& str)
  {
    frm_vget<std::string>(str);
    return *this;
  }

  Io_stream& operator<<(unsigned int val)
  {
    frm_vset<unsigned int>(val);
    return *this;
  }

  Io_stream& operator>>(unsigned int& val)
  {
    frm_vget<unsigned int>(val);
    return *this;
  }

  Io_stream& operator<<(float val)
  {
    frm_vset<float>(val);
    return *this;
  }

  Io_stream& operator>>(float& val)
  {
    frm_vget<float>(val);
    return *this;
  }

  Io_stream& operator<<(bool val)
  {
    frm_vset<bool>(val);
    return *this;
  }

  Io_stream& operator>>(bool& val)
  {
    frm_vget<bool>(val);
    return *this;
  }

private:
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
    unsigned HexVal{};
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

  /**
   * @brief Extract a string from this stream.
   *
   * @param[out] str The result string.
   *
   * @returns `true` on success.
   */
  bool fetch_string(std::string& str)
  {
    /// Start token used for string extraction.
    constexpr Character start_token{' '};
    /// End token used for string extraction.
    constexpr Character end_token{'\0'};

    bool is_extracted{};
    Character token{start_token};
    str.clear();
    while (m_pBuf->in_avail()) {
      Character ch;
      *m_pBuf >> ch;

      if (ch == token) {
        if (is_extracted)
          break;
      } else {
        token = end_token;
        is_extracted = true;
        str += static_cast<char>(ch);
      }
    }
    return is_extracted;
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
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_IO_STREAM_HPP
