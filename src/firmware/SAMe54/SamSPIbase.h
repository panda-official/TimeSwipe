/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamSPIbase
*/


#pragma once

#include "SamPORT.h"
#include "SPI.h"
#include "SamSercom.h"
#include "os.h"

/*!
 * \brief The class implements basic functionality of SAME54 Sercom SPI
 */
class CSamSPIbase : public CSamSercom, public CSPI
{
protected:
    /*!
     * \brief Is acting as a master or as a slave?
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
     * \brief The single character send timeout. Used only in a slave mode to prevent hanging when master device stops providing clock frequency
     *  for some reasons
     */
    static constexpr unsigned long m_SendCharTmt_mS=100;

    /*!
     * \brief The pointer to the internal Sercom chip select pin, if specified in the class constructor
     */
    std::shared_ptr<CSamPin> m_pCS;

    /*!
     * \brief Performs a SPI transfer operation for a single character in a master mode (8/32 bits)
     * \param nChar - a character to transer
     * \return received character
     */
    uint32_t transfer_char(uint32_t nChar);

    /*!
     * \brief Sends single character(8/32) bit to the bus (can be used both in master and slave modes)
     * \param ch
     * \return
     */
    bool send_char(uint32_t ch);

public:
    /*!
     * \brief The class constructor
     * \param bMaster - true=Master SPI device, false=Slave SPI device
     * \param nSercom - SAME54 Sercom unit to be used as SPI
     * \param MOSI - Master-Output-Slave-Input Pin of SAME54 for selected Sercom
     * \param MISO - Master-Input-Slave-Output Pin of SAME54 for selected Sercom
     * \param CLOCK - Clock input pin for selected Sercom
     * \param CS - specify this only if you'd like CS pin to be automaically controlled by SAM's internal logic, otherwise specify CSamPORT::pxy::none
     * \param pCLK - Predefined Generic Clock to be used with this SPI instance in a master mode.
     *  If nullptr is specified a new Generic Clock will be created from available in a master mode. In the Slave mode Generic Clock is not required.
     */

    CSamSPIbase(bool bMaster, typeSamSercoms nSercom,
                CSamPORT::pxy MOSI,  CSamPORT::pxy MISO, CSamPORT::pxy CLOCK, CSamPORT::pxy CS=CSamPORT::pxy::none,
                std::shared_ptr<CSamCLK> pCLK=nullptr);

    /*!
     * \brief Returns the pointer to the CS pin instance
     * \return the pointer to the CS pin instance if it was specified in the class constructor, otherwise nullptr
     */
    std::shared_ptr<CSamPin> GetCSpin(){

        return m_pCS;
    }

    /*!
     * \brief Sends a serial message to this class object
     * \param msg  - the message to send (output parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool send(CFIFO &msg);

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool receive(CFIFO &msg){ return false;}


    /*!
     * \brief Performs a SPI transfer operation: send output message, receive input message of the same length
     * \param out_msg - the message to send (output parameter)
     * \param in_msg - the message to receive (input parameter)
     * \return the operation result: true if successful otherwise - false
     */
    virtual bool transfer(CFIFO &out_msg, CFIFO &in_msg);


    /*!
     * \brief Setups phase & polarity
     * \param bPhase A phase to set: true(1)-shifted, false(0) - not shifted
     * \param bPol A polarity to set: true - bus idle state=HIGH, false - bus idle state=LOW
     */
    virtual void set_phpol(bool bPhase, bool bPol);

    /*!
     * \brief  Setups baudrate divisor
     * \param div A divisor value: baudrate=clock_speed/div
     */
    virtual void set_baud_div(unsigned char div);

    /*!
     * \brief Setups the bus timing profile ---minimal time to HOLD CS HIGH---___delay in between transfers___---delay before SCK is continued---
     * \param CSminDel A minimal time to HOLD CS HIGH
     * \param IntertransDel A delay in between transfers
     * \param BeforeClockDel A delay before SCK is continued
     */
    virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel){}

    /*!
     * \brief Enables IRQ mode
     * \param how true=enable, false=disable
     */
    void EnableIRQs(bool how);
};
