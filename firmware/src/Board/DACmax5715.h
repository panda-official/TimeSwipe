/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//DAC max 5715
#pragma once

enum class typeDac5715chan : int
{ DACA=0, DACB, DACC, DACD};

#include "SPI.h"
#include "DAC.h"
class CDac5715sa : public CDac //dac chan 5715 stand-alone version
{
protected:
	CSPI *m_pBus;
	typeDac5715chan m_chan;

	virtual void DriverSetVal(float val, int out_bin);
	
public:
	//ctor:
	CDac5715sa(CSPI *pBus, typeDac5715chan nChan, float RangeMin, float RangeMax);
     //   virtual ~CDac5715sa(){} //just to keep polymorphic behaviour, should be never called
};
