/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "SamNVMCTRL.h"
#include "sam.h"

/*from SAM D5x/E5x - SmartEEPROM Code Example:
  "User needs to configure SBLK and PSZ fuses,
  to define the SmartEEPROM total size and size of each page.
  User can access SmartEEPROM using it's virtual address.
  Virtual address of SmartEEPROM starts from 0x44000000 to 0x45000000"
*/

//#define SEEPROM_ADDR 0x44000000

CSamNVMCTRL::CSamNVMCTRL()
{
    //Nvmctrl *pNVM=NVMCTRL;
    m_nSmartEEPROMsize= 2*NVMCTRL->SEESTAT.bit.SBLK*8192;
    NVMCTRL->SEECFG.bit.WMODE=1; //set buffered mode
}

bool CSamNVMCTRL::ReadSmartEEPROM(unsigned int nOffs, uint8_t *pBuf, unsigned int nRead)
{
    unsigned int lim=nOffs+nRead;
    if(lim>m_nSmartEEPROMsize)
    {
        return false;
    }

    volatile uint8_t *pSrc=(uint8_t *)SEEPROM_ADDR+nOffs;
    for(int i=0; i<nRead; i++, pSrc++, pBuf++)
    {
        while (NVMCTRL->SEESTAT.bit.BUSY){}
        *pBuf=*pSrc;
    }
    return true;
}
bool CSamNVMCTRL::WriteSmartEEPROM(unsigned int nOffs, const uint8_t *pBuf, unsigned int nWrite, bool bCompareMode)
{
    unsigned int lim=nOffs+nWrite;
    if(lim>m_nSmartEEPROMsize)
    {
        return false;
    }

    if(NVMCTRL->INTFLAG.bit.SEESOVF)
    {
        NVMCTRL->INTFLAG.bit.SEESOVF=1; //clear last error
    }


    volatile uint8_t *pSrc=(uint8_t *)SEEPROM_ADDR+nOffs;
    for(int i=0; i<nWrite; i++, pSrc++, pBuf++)
    {
        if(bCompareMode)
        {
            while (NVMCTRL->SEESTAT.bit.BUSY){}
            if(*pBuf==*pSrc)
                continue;
        }
        while (NVMCTRL->SEESTAT.bit.BUSY){}
        if(NVMCTRL->INTFLAG.bit.SEESOVF)
            return false;

        *pSrc=*pBuf;

    }
    return true;
}
void CSamNVMCTRL::FlushSmartEEPROM()
{
    //Nvmctrl *pNVM=NVMCTRL;
    if(!NVMCTRL->SEECFG.bit.WMODE)
        return;

    if(NVMCTRL->SEESTAT.bit.LOAD)
    {
        while(!NVMCTRL->STATUS.bit.READY){}

        NVMCTRL->CTRLB.reg=(NVMCTRL_CTRLB_CMDEX_KEY|NVMCTRL_CTRLB_CMD_SEEFLUSH);
    }
}



