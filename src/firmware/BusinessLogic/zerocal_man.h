/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CCalMan
*/

#pragma once

#include <memory>
#include <vector>
#include "ADC.h"
#include "DAC.h"
#include "ADpointSearch.h"
#include "json_evsys.h"
#include "Storage.h"
#include "View.h"

/*!
 * \brief A container for collection of CADpointSearch class objects
 * \details Provides a group control over a collection of CADpointSearch class objects
 */

class CCalMan : public ISerialize //public CJSONEvCP
{
protected:

    /*!
     * \brief A collection of CADpointSearch class objects
     */
    std::vector<CADpointSearch>             m_ChanCal;

    /*!
     * \brief A collection of corresponding LEDs for state indication
     */
    std::vector<CView::vischan> m_VisChan;

    /*!
     * \brief A collection of CADpointSearch algorithm states to detect change of the state and forcing a LED action
     */
    std::vector<typePTsrcState>             m_State;

    /*!
     * \brief A time stamp when object state has been updated last time
     */
    unsigned long m_LastTimeUpd;

    /*!
     * \brief State updation/recalculation period (for CCalMan::Update())
     */
    unsigned long m_UpdSpan=100;

    //! A Finite State Machine (FSM) used to control the object state
    enum    FSM{
        halted,     //!<inactive state, no operation performed

        running,    //!<the search algorithms are running
        delay       //!<a delay before leaving the searching mode that user can see overall searching result by the LEDs
    };
    FSM m_PState=FSM::halted;

public:
    CCalMan()
    {
        m_ChanCal.reserve(4);
        m_VisChan.reserve(4);
        m_State.reserve(4);
    }

    /*!
     * \brief Provides the serialization of the object content
     * \param st A reference to the storage from which the object content is downloading or uploading to
     */
     virtual void Serialize(CStorage &st);


    /*!
     * \brief Creates a new object in the collection and binds corresponding LED indicator
     * \param pADC A signal source to be controlled
     * \param pDAC A control signal
     * \param pLED A LED indicator to bind
     */
    void Add(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CDac> &pDAC, CView::vischan nCh)
    {
        m_ChanCal.emplace_back(pADC, pDAC);
        m_VisChan.emplace_back(nCh);
        m_State.emplace_back(typePTsrcState::idle);
    }

    /*!
     * \brief Is group search process started?
     * \return true=at least one search algorithm in collection is started, false=no any searching is performed
     */
    bool IsStarted(){ return (FSM::halted!=m_PState); }

    /*!
     * \brief Starts all searching algorithms in collection
     */
    void Start(int val=2048);

    /*!
     * \brief Stops all searching and resets internal states of the objects
     */
    void StopReset();

    /*!
     * \brief Calls Update() methodes for all objects in the collection
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};
