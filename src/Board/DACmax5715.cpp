/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

//DAC max 5715 impl:

#include "DACmax5715.h"

CDac5715sa::CDac5715sa(CSPI *pBus, typeDac5715chan nChan, float RangeMin, float RangeMax)
{
	m_pBus=pBus;
	m_chan=nChan;
        m_IntRange=4095;
	SetRange(RangeMin, RangeMax);
}

//driver function:
void CDac5715sa::DriverSetVal(float val, int out_bin)
{
	//tune SPI:
	m_pBus->set_phpol(false, true);
	m_pBus->set_tprofile_divs(0xff, 0, 0xff);
	m_pBus->set_baud_div(0xff);
	
	//form a message:
	CFIFO cmd;
	cmd<<(0x30+(int)m_chan)<<((out_bin>>4)&0xff)<<((out_bin<<4)&0xff);
	m_pBus->send(cmd);
}
