/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#ifndef NIXCONSOLE_H
#define NIXCONSOLE_H

#include "Serial.h"
#include <string>
#include <fstream>

class CNixConsole : public CSerial
{
public:
    CNixConsole();

    virtual bool send(CFIFO &msg);
    virtual bool receive(CFIFO &msg);
    virtual bool receive2(CFIFO &msg, char * input);
    virtual bool send(typeSChar ch);
    virtual bool receive(typeSChar &ch);
};

#endif // NIXCONSOLE_H
