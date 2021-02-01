/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A basic Port class implementing simple ANSI text protocol (see CommunicationProtocol.md)
*   CStdPort
*
*
*/

#pragma once

#include "cmd.h"
#include "Serial.h"

/*!
 * \brief A basic Port class implementing simple ANSI text protocol (see CommunicationProtocol.md)
 *
 * \details All commands and data are presented as text in a human-readable format.
 *  The message must ended with a termination character (new line by default)
 */

class CStdPort : public ISerialEvent
{
protected:

    /*!
     * \brief A pointer to a  serial device used for communication
     */
    std::shared_ptr<CSerial> m_pBus;

    /*!
     * \brief A pointer to a command dispatcher object
     */
    std::shared_ptr<CCmdDispatcher> m_pDisp;

    /*!
     * \brief An information about command call and its parameters in protocol-independent form
     */
    CCmdCallDescr           m_CallDescr;

    /*!
     * \brief A FIFO buffer to receive incoming request message
     */
    CFIFO      m_In;

    /*!
     * \brief A FIFO buffer to form an output message
     */
    CFIFO      m_Out;

    //simple parser:

    /*!
     * \brief Shall we automatically remove spaces from the input stream? This variable controlled automatically.
     */
    bool    m_bTrimming=true;

    //! A Finite State Machine (FSM) used to parse incoming stream
    enum    FSM{

        proc_cmd,           //!<processing a command
        proc_function,      //!<waiting for a function type character: '<'="set", '>'="get"
        proc_args,          //!<processing command arguments

        err_protocol        //!<an error happened during processing an incoming request
    };

    /*!
     * \brief Holds a current state of the parser FSM
     */
    FSM    m_PState=FSM::proc_cmd;

    /*!
     * \brief Main parser function, called from on_rec_char
     * \param ch incoming character
     */
    void parser(typeSChar ch);

    /*!
     * \brief Reset the port: buffers, FSM and m_CallDescr
     */
    void reset()
    {
        m_bTrimming=true;
        m_PState=FSM::proc_cmd;
        m_CallDescr.m_strCommand.clear();
        m_In.reset();
        m_Out.reset();
    }

public:

    /*!
     * \brief Termination character used(default is "new line" )
     */
    static const int TERM_CHAR='\n';

    /*!
     * \brief "A new character has been received in a FIFO buffer of a serial device" - reimplemented ISerialEvent::on_rec_char
     * called by a Serial device
     * \param ch
     */
    virtual void on_rec_char(typeSChar ch)
    {
        parser(ch);
    }
public:

    /*!
     * \brief CStdPort Constructor
     * \param pDisp A pointer to a command dispatcher
     * \param pBus A pointer to a serial device that provides ISerial for sending response messages and ISerialEvent callback interface
     *  for listening incoming characters
     */
    CStdPort(const std::shared_ptr<CCmdDispatcher> &pDisp, const std::shared_ptr<CSerial> &pBus)
    {
        m_pDisp=pDisp;
        m_pBus=pBus;

        m_In.reserve(1024);
        m_Out.reserve(1024);
    }

};
