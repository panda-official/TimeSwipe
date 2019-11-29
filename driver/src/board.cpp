#include "board.hpp"
#include <sys/time.h>

// RPI GPIO FUNCTIONS
void pullGPIO(unsigned pin, unsigned high)
{
    GPIO_PULL = high << pin;
}

void initGPIOInput(unsigned pin)
{
    INP_GPIO(pin);
}

void initGPIOOutput(unsigned pin)
{
    INP_GPIO(pin);
    OUT_GPIO(pin);
    pullGPIO(pin, 0);
}

void init(int sensorType)
{
    initGPIOInput(DATA0);
    initGPIOInput(DATA1);
    initGPIOInput(DATA2);
    initGPIOInput(DATA3);
    initGPIOInput(DATA4);
    initGPIOInput(DATA5);
    initGPIOInput(DATA6);
    initGPIOInput(DATA7);

    initGPIOInput(TCO);
    initGPIOInput(PI_OK);
    initGPIOInput(FAIL);
    initGPIOInput(BUTTON);

    // initGPIOOutput(PI_ON);
    initGPIOOutput(CLOCK);
    initGPIOOutput(RESET);

    // Initial Reset
    setGPIOLow(CLOCK);
    setGPIOHigh(RESET);

    // SPI Communication
    // // Select Sensor type
    bInterface.setBridge(sensorType);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // // Start Measurement
    bInterface.setEnableADmes(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    bInterface.setEnableADmes(1);
}

void shutdown()
{
    // Reset Clock
    setGPIOLow(CLOCK);
    // Stop Measurement
    bInterface.setEnableADmes(0);
}

void setGPIOHigh(unsigned pin)
{
    GPIO_SET = 1 << pin;
}

void setGPIOLow(unsigned pin)
{
    GPIO_CLR = 1 << pin;
}

static const uint32_t ALL_32_BITS_ON = 0xFFFFFFFF; // (2^32)-1 - ALL BCM_PINS
void resetAllGPIO()
{
    GPIO_CLR = ALL_32_BITS_ON;
}

void busyWaitMicroseconds(uint64_t us)
{
    struct timeval tnow, tlong, tend;

    gettimeofday (&tnow, NULL) ;
    tlong.tv_sec  = us / 1000000 ;
    tlong.tv_usec = us % 1000000 ;
    timeradd (&tnow, &tlong, &tend) ;

    while (timercmp (&tnow, &tend, <))
      gettimeofday (&tnow, NULL) ;
}

unsigned int readAllGPIO()
{
    return (*(gpio + 13) & ALL_32_BITS_ON); 
}
