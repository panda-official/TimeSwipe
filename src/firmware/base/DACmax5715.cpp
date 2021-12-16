/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/
//DAC max 5715 impl:

#include "../os.h"
#include "DACmax5715.h"

CDac5715sa::CDac5715sa(CSPI *pBus, std::shared_ptr<Pin> pCS, typeDac5715chan nChan)
{
    m_pBus=pBus;
    m_pCS=pCS;
    m_chan=nChan;

    //! setup the raw-binary range and user defined user range
    set_raw_range(120, 3904); // 120 - 24V, 3904 - 2.5V
    SetRawOutput(3904); // 2.5V by default
}

//driver function:
void CDac5715sa::DriverSetVal(float, const int out_bin)
{
    /*!
     * m_pBus->set_phpol(false, true);
     * setup phase & polarity: phase=0(not shifted), polarity=true(1, bus idle state=HIGH)
     */
    m_pBus->set_phpol(false, true);

    //! m_pBus->set_tprofile_divs(0xff, 0, 0xff);
    //! setup the bus timing profile:
    //! ---minimal time to HOLD CS HIGH---___delay in between transfers___---delay before SCK is continued
    m_pBus->set_tprofile_divs(0xff, 0, 0xff);

    //! m_pBus->set_baud_div(0xff);
    //! setup the bus buadrate divisor: rate=clock_speed/255;
    m_pBus->set_baud_div(0xff);

    //! cmd<<(0x30+(int)m_chan)<<((out_bin>>4)&0xff)<<((out_bin<<4)&0xff);
    //! forms a controlling message to be sent via SPI(MAX5715 manual, page 18)
    //! 1st byte: command 3(code value and load value "CODEn_LOADn")+channel number
    //! 2nd byte: control word high-byte
    //! 3nd byte: control word low byte
    const unsigned char command = 0x30 + static_cast<int>(m_chan);
    // We need only 12 bits which must be left-shifted.
    const unsigned int u_out_bin = out_bin;
    static_assert(sizeof(u_out_bin) >= 2);
    const unsigned char data8 = (u_out_bin >> 4) & 0xff; // left 8 bits
    const unsigned char data4 = (u_out_bin << 4) & 0xff; // right 4 bits

    CFIFO cmd;
    cmd += command;
    cmd += data8;
    cmd += data4;

    m_pCS->write(true);
    //os::uwait(80);

    m_pBus->send(cmd);

    m_pCS->write(false);
    //os::uwait(80);
}
