/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//ADC-DAC mux:

#pragma once

enum class typeADgain: int {gainX1=1, gainX2, gainX4, gainX8};
enum class typeDACmode : int {ExtDACs=0, SamAndExtDACs};

#include "DAC.h"
class CADmux
{
protected:
        typeADgain m_CurGain;
        typeDACmode m_CurDACmode;
        bool       m_UBRVoltage;
        bool       m_bADmesEnabled;  //18.06.2019
        bool       m_bFanIsStarted; //31.10.2019

public:
	CADmux();
	
	void EnableADmes(bool how);
        bool IsADmesEnabled(){return m_bADmesEnabled; } //18.06.2019
	
	void SetGain(typeADgain gain);
        typeADgain GetGain(){ return m_CurGain; }
        void SetDACmode(typeDACmode mode);
        void SetUBRvoltage(bool how);
        bool GetUBRVoltage(){ return m_UBRVoltage; }

        void StartFan(bool how);
        bool IsFanStarted(){ return m_bFanIsStarted; }

        //converter:
         void setDACsw(int mode){ SetDACmode(mode ? typeDACmode::SamAndExtDACs : typeDACmode::ExtDACs); }
         int getDACsw(){ return static_cast<int>(m_CurDACmode); }
};

