// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "pin.hpp"

#include <sam.h>

Sam_pin::~Sam_pin()
{
  using Uint = decltype(PORT->Group[0].DIRCLR.reg);
  PORT->Group[group_].DIRCLR.reg = Uint{1}<<number_;
}

Sam_pin::Sam_pin(const Group group, const Number number, const bool bOutput)
  : group_{group}
  , number_{number}
  , pad_{Pad::pad0}
{
  set_setup_time(std::chrono::microseconds{50});

  // TODO: check the pin availability.

  using Uint = decltype(PORT->Group[0].DIRSET.reg);
  PORT->Group[group].DIRSET.reg = Uint{bOutput}<<number;
}

bool Sam_pin::connect(const Id id, const Sam_sercom::Id sercom, Pad& pad)
{
  Peripheral_function pf;
  if (!get_sercom_pad(id, sercom, pad, pf))
    return false;

  // TODO: check the pin availability.

  // Multiplex.
  const auto grp = group(id);
  const auto num = number(id);
  if (num & 1) // odd
    PORT->Group[grp].PMUX[num>>1].bit.PMUXO = pf;
  else         // even
    PORT->Group[grp].PMUX[num>>1].bit.PMUXE = pf;
  PORT->Group[grp].PINCFG[num].bit.PMUXEN = 1;
  return true;
}

void Sam_pin::do_write(const bool state)
{
  if (state) {
    using Uint = decltype(PORT->Group[0].OUTSET.reg);
    PORT->Group[group_].OUTSET.reg = Uint{1}<<number_;
  } else {
    using Uint = decltype(PORT->Group[0].OUTCLR.reg);
    PORT->Group[group_].OUTCLR.reg = Uint{1}<<number_;
  }
}

bool Sam_pin::do_read_back() const noexcept
{
    using Uint = decltype(PORT->Group[0].OUT.reg);
    return PORT->Group[group_].OUT.reg & (Uint{1}<<number_);
}

bool Sam_pin::do_read() const noexcept
{
    using Uint = decltype(PORT->Group[0].IN.reg);
    return PORT->Group[group_].IN.reg & (Uint{1}<<number_);
}

bool Sam_pin::get_sercom_pad(const Id id, const Sam_sercom::Id sercom,
  Pad& pad, Peripheral_function& pf)
{
  constexpr struct {
    Id id;
    Peripheral_function pf;
  } sercom_table[] = {
    {Id::pa04, Peripheral_function::pfd},  // sc0p0
    {Id::pa05, Peripheral_function::pfd},  // sc0p1
    {Id::pa06, Peripheral_function::pfd},  // sc0p2
    {Id::pa07, Peripheral_function::pfd},  // sc0p3

    {Id::pa16, Peripheral_function::pfc},  // sc1p0
    {Id::pa17, Peripheral_function::pfc},  // sc1p1
    {Id::pa18, Peripheral_function::pfc},  // sc1p2
    {Id::pa19, Peripheral_function::pfc},  // sc1p3

    {Id::pa09, Peripheral_function::pfd},  // sc2p0 (+ alt sc0)
    {Id::pa08, Peripheral_function::pfd},  // sc2p1 (+ alt sc0)
    {Id::pa10, Peripheral_function::pfd},  // sc2p2 (+ alt sc0)
    {Id::pa11, Peripheral_function::pfd},  // sc2p2 (+ alt sc0)

    {Id::pa17, Peripheral_function::pfd},  // sc3p0
    {Id::pa16, Peripheral_function::pfd},  // sc3p1
    {Id::pa18, Peripheral_function::pfd},  // sc3p2
    {Id::pa19, Peripheral_function::pfd},  // sc3p3

    {Id::pb12, Peripheral_function::pfc},  // sc4p0
    {Id::pb13, Peripheral_function::pfc},  // sc4p1
    {Id::pb14, Peripheral_function::pfc},  // sc4p2
    {Id::pb15, Peripheral_function::pfc},  // sc4p3

    {Id::pb16, Peripheral_function::pfc},  // sc5p0
    {Id::pb17, Peripheral_function::pfc},  // sc5p1
    {Id::pb18, Peripheral_function::pfc},  // sc5p2
    {Id::pb19, Peripheral_function::pfc},  // sc5p3

#if defined(__SAME54P20A__)
    {Id::pd09, Peripheral_function::pfd},  // sc6p0
    {Id::pd08, Peripheral_function::pfd},  // sc6p1
    {Id::pd10, Peripheral_function::pfd},  // sc6p2
#elif defined(__SAME53N19A__)
    {Id::pc16, Peripheral_function::pfc},  // sc6p0
    {Id::pc17, Peripheral_function::pfc},  // sc6p1
    {Id::pc18, Peripheral_function::pfc},  // sc6p2
#else
#error Unsupported SAM
#endif
    {Id::pd11, Peripheral_function::pfd},  // sc6p3

    {Id::pd08, Peripheral_function::pfc},  // sc7p0
    {Id::pd09, Peripheral_function::pfc},  // sc7p1
    {Id::pd10, Peripheral_function::pfc},  // sc7p2
    {Id::pd11, Peripheral_function::pfc},  // sc7p3

    // -------------------------------------------------------------------------
    // alt-1
    // -------------------------------------------------------------------------

    {Id::pa08, Peripheral_function::pfc},  // sc0p0
    {Id::pa09, Peripheral_function::pfc},  // sc0p1
    {Id::pa10, Peripheral_function::pfc},  // sc0p2
    {Id::pa11, Peripheral_function::pfc},  // sc0p3

    {Id::pa00, Peripheral_function::pfd},  // sc1p0
    {Id::pa01, Peripheral_function::pfd},  // sc1p1
    {Id::pa06, Peripheral_function::pfd},  // sc1p2
    {Id::pa07, Peripheral_function::pfd},  // sc1p3

    {Id::pa12, Peripheral_function::pfc},  // sc2p0
    {Id::pa13, Peripheral_function::pfc},  // sc2p1
    {Id::pa14, Peripheral_function::pfc},  // sc2p2
    {Id::pa15, Peripheral_function::pfc},  // sc2p3
  };

  constexpr auto tab_size = sizeof(sercom_table) / sizeof(*sercom_table);
  for (auto sercom_idx = static_cast<std::size_t>(sercom)<<2;
       sercom_idx <= (tab_size - 4); sercom_idx += 32) {
    for (int i{}; i < 4; ++i) {
      const auto entry = sercom_table[sercom_idx + i];
      if (id == entry.id) {
        pad = static_cast<Pad>(i);
        pf = static_cast<Peripheral_function>(entry.pf);
        return true;
      }
    }
  }
  return false;
}
