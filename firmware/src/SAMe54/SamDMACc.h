#pragma once

#include <memory>
#include <vector>
#include <list>
class CSamDMAChannel;
class CSamDMAC;
class CSamDMABlock
{
protected:
    bool                 m_bFirstBlock;
    unsigned char        *m_pDescriptor;

//public:
    CSamDMABlock(CSamDMAChannel *pCont, bool bFirstBlock);
    ~CSamDMABlock();

//interface:
public:
    void Setup(const void *pSourceAddres, const void *pDestAddress, unsigned int nBlockSize);

};

class CSamDMAChannel
{
protected:
    std::vector<CSamDMABlock> m_Transfer;

//public:
    CSamDMAChannel(CSamDMAC *pCont);
    ~CSamDMAChannel();

protected:
    CSamDMABlock &AddBlock();
    CSamDMABlock &GetBlock(int nInd){ return m_Transfer[nInd]; }

    void StartTransfer(bool how);

};


class CSamDMAC
{
protected:
    enum{
       nMaxChannels=2
    };

    std::list<CSamDMAChannel *> m_Channels;

    const unsigned char *m_pBaseAddr;
    const unsigned char *m_pWrbAddr;

public:
    std::shared_ptr<CSamDMAChannel> Factory();

    static CSamDMAC& Instance()
    {
       static CSamDMAC instance;
       return instance;
    }
private:
    //! Forbid creating other instances of the class object
    CSamDMAC();

    //! Forbid copy constructor
    CSamDMAC(const CSamDMAC&)=delete;

    //! Forbid copying
    CSamDMAC& operator=(const CSamDMAC&)=delete;
};
