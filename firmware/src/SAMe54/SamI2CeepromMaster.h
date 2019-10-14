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
        addrHb,
        addrLb,
        write,

        //reading:
        read,

        //errors:
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


public:
    CSamI2CeepromMaster();

    inline bool    isIRQmode(){return m_bIRQmode;}
    void EnableIRQs(bool how);
};

//#endif // SAMI2CEEPROMMASTER_H
