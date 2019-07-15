/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <stdint.h>

typedef union {
  struct {

    uint32_t DATA      :8;     //1-byte data register
    uint32_t OE        :1;     //Set when RX FIFO is full and a new data character is received. Cleared by writing 0 to I2C SPI Status register
    uint32_t UE        :1;     //1 Set when TX FIFO is empty and I2C master attempt to read a data character from I2C slave. Cleared by writing 0 to I2C SPI Status register .
    uint32_t           :6;     //reserved
    uint32_t TXBUSY    :1;     //1 Transmit operation in operation
    uint32_t RXFE      :1;     //1 When RX FIFO is empty
    uint32_t TXFF      :1;     //1 When TX FIFO is full
    uint32_t RXFF      :1;     //1 When RX FIFO is full
    uint32_t TXFE      :1;     //1 When TX FIFO is empty
    uint32_t RXBUSY    :1;     //1 Receive operation in operation
    uint32_t TXFLEVEL  :5;     //Returns the current level of the TX FIFO use
    uint32_t RXFLEVEL  :5;     //Returns the current level of the RX FIFO use

  } bit;
  uint32_t reg;

} typeBSC_SLV_DR;   //data reg 32bit


typedef union {
  struct {

    uint32_t OE         :1;
    uint32_t UE         :1;
    uint32_t            :30;    //reserved

  } bit;
  uint32_t reg;

} typeBSC_SLV_RSR; //err clear reg 32bit


typedef union {
  struct {

    uint32_t EN         :1;     //1 = Enable I2C SPI Slave.
    uint32_t SPI        :1;     //1 = Enabled SPI mode
    uint32_t I2C        :1;     //1 = Enabled I2C mode
    uint32_t CPHA       :1;
    uint32_t CPOL       :1;
    uint32_t ENSTAT     :1;     //1 = Status register enabled. When enabled the status register is transferred as a first data character on the I2C bus. Status register is transferred to the host.
    uint32_t ENCTRL     :1;     //1 = Control register enabled. When enabled the control register is received as a first data character on the I2C bus.
    uint32_t BRK        :1;     //1 = Stop operation and clear the FIFOs.
    uint32_t TXE        :1;     //1 = Transmit mode enabled
    uint32_t RXE        :1;     //1 = Receive mode enabled
    uint32_t INV_RXF    :1;     //1 = inverted status flags
    uint32_t TESTFIFO   :1;     //1 = TESTT FIFO enabled
    uint32_t HOSTCTRLEN :1;     //1 = Host Control enabled
    uint32_t INV_TXF    :1;     //1 = inverted status flags

  } bit;
  uint32_t reg;

} typeBSC_SLV_CR; //control reg 32bit


typedef struct {

    typeBSC_SLV_DR      DR;
    typeBSC_SLV_RSR     RSR;
    uint32_t            SLV;
    typeBSC_SLV_CR      CR;

    //.......................
    //.......................

}typeBSC_SLV;



/*#ifndef BCMREGS_H
#define BCMREGS_H



#endif // BCMREGS_H*/
