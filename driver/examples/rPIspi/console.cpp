/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#include <iostream>
#include "console.h"
#include <string>
#include <sstream>

CNixConsole::CNixConsole()
{
}
bool CNixConsole::send(CFIFO &msg)
{
    std::cout<<msg<<std::endl;
    return true;
}
bool CNixConsole::receive(CFIFO &msg)
{
    msg.reset(); //!! 13.06.2019
    std::getline(std::cin, msg);
    msg<<'\n';
    for(auto ch : msg)
    {
        Fire_on_rec_char(ch);
    }
    return true;
}

bool CNixConsole::receive2 ( CFIFO &msg, char * input )
{
    msg.reset();
    std::istringstream is ( input);
    std::getline(is, msg);
    msg << '\n';
    for ( auto ch : msg )
    {
        Fire_on_rec_char ( ch );
    }
    return true;
}

bool CNixConsole::send(typeSChar ch)
{
    return false;
}
bool CNixConsole::receive(typeSChar &ch)
{
    return false;
}
