/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamSPI
*/

#pragma once

#include "SPI.h"
#include "SamSercom.h"
#include "SyncCom.h"

/*!
 * \brief A basic SAME54 SPI class used for intercommunication with external device with integrated flow control
 * \details Provides a basic low-level communication protocol ( flow control via CSyncSerComFSM )
 */
class CSamSPI : public CSamSercom, public CSPI
{
protected:

        /*!
         * \brief Is acting as master or as slave?
         */
        bool           m_bMaster;

        /*!
         * \brief Are SERCOM interrupt lines enabled?
         */
        bool           m_bIRQmode;

        /*!
         * \brief Is in interrupt mode (SERCOM interrupt lines are enabled)
         * \return true=interrupt mode is enabled, false=disabled
         */
        inline bool    isIRQmode(){return m_bIRQmode;}


        /*!
         * \brief An associated clock generator: used only in a master mode
         */
        std::shared_ptr<CSamCLK> m_pCLK;

        /*!
         * \brief Is chip select pin activated ?
         */
        bool m_bCSactive=false;

        /*!
         * \brief A flow control object
         */
        CSyncSerComFSM m_ComCntr;

        /*!
         * \brief Primary FIFO buffer to hold input chracters obtained inside interrupt routine
         * \details The writing to this buffer should be as fast as possible to leave interrupt routine and
         *  let it process next incoming characters
         */
        CFIFOlt<256> m_recFIFO;

        /*!
         * \brief Secondary FIFO buffer that's processed in Update method. Incoming message appears in the buffer by swapping with
         * m_recFIFO.
         * \details The buffer is swapped with m_recFIFO when any amount of data is detected in m_recFIFO. Then m_recFIFO dump its data
         *  to this bufer and can continue receive symbols in interrupt routine while message in  m_recFIFOhold is processing.
         * Speed is not critical since m_recFIFOhold is processed in CSamSPI::Update()
         */
        CFIFOlt<256> m_recFIFOhold;

        /*!
         * \brief Interrupt handling routine
         * \details Can be called automatically by the hardware when interrupt mode is enable or
         *  polled by CSamSPI::Update() if IRQ mode is disabled (slow mode)
         */
        void IRQhandler();

        /*!
         * \brief Sends one character and waits until it is sent
         * \param ch A character to send
         * \return An operation result: true=ok, false=error
         */
        bool send_char(typeSChar ch);

        /*!
         * \brief A method that makes a chip selection in a master mode. Has to be overridden in the derived class
         * \param how true=select chip, false=deselect chip
         */
        virtual void chip_select(bool how){}
	
        /*!
         * \brief The class constructor
         * \param nSercom The SERCOM ID
         * \param bMaster A master mode
         * \details The constructor does the following:
         * 1) calls CSamSercom constructor
         * 2) enables communication bus with corresponding SERCOM
         * 3) connects available clock generator via CSom CLK service if in a master mode
         * 4) sets default baudrate
         * 5) Turns device into SPI-master or SPI-slave(default) depending on bMaster
         */
        CSamSPI(typeSamSercoms nSercom, bool bMaster=false);
        virtual ~CSamSPI();

        virtual void OnIRQ0();
        virtual void OnIRQ1();
        virtual void OnIRQ2();
        virtual void OnIRQ3();

public:
        virtual bool send(CFIFO &msg);

        /*!
         * \brief Does nothing
         * \param msg Ignored
         * \return false
         */
        virtual bool receive(CFIFO &msg);
        virtual bool send(typeSChar ch);

        /*!
         * \brief Does nothing
         * \param msg Ignored
         * \return false
         */
        virtual bool receive(typeSChar &ch);
	
        virtual void set_phpol(bool bPhase, bool bPol);
        virtual void set_baud_div(unsigned char div);

        /*!
         * \brief Does nothing
         * \param msg Ignored
         * \return false
         */
        virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);

        /*!
         * \brief The object state update method.
         * \details Gets the CPU time to update internal state of the object.
         *  Must be called from a "super loop" or from corresponding thread
         *  If the IRQ mode is not enabled, CSamSPI::IRQhandler() is called inside this methode
         */
        void Update();

        /*!
         * \brief Enables IRQ mode
         * \param how true=enable, false=disable
         */
        void EnableIRQs(bool how);

        /*!
         * \brief Returns the state of chip select
         * \return true=selected, false=deselected
         */
        bool WasCsTrigerred(){ return m_bCSactive; }
};


