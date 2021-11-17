/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamPORT.h"

#include <sam.h>

std::shared_ptr<CSamPin> CSamPORT::FactoryPin(const CSamPORT::group nGroup,
  const CSamPORT::pin nPin, const bool bOutput)
{
    //check if the pin is hardware occupied:
    //......................................
    using Uint = decltype(PORT->Group[0].DIRSET.reg);
    std::shared_ptr<CSamPin> result{new CSamPin{nGroup, nPin}};
    PORT->Group[nGroup].DIRSET.reg = Uint{bOutput}<<nPin;
    return result;
}

void CSamPORT::SetPin(const CSamPORT::group nGroup, const CSamPORT::pin nPin,
  const bool bHow)
{
    if (bHow) {
      using Uint = decltype(PORT->Group[0].OUTSET.reg);
      PORT->Group[nGroup].OUTSET.reg = Uint{1}<<nPin;
    } else {
      using Uint = decltype(PORT->Group[0].OUTCLR.reg);
      PORT->Group[nGroup].OUTCLR.reg = Uint{1}<<nPin;
    }
}

bool CSamPORT::RbSetPin(const CSamPORT::group nGroup, const CSamPORT::pin nPin)
{
    using Uint = decltype(PORT->Group[0].OUT.reg);
    return PORT->Group[nGroup].OUT.reg & (Uint{1}<<nPin);
}

bool CSamPORT::GetPin(const CSamPORT::group nGroup, const CSamPORT::pin nPin)
{
    using Uint = decltype(PORT->Group[0].IN.reg);
    return PORT->Group[nGroup].IN.reg & (Uint{1}<<nPin);
}

void CSamPORT::ReleasePin(const CSamPORT::group nGroup, const CSamPORT::pin nPin)
{
    using Uint = decltype(PORT->Group[0].DIRCLR.reg);
    PORT->Group[nGroup].DIRCLR.reg = Uint{1}<<nPin;
}

bool CSamPORT::FindSercomPad(const pxy nPin, const typeSamSercoms nSercom,
  pad& nPad, muxf &nMuxF)
{
    //table: sercom->(PXY+fmux)
    constexpr struct {
        char Pin;
        char MuxF;
    } SercomPXYmap[] = {
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

#if defined(__SAME54P20A__)
        {CSamPORT::pxy::PD09, CSamPORT::muxf::fD},  //sc6p0
        {CSamPORT::pxy::PD08, CSamPORT::muxf::fD},  //sc6p1
        {CSamPORT::pxy::PD10, CSamPORT::muxf::fD},  //sc6p2
#elif defined(__SAME53N19A__)
        {CSamPORT::pxy::PC16, CSamPORT::muxf::fC},  //sc6p0
        {CSamPORT::pxy::PC17, CSamPORT::muxf::fC},  //sc6p1
        {CSamPORT::pxy::PC18, CSamPORT::muxf::fC},  //sc6p2
#else
#error Unsupported SAM
#endif
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

    constexpr auto nTabSize = sizeof(SercomPXYmap) / sizeof(*SercomPXYmap);
    for (auto nSercomInd = static_cast<std::size_t>(nSercom)<<2;
         nSercomInd <= (nTabSize - 4); nSercomInd += 32) {
        for(std::size_t i{}; i < 4; i++) {
            const auto& sc = SercomPXYmap[nSercomInd + i];
            if (nPin == sc.Pin) {
                nPad = static_cast<pad>(i);
                nMuxF = static_cast<muxf>(sc.MuxF);
                return true;
            }
        }
    }
    return false;
}

bool CSamPORT::MUX(const pxy nPin, const typeSamSercoms nSercom, pad &nPad)
{
    muxf nMuxF;
    if (!FindSercomPad(nPin, nSercom, nPad, nMuxF))
        return false;

    //check if the pin is hardware occupied:
    //......................................

    //multiplexing:
    const std::size_t g = nPin / 32;
    const std::size_t p = nPin % 32;
    if (p & 1) //odd
        PORT->Group[g].PMUX[p>>1].bit.PMUXO = nMuxF;
    else    //even
        PORT->Group[g].PMUX[p>>1].bit.PMUXE = nMuxF;
    PORT->Group[g].PINCFG[p].bit.PMUXEN = 1;
    return true;
}
