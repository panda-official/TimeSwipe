/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief Implementation file file for  CDac channel methods CDac::SetVal() and CDac::SetRawOutput()
*
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
