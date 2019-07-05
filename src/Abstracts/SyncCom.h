#pragma once

//sync comm alg impl:

#include "Serial.h"
class CSyncSerComFSM
{
public:
    enum    FSM{

        halted,

        //sending:
        sendSilenceFrame,
        sendLengthMSB,
        sendLengthLSB,
        sendBody,
        sendOK,

        //receiving:
        recSilenceFrame,
        recLengthMSB,
        recLengthLSB,
        recBody,
        recOK,

        //errors:
        errLine,
        errTimeout
    };

    bool proc(typeSChar &ch, CFIFO &msg);
    void start(FSM nState);

    bool bad(){ return m_PState>=errLine; }

    FSM get_state(){return  m_PState;}


protected:
    FSM    m_PState=FSM::halted;
    int    m_FrameCnt=0;
    int    m_TargetLength=0;
};
