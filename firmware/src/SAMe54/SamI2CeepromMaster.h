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

        //reading:
        read,

        //errors:
        errTransfer
    };

protected:
    FSM    m_MState=FSM::halted;
    void IRQhandler();
    bool m_bIRQmode=false;

    int  m_nDevAddr=0xA0;           //device address
    int  m_nMemAddr=0;              //memory address
    int  m_nCurMemAddr=0;           //additional variable for processing pages
    int  m_nReadDataCountLim=4096;  //a limit for the data to read out
    bool m_IOdir=false;             //false -read, true write
    static const int m_nPageSize=16;

    unsigned long m_OpTmt_mS=500;

    std::shared_ptr<CSamCLK> m_pCLK;
    CFIFO *m_pBuf=nullptr; //one single buf for R\W?

    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

    //helpers:
    void StartTranfer(bool how);
    void rewindMemBuf();
    int writeB(int val);

    /*!
     * \brief reset_chip_logic: reset EEPROM chip logic if it hangs and makes the bus busy
     */
    void reset_chip_logic();

    /*!
     * \brief setup_bus: initial bus setup(pinout, modes, speed with an initial reset)
     */

    void setup_bus();

    /*!
     * \brief check_reset: check bus state and perfom a chip reset/bus reinit if needed.
     */

    void check_reset();


public:
    CSamI2CeepromMaster();

    inline bool    isIRQmode(){return m_bIRQmode;}
    inline void    SetDeviceAddr(int nDevAddr){m_nDevAddr=nDevAddr; }
    inline void    SetDataAddrAndCountLim(int nDataAddr, int nCountLim=4096){m_nMemAddr=nDataAddr;  m_nReadDataCountLim=nCountLim;}
    void EnableIRQs(bool how);

    //serial:
    virtual bool send(CFIFO &msg){ return false;}
    virtual bool receive(CFIFO &msg); //{return false;}
    virtual bool send(typeSChar ch){return false;}
    virtual bool receive(typeSChar &ch){return false;}
};

//#endif // SAMI2CEEPROMMASTER_H
