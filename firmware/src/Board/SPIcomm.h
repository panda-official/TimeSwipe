/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#ifndef SPICOMM_H
#define SPICOMM_H

#include "SamSPIbase.h"
#include "SyncCom.h"

class CSPIcomm : public CSamSPIbase
{
public:
    CSPIcomm(typeSamSercoms nSercom,
                         CSamPORT::pxy MOSI,  CSamPORT::pxy MISO, CSamPORT::pxy CLOCK, CSamPORT::pxy CS=CSamPORT::pxy::none,
                         std::shared_ptr<CSamCLK> pCLK=nullptr) :
    CSamSPIbase(false, nSercom, MOSI, MISO, CLOCK, CS, pCLK)
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

    virtual void OnIRQ0();
    virtual void OnIRQ1();
    virtual void OnIRQ2();
    virtual void OnIRQ3();

    virtual bool send(CFIFO &msg);

};

#endif // SPICOMM_H
