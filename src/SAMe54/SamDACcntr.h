/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//SAM DAC controller class:

#pragma once

enum class typeSamDAC{Dac0, Dac1};


#include "DAC.h"
#include "SamCLK.h"
class CSamDACcntr : public CDac
{
protected:
    typeSamDAC m_chan;
    static bool       m_bInitialized;

    std::shared_ptr<CSamCLK> m_pCLK;        //driving clock 23.05.2019

    //helpers:
    //void connect_gclk(int nGen); //+++dbg
    void common_init(); //common settings for both dacs

    virtual void DriverSetVal(float val, int out_bin);

public:
    //ctor:
    CSamDACcntr(typeSamDAC nChan, float RangeMin, float RangeMax);
};
