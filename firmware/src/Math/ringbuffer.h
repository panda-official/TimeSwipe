/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#pragma once

#include <vector>

template <typename T>
class CRingBuffer
{
protected:
    std::vector<T> m_Buffer;

    int            m_CurSize=0;
    int            m_MaxSize=0;
    int            m_TailIndex=0;

public:
    void Push(const T &val)
    {
        m_Buffer[m_TailIndex++]=val;
        if(m_CurSize<m_MaxSize)
            m_CurSize++;
        if(m_TailIndex>m_MaxSize)
            m_TailIndex=0;

    }

    const T& GetFromTail(int nInd) const
    {
        int offs=m_TailIndex-nInd-1;
        if(offs<0)
            offs+=m_MaxSize;

        return m_Buffer[offs];
    }

    T Summ() const
    {
        T rv=0;
        for(int i=0; i<m_CurSize; i++)
        {
            rv+=m_Buffer[i];
        }
        return rv;
    }

    int GetCurSize() const {return m_CurSize;}
    void SetMaxSize(int nSize){ m_Buffer.resize(nSize); m_MaxSize=nSize; }
    void Reset(){ m_CurSize=0; m_TailIndex=0; }
    bool IsFull() const
    {
        return (m_CurSize==m_MaxSize);
    }
};
