/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include <iostream>

using namespace std;

#include "console.h"
#include "bcmspi.h"

void Wait(unsigned long time_mS);
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

    std::cout<<"+++SPI terminal+++"<<std::endl;
    if(bMasterMode){

    CBcmSPI     spi(nSPI ? CBcmLIB::iSPI::SPI1  :  CBcmLIB::iSPI::SPI0);
    if(!spi.is_initialzed())
    {
        std::cout<<"Failed to initialize BCM SPI-"<<nSPI<<"Master. Try sudo"<<std::endl;
        return 0;
    }

    CNixConsole cio;
    CFIFO  msg;
    CFIFO answer;

     std::cout<<"SPI-"<<nSPI<<" Master"<<std::endl<<"type the commands:"<<std::endl<<"->"<<std::endl;
    while(true)
    {
        if(cio.receive(msg))
        {
            spi.send(msg);
            if(spi.receive(answer))
            {
                cio.send(answer);
            }
            else
            {
                switch(spi.m_ComCntr.get_state())
                {
                    case CSyncSerComFSM::errLine:
                        std::cout<<"!Line_err!";
                    break;

                    case CSyncSerComFSM::errTimeout:
                        std::cout<<"!Timeout_err!";
                    break;
                }
            }
            cout<<std::endl<<"->"<<std::endl;
        }
    } }
    else {

        //slave mode:
        std::cout<<"Slave mode is not supported currently..."<<std::endl; 
    }

    return 0;
}
