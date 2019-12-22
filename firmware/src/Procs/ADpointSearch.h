/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CADpointSearch
*/

#pragma once

#include <memory>
#include <vector>
#include "ADC.h"
#include "DAC.h"

/*!
 * \brief The Finite States for search for the required value of the control signal to obtain the output signal of the desired value algorithm
 */
enum class typePTsrcState
{
    idle,           //!<inactive state, no operation performed
    searching,      //!<searching
    found,          //!<the point is found
    error           //!<searching failed
};
/*!
 * \brief The class implements search algorithm for the required value of the control signal to obtain the output signal of the desired value
 * \details The control signal is changed as following: For the most significant byte of a control word, a trial "1" is set.
 * If the measured signal exceeds the desired value "1" is replaced by "0". if not, "1" is kept.
 * Then procedure is repeated for the next bit toward a least significant bit of the control word untill
 *  all bits in the word will be processed.
 * Search successful if the final measured value fits into the range TargetValue+-TargErrTolerance
 *
 */
class CADpointSearch
{
protected:

    /*!
     * \brief Holding the current FSM (Finite State)
     */
    typePTsrcState m_State;

    /*!
     * \brief The number of bits left to process
     */
    int            m_ProcBits;

    /*!
     * \brief A desired value of a measured signal (target to search for)
     */
    int m_TargPoint;

    /*!
     * \brief A value that defines acceptable deviation from the target value (+-)
     */
    static int m_TargErrTolerance;

    /*!
     * \brief A signal source to be controlled
     */
    std::shared_ptr<CAdc> m_pADC;

    /*!
     * \brief A control signal
     */
    std::shared_ptr<CDac> m_pDAC;

public:
    /*!
     * \brief A getter for m_TargErrTolerance
     * \return m_TargErrTolerance value
     */
    static int  GetTargErrTol() {return m_TargErrTolerance; }

    /*!
     * \brief A setter for m_TargErrTolerance
     * \param val A value to be set for m_TargErrTolerance
     */
    static void SetTargErrTol(int val){ if(val<1) val=1; m_TargErrTolerance=val;}

    /*!
     * \brief Returns the  algorithm state
     * \return The algorithm state
     */
    typePTsrcState state(){return m_State;}

    /*!
     * \brief The class constructor
     * \param pADC A signal source to be controlled
     * \param pDAC A control signal
     */
    CADpointSearch(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CDac> &pDAC)
    {
        m_State=typePTsrcState::idle;

        m_pADC=pADC;
        m_pDAC=pDAC;
        m_TargErrTolerance=25;
    }

    /*!
     * \brief Starts searching the control signal level to obtain desired level of the input signal
     * \param val A target input signal level
     * \return The  algorithm state
     */
    typePTsrcState Search(int val)
    {
        m_TargPoint=val;
        m_State=typePTsrcState::searching;
        m_ProcBits=12;
        m_pDAC->SetRawBinVal(0);

        return m_State;
    }

    /*!
     * \brief Stop searching, reset internal state
     */
    void StopReset()
    {
        m_State=typePTsrcState::idle;
    }

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
