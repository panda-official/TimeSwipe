/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A glue between the LED classes and Adafruit NeoPixel library
*   \details Contains prototypes of functions that are called from Adafruit library and have to be implemented
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

/*!
 * \brief Setups the control pin
 * \param pin Ignored, overridden by the firmware
 * \param mode Ignored
 */
void pinMode(int pin, int mode);

/*!
 * \brief Forses the output pin to "HIGH" or "LOW" state
 * \param pin Ignored, overridden by the firmware
 * \param how "HIGH" turns pin to HIGH state, "LOW" - turns pin to LOW state
 */
void digitalWrite(int pin, int how);

/*!
 * \brief Disables CPU interrupts
 */
void noInterrupts(void);

/*!
 * \brief Enables CPU interrupts
 */
void interrupts(void); 

/*!
 * \brief Stub function: just returns input *parameter
 * \param pByte A pointer to a byte that will be returned
 * \return *pByte
 */
uint8_t pgm_read_byte(const uint8_t *pByte);

/*!
 * \brief Returns control pin mask
 * \param pin Ignored
 * \return Control pin mask
 */
uint32_t get_pin_mask(int pin);

/*!
 * \brief Returns the physical address of the control PIN SET register
 * \param pin Ignored
 * \return A &PORT[x].OUTSET.reg)
 */
uint32_t *get_OUTSET_addr(int pin);

/*!
 * \brief Returns the physical address of the control PIN CLEAR register
 * \param pin Ignored
 * \return A &PORT[x].OUTCLR.reg)
 */
uint32_t *get_OUTCLR_addr(int pin);


