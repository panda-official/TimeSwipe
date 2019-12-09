/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#pragma once

#include "SamSercom.h"

/*!
 * \brief The CSamI2Cmem class emulates CAT24C32 EEPROM chip in the read-only mode
 *
 * \details
 *
 */


class CSamI2Cmem : public CSamSercom
{
public:

    //! Finite State Machine used to handle I2C bus states according to communication algorithm (see CAT24C32 manual)
    enum    FSM{

        halted,     //!< stopped, idle state

        addrHb,     //!< waiting memory address high byte
        addrLb,     //!< waiting memory address low byte
        waiting_rs, //!< waiting repeated start condition after receiving the address

        read,       //!< switching to continuously reading mode after repeated start
    };

protected:
    FSM    m_MState=FSM::halted; //! holds the current finite state
    void IRQhandler();           //! I2C bus IRQ handler
    bool m_bIRQmode=false;       //! is the IRQ mode enabled?

    std::shared_ptr<CFIFO>   m_pFIFObuf; //! pointer to a FIFO bufer to readout data from

    /*! implementation of the small kind of "hardware independent" memory interface (stream-like)
    * used in the I2C IRQ handler:
    */

    /*!
     * \brief readB fetch a byte from the bufer pointed by m_pMem at the index m_nMemCurInd, increments the index by one
     * \return byte read by success or -1 in the case of EOF
     */

    int readB();

    /*!
     * \brief set_addr_H sets a high-byte of m_nMemCurInd (bits 8-15)
     * \param addr high byte value (8bit)
     */

    void set_addr_H(int addr);

    /*!
     * \brief set_addr_H sets a low-byte of m_nMemCurInd (bits 0-7)
     * \param addr low byte value (8bit)
     */

    void set_addr_L(int addr);

    //! interface variables
    unsigned char *m_pMem=nullptr;
    unsigned int m_nMemSize=0;
    unsigned int m_nMemCurInd=0;

    /*!
     * \brief obtain_membuf: bridge from FIFO bufer to memory inerface (called to obtainmemory interface variables)
     */

    inline void obtain_membuf()
    {
        m_pMem=(unsigned char*)(m_pFIFObuf->c_str()); //dbg only
        m_nMemSize=m_pFIFObuf->size(); //??? not good....
    }


    //! Current SERCOM IRQ lines (overriden)
    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

public:
    CSamI2Cmem(typeSamSercoms nSercom);
    //virtual ~CSamI2Cmem(); //just to keep polymorphic behaviour, should be never called

    inline bool    isIRQmode(){return m_bIRQmode;}
    void EnableIRQs(bool how);

    //02.11.2019:
    void SetMemBuf(std::shared_ptr<CFIFO> &pFIFObuf)
    {
        m_pFIFObuf=pFIFObuf;
        obtain_membuf();
    }

    //serial:
    virtual bool send(CFIFO &msg); //{ return false;}
    virtual bool receive(CFIFO &msg); //{return false;}
    virtual bool send(typeSChar ch); //{return false;}
    virtual bool receive(typeSChar &ch); //{return false;}
};
