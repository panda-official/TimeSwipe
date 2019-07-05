#include <iostream>
#include "console.h"

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

bool CNixConsole::send(typeSChar ch)
{
    return false;
}
bool CNixConsole::receive(typeSChar &ch)
{
    return false;
}
