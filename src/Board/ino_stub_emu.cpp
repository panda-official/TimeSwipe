//lets build and test with adafruit in emulation mode:
#include <iostream>
#include "../Adafruit_NeoPixel/ino_stub.h"

static uint32_t PORT;

void pinMode(int pin, int mode)
{
	std::cout<<"setPinMode:"<<pin<<"->"<<mode<<std::endl;
}
void digitalWrite(int pin, int how)
{
	std::cout<<"setPin:"<<pin<<"->"<<how<<std::endl;
}
void noInterrupts(void)
{
	std::cout<<"IRQdisabled"<<std::endl;
}
void interrupts(void)
{
	std::cout<<"IRQenabled"<<std::endl;
}
uint8_t pgm_read_byte(const uint8_t *pByte){return *pByte;}


//now global funct:
uint32_t get_pin_mask(int pin)
{
	return (1L<<pin);
}
uint32_t *get_OUTSET_addr(int pin)
{
	std::cout<<"Adafruit runs..."<<std::endl;
	return &PORT;
}
uint32_t *get_OUTCLR_addr(int pin)
{
	return &PORT;
}
