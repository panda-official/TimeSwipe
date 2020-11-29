/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CADbulkMes
*/

#pragma once

#include "frm_stream.h"

class CADbulkMes
{

protected:
    unsigned int m_nMeasMode=0;
    unsigned int m_nMeasMask=0x0f;
    unsigned int m_nMeasRate=1;

    CFIFO m_DataBuf;

public:
    CADbulkMes();

    //interface:
    void SetMeasMode(unsigned int nMode);
    unsigned int  GetMeasMode(){
        return m_nMeasMode;
    }

    void SetMeasChanMask(unsigned int nMask);
    unsigned int GetMeasChanMask(){
        return m_nMeasMask;
    }

    void SetMeasRateHz(unsigned int nRate);
    unsigned int GetMeasRateHz(){
        return m_nMeasRate;
    }

    void MeasStart(unsigned int nDuration);

    int ReadBuffer(CFrmStream &Stream, unsigned int nStartPos, unsigned int nRead);
};

