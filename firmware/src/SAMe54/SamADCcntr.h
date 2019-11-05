/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM ADC controller class:

#pragma once
#include  <memory>
#include <list>

#include "os.h"
#include "ADC.h"
#include "SamCLK.h"

enum class typeSamADC{Adc0, Adc1};

enum class typeSamADCmuxpos : int {AIN0=0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7, SCALEDCOREVCC=0x18};
enum class typeSamADCmuxneg : int {none=-1, AIN0=0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7};


//SAM's ADC channel:
class CSamADCcntr;
class CSamADCchan : public CAdc
{
friend class CSamADCcntr;
protected:
    std::shared_ptr<CSamADCcntr> m_pCont;

    typeSamADCmuxpos m_posIN;
    typeSamADCmuxneg m_negIN;

    //23.05.2019:
    unsigned long m_MesTStamp=os::get_tick_mS();
    float         m_FilteredRawVal=0;
    int           m_UnfilteredRawVal=0;
    float         m_filter_t_mSec=50.0f;

    unsigned long data_age(){ return (os::get_tick_mS()-m_MesTStamp); }
    void SetRawBinVal(int RawVal);


public:
    //ctor/dtor:
    CSamADCchan(std::shared_ptr<CSamADCcntr> &pCont, typeSamADCmuxpos posIN, typeSamADCmuxneg negIN, float RangeMin, float RangeMax);
    virtual ~CSamADCchan(); //just to keep polymorphic behaviour

    virtual int DirectMeasure()
    {
        return CSamADCchan::DirectMeasure(50, 0.8f);
    }

    int DirectMeasure(int nMesCnt, float alpha); //27.05.2019
};


class CSamADCcntr
{
friend class CSamADCchan;
protected:
	typeSamADC m_nADC;

        //channels array:
        std::list<CSamADCchan *> m_Chans; 
        std::shared_ptr<CSamCLK> m_pCLK;        //driving clock 23.05.2019
	
public:
	CSamADCcntr(typeSamADC nADC);

        void SelectInput(typeSamADCmuxpos nPos, typeSamADCmuxneg nNeg=typeSamADCmuxneg::none);
        short SingleConv();

        //25.04.2019: - updation:
        bool Update();
};
