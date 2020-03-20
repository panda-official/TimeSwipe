/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include "SamDMACc.h"
#include "sam.h"

static unsigned char *MemAlign128(unsigned char *pMem)
{
    unsigned char *pAligned=(unsigned char*)(((unsigned int)pMem>>4)<<4); //16
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
        m_pDescriptor=pCont->get_descr_base_addr();
        m_pDescrMemBlock=nullptr;
    }
    else
    {
        m_pDescrMemBlock=new unsigned char [sizeof(DmacDescriptor) + 16];
        m_pDescriptor=MemAlign128( m_pDescrMemBlock);
    }
}
CSamDMABlock::~CSamDMABlock()
{
    if(m_pDescrMemBlock)
        delete [] m_pDescrMemBlock;
}

unsigned char *CSamDMAChannel::get_descr_base_addr(){ return m_pCont->get_chan_descr_base_addr(*this); }
CSamDMAChannel::CSamDMAChannel(CSamDMAC *pCont, int nInd)
{
    m_pCont=pCont;
    m_nInd=nInd;
}
/*CSamDMAChannel::~CSamDMAChannel()
{

}*/


CSamDMAC::CSamDMAC()
{
    static unsigned char BaseAddressMem[16*(nMaxChannels+2)];
    static unsigned char WrbAddressMem[16*(nMaxChannels+2)];

    m_pBaseAddr=MemAlign128(BaseAddressMem);
    m_pWrbAddr=MemAlign128(WrbAddressMem);
}

