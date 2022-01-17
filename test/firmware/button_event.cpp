/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2021 Panda Team
*/

#include "../../src/firmware/button.hpp"
#include "../../src/firmware/os.h"
#include "../../src/firmware/settings.hpp"
#include "../../src/firmware/timer.h"
#include "../../src/firmware/base/SPIcomm.h"
#include "../../src/firmware/json/jsondisp.h"
#include "../../src/firmware/json/json_evsys.h"
#include "../../src/firmware/led/nodeLED.h"
#include "../../src/firmware/sam/button.hpp"
#include "../../src/firmware/sam/SamService.h"
#include "../../src/firmware/sam/system_clock.hpp"

class CButtonLogic final : public CTimerEvent
                         , public Button_event
                         , public IJSONEvent
                         , public CJSONEvCP {
public:
  static constexpr typeLEDcol MAIN_COLOR = LEDrgb(0x32, 0x97, 0xF7);
  static constexpr typeLEDcol RECORDING_COLOR = LEDrgb(0xFF, 0x40, 0x81);

  CButtonLogic()
  {
    nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, MAIN_COLOR);
  }

  void handle_state(const Button_state state) override
  {
    last_button_state_ = state;
    if (state == Button_state::released) {
      is_recording_ = !is_recording_;
      nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, is_recording_ ? RECORDING_COLOR:MAIN_COLOR);
    }
  }

  void OnTimer(int) override
  {}

  void on_event(const char*, rapidjson::Value&) override
  {}

protected:
  Button_state last_button_state_{Button_state::released};
  bool is_recording_{};
};

/*!
*  \brief The current firmware assemblage point
*
*  \details Here is all neccesary firmware objects and modules are created at run-time
*  and corresponding bindings and links are established between them
*
*  \todo Add or remove desired objects to change the firmware behavior,
*  or add/remove desired functionality
*
*/

int main(void)
{
    initialize_system_clock();

    nodeLED::init();
    auto pLED1      =std::make_shared<CLED>(typeLED::LED1);
    auto pLED2      =std::make_shared<CLED>(typeLED::LED2);
    auto pLED3      =std::make_shared<CLED>(typeLED::LED3);
    auto pLED4      =std::make_shared<CLED>(typeLED::LED4);

    //communication bus:
    auto pSPIsc2    =std::make_shared<CSPIcomm>(Sam_sercom::Id::sercom2, Sam_pin::Id::pa12, Sam_pin::Id::pa15, Sam_pin::Id::pa13, Sam_pin::Id::pa14);
    pSPIsc2->EnableIRQs(true);
    auto pDisp=         std::make_shared<CCmdDispatcher>();
    auto pStdPort=      std::make_shared<Setting_parser>(pDisp, pSPIsc2);
    pSPIsc2->AdviseSink(pStdPort);

    //example command:
    pDisp->Add("ARMID", std::make_shared<CCmdSGHandler<std::string>>(
        &CSamService::GetSerialString));

    //----------------menu+button----------------
    auto pMenu=std::make_shared<CButtonLogic>();
    Sam_button::instance().AdviseSink(pMenu);

    //------------------JSON---------------------
    auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
    pDisp->Add("js", pJC);

    //------------------EVENTS-------------------
    auto pJE=std::make_shared<CJSONEvDispatcher>(pDisp);
    pDisp->Add("je", pJE);
    pMenu->AdviseSink(pJE);
    //--------------------------------------------------------------------------------------------------------------


    while(1) //endless loop
      {
        //update LEDs:
        nodeLED::Update();

        //upd button:
        Sam_button::instance().update();

        pSPIsc2->Update();
      }
}
