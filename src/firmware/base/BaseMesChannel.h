/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for CMesChannel class aka CIEPEchannel
*/

#pragma once

#include "../adcdac.hpp"
#include "../control/DataVis.h"
#include "../pin.hpp"

#include <memory>

class nodeControl;

/*!
 * \brief The basic class representing board measurement channel functionality
 * \details Defines the basic interface of the board measurement channel.
 *  Must be overriden in the concrete implementation of the measurement channel for IEPE and DMS boards
 */
class CMesChannel
{
friend class nodeControl;
public:

    /*!
     * \brief The possible measurement modes
     */
    enum mes_mode{

        Voltage=0,  //!<Voltage mode
        Current     //!<Current mode
    };


    /*!
     * \brief Returns a raw binary value measured by the channel's ADC
     * \return ADC raw binary value
     */
    inline int GetADCmesRawVal(){

        return m_pADC->GetRawBinVal();
    }

    /*!
     * \brief Turns IEPEmode on/off
     * \param bHow - true=IEPE mode ON, false=IEPE mode off
     */
    virtual void IEPEon(bool bHow){

        m_bIEPEon=bHow;
    }

    /*!
     * \brief Checks if IEPE measurement mode is on or not
     * \return true=IEPE mode is ON, false IEPEmode is OFF
     */
    bool IsIEPEon() const noexcept
    {
        return  m_bIEPEon;
    }

    /*!
     * \brief Sets the measurement mode (Voltage or Current)
     * \param nMode - the measuremnt mode to be set
     */
    virtual void SetMesMode(mes_mode nMode){

        m_MesMode=nMode;
    }

    /*!
     * \brief Sets the channel amplification gain
     * \param GainValue - the Gain value to be set
     */
    virtual void SetAmpGain(float GainValue){

        m_ActualAmpGain=GainValue;
    }

    /*!
     * \brief Actualise channel offset values
     */
    virtual void UpdateOffsets(){}


    /*!
     * \brief Returns current amplification gain of the channel
     * \return Current amplification gain of the channel
     */
    float GetActualAmpGain() const noexcept
    {
        return m_ActualAmpGain;
    }

    /*!
     * \brief Returns current measurement mode. This is a wrapper to be used with a command processor
     * \return Integer value of the current measurement mode
     */
    unsigned int CmGetMesMode() const noexcept
    {
        return static_cast<int>(m_MesMode);
    }

    /*!
     * \brief Sets current measurement mode. This is a wrapper to be used with a command processor
     * \param nMode - the measuremnt mode to be set
     */
    inline void CmSetMesMode(unsigned int nMode){

        if(nMode>mes_mode::Current)
            nMode=mes_mode::Current;

        SetMesMode( static_cast<mes_mode>(nMode) );
    }

protected:
    /*!
     * \brief The zero-based channel index.
     */
    int m_nChanInd;

    /*!
     * \brief The pointer to the control class containing this channel
     */
    nodeControl *m_pCont=nullptr;

    /*!
     * \brief The state of IEPE mode (ON or OFF)
     */
    bool m_bIEPEon=false;

    /*!
     * \brief The current measurement mode Voltage or Current
     */
    mes_mode m_MesMode=mes_mode::Voltage;

    /*!
     * \brief The actual channel amplification gain
     */
    float m_ActualAmpGain=1.0f;

    /*!
     * \brief The pointer to channel's offset control DAC
     */
    std::shared_ptr<CAdc> m_pADC;

    /*!
     * \brief The pointer to the channel's ADC
     */
    std::shared_ptr<CDac> m_pDAC;

    /*!
     * \brief The visualization index of the channel. Used to bind the channel with the visualization LED
     */
    CDataVis m_VisChan;

    /*!
     * \brief The visualisation enable flag
     */
    bool m_bVisEnabled;

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     */
    void Update(){

        if(!m_bVisEnabled)
            return;

        m_VisChan.Update( m_pADC->GetRawBinVal() );
    }

public:
    /*!
     * \brief The class constructor
     * \param nChanInd Zero-based channel index.
     * \param pADC - The pointer to channel's offset control DAC
     * \param pDAC - The pointer to the channel's ADC
     * \param nCh  - The visualization index of the channel
     * \param bVisEnabled - The visualisation enable flag
     */
    CMesChannel(const int nChanInd, const std::shared_ptr<CAdc> &pADC,  const std::shared_ptr<CDac> &pDAC,  CView::vischan nCh, bool bVisEnabled) : m_VisChan(nCh){

        m_nChanInd=nChanInd;
        m_pADC=pADC;
        m_pDAC=pDAC;
        m_bVisEnabled=bVisEnabled;
    }

    /*!
     * \brief Sets the color of the channel's  LED
     * \param Clr A color to set
     */
    void SetColor(typeLEDcol col){

        CView::Instance().GetChannel(m_VisChan.GetVisChannel()).SetColor(col);
    }

    /*!
     * \brief Returns setpoint color of the channel's LED
     * \return setpoint color of the LED
     */
    typeLEDcol GetColor(){

        return CView::Instance().GetChannel(m_VisChan.GetVisChannel()).GetColor();
    }
};

typedef CMesChannel CIEPEchannel;
