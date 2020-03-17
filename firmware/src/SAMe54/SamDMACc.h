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

    CSamDMABlock(CSamDMAChannel *pCont, bool bFirstBlock);

public:
    ~CSamDMABlock();

//interface:
public:
    void Setup(const void *pSourceAddres, const void *pDestAddress, unsigned int nBlockSize);

};

class CSamDMAChannel
{
protected:
    std::vector<CSamDMABlock> m_Transfer;

    CSamDMAChannel(CSamDMAC *pCont);

public:
    ~CSamDMAChannel();

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

  //  std::list<CSamDMAChannel *> m_Channels;

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
