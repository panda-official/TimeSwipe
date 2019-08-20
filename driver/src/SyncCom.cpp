/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#include "SyncCom.h"

 bool CSyncSerComFSM::proc(typeSChar &ch, CFIFO &msg)
 {
     switch(m_PState)
     {

        //sending:
        case FSM::sendSilenceFrame:
            ch=0;
            if(m_FrameCnt++>3)
            {
                m_FrameCnt=0;
                m_PState=FSM::sendLengthMSB;
            }
        return true;


        case FSM::sendLengthMSB:
            ch=(msg.in_avail()>>8)|0x80;
            m_PState=FSM::sendLengthLSB;
        return true;


        case FSM::sendLengthLSB:
            ch=(msg.in_avail())&0xff;
            m_PState=FSM::sendBody;
        return true;


        case FSM::sendBody:
            if(0==msg.in_avail())
            {
                m_PState=FSM::sendOK;
                return false;
            }
            msg>>ch;
        return true;



        //receiving:
        case FSM::recSilenceFrame:

            //collision check:
            if(0!=ch)
            {
                //line error:
                m_PState=FSM::errLine;
                return false;
            }
            if(m_FrameCnt++>3)
            {
                m_FrameCnt=0;
                m_PState=FSM::recLengthMSB;
            }
        return true;


        case FSM::recLengthMSB:
            if(0!=ch)
            {
                m_TargetLength=((int)(ch&0x7f))<<8;
                m_PState=recLengthLSB;
                return true;
            }
            if(m_FrameCnt++>10000) //timeout err ->1000 05.06.2019
            {
                m_PState=FSM::errTimeout;
                return false;
            }
        return true;


        case FSM::recLengthLSB:
            m_TargetLength|=ch;
            m_PState=FSM::recBody;
        return true;


        case FSM::recBody:
            msg<<ch;
            if(msg.in_avail()>=m_TargetLength)
            {
                m_PState=FSM::recOK;
                return false;
            }
        return true;


        default: return false;

     }
 }

 void CSyncSerComFSM::start(FSM nState)
 {
     m_FrameCnt=0;
     m_PState=nState;
 }
