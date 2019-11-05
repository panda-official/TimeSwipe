#include "I2CmemHAT.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMI2C(nSercom) &(glob_GetSercomPtr(nSercom)->I2CS)

CSamI2CmemHAT::CSamI2CmemHAT() : CSamI2Cmem(typeSamSercoms::Sercom1)
{
    //----------setup PINs: Version2: PA16,PA17----------------
    PORT->Group[0].PMUX[8].bit.PMUXE=0x02; //(PAD0)
    PORT->Group[0].PINCFG[16].bit.PMUXEN=1; //enable

    PORT->Group[0].PMUX[8].bit.PMUXO=0x02; //(PAD1)
    PORT->Group[0].PINCFG[17].bit.PMUXEN=1; //enable
    //---------------------------------------------------------

    SercomI2cs *pI2C=SELECT_SAMI2C(m_nSercom);
    //enabling:
    pI2C->CTRLA.bit.ENABLE=1;
}
