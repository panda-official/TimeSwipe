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
