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

#include "sercom.hpp"

#include <sam.h>

#include <exception>

static Sam_sercom* glob_pSC[8];

Sam_sercom::~Sam_sercom()
{
  glob_pSC[static_cast<int>(id_)] = nullptr;
}

Sam_sercom::Sam_sercom(const Id id)
  : id_{id}
{
  glob_pSC[static_cast<int>(id_)] = this;
}

void Sam_sercom::enable_irq(const Irq irq, const bool enable)
{
  const auto num = static_cast<IRQn_Type>(SERCOM0_0_IRQn +
    static_cast<int>(id_)*4 + static_cast<int>(irq));
  if (enable)
    __NVIC_EnableIRQ(num);
  else
    __NVIC_DisableIRQ(num);
}

void Sam_sercom::enable_internal_bus(const bool enable)
{
  switch(id_) {
  case Id::Sercom0: MCLK->APBAMASK.bit.SERCOM0_ = enable; break;
  case Id::Sercom1: MCLK->APBAMASK.bit.SERCOM1_ = enable; break;
  case Id::Sercom2: MCLK->APBBMASK.bit.SERCOM2_ = enable; break;
  case Id::Sercom3: MCLK->APBBMASK.bit.SERCOM3_ = enable; break;
  case Id::Sercom4: MCLK->APBDMASK.bit.SERCOM4_ = enable; break;
  case Id::Sercom5: MCLK->APBDMASK.bit.SERCOM5_ = enable; break;
  case Id::Sercom6: MCLK->APBDMASK.bit.SERCOM6_ = enable; break;
  case Id::Sercom7: MCLK->APBDMASK.bit.SERCOM7_ = enable; break;
  }
}

//IRQs handling:
void SERCOM0_0_Handler(void)
{
    glob_pSC[0]->OnIRQ0();
}
void SERCOM0_1_Handler(void)
{
    glob_pSC[0]->OnIRQ1();
}
void SERCOM0_2_Handler(void)
{
    glob_pSC[0]->OnIRQ2();
}
void SERCOM0_3_Handler(void)
{
     glob_pSC[0]->OnIRQ3();
}

void SERCOM1_0_Handler(void)
{
    glob_pSC[1]->OnIRQ0();
}
void SERCOM1_1_Handler(void)
{
    glob_pSC[1]->OnIRQ1();
}
void SERCOM1_2_Handler(void)
{
    glob_pSC[1]->OnIRQ2();
}
void SERCOM1_3_Handler(void)
{
     glob_pSC[1]->OnIRQ3();
}

void SERCOM2_0_Handler(void)
{
    glob_pSC[2]->OnIRQ0();
}
void SERCOM2_1_Handler(void)
{
    glob_pSC[2]->OnIRQ1();
}
void SERCOM2_2_Handler(void)
{
    glob_pSC[2]->OnIRQ2();
}
void SERCOM2_3_Handler(void)
{
     glob_pSC[2]->OnIRQ3();
}

void SERCOM3_0_Handler(void)
{
    glob_pSC[3]->OnIRQ0();
}
void SERCOM3_1_Handler(void)
{
    glob_pSC[3]->OnIRQ1();
}
void SERCOM3_2_Handler(void)
{
    glob_pSC[3]->OnIRQ2();
}
void SERCOM3_3_Handler(void)
{
     glob_pSC[2]->OnIRQ3();
}

void SERCOM4_0_Handler(void)
{
    glob_pSC[4]->OnIRQ0();
}
void SERCOM4_1_Handler(void)
{
    glob_pSC[4]->OnIRQ1();
}
void SERCOM4_2_Handler(void)
{
    glob_pSC[4]->OnIRQ2();
}
void SERCOM4_3_Handler(void)
{
     glob_pSC[4]->OnIRQ3();
}

void SERCOM5_0_Handler(void)
{
    glob_pSC[5]->OnIRQ0();
}
void SERCOM5_1_Handler(void)
{
    glob_pSC[5]->OnIRQ1();
}
void SERCOM5_2_Handler(void)
{
    glob_pSC[5]->OnIRQ2();
}
void SERCOM5_3_Handler(void)
{
     glob_pSC[5]->OnIRQ3();
}

void SERCOM6_0_Handler(void)
{
    glob_pSC[6]->OnIRQ0();
}
void SERCOM6_1_Handler(void)
{
    glob_pSC[6]->OnIRQ1();
}
void SERCOM6_2_Handler(void)
{
    glob_pSC[6]->OnIRQ2();
}
void SERCOM6_3_Handler(void)
{
     glob_pSC[6]->OnIRQ3();
}

void SERCOM7_0_Handler(void)
{
    glob_pSC[7]->OnIRQ0();
}
void SERCOM7_1_Handler(void)
{
    glob_pSC[7]->OnIRQ1();
}
void SERCOM7_2_Handler(void)
{
    glob_pSC[7]->OnIRQ2();
}
void SERCOM7_3_Handler(void)
{
     glob_pSC[7]->OnIRQ3();
}

Sercom *glob_GetSercomPtr(const Sam_sercom::Id sercom)
{
  using Id = Sam_sercom::Id;
  switch (sercom) {
  case Id::Sercom0: return SERCOM0;
  case Id::Sercom1: return SERCOM1;
  case Id::Sercom2: return SERCOM2;
  case Id::Sercom3: return SERCOM3;
  case Id::Sercom4: return SERCOM4;
  case Id::Sercom5: return SERCOM5;
  case Id::Sercom6: return SERCOM6;
  case Id::Sercom7: return SERCOM7;
  }
  return nullptr;
}

void Sam_sercom::connect_clock_generator(const typeSamCLK nCLK)
{
  const int pind = [this]
  {
    switch(id_) {
    case Id::Sercom0: return 7;
    case Id::Sercom1: return 8;
    case Id::Sercom2: return 23;
    case Id::Sercom3: return 24;
    case Id::Sercom4: return 34;
    case Id::Sercom5: return 35;
    case Id::Sercom6: return 36;
    case Id::Sercom7: return 37;
    }
    std::terminate();
  }();

  if(nCLK == typeSamCLK::none) {
    GCLK->PCHCTRL[pind].bit.CHEN = 0; //remove
  } else {
    GCLK->PCHCTRL[3].bit.GEN = static_cast<std::uint32_t>(nCLK); // slow
    GCLK->PCHCTRL[3].bit.CHEN = 1; // add

    GCLK->PCHCTRL[pind].bit.GEN = static_cast<std::uint32_t>(nCLK);
    GCLK->PCHCTRL[pind].bit.CHEN = 1; // add
  }
}
