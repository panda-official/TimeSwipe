/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2021 Panda Team
*/

#include "../../src/common/button.h"
#include "../../src/common/cmd.h"
#include "../../src/common/os.h"
#include "../../src/common/std_port.h"
#include "../../src/common/timer.h"
#include "../../src/firmware/base/SPIcomm.h"
#include "../../src/firmware/base/SAMbutton.h"
#include "../../src/firmware/json/jsondisp.h"
#include "../../src/firmware/json/json_evsys.h"
#include "../../src/firmware/led/nodeLED.h"
#include "../../src/firmware/sam/SamService.h"

class CButtonLogic final : public CTimerEvent
                         , public CButtonEvent
                         , public IJSONEvent
                         , public CJSONEvCP {
public:
  static constexpr typeLEDcol MAIN_COLOR = LEDrgb(0x32, 0x97, 0xF7);
  static constexpr typeLEDcol RECORDING_COLOR = LEDrgb(0xFF, 0x40, 0x81);

  CButtonLogic()
  {
    nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, MAIN_COLOR);
  }

  void OnButtonState(const typeButtonState nState) override
  {
    last_button_state_ = nState;
    if (typeButtonState::released == nState) {
      is_recording_ = !is_recording_;
      nodeLED::setMultipleLED(typeLED::LED1, typeLED::LED4, is_recording_ ? RECORDING_COLOR:MAIN_COLOR);
    }
  }

  void OnTimer(int) override
  {}

  void on_event(const char*, nlohmann::json&) override
  {}

protected:
  typeButtonState last_button_state_{typeButtonState::released};
  bool is_recording_{};
};


/*!
 * \brief Setups the CPU main clock frequency to 120MHz
 * \return 0=frequency tuning was successful
 */
int sys_clock_init(void);



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
    //step 0: clock init:
    sys_clock_init(); //->120MHz

    nodeLED::init();
    auto pLED1      =std::make_shared<CLED>(typeLED::LED1);
    auto pLED2      =std::make_shared<CLED>(typeLED::LED2);
    auto pLED3      =std::make_shared<CLED>(typeLED::LED3);
    auto pLED4      =std::make_shared<CLED>(typeLED::LED4);

    //communication bus:
    auto pSPIsc2    =std::make_shared<CSPIcomm>(typeSamSercoms::Sercom2, CSamPORT::pxy::PA12, CSamPORT::pxy::PA15, CSamPORT::pxy::PA13, CSamPORT::pxy::PA14);
    pSPIsc2->EnableIRQs(true);
    auto pDisp=         std::make_shared<CCmdDispatcher>();
    auto pStdPort=      std::make_shared<CStdPort>(pDisp, pSPIsc2);
    pSPIsc2->AdviseSink(pStdPort);

    //example command:
    pDisp->Add("ARMID", std::make_shared< CCmdSGHandlerF<std::string> >(&CSamService::GetSerialString) );

    //----------------menu+button----------------
    auto pMenu=std::make_shared<CButtonLogic>();
    SAMButton::Instance().AdviseSink(pMenu);

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
        SAMButton::Instance().update();

        pSPIsc2->Update();
      }
}
