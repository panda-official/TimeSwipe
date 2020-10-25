/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A software implementation of flow-control for SPI bus
*   CSyncSerComFSM
*/

#include "Serial.h"

/*!
 * \brief A software implementation of flow-control for SPI bus
 *
 * \details Since the clock frequency is provided by a SPI master device to push bytes through the bus, at least one problem occurs:
 * how many clocks are required to fetch responce message from the slave if the message has variable lenght?
 * This problem is solved by using simple software flow-control:
 * each transaction is started with a "silence frame" (a sequence of zeros) that is used as marker of transfer begin.
 * Then a number of bytes that should be received is transmitted in two bytes in MSB-LSB order. Where MSB is marked with 0x80 flag.
 * This works symmetric both for master and slave: first, the master sends a silence frame followed by the length of the message,
 * so the slave knows the number of bytes to receive,
 * then the master fetches the same message header from the slave and
 * knows how many clocks should be provided to fetch the whole message from the slave.
 */

//template <typename typeFIFO>
class CSyncSerComFSM
{
public:

    //! A Finite State Machine (FSM) used to control the communication flow
    enum    FSM{

        halted,             //!<inactive state, no operation performed

        //sending:
        sendSilenceFrame,   //!<send a silence frame (a sequence of zeros) to a destination device
        sendLengthMSB,      //!<send a most significant byte of a message length to a destination device
        sendLengthLSB,      //!<send a least significant byte of a message length to a destination device
        sendBody,           //!<send message of given length to a destination device
        sendCsMSB,          //!<send a most significant byte of a message CRC16 to a destination device
        sendCsLSB,          //!<send a least significant byte of a message CRC16 to a destination device

        sendOK,             //!<sending operation was successfully finished

        //receiving:
        recSilenceFrame,    //!<receive a silence frame (a sequence of zeros) from a destination device
        recLengthMSB,       //!<receive a most significant byte of a message length from a destination device
        recLengthLSB,       //!<receive a least significant byte of a message length from a destination device
        recBody,            //!<receive message of given length from a destination device
        recCsMSB,           //!<receive a most significant byte of a message CRC16 from a destination device
        recCsLSB,           //!<receive a least significant byte of a message CRC16 from a destination device
        recOK,              //!<receiving operation was successfully finished

        //errors:
        errLine,            //!<a silence frame was disrupted (the byte other than zero was received during the silence frame)
        errTimeout,         //!<a message length bytes were not received after the silence frame
        errCs               //!<a checksumm error
    };

protected:

    /*!
     * \brief Holding the current FSM state
     */
    FSM    m_PState=FSM::halted;

    /*!
     * \brief Current bytes count in a silence frame
     */
    int    m_FrameCnt=0;

    /*!
     * \brief Obtained message length
     */
    int    m_TargetLength=0;

    /*!
     * \brief Cashed checksumm value;
     */
    uint16_t m_tCS;

    /*!
     * \brief Calculates 16-bit checksum based on 0xa001 polynom
     * \param pstr - the pointer to the message
     * \param count - the message length
     * \return 16-bit checksum value
     */
    uint16_t cs_hash(const char *pstr, int count) const
    {
        int n;
        unsigned short carry, next, crc =0xffff;

            while (count--)
            {	next = (unsigned char)*pstr;
                crc ^= next;
                for(n=0;n<8;n++)
                {	carry = (unsigned short) (crc & 1);
                    crc >>= 1;
                    if (carry)
                        crc ^= 0xa001;
                }	// for
                pstr++;
            }	// while
            return crc;
    }
    template <typename typeFIFO>
    uint16_t cs_hash(const typeFIFO &msg)
    {
        return cs_hash(msg.data(), msg.in_avail());
    }


public:
    /*!
     * \brief Turn FSM to sending(sendSilenceFrame) or receiving(recSilenceFrame) mode or stop it by setting halted state
     * \param nState A state to set
     */
    inline void start(FSM nState)
    {
        m_FrameCnt=0;
        m_PState=nState;
    }

    /*!
     * \brief Had an error happened during transaction?
     * \return true or false
     */
    inline bool bad(){ return m_PState>=errLine; }

    /*!
     * \brief Actual FSM state
     * \return FSM state
     */
    inline FSM get_state(){return  m_PState;}


    /*!
     * \brief Force execution of SPI flow-control.
     * \param ch In case of sending message operation: a character to send to SPI bus generated on flow-control logic and message
     *  to send (msg)
     * In case of receiving message operation: a received character from SPI bus that is processed according flow-control logic and
     *  received message will be saved in msg parameter.
     * \param msg A buffer for message to be send or for receiving message
     * \return
     */
    template <typename typeFIFO>
    bool proc(typeSChar &ch, typeFIFO &msg)
    {
        switch(m_PState)
        {

           //sending:
           case FSM::sendSilenceFrame:
            {
               //pre-calc CS here:
               msg.rewind();
               m_tCS=cs_hash(msg);

               ch=0;
               if(m_FrameCnt++>3)
               {
                   m_FrameCnt=0;
                   m_PState=FSM::sendLengthMSB;
               }
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
                   ch=m_tCS>>8;
                   m_PState=FSM::sendCsLSB;
                   return true;
               }
               msg>>ch;
           return true;


           case FSM::sendCsLSB:
                ch=m_tCS&0xff;
                m_PState=FSM::sendOK;
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
               if(m_FrameCnt++>10000)
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
                   m_PState=FSM::recCsMSB;
                   return true;
               }
           return true;

           case FSM::recCsMSB:
                m_tCS=ch;
                m_PState=FSM::recCsLSB;
           return true;

           case FSM::recCsLSB:
                if( cs_hash(msg) == ((m_tCS<<8)|ch )  )
                    m_PState=FSM::recOK;
                else
                    m_PState=FSM::errCs;

           return false;

           default: return false;

        }
     }
};


