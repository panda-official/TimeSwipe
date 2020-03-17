/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamDMACc.h"
#include "sam.h"

static const unsigned char *MemAlign128(const unsigned char *pMem)
{
    const unsigned char *pAligned=(const unsigned char*)(((unsigned int)pMem>>4)<<4); //16
    if(pAligned<pMem)
    {
        pAligned+=16;
    }
    return pAligned;
}

CSamDMABlock::CSamDMABlock(CSamDMAChannel *pCont, bool bFirstBlock)
{
    if(bFirstBlock)
    {

    }
    else
    {

    }
}
CSamDMABlock::~CSamDMABlock()
{

}

CSamDMAChannel::CSamDMAChannel(CSamDMAC *pCont)
{

}
/*CSamDMAChannel::~CSamDMAChannel()
{

}*/


CSamDMAC::CSamDMAC()
{
    static unsigned char BaseAddressMem[16*(nMaxChannels+2)];
    static unsigned char WrbAddressMem[16*(nMaxChannels+2)];

    m_pBaseAddr=MemAlign128((const unsigned char*)BaseAddressMem);
    m_pWrbAddr=MemAlign128((const unsigned char*)WrbAddressMem);
}

