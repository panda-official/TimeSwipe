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
//#define NVM_USER_PAGE 0x00804000
#include <string.h>
CSamNVMCTRL::CSamNVMCTRL()
{
    Nvmctrl *pNVM=NVMCTRL;

    //initial check of the user page:
    NVM_UserPage up;
    ReadUserPage(up);

    if(3!=up.Fuses.SEEPSZ || 1!=up.Fuses.SEESBLK)
    {
        up.Fuses.SEEPSZ=3;
        up.Fuses.SEESBLK=1;
        WriteUserPage(up);
    }
    m_nSmartEEPROMsize=4096;
    NVMCTRL->SEECFG.bit.WMODE=1; //set buffered mode
}

bool CSamNVMCTRL::EraseBlock(unsigned int nBlock)
{
    Nvmctrl *pNVM=NVMCTRL;

    while(!NVMCTRL->STATUS.bit.READY){}
    NVMCTRL->INTFLAG.bit.DONE=1;
    NVMCTRL->ADDR.bit.ADDR=(nBlock<<15);

    NVMCTRL->CTRLB.reg=(NVMCTRL_CTRLB_CMDEX_KEY|NVMCTRL_CTRLB_CMD_UR);

    while(!NVMCTRL->STATUS.bit.READY){}
    NVMCTRL->INTFLAG.bit.DONE=1;
    NVMCTRL->CTRLB.reg=(NVMCTRL_CTRLB_CMDEX_KEY|NVMCTRL_CTRLB_CMD_EB);

    return (!NVMCTRL->INTFLAG.bit.NVME);
}

bool CSamNVMCTRL::ReadUserPage(struct NVM_UserPage &page)
{
    page=*(  (struct NVM_UserPage *)NVMCTRL_USER );
}
bool CSamNVMCTRL::SetUserPageDefaults()
{
    static const uint32_t def_fuses[]={0xFE9A9239, 0xAEECFFB1, 0xffffffff, 0xffffffff, 0x00804010, 0xffffffff, 0xffffffff, 0xffffffff};

    NVM_UserPage up;
    memset(&up, 0xff, sizeof(up));
    memcpy(&up, def_fuses, sizeof(def_fuses));
    return WriteUserPage(up);
}

bool CSamNVMCTRL::WriteUserPage(struct NVM_UserPage &page)
{
    Nvmctrl *pNVM=NVMCTRL;

    //1: erase the page:
    while(!NVMCTRL->STATUS.bit.READY){}
    NVMCTRL->ADDR.bit.ADDR=NVMCTRL_USER;
    NVMCTRL->CTRLB.reg=(NVMCTRL_CTRLB_CMDEX_KEY|NVMCTRL_CTRLB_CMD_EP);

    //2: clear page bufer:
    while(!NVMCTRL->STATUS.bit.READY){}
    NVMCTRL->INTFLAG.bit.DONE=1;
    NVMCTRL->CTRLB.reg=(NVMCTRL_CTRLB_CMDEX_KEY|NVMCTRL_CTRLB_CMD_PBC);

    //3: set auto QUAD-WORD MODE:
    NVMCTRL->CTRLA.bit.WMODE=2;

    uint32_t *pDest=(uint32_t *)NVMCTRL_USER;
    uint32_t *pSource=(uint32_t *)&page;
    uint32_t *pSourceEnd=pSource + (sizeof(NVM_UserPage)/4);

    while(pSource<pSourceEnd)
    {
        while(!NVMCTRL->STATUS.bit.READY){}
        NVMCTRL->INTFLAG.bit.DONE=1;

        if(NVMCTRL->INTFLAG.bit.NVME)
            return false;

        for(int i=0; i<4; i++)
        {
            *pDest++=*pSource++;
        }
    }
    return true;
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



