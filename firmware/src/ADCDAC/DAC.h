/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
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
