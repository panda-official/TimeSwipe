/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#include <iostream>

using namespace std;
#include <unistd.h>
#include <signal.h>

#include "console.h"
#include "bcmspi.h"
#include "BSC_SLV_SPI.h"
#include "frm_stream.h"
#include "bcm2835.h"

void Wait(unsigned long time_mS);
unsigned long get_tick_mS(void);

static bool bRunning=true;
static void sigexit_handler(int signum) {
   bRunning=0;
   signal(signum, sigexit_handler);
}


int main(int argc, char *argv[])
{
    int nSPI=0;
    bool bMasterMode=true;
    if(argc>1)
    {
        nSPI=atoi(argv[1]);
        if(0!=nSPI && 1!=nSPI)
        {
            std::cout<<"Wrong SPI number: must be 0 or 1!"<<std::endl;
            return 0;
        }
    }
    if(argc>2)
    {
        if(*argv[2]!='s')
        {
            std::cout<<"Unrecognized key: must be s!"<<std::endl;
            return 0;
        }
        bMasterMode=false;
        if(1!=nSPI)
        {
            std::cout<<"Only SPI1 can work in a slave mode!"<<std::endl;
            return 0;
        }
    }

    std::cout<<"+++Event Monitor+++"<<std::endl;


    CBcmSPI     spi(nSPI ? CBcmLIB::iSPI::SPI1  :  CBcmLIB::iSPI::SPI0);
    if(!spi.is_initialzed())
    {
        std::cout<<"Failed to initialize BCM SPI-"<<nSPI<<"Master. Try sudo"<<std::endl;
        return 0;
    }

    //signal(SIGHUP, sigexit_handler); // send me a SIGHUP and I'll exit!
    //signal(SIGQUIT, sigexit_handler);
    signal(SIGINT, sigexit_handler);
    signal(SIGTERM, sigexit_handler);

    CNixConsole cio;
    CFIFO  msg;
    CFIFO answer;
    msg+="je>\n";

    unsigned long BeatHPstartTime=get_tick_mS();
    bool bBeatLev=false;
    int  nBeatPin=RPI_V2_GPIO_P1_18;

    bcm2835_gpio_fsel(nBeatPin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_clr(nBeatPin);
    while(bRunning)
    {
        //--adding a "heartbeat" 18.06.2019-----
        if( (get_tick_mS()-BeatHPstartTime)>300 )
        {
            BeatHPstartTime=get_tick_mS();

            //toggle the pin:
            bBeatLev=!bBeatLev;
            if(bBeatLev)
                 bcm2835_gpio_set(nBeatPin);
            else
                 bcm2835_gpio_clr(nBeatPin);
        }
        //--------------------------------------


        if(bcm2835_gpio_lev(RPI_V2_GPIO_P1_16))
        {
            msg.rewind();
            spi.send(msg);
            if(spi.receive(answer))
            {
                cout<<std::endl<<"->";
                cio.send(answer);
                cout<<std::endl;
            }
        }
    }
    bcm2835_gpio_fsel(nBeatPin, BCM2835_GPIO_FSEL_INPT);
    return 0;
}
