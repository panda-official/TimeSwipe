/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*#ifndef FRM_STREAM_H
#define FRM_STREAM_H

#endif // FRM_STREAM_H*/

#pragma once

//base class for a formated stream:

//1: std:

//#include <istream>
//typedef std::iostream CFrmStream;


//2: custom: iostream is too heavy.....

//just a stub for testing:
#include "Serial.h"

class CFrmStream
{
protected:
   CFIFO *m_pBuf=nullptr;
   bool  m_bErr=false;

   virtual void get(void *pVar, const std::type_info &ti);
   virtual void set(const void *pVar, const std::type_info &ti);

   typeSChar m_chStartTocken=' ';
   typeSChar m_chEndTocken='\0';
   bool fetch_string(std::string &str);

   template<typename type>
   void frm_vget(type &var){
       get(&var, typeid(var) );
   }
   template<typename type>
   void frm_vset(type &var){        //ref????
       set(&var, typeid(var) );
   }

public:
    bool bad(){return m_bErr;}

    CFrmStream(CFIFO *pBuf)
    {
        m_pBuf=pBuf;
    }

    CFrmStream & operator <<(typeSChar ch)
    {
         frm_vset<typeSChar>(ch);
         return *this;
    }
    CFrmStream & operator >>(typeSChar &ch)
    {
         frm_vget<typeSChar>(ch);
         return *this;
    }
    CFrmStream & operator <<(const char *ch)
    {
         frm_vset<const char *>(ch);
         return *this;
    }
    CFrmStream & operator >>(char *ch)
    {
         frm_vget<char *>(ch);
         return *this;
    }

    //03.06.2019 - string support:
    CFrmStream & operator <<(const std::string &str)
    {
         frm_vset<const std::string>(str);
         return *this;
    }
    CFrmStream & operator >>(std::string &str)
    {
         frm_vget<std::string>(str);
         return *this;
    }


    CFrmStream & operator <<(unsigned int val)
    {
        frm_vset<unsigned int>(val);
         return *this;
    }
    CFrmStream & operator >>(unsigned int &val)
    {
         frm_vget<unsigned int>(val);
         return *this;
    }
    CFrmStream & operator <<(float val)
    {
        frm_vset<float>(val);
         return *this;
    }
    CFrmStream & operator >>(float &val)
    {
         frm_vget<float>(val);
         return *this;
    }
    CFrmStream & operator <<(bool val)
    {
        frm_vset<bool>(val);
         return *this;
    }
    CFrmStream & operator >>(bool &val)
    {
        frm_vget<bool>(val);
         return *this;
    }
};
