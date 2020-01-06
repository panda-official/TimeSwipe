/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


/*!
*   \file
*   \brief A definition file for
*   CSamI2Cmem
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
    //! holds the current finite state
    FSM    m_MState=FSM::halted;

    //! I2C bus IRQ handler
    void IRQhandler();

    //! Is the IRQ mode enabled?
    bool m_bIRQmode=false;

    //! A pointer to a FIFO bufer to readout data from
    std::shared_ptr<CFIFO>   m_pFIFObuf;

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

    //! A pointer to a memory buffer
    unsigned char *m_pMem=nullptr;

    //! The memory buffer size
    unsigned int m_nMemSize=0;

    //! A current reading index
    unsigned int m_nMemCurInd=0;

    /*!
     * \brief A bridge from FIFO buffer to memory interface (called to obtain memory interface variables)
     */

    inline void obtain_membuf()
    {
        m_pMem=(unsigned char*)(m_pFIFObuf->c_str());
        m_nMemSize=m_pFIFObuf->size();
    }


    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

public:
    /*!
     * \brief The class constructor
     * \param nSercom The SERCOM ID
     * \details The constructor does the following:
     * 1) calls CSamSercom constructor
     * 2) enables communication bus with corresponding SERCOM
     * 3) turns SERCOM to I2Cslave
     * 4) performs final tuning and enables SERCOM I2Cslave
     */
    CSamI2Cmem(typeSamSercoms nSercom);

    /*!
     * \brief Is in interrupt mode (SERCOM interrupt lines are enabled)
     * \return true=interrupt mode is enabled, false=disabled
     */
    inline bool    isIRQmode(){return m_bIRQmode;}

    /*!
     * \brief Enables IRQ mode
     * \param how true=enable, false=disable
     */
    void EnableIRQs(bool how);

    /*!
     * \brief Setups the buffer to read EEPROM data from
     * \param pFIFObuf A pointer to the buffer
     */
    void SetMemBuf(std::shared_ptr<CFIFO> &pFIFObuf)
    {
        m_pFIFObuf=pFIFObuf;
        obtain_membuf();
    }

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool send(CFIFO &msg);

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool receive(CFIFO &msg);

    /*!
     * \brief Does nothing
     * \param ch Ignored
     * \return false
     */
    virtual bool send(typeSChar ch);

    /*!
     * \brief Does nothing
     * \param ch Ignored
     * \return false
     */
    virtual bool receive(typeSChar &ch);
};
