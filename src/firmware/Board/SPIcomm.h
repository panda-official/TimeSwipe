/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSPIcomm
*/


#ifndef SPICOMM_H
#define SPICOMM_H

#include "SamSPIbase.h"
#include "SyncCom.h"

/*!
 * \brief The class providing functionality for external communication via SPI with integrated flow-control (CSyncSerComFSM)
 * \details The external communication via SPI is based on a simple flow control protocol, please see CSyncSerComFSM description for details
 */
class CSPIcomm : public CSamSPIbase
{
public:

    /*!
     * \brief The class constructor
     * \param nSercom - SAME54 Sercom unit to be used as SPI
     * \param MOSI    - Master-Output-Slave-Input Pin of SAME54 for selected Sercom
     * \param MISO    - Master-Input-Slave-Output Pin of SAME54 for selected Sercom
     * \param CLOCK   - Clock input for selected Sercom
     * \param CS      - Chip Select input for selected Sercom
     */
    CSPIcomm(typeSamSercoms nSercom,
                         CSamPORT::pxy MOSI,  CSamPORT::pxy MISO, CSamPORT::pxy CLOCK, CSamPORT::pxy CS=CSamPORT::pxy::none) :
    CSamSPIbase(false, nSercom, MOSI, MISO, CLOCK, CS, nullptr)
    {

    }

    /*!
     * \brief The object state update method.
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     *  If the IRQ mode is not enabled, CSamSPI::IRQhandler() is called inside this methode
     */
    void Update();

protected:

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
     * \brief The handler of the 1st IRQ line of the Sercom
     */
    virtual void OnIRQ0();

    /*!
     * \brief The handler of the 2nd IRQ line of the Sercom
     */
    virtual void OnIRQ1();

    /*!
     * \brief The handler of the 3rd IRQ line of the Sercom
     */
    virtual void OnIRQ2();

    /*!
     * \brief The handler of the 4th IRQ line of the Sercom
     */
    virtual void OnIRQ3();

    /*!
     * \brief Sends a serial message to the SPI bus
     * \param msg  A message to send (output parameter)
     * \return The operation result: true if successful otherwise - false
     */
    virtual bool send(CFIFO &msg);

};

#endif // SPICOMM_H
