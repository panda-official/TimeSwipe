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

#include "clock_generator.hpp"

#include <sam.h>

Sam_clock_generator::~Sam_clock_generator()
{
  instances_[static_cast<int>(id_)] = nullptr;
}

Sam_clock_generator::Sam_clock_generator(const Id id)
  : id_{id}
{}

std::shared_ptr<Sam_clock_generator> Sam_clock_generator::make()
{
  for (int i = static_cast<int>(Id::GCLK2); i <= static_cast<int>(Id::GCLK11); ++i) {
    if (!instances_[i]) {
      std::shared_ptr<Sam_clock_generator> instance{new
        Sam_clock_generator{static_cast<Id>(i)}};
      GCLK->GENCTRL[i].bit.SRC = GCLK_GENCTRL_SRC_DFLL; // def source
      instance->wait_sync();
      instances_[i] = instance.get();
      return instance;
    }
  }
  return nullptr;
}

void Sam_clock_generator::wait_sync()
{
  while (GCLK->SYNCBUSY.reg & (4UL<<static_cast<int>(id_)));
}

void Sam_clock_generator::SetDiv(unsigned short div)
{
  GCLK->GENCTRL[static_cast<int>(id_)].bit.DIV = div;
  wait_sync();
}

void Sam_clock_generator::Enable(const bool how)
{
  GCLK->GENCTRL[static_cast<int>(id_)].bit.GENEN = how;
  wait_sync();
}
