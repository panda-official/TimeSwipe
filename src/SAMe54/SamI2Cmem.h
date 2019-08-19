//#ifndef SAMI2CMEM_H
//#define SAMI2CMEM_H

#pragma once

#include "SamSercom.h"

class CSamI2Cmem : public CSamSercom
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

    std::shared_ptr<CSamCLK> m_pCLK;

    //interface:
    char *m_pMem=nullptr;
    unsigned int m_nMemSize=0;
    unsigned int m_nMemCurInd=0;

    int readB();
    int writeB(int val);
    void set_addr_H(int addr);
    void set_addr_L(int addr);

    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

public:
    CSamI2Cmem();
    //virtual ~CSamI2Cmem(); //just to keep polymorphic behaviour, should be never called

    inline bool    isIRQmode(){return m_bIRQmode;}
    void EnableIRQs(bool how);

    inline void SetMemBuf(char *pBuf, unsigned int nSize)
    {
        m_pMem=pBuf;
        m_nMemSize=nSize;
    }

    inline unsigned int GetCurMemInd(){ return m_nMemCurInd; }
    void    SetCurMemInd(unsigned int nInd){ m_nMemCurInd=nInd; }

    //serial:
    virtual bool send(CFIFO &msg); //{ return false;}
    virtual bool receive(CFIFO &msg); //{return false;}
    virtual bool send(typeSChar ch); //{return false;}
    virtual bool receive(typeSChar &ch); //{return false;}
};

//#endif // SAMI2CMEM_H
