/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


//implementing emulation of node control:

#include "nodeControl.h"
#include "DataVis.h"

#include "colour_codes.h"

std::shared_ptr<CADmux>  nodeControl::m_pMUX;
std::shared_ptr<CCalMan> nodeControl::m_pZeroCal;

//CADmux *m_pMUX;
//CCalMan *m_pZeroCal;

static bool brecord=false;

//hidden???
static std::vector<CDataVis>  m_DataVis;


//17.07.2019:
void nodeControl::CreateDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED)
{
    m_DataVis.emplace_back(pADC, pLED);
}
void nodeControl::StartDataVis(bool bHow, unsigned long nDelay_mS)
{
    for(auto &el : m_DataVis) el.Start(bHow, nDelay_mS);
}
void nodeControl::Set_board_colour(unsigned int* pCol_act, typeBoard nBoard)
{
    switch (nBoard)
    {
    case typeBoard::DMSBoard:
        for(int i = 0; i < 3; i++)
        {
        *(pCol_act + i) = col_DMS[i];
        }
      break;
    case typeBoard::IEPEBoard:
        for(int i = 0; i < 3; i++)
        {
        *(pCol_act + i) = col_IEPE[i];
        }
        break;
    default:
        break;
    }
}
void nodeControl::BlinkAtStart(typeBoard boardtype)
{
    Set_board_colour(col_act, boardtype);  

    unsigned int col_act_sca = col_act[0]*65536 + col_act[1]*256 + col_act[2]; 

    nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, col_act_sca, 2, 300);
}
void nodeControl::on_event(const char *key, nlohmann::json &val) //17.07.2019 now can rec an event
{
    //this all should be moved into the "View" instance later...

    if(0==strcmp("Zero", key))
    {
        if(val) //proc started
        {
            StartDataVis(false);
        }
        else
        {
            //reset:
            for(auto &el : m_DataVis) el.reset();

            StartDataVis(true);
        }
        return;
    }

    //menu selection:
    if(0==strcmp("Menu", key))
    {
        if(val>0) //menu is selected
        {
            StartDataVis(false);
        }
        else
        {
             StartDataVis(true, 2000);
        }
    }

}

void nodeControl::Update()
{
    for(auto &el : m_DataVis) el.Update();
}

bool nodeControl::IsRecordStarted(){ return brecord;}
void nodeControl::StartRecord(const bool how)
{
   // brecord=how;

    //17.07.2019:
    StartDataVis(false);

    static unsigned long count_mark=0;
    //make a stamp:
    typeLEDcol rstamp=nodeLED::gen_rnd_col();
    count_mark++;

    //19.06.2019: generate an event:
//    nlohmann::json v=rstamp;
    nlohmann::json v=count_mark;
    Instance().Fire_on_event("Record", v);

    //blink: here???
 //   nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, rstamp, 3, 300);
    nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, LEDrgb(255, 255, 255));

    //17.07.2019: restart data vis:
    StartDataVis(true, 300);
}

int nodeControl::gain_out(int val)
{
    typeADgain sgain=typeADgain::gainX1;
     switch(val)
     {
       // case 1: sgain=typeADgain::gainX1; break;
        case 2: sgain=typeADgain::gainX2; break;
        case 3: sgain=typeADgain::gainX4; break;
        case 4: sgain=typeADgain::gainX8;
     }
     m_pMUX->SetGain(sgain);
     int rv=(int)m_pMUX->GetGain();

     //19.06.2019: generate an event:
     nlohmann::json v=rv;
     Instance().Fire_on_event("Gain", v);

     return rv;
}
int nodeControl::GetGain(){ return (int)m_pMUX->GetGain(); }
bool nodeControl::GetBridge()
{
    return m_pMUX->GetUBRVoltage();
}
void nodeControl::SetBridge(bool how)
{
    m_pMUX->SetUBRvoltage(how);

    //19.06.2019: generate an event:
    nlohmann::json v=how;
    Instance().Fire_on_event("Bridge", v);
}
void nodeControl::SetZero(bool how)
{
    //put an event inside zero proc???
    //no, just reflect the action...

   /* nlohmann::json v=how;
    Instance().Fire_on_event("Zero", v);*/ //15.07.2019 ->moved to zero_cal_man

    if(how)
        m_pZeroCal->Start();
    else
        m_pZeroCal->StopReset();
}
