//  How to access GPIO registers from C-code on the Raspberry-Pi
//  Example program
//  15-January-2012
//  Dom and Gert
//  Revised: 15-Feb-2013

#ifndef GPIO_H
#define GPIO_H

/*! On all recent OSs, the base of the peripherals is read from a /proc file */
#define BMC2835_RPI2_DT_FILENAME "/proc/device-tree/soc/ranges"

/*! Physical addresses for various peripheral register sets
  Base Physical Address of the BCM 2835 peripheral registers
  Note this is different for the RPi2 BCM2836, where this is derived from /proc/device-tree/soc/ranges
  If /proc/device-tree/soc/ranges exists on a RPi 1 OS, it would be expected to contain the
  following numbers:
*/
/*! Peripherals block base address on RPi 1 */
#define BCM2835_PERI_BASE               0x20000000
/*! Size of the peripherals block on RPi 1 */
#define BCM2835_PERI_SIZE               0x01000000
/*! Alternate base address for RPI  2 / 3 */
#define BCM2835_RPI2_PERI_BASE          0x3F000000
/*! Alternate base address for RPI  4 */
#define BCM2835_RPI4_PERI_BASE          0xFE000000
/*! Alternate size for RPI  4 */
#define BCM2835_RPI4_PERI_SIZE          0x01800000

/*! Base Address of the GPIO registers */
#define BCM2835_GPIO_BASE               0x200000

#define PAGE_SIZE (4*1024)
#define BLOCK_SIZE (4*1024)

// I/O access
extern volatile unsigned int *gpio;

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio+((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g) *(gpio+((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio+(((g)/10))) |= (((a)<=3?(a)+4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET *(gpio+7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR *(gpio+10) // clears bits which are 1 ignores bits which are 0

#define GET_GPIO(g) (*(gpio+13)&(1<<g)) // 0 if LOW, (1<<g) if HIGH

#define GPIO_PULL *(gpio+37) // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio+38) // Pull up/pull down clock

// Only for binary Output reasons
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)   \
    (byte & 0x80 ? '1' : '0'), \
    (byte & 0x40 ? '1' : '0'), \
    (byte & 0x20 ? '1' : '0'), \
    (byte & 0x10 ? '1' : '0'), \
    (byte & 0x08 ? '1' : '0'), \
    (byte & 0x04 ? '1' : '0'), \
    (byte & 0x02 ? '1' : '0'), \
    (byte & 0x01 ? '1' : '0')

//
// Set up a memory regions to access GPIO
//
void setup_io();

#include "gpio.c"

#endif //GPIO_H
