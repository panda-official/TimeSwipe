#include "gpio.hpp"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

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

volatile unsigned int *gpio;

// Set up a memory regions to access GPIO.
void setup_io()
{
   int mem_fd = -1;
   if ( (mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) { // test /dev/gpiomem!!! <- not root necessary
       printf("can't open /dev/mem \n");
       exit(-1);
   }

   //------ copied and modified from bcm2835_init() -----------
   FILE *fp;
   uint32_t base_address = 0;
   uint32_t peri_size = 0;

    /* Figure out the base and size of the peripheral address block
    // using the device-tree. Required for RPi2/3/4, optional for RPi 1
    */
    if ( (fp = fopen(BMC2835_RPI2_DT_FILENAME , "rb")))
    {
      unsigned char buf[16];
      if (fread(buf, 1, sizeof(buf), fp) >= 8)
      {
         base_address = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | (buf[7] << 0);
         peri_size = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | (buf[11] << 0);

         if (!base_address)
         {
            /* looks like RPI 4 */
            base_address = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | (buf[11] << 0);
            peri_size = (buf[12] << 24) | (buf[13] << 16) | (buf[14] << 8) | (buf[15] << 0);
         }
         /* check for valid known range formats */
         if (!((buf[0] == 0x7e) && (buf[1] == 0x00) && (buf[2] == 0x00) && (buf[3] == 0x00) &&
               ((base_address == BCM2835_PERI_BASE) || (base_address == BCM2835_RPI2_PERI_BASE) || (base_address == BCM2835_RPI4_PERI_BASE))))
         {
            printf("wrong base address");
            exit(-1);
         }
      }
       fclose(fp);
   }

   if(base_address == 0 || peri_size == 0)   //if detection failed
   {
      printf("rpi detection error!");
      exit(-1);
   }

    /* mmap GPIO */
    void *gpio_map = mmap(
      NULL,                   //Any adddress in our space will do
      peri_size,             //Map length -> 4 KB
      PROT_READ | PROT_WRITE, //Enable reading & writting to mapped memory
      MAP_SHARED,             //Shared with other processes
      mem_fd,                 //File to map
      (base_address + BCM2835_GPIO_BASE)    //Offset to GPIO peripheral
    );

   close(mem_fd); // Ok to close() after mmap
   mem_fd = -1;

   if (gpio_map == MAP_FAILED) {
      printf("mmap error %d\n", (int)(size_t)gpio_map);//errno also set!
      exit(-1);
   }

   // Always use volatile pointer!
   gpio = (volatile unsigned int *)gpio_map;
}
