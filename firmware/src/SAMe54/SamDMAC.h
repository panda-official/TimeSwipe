/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <memory>
#include <vector>
#include <list>
class CSamDMAChannel;
class CSamDMAC;
class CSamDMABlock
{
friend class CSamDMAChannel;
//friend class new_allocator<CSamDMABlock>;
protected:
    bool                 m_bFirstBlock;
    unsigned char        *m_pDescriptor;
    unsigned char        *m_pDescrMemBlock;

public:
    CSamDMABlock(CSamDMAChannel *pCont, int nInd);

public:
    ~CSamDMABlock();

//interface:
public:
    enum beatsize{

        BYTE=0,
        HWORD16,
        WORD32
    };

    void Setup(const void *pSourceAddres, const void *pDestAddress, unsigned int nBeats=1,
               CSamDMABlock::beatsize nBsize= CSamDMABlock::beatsize::BYTE);

};

class CSamDMAChannel
{
friend class CSamDMABlock;
friend class CSamDMAC;
protected:
    int m_nInd;
    CSamDMAC    *m_pCont;
    std::vector<CSamDMABlock> m_Transfer;

    inline unsigned char *get_descr_base_addr(); //{ return m_pCont->get_chan_descr_base_addr(this); }

    CSamDMAChannel(CSamDMAC *pCont, int nInd);

public:
    enum trigact{

        BLOCK=0,
        BURST=2,
        TRANSACTION=3
    };

    enum trigsrc{

        TC0OVF=0x2C,
        TC0MC0=0x2E,
        TC0MC1=0x2D,

        TC1OVF=0x2F,
        TC1MC0=0x30,
        TC1MC1=0x31,

        TC2OVF=0x32,
        TC2MC0=0x33,
        TC2MC1=0x34
    };

    ~CSamDMAChannel();

    CSamDMABlock &AddBlock();
    CSamDMABlock &GetBlock(int nInd){ return m_Transfer[nInd]; }

    void StartTransfer(bool how);
    void SetupTrigger(CSamDMAChannel::trigact act, CSamDMAChannel::trigsrc src);
    void SetLoopMode(bool how=true);
    void Enable(bool how);

};


class CSamDMAC
{
friend class CSamDMAChannel;
protected:
    enum{
       nMaxChannels=4
    };

    bool m_bChannelOcupied[nMaxChannels]={false};
    unsigned char *m_pBaseAddr;
    unsigned char *m_pWrbAddr;

    unsigned char *get_chan_descr_base_addr(CSamDMAChannel &chan);

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
