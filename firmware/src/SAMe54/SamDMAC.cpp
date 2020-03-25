/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#include <string.h>
#include "SamDMAC.h"
#include "sam.h"

static unsigned char *MemAlign128(unsigned char *pMem)
{
    unsigned char *pAligned=(unsigned char*)(((unsigned int)pMem>>4)<<4); //16
    if(pAligned<pMem)
    {
        pAligned+=sizeof(DmacDescriptor);
    }
    return pAligned;
}

CSamDMABlock::CSamDMABlock(CSamDMAChannel *pCont, int nInd)
{
    if(0==nInd)
    {
        m_pDescriptor=pCont->get_descr_base_addr();
        m_pDescrMemBlock=nullptr;
    }
    else
    {
        m_pDescrMemBlock=new unsigned char [sizeof(DmacDescriptor) + sizeof(DmacDescriptor)];
        m_pDescriptor=MemAlign128( m_pDescrMemBlock);
    }
    memset(m_pDescriptor, 0, sizeof(DmacDescriptor));

    //setup initial block chain:
    if(nInd)
    {
        ((DmacDescriptor*)(pCont->GetBlock(nInd-1).m_pDescriptor))->DESCADDR.reg=(uint32_t)m_pDescriptor;
    }
}
CSamDMABlock::~CSamDMABlock()
{
    if(m_pDescrMemBlock)
        delete [] m_pDescrMemBlock;
}

void CSamDMABlock::Setup(const void *pSourceAddres, const void *pDestAddress, unsigned int nBeats,
           CSamDMABlock::beatsize nBsize)
{

    DmacDescriptor *pDescr=(DmacDescriptor*)m_pDescriptor;

    pDescr->BTCTRL.bit.VALID=1;
    pDescr->BTCTRL.bit.BEATSIZE=nBsize;
    pDescr->SRCADDR.reg=(uint32_t)pSourceAddres;
    pDescr->DSTADDR.reg=(uint32_t)pDestAddress;
    pDescr->BTCNT.reg=nBeats;
}


unsigned char *CSamDMAChannel::get_descr_base_addr(){ return m_pCont->get_chan_descr_base_addr(*this); }
CSamDMAChannel::CSamDMAChannel(CSamDMAC *pCont, int nInd)
{
    m_pCont=pCont;
    m_nInd=nInd;

    pCont->m_bChannelOcupied[nInd]=true;
}
CSamDMAChannel::~CSamDMAChannel()
{
    m_pCont->m_bChannelOcupied[m_nInd]=false;
}

CSamDMABlock &CSamDMAChannel::AddBlock()
{
    m_Transfer.emplace_back(this, m_Transfer.size());
    return m_Transfer.back();
}

void CSamDMAChannel::StartTransfer(bool how)
{

}
void CSamDMAChannel::SetupTrigger(CSamDMAChannel::trigact act, CSamDMAChannel::trigsrc src)
{
    Dmac *pDMA=DMAC;

    DMAC->Channel[m_nInd].CHCTRLA.bit.TRIGACT=act;
    DMAC->Channel[m_nInd].CHCTRLA.bit.TRIGSRC=src;
}
void CSamDMAChannel::SetLoopMode(bool how)
{
    if(0==m_Transfer.size())
        return;

    DmacDescriptor *pFirstDescr=(DmacDescriptor*)m_Transfer.front().m_pDescriptor;
    DmacDescriptor *pLastDescr=(DmacDescriptor*)m_Transfer.back().m_pDescriptor;

    pLastDescr->DESCADDR.reg= how ? (uint32_t)pFirstDescr : 0;
}

void CSamDMAChannel::Enable(bool how)
{
    Dmac *pDMA=DMAC;

    DMAC->Channel[m_nInd].CHCTRLA.bit.ENABLE=how ? 1:0;
}


CSamDMAC::CSamDMAC()
{
    Dmac *pDMA=DMAC;

    static unsigned char BaseAddressMem[sizeof(DmacDescriptor)*(nMaxChannels+1)];
    static unsigned char WrbAddressMem[sizeof(DmacDescriptor)*(nMaxChannels+1)];

    m_pBaseAddr=MemAlign128(BaseAddressMem);
    m_pWrbAddr=MemAlign128(WrbAddressMem);

    MCLK->AHBMASK.bit.DMAC_=1;

    DMAC->BASEADDR.reg=(uint32_t)m_pBaseAddr;
    DMAC->WRBADDR.reg=(uint32_t)m_pWrbAddr;
    DMAC->CTRL.bit.LVLEN0=1;
    DMAC->CTRL.bit.DMAENABLE=1;
}
unsigned char *CSamDMAC::get_chan_descr_base_addr(CSamDMAChannel &chan)
{
    return m_pBaseAddr+(chan.m_nInd*sizeof(DmacDescriptor));
}

std::shared_ptr<CSamDMAChannel> CSamDMAC::Factory()
{
    for(int i=0; i<nMaxChannels; i++) if(!m_bChannelOcupied[i])
    {
        CSamDMAChannel *pCh=new CSamDMAChannel(this, i);
        return std::shared_ptr<CSamDMAChannel>(pCh);
    }
    return nullptr;
}


