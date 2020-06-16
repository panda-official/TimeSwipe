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




bool CSamPORT::FindSercomPad(pxy nPin, typeSamSercoms nSercom, pad &nPad, muxf &nMuxF)
{
    //table: sercom->(PXY+fmux)
    static const struct SercomMUXtab{

        char Pin;
        char MuxF;

    }SercomPXYmap[]={

        {CSamPORT::pxy::PA04, CSamPORT::muxf::fD},  //sc0p0
        {CSamPORT::pxy::PA05, CSamPORT::muxf::fD},  //sc0p1
        {CSamPORT::pxy::PA06, CSamPORT::muxf::fD},  //sc0p2
        {CSamPORT::pxy::PA07, CSamPORT::muxf::fD},  //sc0p3


        {CSamPORT::pxy::PA16, CSamPORT::muxf::fC},  //sc1p0
        {CSamPORT::pxy::PA17, CSamPORT::muxf::fC},  //sc1p1
        {CSamPORT::pxy::PA18, CSamPORT::muxf::fC},  //sc1p2
        {CSamPORT::pxy::PA19, CSamPORT::muxf::fC},  //sc1p3


        {CSamPORT::pxy::PA09, CSamPORT::muxf::fD},  //sc2p0 (+ alt sc0)
        {CSamPORT::pxy::PA08, CSamPORT::muxf::fD},  //sc2p1 (+ alt sc0)
        {CSamPORT::pxy::PA10, CSamPORT::muxf::fD},  //sc2p2 (+ alt sc0)
        {CSamPORT::pxy::PA11, CSamPORT::muxf::fD},  //sc2p2 (+ alt sc0)


        {CSamPORT::pxy::PA17, CSamPORT::muxf::fD},  //sc3p0
        {CSamPORT::pxy::PA16, CSamPORT::muxf::fD},  //sc3p1
        {CSamPORT::pxy::PA18, CSamPORT::muxf::fD},  //sc3p2
        {CSamPORT::pxy::PA19, CSamPORT::muxf::fD},  //sc3p3


        {CSamPORT::pxy::PB12, CSamPORT::muxf::fC},  //sc4p0
        {CSamPORT::pxy::PB13, CSamPORT::muxf::fC},  //sc4p1
        {CSamPORT::pxy::PB14, CSamPORT::muxf::fC},  //sc4p2
        {CSamPORT::pxy::PB15, CSamPORT::muxf::fC},  //sc4p3


        {CSamPORT::pxy::PB16, CSamPORT::muxf::fC},  //sc5p0
        {CSamPORT::pxy::PB17, CSamPORT::muxf::fC},  //sc5p1
        {CSamPORT::pxy::PB18, CSamPORT::muxf::fC},  //sc5p2
        {CSamPORT::pxy::PB19, CSamPORT::muxf::fC},  //sc5p3


        {CSamPORT::pxy::PD09, CSamPORT::muxf::fD},  //sc6p0
        {CSamPORT::pxy::PD08, CSamPORT::muxf::fD},  //sc6p1
        {CSamPORT::pxy::PD10, CSamPORT::muxf::fD},  //sc6p2
        {CSamPORT::pxy::PD11, CSamPORT::muxf::fD},  //sc6p3


        {CSamPORT::pxy::PD08, CSamPORT::muxf::fC},  //sc7p0
        {CSamPORT::pxy::PD09, CSamPORT::muxf::fC},  //sc7p1
        {CSamPORT::pxy::PD10, CSamPORT::muxf::fC},  //sc7p2
        {CSamPORT::pxy::PD11, CSamPORT::muxf::fC},  //sc7p3



        //----------------alt-1----------------------------
        {CSamPORT::pxy::PA08, CSamPORT::muxf::fC},  //sc0p0
        {CSamPORT::pxy::PA09, CSamPORT::muxf::fC},  //sc0p1
        {CSamPORT::pxy::PA10, CSamPORT::muxf::fC},  //sc0p2
        {CSamPORT::pxy::PA11, CSamPORT::muxf::fC},  //sc0p3


        {CSamPORT::pxy::PA00, CSamPORT::muxf::fD},  //sc1p0
        {CSamPORT::pxy::PA01, CSamPORT::muxf::fD},  //sc1p1
        {CSamPORT::pxy::PA06, CSamPORT::muxf::fD},  //sc1p2
        {CSamPORT::pxy::PA07, CSamPORT::muxf::fD},  //sc1p3

        {CSamPORT::pxy::PA12, CSamPORT::muxf::fC},  //sc2p0
        {CSamPORT::pxy::PA13, CSamPORT::muxf::fC},  //sc2p1
        {CSamPORT::pxy::PA14, CSamPORT::muxf::fC},  //sc2p2
        {CSamPORT::pxy::PA15, CSamPORT::muxf::fC},  //sc2p3

    };


    size_t nTabSize=sizeof(SercomPXYmap)/sizeof(struct SercomMUXtab);
    size_t nSercomInd=static_cast<size_t>(nSercom)<<2;
    while(nSercomInd<=(nTabSize-4))
    {
        for(size_t i=0; i<4; i++)
        {
            const struct SercomMUXtab &sc=SercomPXYmap[nSercomInd+i];
            if(nPin==sc.Pin)
            {
                nPad=static_cast<pad>(i);
                nMuxF=static_cast<muxf>(sc.MuxF);
                return true;
            }
        }
        nSercomInd+=32;
    }
    return false;
}


bool CSamPORT::MUX(pxy nPin, typeSamSercoms nSercom, pad &nPad)
{
    muxf nMuxF;

    if(!FindSercomPad(nPin, nSercom, nPad, nMuxF))
        return false;

    //check if the pin is hardware occupied:
    //......................................


    //multiplexing:
    size_t g=nPin/32;
    size_t p=nPin%32;

    if(p&1) //odd
    {
        PORT->Group[g].PMUX[p>>1].bit.PMUXO=nMuxF;
    }
    else    //even
    {
        PORT->Group[g].PMUX[p>>1].bit.PMUXE=nMuxF;
    }
    PORT->Group[g].PINCFG[p].bit.PMUXEN=1;

    return true;
}
