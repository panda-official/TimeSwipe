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
