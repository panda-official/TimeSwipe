/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "SamPORT.h"
#include "SPI.h"
#include "SamSercom.h"
#include "os.h"


class CSamSPIbase : public CSamSercom, public CSPI
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

    static constexpr unsigned long m_SendCharTmt_mS=100;

    unsigned long CSminDel_uS=70;

    std::shared_ptr<CSamPin> m_pCS;

  /*  inline void chip_select(bool bHow)
    {
        if(!m_bMaster)
            return;

        if(!m_pCS)
            return;
        m_pCS->Set(bHow);

        os::uwait(CSminDel_uS);
    }*/
    uint32_t transfer_char(uint32_t nChar);
    bool send_char(uint32_t ch);

public:
    /*!
     * \brief CSamSPIbase
     * \param bMaster
     * \param nSercom
     * \param MOSI
     * \param MISO
     * \param CLOCK
     * \param CS - specify this only if you'd like CS pin to be automaically controlled by SAM's internal logic, otherwise specify CSamPORT::pxy::none
     * \param pCLK
     */

    CSamSPIbase(bool bMaster, typeSamSercoms nSercom,
                CSamPORT::pxy MOSI,  CSamPORT::pxy MISO, CSamPORT::pxy CLOCK, CSamPORT::pxy CS=CSamPORT::pxy::none,
                std::shared_ptr<CSamCLK> pCLK=nullptr);

    std::shared_ptr<CSamPin> GetCSpin(){

        return m_pCS;
    }

    virtual bool send(CFIFO &msg);

    /*!
     * \brief Does nothing
     * \param msg Ignored
     * \return false
     */
    virtual bool receive(CFIFO &msg){ return false;}


    virtual bool transfer(CFIFO &out_msg, CFIFO &in_msg);


    virtual void set_phpol(bool bPhase, bool bPol);
    virtual void set_baud_div(unsigned char div);
    virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel)
    {
        CSminDel_uS=CSminDel;
    }

    /*!
     * \brief Enables IRQ mode
     * \param how true=enable, false=disable
     */
    void EnableIRQs(bool how);
};
