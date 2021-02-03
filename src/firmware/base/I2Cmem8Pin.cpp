#include "I2Cmem8Pin.h"
#include "sam.h"

Sercom *glob_GetSercomPtr(typeSamSercoms nSercom);
#define SELECT_SAMI2C(nSercom) &(glob_GetSercomPtr(nSercom)->I2CS)

CSamI2Cmem8Pin::CSamI2Cmem8Pin() : CSamI2Cmem(typeSamSercoms::Sercom3)
{
    //----------setup PINs: Version2: PA22,PA23----------------
    PORT->Group[0].PMUX[11].bit.PMUXE=0x02; //(PAD0)
    PORT->Group[0].PINCFG[22].bit.PMUXEN=1; //enable

    PORT->Group[0].PMUX[11].bit.PMUXO=0x02; //(PAD1)
    PORT->Group[0].PINCFG[23].bit.PMUXEN=1; //enable
    //---------------------------------------------------------

    SercomI2cs *pI2C=SELECT_SAMI2C(m_nSercom);
    //enabling:
    pI2C->CTRLA.bit.ENABLE=1;
}
