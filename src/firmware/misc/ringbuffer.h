/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CRingBuffer
*/

#pragma once

#include <vector>

template <typename T>
/*!
 * \brief The simple ring buffer
 */
class CRingBuffer
{
protected:

    /*!
     * \brief The buffer to store data
     */
    std::vector<T> m_Buffer;

    /*!
     * \brief The current buffer size
     */
    int            m_CurSize=0;

    /*!
     * \brief The maximum possible buffer size
     */
    int            m_MaxSize=0;

    /*!
     * \brief The index of the last pushed data (tail)
     */
    int            m_TailIndex=0;

public:

    /*!
     * \brief Push new data to the ring buffer
     * \param val - a value tu push
     */
    void Push(const T &val)
    {
        m_Buffer[m_TailIndex++]=val;
        if(m_CurSize<m_MaxSize)
            m_CurSize++;
        if(m_TailIndex>m_MaxSize)
            m_TailIndex=0;

    }

    /*!
     * \brief Retrieves data from the buffer indexed from the tail (from the last push)
     * \param nInd - offset from the tail toward the head
     * \return
     */
    const T& GetFromTail(int nInd) const
    {
        int offs=m_TailIndex-nInd-1;
        if(offs<0)
            offs+=m_MaxSize;

        return m_Buffer[offs];
    }

    /*!
     * \brief Summing all elements in the buffer
     * \return the sum of buffer elements
     */
    T Summ() const
    {
        T rv=0;
        for(int i=0; i<m_CurSize; i++)
        {
            rv+=m_Buffer[i];
        }
        return rv;
    }

    /*!
     * \brief Retrieves the current size of the buffer
     * \return the current size of the buffer
     */
    int GetCurSize() const {return m_CurSize;}

    /*!
     * \brief Sets the maximum size(capacity) of the ring buffer
     * \param nSize - the size to set
     */
    void SetMaxSize(int nSize){ m_Buffer.resize(nSize); m_MaxSize=nSize; }

    /*!
     * \brief Clears the buffer
     */
    void Reset(){ m_CurSize=0; m_TailIndex=0; }

    /*!
     * \brief Is buffer full? Means current buffer size is equal to the buffer capacity and data is routed by a ring
     * \return true=buffer is full, false=buffer is not full
     */
    bool IsFull() const
    {
        return (m_CurSize==m_MaxSize);
    }
};
