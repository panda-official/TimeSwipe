/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#include "DAC.h"


void CDac::SetVal(float val)
{
        SetRealVal(val);
	
	//call driver function:
        DriverSetVal(m_RealVal, m_RawBinaryVal);
}
void CDac::SetRawOutput(int val)
{
        SetRawBinVal(val);

        //call driver function:
        DriverSetVal(m_RealVal, m_RawBinaryVal);
}
