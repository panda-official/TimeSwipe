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
