/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CView
*/


#pragma once

#include <vector>
#include "board_type.h"
#include "rgbacol.h"
#include "nodeLED.h"
#include "os.h"
#include "SAMbutton.h"

/*!
 * \brief The application View class
 * \details The class can be considered as somewhat usually called "view" in MCV pattern.
 * It should determine the overall behaviour of board visualization elements.
 * The actual view instance is accesible via Instance() method.
 *
 * \todo This is a prototype of a view class. All colour constants and visualization routines should be moved here.
 * Currently designed as "singleton", but the possibility of changing views can be added.
 * The CView::Instance() that returns current view must remain.
 *
 */
class CView;
typedef void (CView::*pfn_ViewProc)();

class CViewChannel
{
friend class CView;
protected:
    CLED m_LED;
    typeLEDcol m_LastBackgroundCol;

    enum vismode{

        background,
        UI
    };
    vismode m_VisMode;
    void SelectVisMode(vismode nMode);

public:
    CViewChannel(typeLED nLED) : m_LED(nLED)
    {
        m_LastBackgroundCol=0;
        m_VisMode=background;
    }

    //Background API:
    void SetSensorIntensity(float normI); //visualization

    //UI API:
    void SetZeroSearchingMark();
    void SetZeroFoundMark();
    void SetZeroSearchErrorMark();
};

class CView
{
friend class CViewChannel;

public:
    enum menu
    {
       Gains=0,
       Bridge,
       Offsets,
       SetSecondary,

       total=4
    };


protected:
    //! board basic colours in a compressed form:
    static const constexpr typeLEDcol DMS_COLOR = LEDrgb(24, 250, 208);
    static const constexpr typeLEDcol IEPE_COLOR = LEDrgb(73, 199, 255);

    static const constexpr typeLEDcol MARKER_COLOR = LEDrgb(255, 10, 10);
    static const constexpr typeLEDcol RESET_COLOR = LEDrgb(255, 255, 255);
    static const constexpr typeLEDcol ERROR_COLOR = LEDrgb(255, 0, 0);
    static const constexpr typeLEDcol MENU_COLORS[menu::total][2]={

        { LEDrgb(10, 0, 0), LEDrgb(255, 0, 0) },
        { LEDrgb(0, 10, 0), LEDrgb(0, 255, 0) },
        { LEDrgb(0, 0, 10), LEDrgb(0, 0, 255) },
        { LEDrgb(10, 10, 0), LEDrgb(250, 250, 0) }
    };

    //! current board basic color
    typeLEDcol m_BasicBoardCol;
    std::vector<CViewChannel> m_Channels;

    unsigned int m_ButtonLEDphase;
    unsigned long m_ButtonLEDphaseBeginTime_mS;

    unsigned long m_WaitBeginTime_mS;
    unsigned long m_SetDelay;
    pfn_ViewProc m_CurStep=nullptr;
    void EndProc(){ m_CurStep=nullptr; }
    void NextStep(pfn_ViewProc pNext){m_CurStep=pNext;}
    pfn_ViewProc m_procDelayEnd=&CView::EndProc;

    void SelectVisMode(CViewChannel::vismode nMode)
    {
        for(auto &ch : m_Channels)
            ch.SelectVisMode(nMode);
    }
    void procDelay()
    {
        if( (os::get_tick_mS()-m_WaitBeginTime_mS)<m_SetDelay  )
            return;
        NextStep(m_procDelayEnd);
    }
    void Delay(unsigned long nDelay_mS, pfn_ViewProc EndProc)
    {
        m_SetDelay=nDelay_mS;
        m_WaitBeginTime_mS=os::get_tick_mS();
        m_procDelayEnd=EndProc;
        NextStep(&CView::procDelay);
    }
    void procResetSettingsEnd()
    {
        SelectMenuPrevew(m_ActSelMenu);
        EndProc();
    }

    unsigned int m_ActSelMenu, m_ActSelElement;
    unsigned int m_nSelRangeMin, m_nSelRangeMax;
    void procApplySettingsEnd()
    {
        //SelectMenu(m_ActSelMenu, m_ActSelElement, m_nSelRangeMin, m_nSelRangeMax);
        SelectMenuPrevew(m_ActSelMenu);
        EndProc();
    }

public:
    enum vischan{

        ch1=0,
        ch2,
        ch3,
        ch4
    };
    CViewChannel & GetChannel(vischan nCh){ return m_Channels[nCh]; }

    void ExitMenu()
    {
        EndProc();
        SelectVisMode(CViewChannel::vismode::background);
    }


    /*!
     * \brief The current view access interface
     * \return The current view
     */
    static CView& Instance()
    {
       static CView singleton;
       return singleton;
    }

    /*!
     * \brief Sets the current board type
     * \param nBrd The board type
     * \todo should be placed to nodeControl or some super-control class
     */
    void SetupBoardType(typeBoard nBrd)
    {
        m_BasicBoardCol=typeBoard::DMSBoard==nBrd ? DMS_COLOR : IEPE_COLOR;
    }

    /*!
     * \brief Returns the current board basic color
     * \return Current board basic color
     */
    CRGBAcol GetBasicColor() const {return m_BasicBoardCol;}


    //API:
    void SetDefaultModeAfter(unsigned long nDelay_mS)
    {
        Delay(nDelay_mS, &CView::ExitMenu);
    }


    /*!
     * \brief The view routine: "Hello" blinking at board startup
     */
    void BlinkAtStart();
    void SetRecordMarker();
    void SelectMenuPrevew(unsigned int nMenu);
    void SelectMenu(unsigned int nMenu, unsigned int nActive, unsigned int nSelMin, unsigned int nSelMax);
    void ApplyMenu();
    void ResetSettings();
    void ZeroSearchCompleted();
    void SetButtonHeartbeat(bool how);

    void Update();

private:
        /*!
         * \brief Private class constructor
         */
        CView(){

            SetupBoardType(typeBoard::IEPEBoard);

            nodeLED::init();
            m_Channels.reserve(4);
            m_Channels.emplace_back(typeLED::LED1);
            m_Channels.emplace_back(typeLED::LED2);
            m_Channels.emplace_back(typeLED::LED3);
            m_Channels.emplace_back(typeLED::LED4);
        }

        //! Forbid copy constructor
        CView(const CView&)=delete;

        //! Forbid copying
        CView& operator=(const CView&)=delete;
};
