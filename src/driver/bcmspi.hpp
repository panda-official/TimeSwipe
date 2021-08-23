// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_SPI_HPP
#define PANDA_TIMESWIPE_SPI_HPP

#include "../common/SPI.h"
#include "../common/SyncCom.h"

class BcmLib {
protected:
    static bool m_bLibInitialized;
    static bool m_bSPIInitialized[2];

    BcmLib();
    ~BcmLib();

public:
    enum iSPI{

        SPI0=0,
        SPI1
    };

    bool init_SPI(iSPI nSPI);
    Character SPItransfer(iSPI nSPI, Character ch);
    void      SPI_purge(iSPI nSPI);
    void      SPI_setCS(iSPI nSPI, bool how);
    void      SPI_waitDone(iSPI nSPI);
    void	  SPI_set_speed_hz(iSPI nSPI, uint32_t speed_hz);

};

class CBcmSPI : public CSPI, public BcmLib
{
protected:
    //bool  m_bInitialized=false;

    iSPI m_nSPI;

    CFIFO m_recFIFO;
public:
    CSyncSerComFSM m_ComCntr;

public:
    CBcmSPI(iSPI nSPI=iSPI::SPI0);
    virtual ~CBcmSPI();

    bool is_initialzed(){ return m_bSPIInitialized[m_nSPI]; }

    inline Character SPItransfer(Character ch){ return BcmLib::SPItransfer(m_nSPI, ch); }
    inline void      SPI_purge(){ BcmLib::SPI_purge(m_nSPI); }
    inline void      SPI_setCS(bool how) { BcmLib::SPI_setCS(m_nSPI, how); }
    inline void      SPI_waitDone(){ BcmLib::SPI_waitDone(m_nSPI); }
    inline void		 SPI_set_speed_hz(uint32_t speed_hz){ BcmLib::SPI_set_speed_hz(m_nSPI, speed_hz); }


    bool send(CFIFO &msg) override;
    bool receive(CFIFO &msg) override;
    virtual bool send(Character ch);
    virtual bool receive(Character &ch);

    void set_phpol(bool bPhase, bool bPol) override;
    void set_baud_div(unsigned char div) override;
    void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel) override;
};

#endif  // PANDA_TIMESWIPE_SPI_HPP
