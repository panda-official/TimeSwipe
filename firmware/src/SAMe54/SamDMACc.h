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
protected:
    bool                 m_bFirstBlock;
    unsigned char        *m_pDescriptor;
    unsigned char        *m_pDescrMemBlock;

    CSamDMABlock(CSamDMAChannel *pCont, bool bFirstBlock);

public:
    ~CSamDMABlock();

//interface:
public:
    void Setup(const void *pSourceAddres, const void *pDestAddress, unsigned int nBlockSize);

};

class CSamDMAChannel
{
friend class CSamDMABlock;
protected:
    int m_nInd;
    CSamDMAC    *m_pCont;
    std::vector<CSamDMABlock> m_Transfer;

    inline unsigned char *get_descr_base_addr(); //{ return m_pCont->get_chan_descr_base_addr(this); }

    CSamDMAChannel(CSamDMAC *pCont, int nInd);

public:
    ~CSamDMAChannel();

    CSamDMABlock &AddBlock();
    CSamDMABlock &GetBlock(int nInd){ return m_Transfer[nInd]; }

    void StartTransfer(bool how);

};


class CSamDMAC
{
friend class CSamDMAChannel;
protected:
    enum{
       nMaxChannels=2
    };

  //  std::list<CSamDMAChannel *> m_Channels;

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
