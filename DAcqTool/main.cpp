/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include <iostream>
#include <fstream>
#include "bcmspi.h"
#include "frm_stream.h"

using namespace std;

void Wait(unsigned long time_mS);
int main(int argc, char *argv[])
{
    //parameters: 1 - ADCnum, 2 - filename, 3 - datacount, 4 - delay

    if(argc<4)
    {
        std::cout<<"insuficcent input data!"<<std::endl;
        return 1;
    }
    int nADC=atoi(argv[1]);
    int nCount=atoi(argv[3]);
    int nDelay_mS=0;
    if(argc>4)
    {
        nDelay_mS=atoi(argv[4]);
    }

    CBcmSPI     spi(CBcmLIB::iSPI::SPI0);
    if(!spi.is_initialzed())
    {
        std::cout<<"Failed to initialize BCM SPI-0 master. Try sudo"<<std::endl;
        return 0;
    }

    CFIFO  msg;
    CFIFO answer;
    CFrmStream out(&msg);
    out<<"ADC"<<nADC<<".raw>\n";


    //open create the file:
    std::fstream fs(argv[2], std::fstream::out|std::fstream::trunc);

    fs<<"Fetching data for ADC"<<nADC<<" "<<"Delay="<<nDelay_mS<<"mS"<<std::endl;

    int err_cnt=0;
    for(int i=0; i<nCount;)
    {
        spi.send(msg);
        if(spi.receive(answer))
        {
            err_cnt=0;
            i++;

            CFrmStream in(&answer);
            float val;
            in>>val;
            fs<<val<<std::endl;

        }
        if(++err_cnt>3)
        {
            std::cout<<"Communication error!"<<std::endl;
            break;
        }
        Wait(nDelay_mS);
    }
    fs.close();
    return 0;
}
