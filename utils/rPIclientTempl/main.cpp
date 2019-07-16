/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#include <iostream>

using namespace std;


#include "bcmspi.h"

int main(int argc, char *argv[])
{
    int how=1; //by default (without cmdline args)
    if(argc>1)  //use a command-line arg
    {
        how=atoi(argv[1]);

        //additional check of the input arg can be applied here
        //.....................................................
    }

    //create spi comm obj:
    CBcmSPI     spi;

    //optional: check BCM lib is initialized:
    if(!spi.is_initialzed())
    {
        std::cout<<"Failed to initialize BCM SPI-0 master. Try sudo"<<std::endl;
        return 0;
    }

    //form a message for control EnableADmes for example:
    CFIFO  msg;
    msg+="EnableADmes<";
    msg+=std::to_string(how);
    msg+='\n';
    spi.send(msg);


    //this part is optional:
    CFIFO answer;
    if(spi.receive(answer))
    {
       std::cout<<answer;
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
    std::cout<<std::endl;
    return 0;
}
