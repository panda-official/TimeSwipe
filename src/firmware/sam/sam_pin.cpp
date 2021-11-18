/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "sam_pin.hpp"

#include <sam.h>

std::shared_ptr<Sam_pin> Sam_pin::FactoryPin(const Group nGroup,
  const Sam_pin::pin nPin, const bool bOutput)
{
    //check if the pin is hardware occupied:
    //......................................
    using Uint = decltype(PORT->Group[0].DIRSET.reg);
    std::shared_ptr<Sam_pin> result{new Sam_pin{nGroup, nPin}};
    PORT->Group[nGroup].DIRSET.reg = Uint{bOutput}<<nPin;
    return result;
}

void Sam_pin::SetPin(const Group nGroup, const Sam_pin::pin nPin,
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

bool Sam_pin::RbSetPin(const Group nGroup, const Sam_pin::pin nPin)
{
    using Uint = decltype(PORT->Group[0].OUT.reg);
    return PORT->Group[nGroup].OUT.reg & (Uint{1}<<nPin);
}

bool Sam_pin::GetPin(const Group nGroup, const Sam_pin::pin nPin)
{
    using Uint = decltype(PORT->Group[0].IN.reg);
    return PORT->Group[nGroup].IN.reg & (Uint{1}<<nPin);
}

void Sam_pin::ReleasePin(const Group nGroup, const Sam_pin::pin nPin)
{
    using Uint = decltype(PORT->Group[0].DIRCLR.reg);
    PORT->Group[nGroup].DIRCLR.reg = Uint{1}<<nPin;
}

bool Sam_pin::FindSercomPad(const pxy nPin, const typeSamSercoms nSercom,
  pad& nPad, muxf &nMuxF)
{
    //table: sercom->(PXY+fmux)
    constexpr struct {
        char Pin;
        char MuxF;
    } SercomPXYmap[] = {
        {Sam_pin::pxy::PA04, Sam_pin::muxf::fD},  //sc0p0
        {Sam_pin::pxy::PA05, Sam_pin::muxf::fD},  //sc0p1
        {Sam_pin::pxy::PA06, Sam_pin::muxf::fD},  //sc0p2
        {Sam_pin::pxy::PA07, Sam_pin::muxf::fD},  //sc0p3

        {Sam_pin::pxy::PA16, Sam_pin::muxf::fC},  //sc1p0
        {Sam_pin::pxy::PA17, Sam_pin::muxf::fC},  //sc1p1
        {Sam_pin::pxy::PA18, Sam_pin::muxf::fC},  //sc1p2
        {Sam_pin::pxy::PA19, Sam_pin::muxf::fC},  //sc1p3

        {Sam_pin::pxy::PA09, Sam_pin::muxf::fD},  //sc2p0 (+ alt sc0)
        {Sam_pin::pxy::PA08, Sam_pin::muxf::fD},  //sc2p1 (+ alt sc0)
        {Sam_pin::pxy::PA10, Sam_pin::muxf::fD},  //sc2p2 (+ alt sc0)
        {Sam_pin::pxy::PA11, Sam_pin::muxf::fD},  //sc2p2 (+ alt sc0)

        {Sam_pin::pxy::PA17, Sam_pin::muxf::fD},  //sc3p0
        {Sam_pin::pxy::PA16, Sam_pin::muxf::fD},  //sc3p1
        {Sam_pin::pxy::PA18, Sam_pin::muxf::fD},  //sc3p2
        {Sam_pin::pxy::PA19, Sam_pin::muxf::fD},  //sc3p3

        {Sam_pin::pxy::PB12, Sam_pin::muxf::fC},  //sc4p0
        {Sam_pin::pxy::PB13, Sam_pin::muxf::fC},  //sc4p1
        {Sam_pin::pxy::PB14, Sam_pin::muxf::fC},  //sc4p2
        {Sam_pin::pxy::PB15, Sam_pin::muxf::fC},  //sc4p3

        {Sam_pin::pxy::PB16, Sam_pin::muxf::fC},  //sc5p0
        {Sam_pin::pxy::PB17, Sam_pin::muxf::fC},  //sc5p1
        {Sam_pin::pxy::PB18, Sam_pin::muxf::fC},  //sc5p2
        {Sam_pin::pxy::PB19, Sam_pin::muxf::fC},  //sc5p3

#if defined(__SAME54P20A__)
        {Sam_pin::pxy::PD09, Sam_pin::muxf::fD},  //sc6p0
        {Sam_pin::pxy::PD08, Sam_pin::muxf::fD},  //sc6p1
        {Sam_pin::pxy::PD10, Sam_pin::muxf::fD},  //sc6p2
#elif defined(__SAME53N19A__)
        {Sam_pin::pxy::PC16, Sam_pin::muxf::fC},  //sc6p0
        {Sam_pin::pxy::PC17, Sam_pin::muxf::fC},  //sc6p1
        {Sam_pin::pxy::PC18, Sam_pin::muxf::fC},  //sc6p2
#else
#error Unsupported SAM
#endif
        {Sam_pin::pxy::PD11, Sam_pin::muxf::fD},  //sc6p3

        {Sam_pin::pxy::PD08, Sam_pin::muxf::fC},  //sc7p0
        {Sam_pin::pxy::PD09, Sam_pin::muxf::fC},  //sc7p1
        {Sam_pin::pxy::PD10, Sam_pin::muxf::fC},  //sc7p2
        {Sam_pin::pxy::PD11, Sam_pin::muxf::fC},  //sc7p3

        //----------------alt-1----------------------------
        {Sam_pin::pxy::PA08, Sam_pin::muxf::fC},  //sc0p0
        {Sam_pin::pxy::PA09, Sam_pin::muxf::fC},  //sc0p1
        {Sam_pin::pxy::PA10, Sam_pin::muxf::fC},  //sc0p2
        {Sam_pin::pxy::PA11, Sam_pin::muxf::fC},  //sc0p3

        {Sam_pin::pxy::PA00, Sam_pin::muxf::fD},  //sc1p0
        {Sam_pin::pxy::PA01, Sam_pin::muxf::fD},  //sc1p1
        {Sam_pin::pxy::PA06, Sam_pin::muxf::fD},  //sc1p2
        {Sam_pin::pxy::PA07, Sam_pin::muxf::fD},  //sc1p3

        {Sam_pin::pxy::PA12, Sam_pin::muxf::fC},  //sc2p0
        {Sam_pin::pxy::PA13, Sam_pin::muxf::fC},  //sc2p1
        {Sam_pin::pxy::PA14, Sam_pin::muxf::fC},  //sc2p2
        {Sam_pin::pxy::PA15, Sam_pin::muxf::fC},  //sc2p3
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

bool Sam_pin::MUX(const pxy nPin, const typeSamSercoms nSercom, pad &nPad)
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
