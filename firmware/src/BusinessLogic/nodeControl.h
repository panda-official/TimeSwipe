/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include <memory>
#include "ADmux.h"
#include "zerocal_man.h"
#include "json_evsys.h"

#include "board_type.h"


//enum class typeBoard: int {DMSBoard=0, IEPEBoard};

//this seems to be a "model" in MVC...
class nodeControl : public std::enable_shared_from_this<nodeControl>, public CJSONEvCP,  public IJSONEvent{
protected:
        static std::shared_ptr<CADmux>  m_pMUX;
        static std::shared_ptr<CCalMan> m_pZeroCal;
        static int gain_out(int val); //impl helper

        virtual void on_event(const char *key, nlohmann::json &val); //17.07.2019 now can rec an event


        //19.06.2019: make a singleton from this
public:
        /*static nodeControl& Instance()
        {
           static nodeControl singleton;
           return singleton;
        }*/

        static nodeControl& Instance()
        {
           static std::shared_ptr<nodeControl> ptr(new nodeControl);
           return *ptr;
        }
private:
        nodeControl() {}                                         // Private constructor
       // ~nodeControl(){}
        nodeControl(const nodeControl&)=delete;                 // Prevent copy-construction
        nodeControl& operator=(const nodeControl&)=delete;      // Prevent assignment
	
public:
        static void SetControlItems(std::shared_ptr<CADmux>  &pMUX, std::shared_ptr<CCalMan> &pZeroCal)
        {
            m_pMUX=pMUX;
            m_pZeroCal=pZeroCal;
        }

        //add data vis: 17.07.2019:
        //added blinking at start-up 02.12.2019:
        static void CreateDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED);
        static void StartDataVis(bool bHow, unsigned long nDelay_mS=0);

        //enum class typeBoard {DMSBoard, IEPEBoard};
        static void Set_board_colour(unsigned int* pCol_act, typeBoard nBoard);
        static void BlinkAtStart(typeBoard boardtype);

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
        inline static bool GetZeroRunSt(){ return m_pZeroCal->IsStarted(); }

        static void Update(); //17.07.2019
};
