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


class CSamSPIbase : public CSamSercom, public CSPI
{
protected:
    /*!
     * \brief An associated clock generator: used only in a master mode
     */
    std::shared_ptr<CSamCLK> m_pCLK;

    uint32_t transfer_char(uint32_t nChar);

public:
    CSamSPIbase(bool bMaster, typeSamSercoms nSercom, CSamPORT::pxy MOSI,  CSamPORT::pxy MISO, CSamPORT::pxy CLOCK, CSamPORT::pxy CS=CSamPORT::pxy::none);

    virtual bool transfer(CFIFO &out_msg, CFIFO &in_msg);
};
