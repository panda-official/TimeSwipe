/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#ifndef BCMSPI_H
#define BCMSPI_H

#include "SPI.h"
#include "SyncCom.h"

class CBcmLIB //a lib wrapper
{
protected:
    static bool m_bLibInitialized;
    static bool m_bSPIInitialized[2];

    CBcmLIB();
    ~CBcmLIB();
    
public:
    enum iSPI{

        SPI0=0,
        SPI1
    };

    bool init_SPI(iSPI nSPI);
    typeSChar SPItransfer(iSPI nSPI, typeSChar ch);
    void      SPI_purge(iSPI nSPI);
    void      SPI_setCS(iSPI nSPI, bool how);
    void      SPI_waitDone(iSPI nSPI);
    void	  SPI_set_speed_hz(iSPI nSPI, uint32_t speed_hz);

};

class CBcmSPI : public CSPI, public CBcmLIB
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

    inline typeSChar SPItransfer(typeSChar ch){ return CBcmLIB::SPItransfer(m_nSPI, ch); }
    inline void      SPI_purge(){ CBcmLIB::SPI_purge(m_nSPI); }
    inline void      SPI_setCS(bool how) { CBcmLIB::SPI_setCS(m_nSPI, how); }
    inline void      SPI_waitDone(){ CBcmLIB::SPI_waitDone(m_nSPI); }
    inline void		 SPI_set_speed_hz(uint32_t speed_hz){ CBcmLIB::SPI_set_speed_hz(m_nSPI, speed_hz); }


    virtual bool send(CFIFO &msg);
    virtual bool receive(CFIFO &msg);
    virtual bool send(typeSChar ch);
    virtual bool receive(typeSChar &ch);

    virtual void set_phpol(bool bPhase, bool bPol);
    virtual void set_baud_div(unsigned char div);
    virtual void set_tprofile_divs(unsigned char CSminDel, unsigned char IntertransDel, unsigned char BeforeClockDel);
};

#endif // BCMSPI_H
