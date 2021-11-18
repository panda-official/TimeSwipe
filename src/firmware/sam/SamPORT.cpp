/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamPORT.h"

#include <sam.h>

std::shared_ptr<CSamPin> CSamPin::FactoryPin(const CSamPin::group nGroup,
  const CSamPin::pin nPin, const bool bOutput)
{
    //check if the pin is hardware occupied:
    //......................................
    using Uint = decltype(PORT->Group[0].DIRSET.reg);
    std::shared_ptr<CSamPin> result{new CSamPin{nGroup, nPin}};
    PORT->Group[nGroup].DIRSET.reg = Uint{bOutput}<<nPin;
    return result;
}

void CSamPin::SetPin(const CSamPin::group nGroup, const CSamPin::pin nPin,
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

bool CSamPin::RbSetPin(const CSamPin::group nGroup, const CSamPin::pin nPin)
{
    using Uint = decltype(PORT->Group[0].OUT.reg);
    return PORT->Group[nGroup].OUT.reg & (Uint{1}<<nPin);
}

bool CSamPin::GetPin(const CSamPin::group nGroup, const CSamPin::pin nPin)
{
    using Uint = decltype(PORT->Group[0].IN.reg);
    return PORT->Group[nGroup].IN.reg & (Uint{1}<<nPin);
}

void CSamPin::ReleasePin(const CSamPin::group nGroup, const CSamPin::pin nPin)
{
    using Uint = decltype(PORT->Group[0].DIRCLR.reg);
    PORT->Group[nGroup].DIRCLR.reg = Uint{1}<<nPin;
}

bool CSamPin::FindSercomPad(const pxy nPin, const typeSamSercoms nSercom,
  pad& nPad, muxf &nMuxF)
{
    //table: sercom->(PXY+fmux)
    constexpr struct {
        char Pin;
        char MuxF;
    } SercomPXYmap[] = {
        {CSamPin::pxy::PA04, CSamPin::muxf::fD},  //sc0p0
        {CSamPin::pxy::PA05, CSamPin::muxf::fD},  //sc0p1
        {CSamPin::pxy::PA06, CSamPin::muxf::fD},  //sc0p2
        {CSamPin::pxy::PA07, CSamPin::muxf::fD},  //sc0p3

        {CSamPin::pxy::PA16, CSamPin::muxf::fC},  //sc1p0
        {CSamPin::pxy::PA17, CSamPin::muxf::fC},  //sc1p1
        {CSamPin::pxy::PA18, CSamPin::muxf::fC},  //sc1p2
        {CSamPin::pxy::PA19, CSamPin::muxf::fC},  //sc1p3

        {CSamPin::pxy::PA09, CSamPin::muxf::fD},  //sc2p0 (+ alt sc0)
        {CSamPin::pxy::PA08, CSamPin::muxf::fD},  //sc2p1 (+ alt sc0)
        {CSamPin::pxy::PA10, CSamPin::muxf::fD},  //sc2p2 (+ alt sc0)
        {CSamPin::pxy::PA11, CSamPin::muxf::fD},  //sc2p2 (+ alt sc0)

        {CSamPin::pxy::PA17, CSamPin::muxf::fD},  //sc3p0
        {CSamPin::pxy::PA16, CSamPin::muxf::fD},  //sc3p1
        {CSamPin::pxy::PA18, CSamPin::muxf::fD},  //sc3p2
        {CSamPin::pxy::PA19, CSamPin::muxf::fD},  //sc3p3

        {CSamPin::pxy::PB12, CSamPin::muxf::fC},  //sc4p0
        {CSamPin::pxy::PB13, CSamPin::muxf::fC},  //sc4p1
        {CSamPin::pxy::PB14, CSamPin::muxf::fC},  //sc4p2
        {CSamPin::pxy::PB15, CSamPin::muxf::fC},  //sc4p3

        {CSamPin::pxy::PB16, CSamPin::muxf::fC},  //sc5p0
        {CSamPin::pxy::PB17, CSamPin::muxf::fC},  //sc5p1
        {CSamPin::pxy::PB18, CSamPin::muxf::fC},  //sc5p2
        {CSamPin::pxy::PB19, CSamPin::muxf::fC},  //sc5p3

#if defined(__SAME54P20A__)
        {CSamPin::pxy::PD09, CSamPin::muxf::fD},  //sc6p0
        {CSamPin::pxy::PD08, CSamPin::muxf::fD},  //sc6p1
        {CSamPin::pxy::PD10, CSamPin::muxf::fD},  //sc6p2
#elif defined(__SAME53N19A__)
        {CSamPin::pxy::PC16, CSamPin::muxf::fC},  //sc6p0
        {CSamPin::pxy::PC17, CSamPin::muxf::fC},  //sc6p1
        {CSamPin::pxy::PC18, CSamPin::muxf::fC},  //sc6p2
#else
#error Unsupported SAM
#endif
        {CSamPin::pxy::PD11, CSamPin::muxf::fD},  //sc6p3

        {CSamPin::pxy::PD08, CSamPin::muxf::fC},  //sc7p0
        {CSamPin::pxy::PD09, CSamPin::muxf::fC},  //sc7p1
        {CSamPin::pxy::PD10, CSamPin::muxf::fC},  //sc7p2
        {CSamPin::pxy::PD11, CSamPin::muxf::fC},  //sc7p3

        //----------------alt-1----------------------------
        {CSamPin::pxy::PA08, CSamPin::muxf::fC},  //sc0p0
        {CSamPin::pxy::PA09, CSamPin::muxf::fC},  //sc0p1
        {CSamPin::pxy::PA10, CSamPin::muxf::fC},  //sc0p2
        {CSamPin::pxy::PA11, CSamPin::muxf::fC},  //sc0p3

        {CSamPin::pxy::PA00, CSamPin::muxf::fD},  //sc1p0
        {CSamPin::pxy::PA01, CSamPin::muxf::fD},  //sc1p1
        {CSamPin::pxy::PA06, CSamPin::muxf::fD},  //sc1p2
        {CSamPin::pxy::PA07, CSamPin::muxf::fD},  //sc1p3

        {CSamPin::pxy::PA12, CSamPin::muxf::fC},  //sc2p0
        {CSamPin::pxy::PA13, CSamPin::muxf::fC},  //sc2p1
        {CSamPin::pxy::PA14, CSamPin::muxf::fC},  //sc2p2
        {CSamPin::pxy::PA15, CSamPin::muxf::fC},  //sc2p3
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

bool CSamPin::MUX(const pxy nPin, const typeSamSercoms nSercom, pad &nPad)
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
