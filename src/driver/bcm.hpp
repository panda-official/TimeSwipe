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

#ifndef PANDA_TIMESWIPE_BCM_HPP
#define PANDA_TIMESWIPE_BCM_HPP

#include "../common/SyncCom.h"

#include "../3rdparty/BCMsrc/bcm2835.h"

class BcmLib {
protected:
  inline static bool is_initialized_;
  inline static bool is_spi_initialized_[2];

  ~BcmLib()
  {
    if (is_spi_initialized_[iSPI::SPI0])
      bcm2835_spi_end();

    if (is_spi_initialized_[iSPI::SPI1])
      bcm2835_aux_spi_end();

    if (is_initialized_)
      bcm2835_close();
  }

  BcmLib()
  {
    if (is_initialized_)
      return;

    if (!bcm2835_init())
      return;

    is_initialized_ = true;
  }

public:
  enum iSPI {
    SPI0,
    SPI1
  };

  bool init_SPI(iSPI nSPI)
  {
    if (is_spi_initialized_[nSPI])
      return true;

    return is_spi_initialized_[nSPI] =
      (iSPI::SPI0 == nSPI) ? bcm2835_spi_begin() : bcm2835_aux_spi_begin();
  }

  Character SPItransfer(iSPI nSPI, Character ch)
  {
    if (iSPI::SPI0 != nSPI) {
      char t=ch;
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, 1);
      return r;
    } else {
      _bcm_spi_send_char(ch);
      return _bcm_spi_rec_char();
    }
  }

  void SPI_purge(iSPI nSPI)
  {
    if (iSPI::SPI0 == nSPI)
      _bcm_spi_purge();
  }

  void SPI_setCS(iSPI nSPI, bool how)
  {
    if (iSPI::SPI0 != nSPI) {
      char t{};
      char r;
      _bcm_aux_spi_transfernb(&t, &r, 1, how ? 1:0);
    } else
      _bsm_spi_cs( how ? 1:0);
  }

  void SPI_waitDone(iSPI nSPI)
  {
    if (iSPI::SPI0 == nSPI) while (!_bsm_spi_is_done()){}
  }

  void SPI_set_speed_hz(iSPI nSPI, uint32_t speed_hz)
  {
    if (iSPI::SPI0 == nSPI)
      bcm2835_spi_set_speed_hz(speed_hz);
    else
      bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(speed_hz));
  }
};

#endif  // PANDA_TIMESWIPE_BCM_HPP
