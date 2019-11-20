/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
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
