/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "SamNVMCTRL.h"
#include "sam.h"

CSamNVMCTRL::CSamNVMCTRL()
{
    Nvmctrl *pNVM=NVMCTRL;

    m_pSmartEEPROM= (uint8_t*)(NVMCTRL->PARAM.bit.NVMP*512
            - 2*NVMCTRL->SEESTAT.bit.SBLK*8192);
    m_nSmartEEPROMsize= 2*NVMCTRL->SEESTAT.bit.SBLK*8192;

    NVMCTRL->SEECFG.bit.WMODE=1; //set buffered mode
}

bool CSamNVMCTRL::ReadSmartEEPROM(unsigned int nOffs, uint8_t *pBuf, unsigned int nRead)
{
   // Nvmctrl *pNVM=NVMCTRL;

    //protection:
   // if( (nOffs+nRead) > m_nSmartEEPROMsize )
    //    return false;

    unsigned int lim=nOffs+nRead;
    if(lim>m_nSmartEEPROMsize)
    {
        return false;
    }

    uint8_t *pSrc=m_pSmartEEPROM+nOffs;

    for(int i=0; i<nRead; i++, pSrc++, pBuf++)
    {
        while (NVMCTRL->SEESTAT.bit.BUSY){}
        *pBuf=*pSrc;
    }
    return true;
}
bool CSamNVMCTRL::WriteSmartEEPROM(unsigned int nOffs, const uint8_t *pBuf, unsigned int nWrite, bool bCompareMode)
{
    //Nvmctrl *pNVM=NVMCTRL;

    //protection:
   // if( (nOffs+nWrite) > m_nSmartEEPROMsize )
    //    return false;

    unsigned int lim=nOffs+nWrite;
    if(lim>m_nSmartEEPROMsize)
    {
        return false;
    }


    uint8_t *pSrc=m_pSmartEEPROM+nOffs;

    for(int i=0; i<nWrite; i++, pSrc++, pBuf++)
    {
        if(bCompareMode)
        {
            while (NVMCTRL->SEESTAT.bit.BUSY){}
            if(*pBuf==*pSrc)
                continue;
        }
        while (NVMCTRL->SEESTAT.bit.BUSY){}
        *pSrc=*pBuf;

    }
    return true;
}
void CSamNVMCTRL::FlushSmartEEPROM()
{
    Nvmctrl *pNVM=NVMCTRL;
    if(!NVMCTRL->SEECFG.bit.WMODE)
        return;

    if(NVMCTRL->SEESTAT.bit.LOAD)
    {
        while(!NVMCTRL->STATUS.bit.READY){}

        NVMCTRL->CTRLB.bit.CMD=(NVMCTRL_CTRLB_CMDEX_KEY|NVMCTRL_CTRLB_CMD_SEEFLUSH);
    }
}



