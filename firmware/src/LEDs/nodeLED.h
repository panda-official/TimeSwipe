/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CLED and nodeLED
*/


#pragma once
#include <memory>
#include <list>

/*!
 * \brief A type for LED color
 */
typedef unsigned int typeLEDcol;

/*!
 * \brief Calculates a 32bit RGB value at compile time
 * \param r The value of red (0-255)
 * \param g The value of green (0-255)
 * \param b The value of blue (0-255)
 * \return 32bit composition of rgb values
 */
inline constexpr unsigned int LEDrgb(unsigned int r, unsigned int g, unsigned int b){return ((r<<16)|(g<<8)|b); }


/*!
 * \brief An enumeration of possible LEDs (started from 1 for compatibility)
 */
enum class typeLED:int {LED1=1, LED2, LED3, LED4}; //start from 1 for comp
typedef typeLED typeLEDind;


class nodeLED;

/*!
 * \brief A class providing an interface for controlling a single LED
 */
class CLED
{
friend class nodeLED;
protected:

    /*!
     * \brief The index of this LED (used as ID and for obtaining a LED index for 3rd party library (Adafruit) )
     */
    typeLED m_nLED;

    /*!
     * \brief The LED on/off state
     */
    bool m_bON=false;

    /*!
     * \brief The LED blink mode. When the mode=true LED is blinking when on.
     */
    bool m_bBlinking=false;

    /*!
     * \brief The LED blinking period, milliseconds
     */
    unsigned long  m_BlinkPeriod_mS=400;

    /*!
     * \brief Setpoint color of the LED
     */
    typeLEDcol m_Clr=0;

    /*!
     * \brief A time stamp when object state has been updated last time
     */
    unsigned int m_LastTimeUpd;

    /*!
     * \brief A blink phase: false=a half-period when LED is off, true=a half period when LED is on
     */
    bool         m_bPhase=false;

    /*!
     * \brief A counter for blinking periods. Used for mode of limited blink times:
     *  when counter is exceeded m_BlinkingPeriod Limit the LED will be switched off automatically
     */
    int m_CurBlinkingPeriod=0;

    /*!
     * \brief Holds a number of blink times (limit)
     */
    int m_BlinkingPeriodLimit=0;

public:

    /*!
     * \brief Obtains a LED zero-based index to be used in 3rd-party library (Adafruit)
     * \return A zero based index for Adafruit
     */
    inline int get_zerob_ind(){ return (4 - static_cast<int>(m_nLED)); }

//public:

    /*!
     * \brief The class constructor
     * \param nLED A LED ID
     * \details The LED object will be added into the nodeLED list
     */
    CLED(typeLED nLED);

    /*!
     * \brief The class destructor
     * \details The LED object will be removed from the nodeLED list
     *
     */
    ~CLED();

    /*!
     * \brief Sets blinking mode with limited number of blink times
     * \param nPeriods blink times (periods)
     */
    void Blink(int nPeriods);

    /*!
     * \brief Turns LED on/off. If blinking mode is set, blinking times are unlimited
     * \param how true=turns LED on, turns LED off
     */
    void ON(bool how);

    /*!
     * \brief Sets the color of the LED
     * \param Clr A color to set
     */
    void SetColor(typeLEDcol Clr);

    /*!
     * \brief Sets blinking mode: on or off
     * \param how true=blink when on, false=steady behaviour
     */
    void SetBlinkMode(bool how)
    {
        m_bBlinking=how;
    }
    void SetBlinkPeriodAndCount(unsigned long  BlinkPeriod_mS, unsigned long BlinkCount=0)
    {
        m_BlinkPeriod_mS=BlinkPeriod_mS;
        m_BlinkingPeriodLimit=BlinkCount;
    }

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    void Update();
};


/*!
 * \brief A container for collection of LED class objects
 * \details Provides a group control over a collection of LED class objects
 */
class nodeLED{
friend class CLED;
protected:

    /*!
     * \brief A list of LED class object. A LED object is added automatically to the list when its constructor is called.
     * And removed from the list by its destructor
     */
    static std::list<CLED *> m_LEDs;

    /*!
     * \brief The flag is set when any of the LEDs is changed(on/off/color)->an update function will be called in Adafruit
     * \details the flag is used to call Adafruit update function only once per nodeLED::Update() more than one LED can be changed
     * before update. Thus function will not be called on every LED change, but will apply multiple changes at once
     */
    static bool              m_bLEDisChanged;

public:
    enum{
        maxLEDs=4
    };


    /*!
      * \brief Generates a random color
      * \return A random color value
      */
     static typeLEDcol gen_rnd_col();

     /*!
      * \brief Blinks all LEDs in collection with a random color
      * \param nBlink Anumber of blinks
      */
     static void random(int nBlink);

     /*!
     * \brief Reset and switch off all LEDsin collection
     */
	static void resetALL();

    /*!
     * \brief Draws a "selection" GUI element from range (usually "selected" LED is marked with more bright color than others)
     * \param sel A selected LED
     * \param sel_color A selection color
     * \param range_begin LED ID range begin
     * \param range_end   LED ID range end
     * \param back_color  A background color
     */
	static void selectLED(typeLEDind sel, typeLEDcol sel_color, typeLEDind range_begin, typeLEDind range_end, typeLEDcol back_color);

    /*!
     * \brief Sets multiple LEDs from the range with a given color
     * \param range_begin LED ID range begin
     * \param range_end  LED ID range end
     * \param back_color A color to set
     */
	static void setMultipleLED(typeLEDind range_begin, typeLEDind range_end, typeLEDcol back_color);

    /*!
     * \brief Blinks a single LED
     * \param sel LED ID
     * \param blink_color A color to blink
     */
	static void blinkLED(typeLEDind sel, typeLEDcol blink_color);

    /*!
     * \brief Blinks multiple LEDs from the range
     * \param firstLED LED ID range begin
     * \param lastLED LED ID range end
     * \param color  A color to blink
     * \param replication Number of blinks
     * \param duration Blink period
     */
	static void blinkMultipleLED(typeLEDind firstLED,  typeLEDind lastLED, typeLEDcol color, int replication, int duration=500);
	
    /*!
     * \brief Inits 3rd-party library (Adafruit)
     * \details The library has to be initialized after corresponding CPU clock frequency is established (120MHz is required)
     */
	static void init(void);

    /*!
     * \brief The object state update method
     * \details Gets the CPU time to update internal state of the object.
     *  Must be called from a "super loop" or from corresponding thread
     */
    static void Update();
	
};
