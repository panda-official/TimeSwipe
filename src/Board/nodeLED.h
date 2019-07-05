#pragma once
#include <memory>
#include <list>

//typedef int typeLEDind;
typedef unsigned int typeLEDcol;

inline constexpr unsigned int LEDrgb(unsigned int r, unsigned int g, unsigned int b){return ((r<<16)|(g<<8)|b); } //this is hardware indep. format...

enum class typeLED:int {LED1=1, LED2, LED3, LED4}; //start from 1 for comp
typedef typeLED typeLEDind;

//01.05.2019:
class nodeLED;
class CLED
{
friend class nodeLED;
protected:
    //std::shared_ptr<nodeLED> m_pCont; //keep it static???

    typeLED m_nLED;

    bool m_bON=false;
    bool m_bBlinking=false;
    unsigned int  m_BlinkPeriod_mS=400;
    typeLEDcol m_Clr=0;

    unsigned int m_LastTimeUpd;
    bool         m_bPhase=false;

    //21.06.2019: adding blinking times:
    int m_CurBlinkingPeriod=0;
    int m_BlinkingPeriodLimit=0;

    inline uint16_t get_zerob_ind(){ return static_cast<uint16_t>(m_nLED)-1; }

public:
    CLED(typeLED nLED);
    ~CLED();
    void Blink(int nPeriods); //21.06.2019
    void ON(bool how);
    void SetColor(typeLEDcol Clr);
    void SetBlinkMode(bool how)
    {
        m_bBlinking=how;
    }
    void Update();
};


class nodeLED{
friend class CLED;
protected:
    //leds list:
    static std::list<CLED *> m_LEDs; //keep it static???
    static bool              m_bLEDisChanged;

public:
        //21.06.2019:
        static typeLEDcol gen_rnd_col();

        static void random(int nBlink);

	static void resetALL();
	static void selectLED(typeLEDind sel, typeLEDcol sel_color, typeLEDind range_begin, typeLEDind range_end, typeLEDcol back_color);
	static void setMultipleLED(typeLEDind range_begin, typeLEDind range_end, typeLEDcol back_color);
	static void blinkLED(typeLEDind sel, typeLEDcol blink_color);
	static void blinkMultipleLED(typeLEDind firstLED,  typeLEDind lastLED, typeLEDcol color, int replication, int duration=500);
	
	static void init(void);
        static void Update();
	
};
