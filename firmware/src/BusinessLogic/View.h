/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CView, CViewChannel
*/


#pragma once

#include <vector>
#include "board_type.h"
#include "rgbacol.h"
#include "nodeLED.h"
#include "os.h"
#include "SAMbutton.h"


class CView;
typedef void (CView::*pfn_ViewProc)();

/*!
 * \brief The View class for a single visualization channel
 * \details The single visualization channel is linked with corresponding LED
 */
class CViewChannel
{
friend class CView;
protected:

    /*!
     * \brief Controlled LED
     */
    CLED m_LED;

    /*!
     * \brief The last color value that was set in the default visualization mode
     */
    typeLEDcol m_LastBackgroundCol;

    /*!
     * \brief The visulization modes
     */
    enum vismode{

        background, //!<default mode, data visualization is runnig, menu is not active
        UI          //!<User Interface mode, user menu is active, visualization data is stored in the m_LastBackgroundCol bypassing LED
    };

    /*!
     * \brief The current visualization mode
     */
    vismode m_VisMode;

    /*!
     * \brief Selects the visualization mode
     * \param nMode - the visualization mode to be set
     */
    void SelectVisMode(vismode nMode);

public:
    /*!
     * \brief The class constructor
     * \param nLED - the index of the corresponding LED to link to the channel
     */
    CViewChannel(typeLED nLED) : m_LED(nLED)
    {
        m_LastBackgroundCol=0;
        m_VisMode=background;
    }

    //Background API:
    /*!
     * \brief Sets LED intensity depending on sensor signal in default visualization mode (or store the value in m_LastBackgroundCol in case of UI mode)
     * \param normI - normalized (0-1) intensity value
     */
    void SetSensorIntensity(float normI);

    //UI API:
    /*!
     * \brief Sets "Searching offset" view for the channel
     */
    void SetZeroSearchingMark();

    /*!
     * \brief Sets "Offset found" view for the channel
     */
    void SetZeroFoundMark();

    /*!
     * \brief Sets "Searching offset error" view for the channel
     */
    void SetZeroSearchErrorMark();
};


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
class CView
{
friend class CViewChannel;

public:
    /*!
     * \brief Possible menus
     */
    enum menu
    {
       Gains=0,          //!<Gain setting menu
       Bridge,           //!<Bridge setting menu
       Offsets,          //!<Offest setting menu
       SetSecondary,     //!<Set Secondary setting menu

       total=4           //!<Total number of menus
    };


protected:
    /*!
     * \brief DMS board base color
     */
    static const constexpr typeLEDcol DMS_COLOR = LEDrgb(24, 250, 208);

    /*!
     * \brief IEPE board base color
     */
    static const constexpr typeLEDcol IEPE_COLOR = LEDrgb(73, 199, 255);

    /*!
     * \brief MARKER color - used for "Record" view
     */
    static const constexpr typeLEDcol MARKER_COLOR = LEDrgb(255, 10, 10);

    /*!
     * \brief Reset settings color
     */
    static const constexpr typeLEDcol RESET_COLOR = LEDrgb(255, 255, 255);

    /*!
     * \brief Error color
     */
    static const constexpr typeLEDcol ERROR_COLOR = LEDrgb(255, 0, 0);

    /*!
     * \brief Menu colors
     */
    static const constexpr typeLEDcol MENU_COLORS[menu::total][2]={

        { LEDrgb(10, 0, 0), LEDrgb(255, 0, 0) },
        { LEDrgb(0, 10, 0), LEDrgb(0, 255, 0) },
        { LEDrgb(0, 0, 10), LEDrgb(0, 0, 255) },
        { LEDrgb(10, 10, 0), LEDrgb(250, 250, 0) }
    };

    //! current board basic color
    typeLEDcol m_BasicBoardCol;

    /*!
     * \brief Visualization channels
     */
    std::vector<CViewChannel> m_Channels;

    /*!
     * \brief The phase of blinking of the button LED
     */
    unsigned int m_ButtonLEDphase;

    /*!
     * \brief The start time of the LED blink
     */
    unsigned long m_ButtonLEDphaseBeginTime_mS;


    //----------------view micro-task-----------------------

    /*!
     * \brief The start time of the view's micro-task delay
     */
    unsigned long m_WaitBeginTime_mS;

    /*!
     * \brief The view's micro-task delay setpoint, mSec
     */
    unsigned long m_SetDelay;

    /*!
     * \brief The current view's micro-task step
     */
    pfn_ViewProc m_CurStep=nullptr;

    /*!
     * \brief Finishes  view's micro-task
     */
    void EndProc(){ m_CurStep=nullptr; }

    /*!
     * \brief Sets next step of the view's micro-task
     * \param pNext - a pointer to the micro-task step function
     */
    void NextStep(pfn_ViewProc pNext){m_CurStep=pNext;}

    /*!
     * \brief The task will be called at the end of "Delay" micro-task
     */
    pfn_ViewProc m_procDelayEnd=&CView::EndProc;

    /*!
     * \brief Sets the visualization mode for the view
     * \param nMode - the visualization mode to be set
     */
    void SelectVisMode(CViewChannel::vismode nMode)
    {
        for(auto &ch : m_Channels)
            ch.SelectVisMode(nMode);
    }

    /*!
     * \brief The view's "Delay" micro-task body
     */
    void procDelay()
    {
        if( (os::get_tick_mS()-m_WaitBeginTime_mS)<m_SetDelay  )
            return;
        NextStep(m_procDelayEnd);
    }

    /*!
     * \brief Starts view's "Delay" micro-task
     * \param nDelay_mS - the delay setpoint, mS
     * \param EndProc - the micro-task step to set when "Delay" is finished
     */
    void Delay(unsigned long nDelay_mS, pfn_ViewProc EndProc)
    {
        m_SetDelay=nDelay_mS;
        m_WaitBeginTime_mS=os::get_tick_mS();
        m_procDelayEnd=EndProc;
        NextStep(&CView::procDelay);
    }

    /*!
     * \brief The end-step of "Reset settings" view
     */
    void procResetSettingsEnd()
    {
        SelectMenuPrevew(m_ActSelMenu);
        EndProc();
    }

    /*!
     * \brief The actual selected menu
     */
    unsigned int m_ActSelMenu;

    /*!
     * \brief The actual menu setting
     */
    unsigned int m_ActSelElement;

    /*!
     * \brief The minimum actual setting value
     */
    unsigned int m_nSelRangeMin;

    /*!
     * \brief The maximum actual setting value
     */
    unsigned int m_nSelRangeMax;

    /*!
     * \brief The end-step of "Apply settings" view
     */
    void procApplySettingsEnd()
    {
        SelectMenuPrevew(m_ActSelMenu);
        EndProc();
    }

    //----------------------------------------------------------

public:

    /*!
     * \brief The  indexes of visualization channels
     */
    enum vischan{

        ch1=0,  //!<chanel-1
        ch2,    //!<chanel-2
        ch3,    //!<chanel-3
        ch4     //!<chanel-4
    };

    /*!
     * \brief Returns a reference to the visualization channel by its index
     * \param nCh - the channel index
     * \return a reference to the channel
     */
    CViewChannel & GetChannel(vischan nCh){ return m_Channels[nCh]; }

    /*!
     * \brief Exit menu mode and switches to default visualization mode
     */
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

    /*!
     * \brief Sets default visualization mode after a delay
     * \param nDelay_mS - the delay setpoint, mS
     */
    void SetDefaultModeAfter(unsigned long nDelay_mS)
    {
        Delay(nDelay_mS, &CView::ExitMenu);
    }


    /*!
     * \brief Sets "Hello" view: blinks at startup
     */
    void BlinkAtStart();

    /*!
     * \brief Sets "Record" view
     */
    void SetRecordMarker();

    /*!
     * \brief Sets "Menu preview" view
     * \param nMenu - the selected menu index
     */
    void SelectMenuPrevew(unsigned int nMenu);

    /*!
     * \brief Sets "Inside menu" view
     * \param nMenu - the selected menu index
     * \param nActive - the menus active setting
     * \param nSelMin - the minimum setting value
     * \param nSelMax - the maximum setting value
     */
    void SelectMenu(unsigned int nMenu, unsigned int nActive, unsigned int nSelMin, unsigned int nSelMax);

    /*!
     * \brief Sets "ApplyMenu" view
     */
    void ApplyMenu();

    /*!
     * \brief Sets "ResetSettings" view
     */
    void ResetSettings();

    /*!
     * \brief Sets "Offset search complete" view
     */
    void ZeroSearchCompleted();

    /*!
     * \brief Turns button's LED heartbeat ON/OFF
     * \param how true=ON, false=OFF
     */
    void SetButtonHeartbeat(bool how);

    /*!
     * \brief The object state update method. Drive micro-tasks and buttons LED heartbeat
     */
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
