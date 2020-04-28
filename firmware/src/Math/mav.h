/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#pragma once

#include <math.h>
#include "ringbuffer.h"

template <typename T>
class CMA
{
protected:
    bool m_bInitialized=false;
    int  m_nPeriod;

    CRingBuffer<T> m_RawData;
    CRingBuffer<T> m_MA;


public:
    int GetCurSize()
    {
        return m_MA.GetCurSize();
    }

    void SetPeriod(int nPeriod)
    {
        m_nPeriod=nPeriod;
        //m_RawData.SetMaxSize(nPeriod<<1); //2x period
        m_RawData.SetMaxSize(nPeriod+5);
        m_MA.SetMaxSize(nPeriod);
        m_RawData.Reset();
        m_MA.Reset();
        m_bInitialized=false;
    }

    T ObtainMA(const T &val)
    {
        T CurMA=0;
        m_RawData.Push(val);
        if(m_bInitialized)
        {
            CurMA=m_MA.GetFromTail(0) + (m_RawData.GetFromTail(0) - m_RawData.GetFromTail(m_nPeriod))/m_nPeriod;
            m_MA.Push(CurMA);
        }
        else
        {
            if(m_RawData.GetCurSize()>=m_nPeriod)
            {
                m_bInitialized=true;
                CurMA=m_RawData.Summ()/m_nPeriod;
                m_MA.Push(CurMA);
            }
        }
        return CurMA;
    }

    T ObtainStdDev(int nPeriod)
    {
        int size=m_MA.GetCurSize();
        if(0==size)
            return 0;
        if(nPeriod>size)
            nPeriod=size;

        if(1==nPeriod)
        {
            return std::abs( m_RawData.GetFromTail(0) - m_MA.GetFromTail(0) );
        }

        T StdDevSumm=0;
        for(int i=0; i<nPeriod; i++)
        {
            T diff=m_RawData.GetFromTail(i) - m_MA.GetFromTail(i);
            StdDevSumm+=(diff*diff);
        }
        return sqrt(StdDevSumm/nPeriod);
    }

};
