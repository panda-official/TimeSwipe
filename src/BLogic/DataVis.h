/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
Copyright (c) 2019 Panda Team
*/


//#ifndef DATAVIS_H
//#define DATAVIS_H

#pragma once

#include <memory>
//#include <vector>
#include "ADC.h"
#include "nodeLED.h"

class CDataVis
{
protected:
    //parameters:
    const float b_brght = 55.0; //should it be const?

    unsigned int meas_max = 2048;
    unsigned int meas_min = 2048;
    unsigned int min_wind = 100;

    unsigned long last_time_vis = 0;

    std::shared_ptr<CAdc> m_pADC; //data source
    std::shared_ptr<CLED> m_pLED; //output led

    //helpers:
    void reset();

public:
    CDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED);

    void Update(); //updation
};

//#endif // DATAVIS_H
