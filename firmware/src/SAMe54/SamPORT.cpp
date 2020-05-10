/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamPORT.h"
#include "sam.h"

std::shared_ptr<CSamPin> CSamPORT::FactoryPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bOutput)
{
    //check if the pin is hardware occupied:
    //......................................

    CSamPin *pPin=new CSamPin(nGroup, nPin);
    if(bOutput)
    {
        PORT->Group[nGroup].DIRSET.reg=(1L<<nPin);
    }
    return std::shared_ptr<CSamPin>(pPin);
}

void CSamPORT::SetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bHow)
{
    if(bHow)
        PORT->Group[nGroup].OUTSET.reg=(1L<<nPin);
    else
        PORT->Group[nGroup].OUTCLR.reg=(1L<<nPin);
}
bool CSamPORT::RbSetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin)
{
    return (PORT->Group[nGroup].OUT.reg & (1L<<nPin));
}
bool CSamPORT::GetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin)
{
    return (PORT->Group[nGroup].IN.reg & (1L<<nPin));
}
void CSamPORT::ReleasePin(CSamPORT::group nGroup, CSamPORT::pin  nPin)
{
    PORT->Group[nGroup].DIRCLR.reg=(1L<<nPin);
}
