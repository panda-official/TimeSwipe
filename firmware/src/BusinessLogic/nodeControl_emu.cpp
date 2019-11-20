/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//implementing emulation of node control:

#include "nodeControl.h"
#ifdef EMU
#include <iostream>
#endif

static bool brecord=false;
static int gain=1;
static bool bridge=1;

bool nodeControl::IsRecordStarted(){ return brecord;}
bool nodeControl::StartRecord(const bool how)
{ 
	brecord=how;
#ifdef EMU
	std::cout<<(how ? "record started" : "record stopped")<<std::endl;
#endif 
	return brecord;
}
int nodeControl::gain_out(int val)
{
	 gain=val;
#ifdef EMU
	std::cout<<"gain="<<gain<<std::endl;
#endif 
	 return gain;
}
int nodeControl::GetGain(){ return gain;}
bool nodeControl::GetBridge(){return bridge;}
bool nodeControl::SetBridge(bool how)
{ 
	bridge=how;
#ifdef EMU
	std::cout<<"bridge="<<bridge<<std::endl;
#endif 
	return bridge;
}
void nodeControl::SetZero(void)
{
#ifdef EMU
	std::cout<<"zero was set"<<std::endl;
#endif 
}
