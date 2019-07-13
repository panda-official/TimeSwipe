/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//ADC abstraction:

#pragma once
#include "ADchan.h"
//typedef CADchan CAdc;

class CAdc : public CADchan{

public:
        //methodes:
        virtual int DirectMeasure(){return CADchan::GetRawBinVal(); }
//        virtual ~CAdc(){}  //just to keep polymorphic behaviour, should be never called

};
