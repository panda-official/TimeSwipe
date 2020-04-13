/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include <random>
#include "os.h"
#include "nodeLED.h"
#include "Adafruit_NeoPixel.h"


//neopix object:
static Adafruit_NeoPixel glob_NeoPix(4, 12);
typedef Adafruit_NeoPixel typeLEDCntr;

bool              nodeLED::m_bLEDisChanged=false;
std::list<CLED *> nodeLED::m_LEDs;

std::default_random_engine generator;
std::uniform_int_distribution<int> distribution(3,220);


CLED::CLED(typeLED nLED)
{
    m_nLED=nLED;
    nodeLED::m_LEDs.push_back(this);
}
CLED::~CLED()
{
    nodeLED::m_LEDs.remove(this);
}
void CLED::Update()
{
    unsigned long cur_time=os::get_tick_mS();
    if( cur_time-m_LastTimeUpd < m_BlinkPeriod_mS)
        return;
    m_LastTimeUpd=cur_time;

    if(!m_bON || !m_bBlinking)
        return;

    m_bPhase=!m_bPhase;

    if(m_BlinkingPeriodLimit>0){
    if(m_bPhase && ++m_CurBlinkingPeriod>=m_BlinkingPeriodLimit)
    {
        ON(false); return;
    }}

    typeLEDCntr &pixels=glob_NeoPix;
    pixels.setPixelColor( get_zerob_ind(), m_bPhase ? m_Clr:0);
    nodeLED::m_bLEDisChanged=true;
}

void CLED::Blink(int nPeriods)
{
    m_bBlinking=true;
    m_BlinkingPeriodLimit=nPeriods;
    ON(true);
}

void CLED::ON(bool how)
{
    m_bON=how;
    m_CurBlinkingPeriod=0;
    if(how)
    {
        m_LastTimeUpd=os::get_tick_mS();
        m_bPhase=true;
    }
   /* else
    {
        m_BlinkingPeriodLimit=0;
         m_BlinkPeriod_mS=400;
    }*/

    typeLEDCntr &pixels=glob_NeoPix;
    pixels.setPixelColor( get_zerob_ind(), m_bON ? m_Clr:0);
    nodeLED::m_bLEDisChanged=true;
}
void CLED::SetColor(typeLEDcol Clr)
{
    m_Clr=Clr;
    if(!m_bON)
        return;

    if(m_bBlinking && !m_bPhase)
        return;

    typeLEDCntr &pixels=glob_NeoPix;
    pixels.setPixelColor( get_zerob_ind(), m_Clr);
    nodeLED::m_bLEDisChanged=true;
}
void nodeLED::Update()
{
    for(const auto led:m_LEDs)
        led->Update();

    if(m_bLEDisChanged)
    {
        m_bLEDisChanged=false;

        typeLEDCntr &pixels=glob_NeoPix;
        pixels.show();
    }
}
typeLEDcol nodeLED::gen_rnd_col()
{
    return (distribution(generator)<<16)|(distribution(generator)<<8)|distribution(generator);
}


void nodeLED::init(void)
{
	glob_NeoPix.begin();
#ifndef KEMU
    resetALL();
#endif
}

void nodeLED::random(int nBlink)
{
    typeLEDcol blink_color=gen_rnd_col();
    for(const auto pLED:m_LEDs)
    {
        pLED->SetColor(blink_color);
        pLED->Blink(nBlink);
    }
}


void nodeLED::resetALL()
{
    for(const auto pLED:m_LEDs)
    {
        pLED->ON(false);
    }
    typeLEDCntr &pixels=glob_NeoPix;
    pixels.show();
}
void nodeLED::selectLED(typeLEDind sel, typeLEDcol sel_color, typeLEDind range_begin, typeLEDind range_end, typeLEDcol back_color)
{
    for(const auto pLED:m_LEDs) if (pLED->m_nLED>=range_begin && pLED->m_nLED<=range_end)
    {
        pLED->m_bBlinking=false;
        pLED->SetColor( sel==pLED->m_nLED ? sel_color : back_color);
        pLED->ON(true);
    }
    else
        pLED->ON(false);

    typeLEDCntr &pixels=glob_NeoPix;
    pixels.show();
}
void nodeLED::setMultipleLED(typeLEDind range_begin, typeLEDind range_end, typeLEDcol back_color)
{
    for(const auto pLED:m_LEDs) if (pLED->m_nLED>=range_begin && pLED->m_nLED<=range_end)
    {
        pLED->m_bBlinking=false;
        pLED->SetColor(back_color);
        pLED->ON(true);
    }
    else
        pLED->ON(false);

    typeLEDCntr &pixels=glob_NeoPix;
    pixels.show();
}
void nodeLED::blinkLED(typeLEDind sel, typeLEDcol blink_color)
{
    for(const auto pLED:m_LEDs) if(sel==pLED->m_nLED)
    {
        pLED->m_BlinkPeriod_mS=100;
        pLED->SetColor(blink_color);
        pLED->Blink(3);
        return;
    }
}
void nodeLED::blinkMultipleLED(typeLEDind firstLED,  typeLEDind lastLED, typeLEDcol blink_color, int replication, int duration)
{
    for(const auto pLED:m_LEDs) if(pLED->m_nLED>=firstLED && pLED->m_nLED<=lastLED)
    {
        pLED->m_BlinkPeriod_mS=duration;
        pLED->SetColor(blink_color);
        pLED->Blink(replication);
    }
    else
        pLED->ON(false);
}
