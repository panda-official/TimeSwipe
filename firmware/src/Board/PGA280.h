/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CPGA280
*/


#pragma once
#include <stdint.h>
#include <vector>
#include "SPI.h"

class CPGA280cmd
{
public:
    enum cmd{

        write=0x40,
        read=0x80,
        directCS=0xC0,
        tbuf=0x20
    };

    bool    m_TBUF;
    cmd     m_Command;
    uint8_t m_Addr;
    uint8_t m_OutData;
    uint8_t m_InData;
    size_t  m_CmdLen; //out=in, symmetric

    CPGA280cmd(cmd Command, uint8_t Addr, uint8_t OutData, bool TBUF=false)
    {
        m_Command=Command;
        m_Addr=Addr;
        m_OutData=OutData;
        m_TBUF=TBUF;
    }

    void PushToStream(CFIFO &ostr, bool bCSmode, bool bLastCmdInChain)
    {
        uint8_t CHKsum=0x9B;

        size_t nSend;
        typeSChar frame[4]={m_Command|m_Addr, 0, 0, 0};
        if(m_TBUF)
        {
            frame[0]|=cmd::tbuf;
        }

        if(cmd::write==m_Command)
        {
            frame[1]=m_OutData;
            nSend=3;
            if(bCSmode)
                frame[2]=(uint8_t)(CHKsum+frame[0]+frame[1]);
        }
        if(cmd::read==m_Command)
        {
            if(bCSmode)
            {
                nSend=4;
                frame[1]=(uint8_t)(CHKsum+frame[0]);

            }
            else
            {
               nSend=3;
            }
        }
        else
        {
            nSend=2;
            if(bCSmode)
                frame[1]=(uint8_t)(CHKsum+frame[0]);
        }

        if(bLastCmdInChain && !bCSmode)
            nSend--;

        m_CmdLen=nSend;
        for(int i=0; i<nSend; i++)
            ostr<<frame[i];

    }
    bool PopFromStream(CFIFO &istr, bool bCSmode, bool bLastCmdInChain)
    {
        if(istr.in_avail()<m_CmdLen)
            return false;   //buffer is corrupted

        //pop the buffer data:
        typeSChar frame[4];
        for(int i=0; i<m_CmdLen; i++)
            istr>>frame[i];

        if(cmd::read!=m_Command)
            return true;

        int roffs=m_CmdLen-2; //always 2 bytes???

        //check cs:
        uint8_t CHKsum=0x9B+m_Command+frame[roffs];
        m_InData=frame[roffs];
        if(CHKsum!=frame[roffs+1])
            return false;

        return true;
    }

};

class CPGA280cmdBuf
{
public:
    bool m_bCSmode=false;
    CFIFO m_istr;
    CFIFO m_ostr;
    std::vector<CPGA280cmd> m_cmd;

    bool transfer(CSPI &spi_bus);
    void reset()
    {
        m_istr.reset();
        m_ostr.reset();
        m_cmd.clear();
    }
};


typedef union {
  struct {

    uint8_t MUX        :3;
    uint8_t IGAIN      :4;
    uint8_t OGAIN      :1;

  } bit;
  uint8_t reg;

} typeCPGA280GainMuxReg; //err clear reg 32bit


class CPGA280
{
//protected:
  //  bool Transfer(CPGA280cmdBuf &buf);

public:

    enum reg{

        gain_mux,
        soft_reset,
        CP,
        BUFtmt,
        error
    };


protected:
    std::shared_ptr<CSPI> m_pSPIbus;
    CPGA280cmdBuf m_CmdBuf;

    bool ReadRegister(reg nReg, uint8_t &RegValue);
    bool WriteRegister(reg nReg, uint8_t RegValue);

    reg m_SelReg=gain_mux;

public:

    //API:
    enum ogain{

        og1,
        og1_3_8
    };
    enum igain{

        ig_1_8,
        ig_1_4,
        ig_1_2,
        ig1,
        ig2,
        ig4,
        ig8,
        ig16,
        ig32,
        ig64,
        ig128
    };

    bool SetGains(ogain og, igain ig);
    bool GetGains(ogain &og, igain &ig);

    CPGA280(std::shared_ptr<CSPI> pSPIbus);

    //for testing:
    void SelectReg(unsigned int nReg)
    {
        m_SelReg=static_cast<reg>(nReg);
    }
    unsigned int GetSelectedReg()
    {
        return m_SelReg;
    }
    int ReadSelectedReg()
    {
        uint8_t rv;
        if(!ReadRegister(m_SelReg, rv))
            return -1;

        return rv;
    }
    void WriteSelectedReg(int nVal)
    {
        WriteRegister(m_SelReg, nVal);
    }

};
