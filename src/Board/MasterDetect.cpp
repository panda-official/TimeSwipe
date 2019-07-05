#include "MasterDetect.h"
#include "sam.h"

unsigned long get_tick_mS(void);

bool CMasterDetect::GetPinState()
{
    return ( (PORT->Group[0].IN.reg) & (1L<<17) ) ? true:false;
}
CMasterDetect::CMasterDetect() //ctor
{
    //SC1.1 (PA17)
    PORT->Group[0].PINCFG[17].bit.INEN=1;

    m_LastPinState=GetPinState();
    m_PinChangeTStamp_mS=get_tick_mS();
}

bool CMasterDetect::IsMasterAlive()
{
    return ( (get_tick_mS()-m_PinChangeTStamp_mS)<m_ActivityTmt_mS );
}

void CMasterDetect::Update()
{
    bool bPinState=GetPinState();
    if(m_LastPinState!=bPinState)
    {
        m_LastPinState=bPinState;
        m_PinChangeTStamp_mS=get_tick_mS();
    }

    //generate an ALive event:
    bool bAlive=IsMasterAlive();
    if(m_bLastAliveState!=bAlive)
    {
        m_bLastAliveState=bAlive;
        nlohmann::json v=bAlive;
        Fire_on_event("MasterIsAlive", v);
    }
}

