#ifndef NIXCONSOLE_H
#define NIXCONSOLE_H

#include "Serial.h"

class CNixConsole : public CSerial
{
public:
    CNixConsole();

    virtual bool send(CFIFO &msg);
    virtual bool receive(CFIFO &msg);
    virtual bool send(typeSChar ch);
    virtual bool receive(typeSChar &ch);
};

#endif // NIXCONSOLE_H
