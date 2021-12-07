/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

//build for ADCs-DACs:

#include "../error.hpp"
#include "../hat.hpp"
#include "../limits.hpp"
#include "../version.hpp"
#include "cmd.h"
#include "os.h"
#include "std_port.h"
#include "base/SPIcomm.h"
#include "dms_channel.hpp"
#include "pga280.hpp"
#include "shiftreg.hpp"
#include "base/I2CmemHAT.h"
#include "base/I2Cmem8Pin.h"
#include "base/DACmax5715.h"
#include "base/DACPWMht.h"
#include "base/RawBinStorage.h"
#include "base/FanControl.h"
#include "control/NewMenu.h"
#include "control/CalFWbtnHandler.h"
#include "control/View.h"
#include "control/nodeControl.h"
#include "control/zerocal_man.h"
#include "control/SemVer.h"
#include "json/jsondisp.h"
#include "led/nodeLED.h"
#include "sam/button.hpp"
#include "sam/SamSPIbase.h"
#include "sam/SamQSPI.h"
#include "sam/i2c_eeprom_master.hpp"
#include "sam/SamADCcntr.h"
#include "sam/SamDACcntr.h"
#include "sam/SamService.h"
#include "sam/SamNVMCTRL.h"

// -----------------------------------------------------------------------------
// volatile bool stopflag{};
// stopflag = true;
// while (stopflag) os::wait(10);
// -----------------------------------------------------------------------------

namespace ts = panda::timeswipe;
using namespace std::placeholders;


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

int main()
{
  try {
    namespace detail = panda::timeswipe::detail;
    constexpr int nChannels{detail::max_channel_count};
    constexpr std::size_t EEPROMsize{2*1024}; // 2kb for EEPROM data

#ifdef CALIBRATION_STATION
    bool bVisEnabled=false;
#else
    bool bVisEnabled=true;
#endif

#ifdef DMS_BOARD
    typeBoard ThisBoard=typeBoard::DMSBoard;
#else
    typeBoard ThisBoard=typeBoard::IEPEBoard;
#endif

    auto pVersion=std::make_shared<CSemVer>(detail::version_major,
      detail::version_minor, detail::version_patch);

    CSamNVMCTRL::Instance(); //check/setup SmartEEPROM before clock init

    //step 0: clock init:
    sys_clock_init(); //->120MHz

    //----------------creating I2C EEPROM-----------------------
    //creating shared mem buf:
    auto pEEPROM_MemBuf=std::make_shared<CFIFO>();
    pEEPROM_MemBuf->reserve(EEPROMsize); // reserve for EEPROM data

    //creating an I2C EEPROM master to operate with an external chip:
    auto pEEPROM_MasterBus= std::make_shared<Sam_i2c_eeprom_master>();
    pEEPROM_MasterBus->EnableIRQs(true);

    //request data from an external chip:
    pEEPROM_MasterBus->SetDataAddrAndCountLim(0, EEPROMsize);
    pEEPROM_MasterBus->SetDeviceAddr(0xA0);
    pEEPROM_MasterBus->receive(*pEEPROM_MemBuf);

    //create 2 I2C slaves for Read-only EEPROM data from extension plugs and connect them to the bufer:
    auto pEEPROM_HAT=std::make_shared<CSamI2CmemHAT>();
    pEEPROM_HAT->SetMemBuf(pEEPROM_MemBuf);
    pEEPROM_HAT->EnableIRQs(true);

    //set iface:
    auto& nc = nodeControl::Instance();
    nc.SetEEPROMiface(pEEPROM_MasterBus, pEEPROM_MemBuf);
    //----------------------------------------------------------


    //communication bus:
    auto pSPIsc2    =std::make_shared<CSPIcomm>(Sam_sercom::Id::sercom2, Sam_pin::Id::pa12, Sam_pin::Id::pa15, Sam_pin::Id::pa13, Sam_pin::Id::pa14);
    pSPIsc2->EnableIRQs(true);
    auto pDisp=         std::make_shared<CCmdDispatcher>();
    auto pStdPort=      std::make_shared<CStdPort>(pDisp, pSPIsc2);
    pSPIsc2->AdviseSink(pStdPort);


    nc.SetBoardType(ThisBoard);
    std::shared_ptr<Pin> pDAConPin;
    std::shared_ptr<Pin> pUB1onPin;
    std::shared_ptr<Pin> pQSPICS0Pin;
    std::shared_ptr<CDMSsr> pDMSsr;


    //1st step:
    if(typeBoard::DMSBoard==ThisBoard)
      {
        pDMSsr=std::make_shared<CDMSsr>(
          std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p05, true),
          std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p06, true),
          std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p07, true));

        pDAConPin=pDMSsr->FactoryPin(CDMSsr::pins::DAC_On);
        pUB1onPin=pDMSsr->FactoryPin(CDMSsr::pins::UB1_On);

        auto pCS0=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS0); pCS0->set_inverted(true);  pQSPICS0Pin=pCS0; pCS0->write(false);

#ifdef DMS_TEST_MODE
        pDisp->Add("SR", std::make_shared< CCmdSGHandler<CDMSsr, unsigned int> >(pDMSsr, &CDMSsr::GetShiftReg, &CDMSsr::SetShiftReg) );
#endif

      }
    else
      {
        pDAConPin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p04, true);
        pUB1onPin=std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p07, true); //pUB1onPin->set_inverted(true); pUB1onPin->Set(false);
        pQSPICS0Pin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p11, true);

        //old IEPE gain switches:
        auto pGain0=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p15, true);
        auto pGain1=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p14, true);
        nc.SetIEPEboardGainSwitches(pGain0, pGain1);

      }
    auto pEnableMesPin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p13, true);
    auto pFanPin=std::make_shared<Sam_pin>(Sam_pin::Group::a, Sam_pin::Number::p09, true);

    //setup control:
    nc.SetUBRpin(pUB1onPin);
    nc.SetDAConPin(pDAConPin);
    nc.SetEnableMesPin(pEnableMesPin);
    nc.SetFanPin(pFanPin);


    auto pSamADC0   =std::make_shared<CSamADCcntr>(typeSamADC::Adc0);
    std::shared_ptr<CSamADCchan> pADC[]={std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN2, typeSamADCmuxneg::none, 0.0f, 4095.0f),
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN3, typeSamADCmuxneg::none, 0.0f, 4095.0f),
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN6, typeSamADCmuxneg::none, 0.0f, 4095.0f),
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN7, typeSamADCmuxneg::none, 0.0f, 4095.0f) };

    CSamQSPI objQSPI;
    std::shared_ptr<CDac5715sa> pDAC[]={std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACA, 0.0f, 4095.0f),
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACB, 0.0f, 4095.0f),
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACC, 0.0f, 4095.0f),
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACD, 0.0f, 4095.0f) };

    auto pSamDAC0   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac0, 0.0f, 4095.0f);
    auto pSamDAC1   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac1, 0.0f, 4095.0f);
    pSamDAC0->SetRawBinVal(2048);
    pSamDAC1->SetRawBinVal(2048);

    //add ADC/DAC commands:
    for(int i=0; i<nChannels; i++)
      {
        char cmd[64];
        int nInd=i+1;
        std::sprintf(cmd, "ADC%d.raw", nInd);
        pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CAdc, int> >(pADC[i], &CAdc::DirectMeasure) );
        std::sprintf(cmd, "DAC%d.raw", nInd);
        pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CDac, int> >(pDAC[i], &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
      }
    pDisp->Add("AOUT3.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pSamDAC0, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
    pDisp->Add("AOUT4.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pSamDAC1, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
    pDisp->Add("DACsw", std::make_shared< CCmdSGHandler<Pin, bool> >(pDAConPin, &Pin::read_back,  &Pin::write) );



    //2nd step:
    if(typeBoard::DMSBoard==ThisBoard)
      {
        auto pCS1=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS1); pCS1->set_inverted(true);  pCS1->write(false);

        //create PGA280 extension bus:
        auto pInaSpi=std::make_shared<CSamSPIbase>(true, Sam_sercom::Id::sercom5,
          Sam_pin::Id::pb16, Sam_pin::Id::pb19, Sam_pin::Id::pb17, std::nullopt, nullptr);


        auto pInaSpiCSpin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p18, true);
        pInaSpiCSpin->set_inverted(true);
        pInaSpiCSpin->write(false);

        auto pDAC2A=std::make_shared<CDac5715sa>(&objQSPI, pCS1, typeDac5715chan::DACA, 2.5f, 24.0f);
        // #ifdef CALIBRATION_STATION
        // pDAC2A->SetLinearFactors(-0.005786666f, 25.2f);
        // #endif
        pDAC2A->SetVal(0);
        nc.SetVoltageDAC(pDAC2A);

#ifdef CALIBRATION_STATION
        //ability to control VSUP dac raw value:
        pDisp->Add("VSUP.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDAC2A, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
#endif

        //create 4 PGAs:
        CDMSsr::pins IEPEpins[]={CDMSsr::pins::IEPE1_On, CDMSsr::pins::IEPE2_On, CDMSsr::pins::IEPE3_On, CDMSsr::pins::IEPE4_On};
        for(int i=0; i<nChannels; i++)
          {
            auto pPGA_CS=std::make_shared<CPGA_CS>(static_cast<CDMSsr::pga_sel>(i), pDMSsr, pInaSpiCSpin);
            auto pIEPEon=pDMSsr->FactoryPin(IEPEpins[i]);
            auto pPGA280=std::make_shared<CPGA280>(pInaSpi, pPGA_CS);

            nc.AddMesChannel( std::make_shared<CDMSchannel>(i, pADC[i], pDAC[i], static_cast<CView::vischan>(i), pIEPEon, pPGA280, bVisEnabled) );
#ifdef DMS_TEST_MODE

            //add commands to each:
            char cmd[64];
            int nInd=i+1;
            //for testing only:
            std::sprintf(cmd, "PGA%d.rsel", nInd);
            pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CPGA280, unsigned int> >(pPGA280, &CPGA280::GetSelectedReg, &CPGA280::SelectReg) );
            std::sprintf(cmd, "PGA%d.rval", nInd);
            pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CPGA280, int> >(pPGA280, &CPGA280::ReadSelectedReg, &CPGA280::WriteSelectedReg) );
#endif

          }
      }
    else
      {
        for(int i=0; i<nChannels; i++)
          {
            nc.AddMesChannel( std::make_shared<CIEPEchannel>(i, pADC[i], pDAC[i], static_cast<CView::vischan>(i), bVisEnabled) );
          }
      }


    //2 DAC PWMs:
    auto pPWM1=std::make_shared<CDacPWMht>(CDacPWMht::PWM1, pDAConPin);
    auto pPWM2=std::make_shared<CDacPWMht>(CDacPWMht::PWM2, pDAConPin);

    //PWM commands:
    pDisp->Add("PWM1", std::make_shared< CCmdSGHandler<CDacPWMht, bool> >(pPWM1, &CDacPWMht::IsStarted,  &CDacPWMht::Start) );
    pDisp->Add("PWM1.repeats", std::make_shared< CCmdSGHandler<CDacPWMht, unsigned int> >(pPWM1, &CDacPWMht::GetRepeats,  &CDacPWMht::SetRepeats) );
    pDisp->Add("PWM1.duty", std::make_shared< CCmdSGHandler<CDacPWMht, float> >(pPWM1, &CDacPWMht::GetDutyCycle,  &CDacPWMht::SetDutyCycle) );
    pDisp->Add("PWM1.freq", std::make_shared< CCmdSGHandler<CDacPWMht, unsigned int> >(pPWM1, &CDacPWMht::GetFrequency,  &CDacPWMht::SetFrequency) );
    pDisp->Add("PWM1.high", std::make_shared< CCmdSGHandler<CDacPWMht, int> >(pPWM1, &CDacPWMht::GetHighLevel,  &CDacPWMht::SetHighLevel) );
    pDisp->Add("PWM1.low", std::make_shared< CCmdSGHandler<CDacPWMht, int> >(pPWM1, &CDacPWMht::GetLowLevel,  &CDacPWMht::SetLowLevel) );


    pDisp->Add("PWM2", std::make_shared< CCmdSGHandler<CDacPWMht, bool> >(pPWM2, &CDacPWMht::IsStarted,  &CDacPWMht::Start) );
    pDisp->Add("PWM2.repeats", std::make_shared< CCmdSGHandler<CDacPWMht, unsigned int> >(pPWM2, &CDacPWMht::GetRepeats,  &CDacPWMht::SetRepeats) );
    pDisp->Add("PWM2.duty", std::make_shared< CCmdSGHandler<CDacPWMht, float> >(pPWM2, &CDacPWMht::GetDutyCycle,  &CDacPWMht::SetDutyCycle) );
    pDisp->Add("PWM2.freq", std::make_shared< CCmdSGHandler<CDacPWMht, unsigned int> >(pPWM2, &CDacPWMht::GetFrequency,  &CDacPWMht::SetFrequency) );
    pDisp->Add("PWM2.high", std::make_shared< CCmdSGHandler<CDacPWMht, int> >(pPWM2, &CDacPWMht::GetHighLevel,  &CDacPWMht::SetHighLevel) );
    pDisp->Add("PWM2.low", std::make_shared< CCmdSGHandler<CDacPWMht, int> >(pPWM2, &CDacPWMht::GetLowLevel,  &CDacPWMht::SetLowLevel) );


    //temp sensor+ PIN PWM:
    auto pTempSens=std::make_shared<CSamTempSensor>(pSamADC0);
    auto pFanPWM=std::make_shared<CPinPWM>(Sam_pin::Group::a, Sam_pin::Number::p09);
    auto pFanControl=std::make_shared<CFanControl>(pTempSens, pFanPWM);

    //temp sens+fan control:
    pDisp->Add("Temp", std::make_shared< CCmdSGHandler<CSamTempSensor, float> >(pTempSens,  &CSamTempSensor::GetTempCD) );
    pDisp->Add("Fan.duty", std::make_shared< CCmdSGHandler<CPinPWM, float> >(pFanPWM, &CPinPWM::GetDutyCycle) );
    pDisp->Add("Fan.freq", std::make_shared< CCmdSGHandler<CPinPWM, unsigned int> >(pFanPWM, &CPinPWM::GetFrequency, &CPinPWM::SetFrequency) );
    pDisp->Add("Fan", std::make_shared< CCmdSGHandler<CFanControl, bool> >(pFanControl, &CFanControl::GetEnabled, &CFanControl::SetEnabled) );

    //button:
#ifdef CALIBRATION_STATION
    auto pBtnHandler=std::make_shared<CCalFWbtnHandler>();
#else
    auto pBtnHandler=std::make_shared<CNewMenu>();
#endif

    auto& button = Sam_button::instance();
    button.set_extra_handler(pBtnHandler);

    //---------------------------------------------------command system------------------------------------------------------
    //channel commands:
    for (int i{}; i < nChannels; ++i) {
      char cmd[64];
      int nInd=i+1;
      auto pCH=nc.GetMesChannel(i);

      std::sprintf(cmd, "CH%d.mode", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<Channel, unsigned int>>(pCH, &Channel::CmGetMesMode, &Channel::CmSetMesMode));
      std::sprintf(cmd, "CH%d.gain", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<Channel, float>>(pCH, &Channel::amplification_gain, &Channel::set_amplification_gain));
      std::sprintf(cmd, "CH%d.iepe", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<Channel, bool>>(pCH, &Channel::is_iepe, &Channel::set_iepe));

#ifdef CALIBRATION_STATION
      std::sprintf(cmd, "CH%d.clr", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<Channel, typeLEDcol>>(pCH, &Channel::color, &Channel::set_color));
#endif
    }



    pDisp->Add("Offset.errtol", std::make_shared< CCmdSGHandlerF<int> >(&CADpointSearch::GetTargErrTol,  &CADpointSearch::SetTargErrTol) );
    pDisp->Add("ARMID", std::make_shared< CCmdSGHandlerF<std::string> >(&CSamService::GetSerialString) );
    pDisp->Add("fwVersion", std::make_shared< CCmdSGHandler<CSemVer, std::string> >(pVersion, &CSemVer::GetVersionString) );

    //control commands:
    const std::shared_ptr<nodeControl> &pNC=nc.shared_from_this();
    pDisp->Add("Gain", std::make_shared< CCmdSGHandler<nodeControl, int> >(pNC, &nodeControl::GetGain, &nodeControl::SetGain) );
    pDisp->Add("Bridge", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC, &nodeControl::GetBridge,  &nodeControl::SetBridge) );
    pDisp->Add("Record", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC, &nodeControl::IsRecordStarted,  &nodeControl::StartRecord) );
    pDisp->Add("Offset", std::make_shared< CCmdSGHandler<nodeControl, int> >(pNC, &nodeControl::GetOffsetRunSt,  &nodeControl::SetOffset) );
    pDisp->Add("EnableADmes", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC, &nodeControl::IsMeasurementsEnabled,  &nodeControl::EnableMeasurements) );
    pDisp->Add("Mode", std::make_shared< CCmdSGHandler<nodeControl, int> >(pNC, &nodeControl::GetMode,  &nodeControl::SetMode) );
    pDisp->Add("CalStatus", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC, &nodeControl::GetCalStatus) );
    pDisp->Add("Voltage", std::make_shared< CCmdSGHandler<nodeControl, float> >(pNC, &nodeControl::GetVoltage, &nodeControl::SetVoltage) );
    pDisp->Add("Current", std::make_shared< CCmdSGHandler<nodeControl, float> >(pNC, &nodeControl::GetCurrent, &nodeControl::SetCurrent) );
    pDisp->Add("MaxCurrent", std::make_shared< CCmdSGHandler<nodeControl, float> >(pNC, &nodeControl::GetMaxCurrent, &nodeControl::SetMaxCurrent) );
    //pDisp->Add("Fan", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC,  &nodeControl::IsFanStarted,  &nodeControl::StartFan) );


    CView &view=CView::Instance();
#ifdef CALIBRATION_STATION
    pDisp->Add("UItest", std::make_shared< CCmdSGHandler<CCalFWbtnHandler, bool> >(pBtnHandler,
        &CCalFWbtnHandler::HasUItestBeenDone,
        &CCalFWbtnHandler::StartUItest) );
    //testing Ext EEPROM:
    pDisp->Add("EEPROMTest", std::make_shared< CCmdSGHandler<Sam_i2c_eeprom_master, bool> >(pEEPROM_MasterBus,
        &Sam_i2c_eeprom_master::GetSelfTestResult,  &Sam_i2c_eeprom_master::RunSelfTest) );

    pDisp->Add("CalEnable", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC,  &nodeControl::IsCalEnabled,  &nodeControl::EnableCal) );

#endif


    //--------------------JSON- ---------------------
    auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
    pDisp->Add("js", pJC);
    pJC->AddSubHandler("cAtom", std::bind(&nodeControl::procCAtom, std::ref(*pNC), _1, _2, _3 ) );

    //#ifdef CALIBRATION_STATION

    //#endif


    //------------------JSON EVENTS-------------------
    auto pJE=std::make_shared<CJSONEvDispatcher>(pDisp);
    pDisp->Add("je", pJE);
    button.CJSONEvCP::AdviseSink(pJE);
    nodeControl::Instance().AdviseSink(pJE);
    //--------------------------------------------------------------------------------------------------------------


    /*
     * nodeControl::LoadSettings() activates the persistent
     * storage handling which is currently broken!
     */
    // nc.LoadSettings();
    nc.SetMode(0); //set default mode
#ifndef CALIBRATION_STATION
    view.BlinkAtStart();
#endif

    // Enable calibration settings.
    nc.EnableCal(true);

    // Loop endlessly.
    while (true) {
      button.update();
      nc.Update();
      view.Update();

      pSPIsc2->Update();
      pSamADC0->Update();
      pFanControl->Update();
    }
  } catch (const std::exception& e) {
  } catch (...) {
  }

  // main() must never return.
  while (true) os::wait(10);
}
