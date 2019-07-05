//iostream based port:

#pragma once

#include "cmd.h"
#include "Serial.h"

class CStdPort : public ISerialEvent
{
protected:
    std::shared_ptr<CSerial> m_pBus;
    std::shared_ptr<CCmdDispatcher> m_pDisp;

    CCmdCallDescr           m_CallDescr;

    //couple of string bufers for IO:
    CFIFO      m_In;
    CFIFO      m_Out;

    //simple parser:
    bool    m_bTrimming=true;
    enum    FSM{

        proc_cmd,
        proc_function,
        proc_args,

        err_protocol    //13.06.2019
    };
    FSM    m_PState=FSM::proc_cmd;
    void parser(typeSChar ch);

    void reset()
    {
        m_bTrimming=true;
        m_PState=FSM::proc_cmd;
        m_CallDescr.m_strCommand.clear();
        m_In.reset();
        m_Out.reset();
    }

public:
    static const int TERM_CHAR='\n';

    virtual void on_rec_char(typeSChar ch)
    {
        parser(ch);
    }

public:
    CStdPort(const std::shared_ptr<CCmdDispatcher> &pDisp, const std::shared_ptr<CSerial> &pBus)
    {
        m_pDisp=pDisp;
        m_pBus=pBus;
        //pBus->AdviseSink() //???
    }

};
