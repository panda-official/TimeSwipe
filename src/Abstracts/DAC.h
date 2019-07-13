/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

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
