/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#include "EventDisp.h"
#include "sam.h"

CEvDisp::CEvDisp(const std::shared_ptr<CCmdDispatcher> &pDisp) : CJSONEvDispatcher(pDisp)
{
   /* PORT->Group[3].DIRSET.bit.DIRSET=(1L<<8);
    PORT->Group[3].OUTCLR.bit.OUTCLR=(1L<<8);  //initial state=LOW*/

    //SC1.2 (PA18)
    PORT->Group[0].DIRSET.bit.DIRSET=(1L<<18);
    PORT->Group[0].OUTCLR.bit.OUTCLR=(1L<<18);  //initial state=LOW

}

void CEvDisp::RaiseEventFlag(bool how)
{
    CJSONEvDispatcher::RaiseEventFlag(how);

    if(how)
    {
        //PORT->Group[3].OUTSET.bit.OUTSET=(1L<<8);
        PORT->Group[0].OUTSET.bit.OUTSET=(1L<<18);
    }
    else
    {
        //PORT->Group[3].OUTCLR.bit.OUTCLR=(1L<<8);
        PORT->Group[0].OUTCLR.bit.OUTCLR=(1L<<18);
    }
}
