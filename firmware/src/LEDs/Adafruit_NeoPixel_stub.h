/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "os.h"
typedef bool boolean;
#define micros() (os::get_tick_mS()*1000)

#define INPUT 	0
#define OUTPUT 	1
#define LOW		0
#define HIGH	1
#define PROGMEM

void pinMode(int pin, int mode);
void digitalWrite(int pin, int how);
void noInterrupts(void);
void interrupts(void); 
uint8_t pgm_read_byte(const uint8_t *pByte);

//now global funct:
uint32_t get_pin_mask(int pin);
uint32_t *get_OUTSET_addr(int pin);
uint32_t *get_OUTCLR_addr(int pin);


