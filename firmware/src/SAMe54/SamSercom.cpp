/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "SamSercom.h"
#include "sam.h"


static CSamSercom *glob_pSC[8]={nullptr};

CSamSercom::CSamSercom(typeSamSercoms nSercom)
{
    m_nSercom=nSercom;
    glob_pSC[static_cast<int>(nSercom)]=this;
}
CSamSercom::~CSamSercom()
{
    glob_pSC[static_cast<int>(m_nSercom)]=nullptr;
}

void CSamSercom::EnableIRQ(typeSamSercomIRQs nLine, bool how)
{
    IRQn_Type nIRQ=static_cast<IRQn_Type>(SERCOM0_0_IRQn + static_cast<int>(m_nSercom)*4 + static_cast<int>(nLine));

    if(how)
    {
        __NVIC_EnableIRQ(nIRQ);
    }
    else
    {
        __NVIC_DisableIRQ(nIRQ);
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


Sercom *glob_GetSercomPtr(typeSamSercoms nSercom)
{
	switch(nSercom)
	{
		case typeSamSercoms::Sercom0: return SERCOM0;
		case typeSamSercoms::Sercom1: return SERCOM1;
		case typeSamSercoms::Sercom2: return SERCOM2;
		case typeSamSercoms::Sercom3: return SERCOM3;
		case typeSamSercoms::Sercom4: return SERCOM4;
		case typeSamSercoms::Sercom5: return SERCOM5;
		case typeSamSercoms::Sercom6: return SERCOM6;
		case typeSamSercoms::Sercom7: return SERCOM7;
	}
	return nullptr;
}
void CSamSercom::EnableSercomBus(typeSamSercoms nSercom, bool how)
{
    unsigned int set=how ? 1:0;
    switch(nSercom)
    {
        case typeSamSercoms::Sercom0: MCLK->APBAMASK.bit.SERCOM0_=set; break;
        case typeSamSercoms::Sercom1: MCLK->APBAMASK.bit.SERCOM1_=set; break;
        case typeSamSercoms::Sercom2: MCLK->APBBMASK.bit.SERCOM2_=set; break;
        case typeSamSercoms::Sercom3: MCLK->APBBMASK.bit.SERCOM3_=set; break;
        case typeSamSercoms::Sercom4: MCLK->APBDMASK.bit.SERCOM4_=set; break;
        case typeSamSercoms::Sercom5: MCLK->APBDMASK.bit.SERCOM5_=set; break;
        case typeSamSercoms::Sercom6: MCLK->APBDMASK.bit.SERCOM6_=set; break;
        case typeSamSercoms::Sercom7: MCLK->APBDMASK.bit.SERCOM7_=set; //break;
    }
}
void CSamSercom::ConnectGCLK(typeSamSercoms nSercom, typeSamCLK nCLK)
{
    int pind;
    switch(nSercom)
    {
        case typeSamSercoms::Sercom0: pind=7;   break;
        case typeSamSercoms::Sercom1: pind=8;   break;
        case typeSamSercoms::Sercom2: pind=23;  break;
        case typeSamSercoms::Sercom3: pind=24;  break;
        case typeSamSercoms::Sercom4: pind=34;  break;
        case typeSamSercoms::Sercom5: pind=35;  break;
        case typeSamSercoms::Sercom6: pind=36;  break;
        case typeSamSercoms::Sercom7: pind=37;
    }
    if(nCLK==typeSamCLK::none)
    {
        GCLK->PCHCTRL[pind].bit.CHEN=0; //remove
    }
    else
    {
        GCLK->PCHCTRL[3].bit.GEN=(uint32_t)nCLK; //sercom slow
        GCLK->PCHCTRL[3].bit.CHEN=1; //add

       GCLK->PCHCTRL[pind].bit.GEN=(uint32_t)nCLK;
       GCLK->PCHCTRL[pind].bit.CHEN=1; //add
    }
}



