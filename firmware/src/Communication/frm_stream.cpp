/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include "frm_stream.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

 bool CFrmStream::fetch_string(std::string &str)
 {
    bool bProcStr=false;
    typeSChar Tocken=m_chStartTocken;

    str.clear();
    while(m_pBuf->in_avail())
    {
        typeSChar ch;
        *m_pBuf>>ch;

        if(ch==Tocken)
        {
            if(bProcStr)
                return true;
        }
        else
        {
            Tocken=m_chEndTocken;
            bProcStr=true;
            str+=(char)ch;
        }
    }
    return bProcStr;
 }

void CFrmStream::get(void *pVar, const std::type_info &ti)
{
    std::string str;
    if(!fetch_string(str))
    {
        m_bErr=true;
        return;
    }

    bool bHexMode=false;
    int HexVal=0;
    if(str.length()>=2)
    {
        if('0'==str[0] && 'x'==str[1])
        {
            bHexMode=true;
            sscanf(str.c_str()+2, "%X", &HexVal );
        }
    }

    if(ti==typeid(bool))
    {
        /*bool bres=std::stoi(str) ? true:false;
         *(static_cast<bool*>(pVar))=bres;*/

        bool bres=false;
        if(str.length()>0)
        {
            if(std::isdigit(str[0]))
            {
                bres=str[0]-0x30;
            }
            else
            {
                bres=(str=="True" || str=="true");
            }
        }
        *(static_cast<bool*>(pVar))=bres;
    }
    else if(ti==typeid(int))
    {
        *(static_cast<int*>(pVar))=bHexMode ? HexVal : std::stoi(str);
    }
    else if(ti==typeid(unsigned int))
    {
        *(static_cast<unsigned int*>(pVar))=bHexMode ? HexVal : std::stoul(str);
    }
    else if(ti==typeid(float))
    {
        *(static_cast<float*>(pVar))=std::stof(str);
    }
    else if(ti==typeid(const char *))
    {
       strcpy(*((char**)pVar), str.c_str());
    }
    else if(ti==typeid(std::string))
    {
        (static_cast<std::string*>(pVar))->swap(str);
    }


}
void CFrmStream::set(const void *pVar, const std::type_info &ti)
{
    char tbuf[64];

    const char *pStr=tbuf;
    if(ti==typeid(bool))
    {
         sprintf(tbuf, "%d", *(static_cast<const bool*>(pVar)) ? 1:0  );
    }
    else if(ti==typeid(int))
    {
        sprintf(tbuf, "%d", *(static_cast<const int*>(pVar))  );
    }
    else if(ti==typeid(unsigned int))
    {
        sprintf(tbuf, "%d", *(static_cast<const unsigned int*>(pVar))  );
    }
    else if(ti==typeid(float))
    {
        sprintf(tbuf, "%g", *(static_cast<const float*>(pVar))  ); //+++not working with NanoLib, use NewLib
    }
    else if(ti==typeid(const char *))
    {
        pStr=*((char**)pVar);
    }
    else if(ti==typeid(const std::string))
    {
        pStr=(static_cast<const std::string*>(pVar))->c_str();
    }
    *m_pBuf+=pStr;
    /*while(*pStr!='\0')
    {
        *m_pBuf<<*pStr;
        pStr++;
    }*/
}
