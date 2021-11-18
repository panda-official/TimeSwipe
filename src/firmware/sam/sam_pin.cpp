/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "sam_pin.hpp"

#include <sam.h>

std::shared_ptr<Sam_pin> Sam_pin::FactoryPin(const Group nGroup,
  const Number nPin, const bool bOutput)
{
    //check if the pin is hardware occupied:
    //......................................
    using Uint = decltype(PORT->Group[0].DIRSET.reg);
    std::shared_ptr<Sam_pin> result{new Sam_pin{nGroup, nPin}};
    PORT->Group[nGroup].DIRSET.reg = Uint{bOutput}<<nPin;
    return result;
}

void Sam_pin::SetPin(const Group nGroup, const Number nPin,
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

bool Sam_pin::RbSetPin(const Group nGroup, const Number nPin)
{
    using Uint = decltype(PORT->Group[0].OUT.reg);
    return PORT->Group[nGroup].OUT.reg & (Uint{1}<<nPin);
}

bool Sam_pin::GetPin(const Group nGroup, const Number nPin)
{
    using Uint = decltype(PORT->Group[0].IN.reg);
    return PORT->Group[nGroup].IN.reg & (Uint{1}<<nPin);
}

void Sam_pin::ReleasePin(const Group nGroup, const Number nPin)
{
    using Uint = decltype(PORT->Group[0].DIRCLR.reg);
    PORT->Group[nGroup].DIRCLR.reg = Uint{1}<<nPin;
}

bool Sam_pin::FindSercomPad(const Id nPin, const typeSamSercoms nSercom,
  Pad& nPad, Peripheral_function &nMuxF)
{
    //table: sercom->(ID+fmux)
    constexpr struct {
        char Pin;
        char MuxF;
    } SercomIDmap[] = {
        {Id::pa04, Peripheral_function::fD},  //sc0p0
        {Id::pa05, Peripheral_function::fD},  //sc0p1
        {Id::pa06, Peripheral_function::fD},  //sc0p2
        {Id::pa07, Peripheral_function::fD},  //sc0p3

        {Id::pa16, Peripheral_function::fC},  //sc1p0
        {Id::pa17, Peripheral_function::fC},  //sc1p1
        {Id::pa18, Peripheral_function::fC},  //sc1p2
        {Id::pa19, Peripheral_function::fC},  //sc1p3

        {Id::pa09, Peripheral_function::fD},  //sc2p0 (+ alt sc0)
        {Id::pa08, Peripheral_function::fD},  //sc2p1 (+ alt sc0)
        {Id::pa10, Peripheral_function::fD},  //sc2p2 (+ alt sc0)
        {Id::pa11, Peripheral_function::fD},  //sc2p2 (+ alt sc0)

        {Id::pa17, Peripheral_function::fD},  //sc3p0
        {Id::pa16, Peripheral_function::fD},  //sc3p1
        {Id::pa18, Peripheral_function::fD},  //sc3p2
        {Id::pa19, Peripheral_function::fD},  //sc3p3

        {Id::pb12, Peripheral_function::fC},  //sc4p0
        {Id::pb13, Peripheral_function::fC},  //sc4p1
        {Id::pb14, Peripheral_function::fC},  //sc4p2
        {Id::pb15, Peripheral_function::fC},  //sc4p3

        {Id::pb16, Peripheral_function::fC},  //sc5p0
        {Id::pb17, Peripheral_function::fC},  //sc5p1
        {Id::pb18, Peripheral_function::fC},  //sc5p2
        {Id::pb19, Peripheral_function::fC},  //sc5p3

#if defined(__SAME54P20A__)
        {Id::pd09, Peripheral_function::fD},  //sc6p0
        {Id::pd08, Peripheral_function::fD},  //sc6p1
        {Id::pd10, Peripheral_function::fD},  //sc6p2
#elif defined(__SAME53N19A__)
        {Id::pc16, Peripheral_function::fC},  //sc6p0
        {Id::pc17, Peripheral_function::fC},  //sc6p1
        {Id::pc18, Peripheral_function::fC},  //sc6p2
#else
#error Unsupported SAM
#endif
        {Id::pd11, Peripheral_function::fD},  //sc6p3

        {Id::pd08, Peripheral_function::fC},  //sc7p0
        {Id::pd09, Peripheral_function::fC},  //sc7p1
        {Id::pd10, Peripheral_function::fC},  //sc7p2
        {Id::pd11, Peripheral_function::fC},  //sc7p3

        //----------------alt-1----------------------------
        {Id::pa08, Peripheral_function::fC},  //sc0p0
        {Id::pa09, Peripheral_function::fC},  //sc0p1
        {Id::pa10, Peripheral_function::fC},  //sc0p2
        {Id::pa11, Peripheral_function::fC},  //sc0p3

        {Id::pa00, Peripheral_function::fD},  //sc1p0
        {Id::pa01, Peripheral_function::fD},  //sc1p1
        {Id::pa06, Peripheral_function::fD},  //sc1p2
        {Id::pa07, Peripheral_function::fD},  //sc1p3

        {Id::pa12, Peripheral_function::fC},  //sc2p0
        {Id::pa13, Peripheral_function::fC},  //sc2p1
        {Id::pa14, Peripheral_function::fC},  //sc2p2
        {Id::pa15, Peripheral_function::fC},  //sc2p3
    };

    constexpr auto nTabSize = sizeof(SercomIDmap) / sizeof(*SercomIDmap);
    for (auto nSercomInd = static_cast<std::size_t>(nSercom)<<2;
         nSercomInd <= (nTabSize - 4); nSercomInd += 32) {
        for(std::size_t i{}; i < 4; i++) {
            const auto& sc = SercomIDmap[nSercomInd + i];
            if (nPin == sc.Pin) {
                nPad = static_cast<Pad>(i);
                nMuxF = static_cast<Peripheral_function>(sc.MuxF);
                return true;
            }
        }
    }
    return false;
}

bool Sam_pin::MUX(const Id nPin, const typeSamSercoms nSercom, Pad &nPad)
{
    Peripheral_function nMuxF;
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
