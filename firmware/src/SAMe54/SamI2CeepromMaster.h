/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


//#ifndef SAMI2CEEPROMMASTER_H
//#define SAMI2CEEPROMMASTER_H
#pragma once

#include "SamSercom.h"
class CSamI2CeepromMaster : public CSamSercom
{
public:
    enum    FSM{

        halted,

        //writing to the mem:
        start,
        addrHb,
        addrLb,
        write,

        //reading:
        read,

        //errors:
        errTransfer

        //errLine,
        //errTimeout
    };

protected:
    FSM    m_MState=FSM::halted;
    void IRQhandler();
    bool m_bIRQmode=false;

    int  m_nDevAddr=0x50;
    bool m_IOdir=false;     //false -read, true write

    std::shared_ptr<CSamCLK> m_pCLK;

    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

    //helpers:
    void StartTranfer(bool how);

    //mem interface:
    /*unsigned char *m_pMem=nullptr;
    unsigned int m_nMemSize=0;
    unsigned int m_nMemCurInd=0;*/

    CFIFO *m_pBuf=nullptr; //one single buf for R\W?

    void rewindMemBuf(); //{m_nMemCurInd=0;}
    int readB();
    int writeB(int val);


public:
    CSamI2CeepromMaster();

    inline bool    isIRQmode(){return m_bIRQmode;}
    void EnableIRQs(bool how);

    //serial:
    virtual bool send(CFIFO &msg); //{ return false;}
    virtual bool receive(CFIFO &msg); //{return false;}
    virtual bool send(typeSChar ch){return false;}
    virtual bool receive(typeSChar &ch){return false;}
};

//#endif // SAMI2CEEPROMMASTER_H
