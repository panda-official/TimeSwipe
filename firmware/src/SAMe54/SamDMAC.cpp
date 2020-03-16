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

