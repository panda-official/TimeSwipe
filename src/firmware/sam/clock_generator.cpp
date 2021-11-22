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

std::list<Sam_clock_generator*> Sam_clock_generator::instances_;
bool Sam_clock_generator::busy_[12];

Sam_clock_generator::~Sam_clock_generator()
{
  instances_.remove(this);
  busy_[static_cast<int>(id_)] = false;
}

//factory:
std::shared_ptr<Sam_clock_generator> Sam_clock_generator::Factory()
{
    for(int i=static_cast<int>(Id::GCLK2); i<=static_cast<int>(Id::GCLK11); i++)
    {
        //check if hardware occupied by a first time
        if (GCLK->GENCTRL[i].bit.GENEN) {
          busy_[i] = true;
          continue;
        }


        if(!busy_[i])
        {
            busy_[i]=true;
            Sam_clock_generator *pClk= new Sam_clock_generator; //because of protected ctor
            pClk->id_ = static_cast<Id>(i);
            instances_.push_back(pClk);
            GCLK->GENCTRL[i].bit.SRC=GCLK_GENCTRL_SRC_DFLL; //def source
            pClk->WaitSync();
            return std::shared_ptr<Sam_clock_generator>(pClk);
        }
    }
    return nullptr;
}

void Sam_clock_generator::WaitSync()
{
  while (GCLK->SYNCBUSY.reg & (4UL<<static_cast<int>(id_)));
}

void Sam_clock_generator::SetDiv(unsigned short div)
{
  GCLK->GENCTRL[static_cast<int>(id_)].bit.DIV = div;
  WaitSync();
}

void Sam_clock_generator::Enable(const bool how)
{
  GCLK->GENCTRL[static_cast<int>(id_)].bit.GENEN = how;
  WaitSync();
}
