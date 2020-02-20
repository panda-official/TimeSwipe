/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


/*!
*   \file
*   \brief A definition file for
*   CSamI2CeepromMaster
*/

#pragma once

#include "SamSercom.h"

/*!
 * \brief An I2C master class for communication with an external EEPROM chip ( CAT2430 )
 * \details This is a user version, only chip reading function is implemented.
 * For reading the chip data, ISerial interface is used.
 * EEPROMmemory address an a count of data to red are set by CSamI2CeepromMaster::SetDataAddrAndCountLim method
 */
class CSamI2CeepromMaster : public CSamSercom
{
public:
    //! Finite State Machine used to handle I2C bus states according to communication algorithm (see CAT24C32 manual)
    enum    FSM{

        halted,     //!< stopped, idle state

        start,      //!< A start/repeated start condition was met
        addrHb,     //!< A high address byte was written
        addrLb,     //!< A low address byte was written

        read,       //!< Continuous data read mode until the EOF

        errTransfer //! An error occurred during transmission
    };

protected:
    //! holds the current finite state
    FSM    m_MState=FSM::halted;

    //! I2C bus IRQ handler
    void IRQhandler();

     //! Is the IRQ mode enabled?
    bool m_bIRQmode=false;

    /*!
     * \brief A EEPROM chip address
     */
    int  m_nDevAddr=0xA0;

    /*!
     * \brief A EEPROM memory address
     */
    int  m_nMemAddr=0;

    /*!
     * \brief A current reading adddress
     */
    int  m_nCurMemAddr=0;

    /*!
     * \brief A maximum count of data to read out
     */
    int  m_nReadDataCountLim=4096;

    /*!
     * \brief The data stream direction: false=read, true=write
     */
    bool m_IOdir=false;

    /*!
     * \brief EEPROM page size
     */
    static const int m_nPageSize=16;

    /*!
     * \brief An operation timeout, milliseconds
     */
    unsigned long m_OpTmt_mS=500;

    /*!
     * \brief An associated clock generator: must be provided to perform operations
     */
    std::shared_ptr<CSamCLK> m_pCLK;

    /*!
     * \brief A pointer to IO buffer
     */
    CFIFO *m_pBuf=nullptr;

    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

    /*!
     * \brief Initiates a transfer process
     * \param how The transfer direction: true=write, false=read
     */
    void StartTranfer(bool how);

    /*!
     * \brief Rewinds internal FIFO bufer
     */
    void rewindMemBuf();

    /*!
     * \brief Writes a single byte to a buffer, increments counter
     * \param val A byte to write
     * \return
     */
    int writeB(int val);

    /*!
     * \brief reset_chip_logic: reset EEPROM chip logic if it hangs and makes the bus busy
     */
    void reset_chip_logic();

    /*!
     * \brief setup_bus: initial bus setup(pinout, modes, speed with an initial reset)
     */

    void setup_bus();

    /*!
     * \brief check_reset: check bus state and perfom a chip reset/bus reinit if needed.
     */

    void check_reset();


public:
    /*!
     * \brief The class constructor
     * \details The constructor does the following:
     * 1) calls CSamSercom constructor
     * 2) enables communication bus with corresponding SERCOM
     * 3) setups corresponding PINs and its multiplexing
     * 4) turns SERCOM to I2C master
     * 5) performs final tuning and enables SERCOM I2 master
     */
    CSamI2CeepromMaster();

    /*!
     * \brief Is in interrupt mode (SERCOM interrupt lines are enabled)
     * \return true=interrupt mode is enabled, false=disabled
     */
    inline bool    isIRQmode(){return m_bIRQmode;}

    /*!
     * \brief Sets the EEPROM chip target address
     * \param nDevAddr
     */
    inline void    SetDeviceAddr(int nDevAddr){m_nDevAddr=nDevAddr; }

    /*!
     * \brief Sets the EEPROM memory address for reading data and the maximum amount of data to read.
     * \param nDataAddr An initial data address to read data from
     * \param nCountLim A maximum data amount to be read
     */
    inline void    SetDataAddrAndCountLim(int nDataAddr, int nCountLim=4096){m_nMemAddr=nDataAddr;  m_nReadDataCountLim=nCountLim;}

    /*!
     * \brief Enables IRQ mode
     * \param how true=enable, false=disable
     */
    void EnableIRQs(bool how);

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool send(CFIFO &msg){ return false;}

    /*!
     * \brief Gets data from the set address with the maximum number m_nReadDataCountLim
     * \param msg A buffer to receive the data
     * \return true if read operation was successful, otherwise - false
     */
    virtual bool receive(CFIFO &msg);

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool send(typeSChar ch){return false;}

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool receive(typeSChar &ch){return false;}
};
