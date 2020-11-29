/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "ftransfer.h"

CCmdFTransferHandler::CCmdFTransferHandler()
{

}

typeCRes CCmdFTransferHandler::Call(CCmdCallDescr &d)
{
    if(d.m_ctype & CCmdCallDescr::ctype::ctSet) //set
    {
            //error:
            return typeCRes::fset_not_supported;
    }
   /* if(d.m_ctype & CCmdCallDescr::ctype::ctGet)
    {
    }*/

    //get params:
    std::string fname;
    unsigned int fpos;
    unsigned int flen;

    *(d.m_pIn)>>fname>>fpos>>flen;
    if( d.m_pIn->bad())
        return typeCRes::parse_err;

    //exec file transfer API call:
    //............................


    return typeCRes::OK;
}
