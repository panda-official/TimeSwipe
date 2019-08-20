/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAME54 PORTS for ino:

#include "Adafruit_NeoPixel_stub.h"
#include "sam.h"

#define PIN_MASK (1L<<12) 	//PB12=041
#define PIN_GROUP 1             //PB

void pinMode(int pin, int mode)
{
        PORT->Group[PIN_GROUP].DIRSET.reg=PIN_MASK;
}
void digitalWrite(int pin, int how)
{
	if(HIGH==how)
                PORT->Group[PIN_GROUP].OUTSET.reg=PIN_MASK;
	else
                PORT->Group[PIN_GROUP].OUTCLR.reg=PIN_MASK;
		
}
void noInterrupts(void)
{
	__disable_irq(); //core M4 function 25.02.2019
}
void interrupts(void)
{
	__enable_irq(); //core M4 function 25.02.2019
}
uint8_t pgm_read_byte(const uint8_t *pByte){return *pByte;}


//now global funct:
uint32_t get_pin_mask(int pin)
{
	return PIN_MASK;
}
uint32_t *get_OUTSET_addr(int pin)
{
        return (uint32_t *)&(PORT->Group[PIN_GROUP].OUTSET.reg);
}
uint32_t *get_OUTCLR_addr(int pin)
{
        return (uint32_t *)&(PORT->Group[PIN_GROUP].OUTCLR.reg);
}

