// -*- C++ -*-

// PANDA Timeswipe Project
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

#include "../../debug.hpp"
// #include "../os.h"
#include "dac_max5715.hpp"

Dac_max5715::Dac_max5715(CSPI* const spi_bus, std::shared_ptr<Pin> pin,
  const Channel channel,
  const int min_raw, const int max_raw)
  : spi_bus_{spi_bus}
  , pin_{std::move(pin)}
  , channel_{channel}
  , min_raw_{min_raw}
  , max_raw_{max_raw}
{
  PANDA_TIMESWIPE_ASSERT(spi_bus_ && pin_ &&
    min_raw_ <= max_raw_ &&
    0 <= min_raw_ && min_raw <= 4095 &&
    0 <= max_raw_ && max_raw_ <= 4095);
}

void Dac_max5715::SetRawBinVal(const int raw)
{
  /*
   * Setup phase & polarity: phase = 0 (not shifted),
   * polarity = 1 (bus idle state is HIGH).
   */
  spi_bus_->set_phpol(false, true);

  /*
   * Setup the bus timing profile: minimal time to HOLD CS HIGH, delay
   * inbetween transfers, delay before SCK is continued.
   */
  spi_bus_->set_tprofile_divs(0xff, 0, 0xff);

  // Setup the bus buadrate divisor: rate = clock_speed / 255.
  spi_bus_->set_baud_div(0xff);

  /*
   * Prepare controlling message to send via SPI:
   * byte 1: command 3 (code value and load value "CODEn_LOADn") + channel number;
   * byte 2: control word high-byte (8 bits);
   * byte 3: control word low byte (4 bits).
   * See MAX5715 manual, page 18.
   */
  const unsigned char command = 0x30 + static_cast<int>(channel_);
  const unsigned int u_raw = raw;
  static_assert(sizeof(u_raw) >= 2);
  const unsigned char data8 = (u_raw >> 4) & 0xff; // left 8 bits
  const unsigned char data4 = (u_raw << 4) & 0xff; // right 4 bits
  CFIFO cmd;
  cmd << command; // byte 1
  cmd << data8;   // byte 2
  cmd << data4;   // byte 3

  pin_->write(true);
  // os::uwait(80);
  spi_bus_->send(cmd);
  pin_->write(false);
  // os::uwait(80);

  raw_ = raw;
}
