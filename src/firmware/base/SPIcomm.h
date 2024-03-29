/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#ifndef SPICOMM_H
#define SPICOMM_H

#include "../../synccom.hpp"
#include "sam/SamSPIbase.h"

#include <optional>

/**
 * @brief An API for external communication via SPI with integrated flow-control.
 *
 * @see CSyncSerComFSM.
 */
class CSPIcomm final : public CSamSPIbase {
public:
    /*!
     * \brief The class constructor
     * \param nSercom - SAME54 Sercom unit to be used as SPI
     * \param MOSI    - Master-Output-Slave-Input Pin of SAME54 for selected Sercom
     * \param MISO    - Master-Input-Slave-Output Pin of SAME54 for selected Sercom
     * \param CLOCK   - Clock input for selected Sercom
     * \param CS      - Chip Select input for selected Sercom
     */
  CSPIcomm(Id nSercom, Sam_pin::Id MOSI, Sam_pin::Id MISO, Sam_pin::Id CLOCK,
    std::optional<Sam_pin::Id> CS)
    : CSamSPIbase(false, nSercom, MOSI, MISO, CLOCK, CS, nullptr)
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
    CFIFOlt<4096> m_recFIFO;

    /*!
     * \brief Secondary FIFO buffer that's processed in Update method. Incoming message appears in the buffer by swapping with
     * m_recFIFO.
     * \details The buffer is swapped with m_recFIFO when any amount of data is detected in m_recFIFO. Then m_recFIFO dump its data
     *  to this bufer and can continue receive symbols in interrupt routine while message in  m_recFIFOhold is processing.
     * Speed is not critical since m_recFIFOhold is processed in CSamSPI::Update()
     */
    CFIFOlt<4096> m_recFIFOhold;

    /*!
     * \brief Interrupt handling routine
     * \details Can be called automatically by the hardware when interrupt mode is enable or
     *  polled by CSamSPI::Update() if IRQ mode is disabled (slow mode)
     */
    void IRQhandler();

    /// @see Sam_sercom::handle_irq0();
    void handle_irq0() override;

    /// @see Sam_sercom::handle_irq1();
    void handle_irq1() override;

    /// @see Sam_sercom::handle_irq2();
    void handle_irq2() override;

    /// @see Sam_sercom::handle_irq3();
    void handle_irq3() override;

    /*!
     * \brief Sends a serial message to the SPI bus
     * \param msg  A message to send (output parameter)
     * \return The operation result: true if successful otherwise - false
     */
    bool send(CFIFO &msg) override;

};

#endif // SPICOMM_H
