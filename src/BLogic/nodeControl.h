/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <memory>
#include "ADmux.h"
#include "zerocal_man.h"
#include "json_evsys.h"

class nodeControl : public CJSONEvCP{
protected:
        static std::shared_ptr<CADmux>  m_pMUX;
        static std::shared_ptr<CCalMan> m_pZeroCal;
        static int gain_out(int val); //impl helper


        //19.06.2019: make a singleton from this
public:
        static nodeControl& Instance()
        {
           static nodeControl singleton;
           return singleton;
        }
private:
        nodeControl(){}                                         // Private constructor
        ~nodeControl(){}
        nodeControl(const nodeControl&)=delete;                 // Prevent copy-construction
        nodeControl& operator=(const nodeControl&)=delete;      // Prevent assignment
	
public:
        static void SetControlItems(std::shared_ptr<CADmux>  &pMUX, std::shared_ptr<CCalMan> &pZeroCal)
        {
            m_pMUX=pMUX;
            m_pZeroCal=pZeroCal;
        }

        static bool IsRecordStarted();
        static void StartRecord(const bool how);

        static int GetGain();
	
	//impl logic here:
        static void SetGain(const int val)
	{ 
		int outp=val;
		if(outp<1)
			outp=1;
		if(outp>4)
			outp=4;
                gain_out(outp);
	}
        static int IncGain(const int step)
	{
		int outp=GetGain()+step;
		if(outp>4)
			outp=1;
			
		return gain_out(outp);
	}

        static bool GetBridge();
        static void SetBridge(bool how);
	
        static void SetZero(bool how);
};
