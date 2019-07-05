//DAC abstraction:

#pragma once

#include "ADchan.h"
class CDac : public CADchan{
	
	//virtual driver funct:
	virtual void DriverSetVal(float val, int out_bin)=0; //???

public:
	//methodes:
	void SetVal(float val);
        void SetRawOutput(int val);
      //  virtual ~CDac(){}  //just to keep polymorphic behaviour, should be never called
};
