// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
#include "sam/system_clock.hpp"
#include "sam/SamSPIbase.h"
#include "sam/SamQSPI.h"
#include "sam/i2c_eeprom_master.hpp"
#include "sam/SamADCcntr.h"
#include "sam/SamDACcntr.h"
#include "sam/SamService.h"
#include "sam/SamNVMCTRL.h"

// #define PANDA_TIMESWIPE_FIRMWARE_DEBUG

#ifdef PANDA_TIMESWIPE_FIRMWARE_DEBUG
volatile bool stopflag{true};
// while (stopflag) os::wait(10);
#endif  // PANDA_TIMESWIPE_FIRMWARE_DEBUG

/**
 * @brief The firmware assemblage point.
 *
 * @details Creates all the neccesary objects and the corresponding bindings,
 * establishing the references between of them.
 */
int main()
{
  try {
    namespace ts = panda::timeswipe;
    namespace detail = panda::timeswipe::detail;

    constexpr int nChannels{detail::max_channel_count};
    constexpr std::size_t max_eeprom_size{2*1024};

#ifdef CALIBRATION_STATION
    bool bVisEnabled=false;
#else
    bool bVisEnabled=true;
#endif

#ifdef DMS_BOARD
    constexpr auto board_type = typeBoard::DMSBoard;
#else
    constexpr auto board_type = typeBoard::IEPEBoard;
#endif

    auto pVersion=std::make_shared<CSemVer>(detail::version_major,
      detail::version_minor, detail::version_patch);

    // Check/setup SmartEEPROM before clock init.
    CSamNVMCTRL::Instance();

    // Initialize the system clock: 120 MHz.
    initialize_system_clock();

    // Create the control instance.
    const auto node_control = nodeControl::Instance().shared_from_this();

    // -------------------------------------------------------------------------
    // Create I2C EEPROM
    // -------------------------------------------------------------------------

    // Create in-memory buffer for EEPROM.
    const auto eeprom_buffer = std::make_shared<CFIFO>();
    eeprom_buffer->reserve(max_eeprom_size);

    // Create I2C EEPROM master to operate with the external chip.
    const auto i2c_eeprom_master = std::make_shared<Sam_i2c_eeprom_master>();
    // auto* volatile eeprom_master = i2c_eeprom_master.get(); // for debugging
    i2c_eeprom_master->enable_irq(true);
    i2c_eeprom_master->set_eeprom_base_address(0);
    i2c_eeprom_master->set_eeprom_max_read_amount(max_eeprom_size);
    i2c_eeprom_master->set_eeprom_chip_address(0xA0);

    // Read the data from the external EEPROM.
    i2c_eeprom_master->receive(*eeprom_buffer);

    /*
     * Create 2 I2C slaves for read-only EEPROM data from extension plugs and
     * attach the EEPROM buffer to them.
     */
    const auto eeprom_hat = std::make_shared<CSamI2CmemHAT>();
    eeprom_hat->SetMemBuf(eeprom_buffer);
    eeprom_hat->EnableIRQs(true);

    //set iface:
    node_control->SetEEPROMiface(i2c_eeprom_master, eeprom_buffer);

    // -------------------------------------------------------------------------
    // Setup communication bus
    // -------------------------------------------------------------------------

    auto pSPIsc2 = std::make_shared<CSPIcomm>(Sam_sercom::Id::sercom2, Sam_pin::Id::pa12, Sam_pin::Id::pa15, Sam_pin::Id::pa13, Sam_pin::Id::pa14);
    pSPIsc2->EnableIRQs(true);
    auto pDisp = std::make_shared<CCmdDispatcher>();
    auto pStdPort = std::make_shared<CStdPort>(pDisp, pSPIsc2);
    pSPIsc2->AdviseSink(pStdPort);


    node_control->SetBoardType(board_type);
    std::shared_ptr<Pin> pDAConPin;
    std::shared_ptr<Pin> pUB1onPin;
    std::shared_ptr<Pin> pQSPICS0Pin;
    std::shared_ptr<CDMSsr> pDMSsr;


    //1st step:
    if constexpr (board_type == typeBoard::DMSBoard) {
      pDMSsr=std::make_shared<CDMSsr>(
        std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p05, true),
        std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p06, true),
        std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p07, true));

      pDAConPin=pDMSsr->FactoryPin(CDMSsr::pins::DAC_On);
      pUB1onPin=pDMSsr->FactoryPin(CDMSsr::pins::UB1_On);

      auto pCS0=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS0);
      pCS0->set_inverted(true);
      pQSPICS0Pin=pCS0;
      pCS0->write(false);

#ifdef DMS_TEST_MODE
      pDisp->Add("SR", std::make_shared<CCmdSGHandler<unsigned>>(
          pDMSsr,
          &CDMSsr::GetShiftReg,
          &CDMSsr::SetShiftReg));
#endif
    } else {
      pDAConPin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p04, true);
      pUB1onPin=std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p07, true);
      // pUB1onPin->set_inverted(true);
      // pUB1onPin->set(false);
      pQSPICS0Pin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p11, true);

      //old IEPE gain switches:
      auto pGain0=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p15, true);
      auto pGain1=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p14, true);
      node_control->SetIEPEboardGainSwitches(pGain0, pGain1);

    }
    auto pEnableMesPin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p13, true);
    auto pFanPin=std::make_shared<Sam_pin>(Sam_pin::Group::a, Sam_pin::Number::p09, true);

    //setup control:
    node_control->SetUBRpin(pUB1onPin);
    node_control->SetDAConPin(pDAConPin);
    node_control->SetEnableMesPin(pEnableMesPin);
    node_control->SetFanPin(pFanPin);

    auto pSamADC0 = std::make_shared<CSamADCcntr>(typeSamADC::Adc0);
    std::shared_ptr<CSamADCchan> pADC[]={
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN2, typeSamADCmuxneg::none, 0.0f, 4095.0f),
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN3, typeSamADCmuxneg::none, 0.0f, 4095.0f),
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN6, typeSamADCmuxneg::none, 0.0f, 4095.0f),
      std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN7, typeSamADCmuxneg::none, 0.0f, 4095.0f)};

    CSamQSPI objQSPI;
    std::shared_ptr<CDac5715sa> pDAC[]={
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACA, 0.0f, 4095.0f),
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACB, 0.0f, 4095.0f),
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACC, 0.0f, 4095.0f),
      std::make_shared<CDac5715sa>(&objQSPI, pQSPICS0Pin, typeDac5715chan::DACD, 0.0f, 4095.0f)};

    auto pSamDAC0=std::make_shared<CSamDACcntr>(typeSamDAC::Dac0, 0.0f, 4095.0f);
    auto pSamDAC1=std::make_shared<CSamDACcntr>(typeSamDAC::Dac1, 0.0f, 4095.0f);
    pSamDAC0->SetRawBinVal(2048);
    pSamDAC1->SetRawBinVal(2048);

    //add ADC/DAC commands:
    for (int i{}; i < nChannels; ++i) {
      char cmd[64];
      const int nInd=i+1;
      std::sprintf(cmd, "ADC%d.raw", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<int>>(
          pADC[i],
          &CAdc::DirectMeasure));
      std::sprintf(cmd, "DAC%d.raw", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<int>>(
          pDAC[i],
          &CDac::GetRawBinVal,
          &CDac::SetRawOutput));
    }
    pDisp->Add("AOUT3.raw", std::make_shared<CCmdSGHandler<int>>(
        pSamDAC0,
        &CDac::GetRawBinVal,
        &CDac::SetRawOutput));
    pDisp->Add("AOUT4.raw", std::make_shared<CCmdSGHandler<int>>(
        pSamDAC1,
        &CDac::GetRawBinVal,
        &CDac::SetRawOutput));
    pDisp->Add("DACsw", std::make_shared<CCmdSGHandler<bool>>(
        pDAConPin,
        &Pin::read_back,
        &Pin::write) );

    //2nd step:
    if constexpr (board_type == typeBoard::DMSBoard) {
      auto pCS1=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS1);
      pCS1->set_inverted(true);
      pCS1->write(false);

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
      node_control->SetVoltageDAC(pDAC2A);

#ifdef CALIBRATION_STATION
      //ability to control VSUP dac raw value:
      pDisp->Add("VSUP.raw", std::make_shared<CCmdSGHandler<int>>(
          pDAC2A,
          &CDac::GetRawBinVal,
          &CDac::SetRawOutput));
#endif

      //create 4 PGAs:
      CDMSsr::pins IEPEpins[]={CDMSsr::pins::IEPE1_On, CDMSsr::pins::IEPE2_On, CDMSsr::pins::IEPE3_On, CDMSsr::pins::IEPE4_On};
      for (int i{}; i < nChannels; ++i) {
        auto pPGA_CS=std::make_shared<CPGA_CS>(static_cast<CDMSsr::pga_sel>(i), pDMSsr, pInaSpiCSpin);
        auto pIEPEon=pDMSsr->FactoryPin(IEPEpins[i]);
        auto pPGA280=std::make_shared<CPGA280>(pInaSpi, pPGA_CS);

        node_control->AddMesChannel(std::make_shared<Dms_channel>(i, pADC[i], pDAC[i], static_cast<CView::vischan>(i), pIEPEon, pPGA280, bVisEnabled));
#ifdef DMS_TEST_MODE

        //add commands to each:
        char cmd[64];
        const int nInd=i+1;
        //for testing only:
        std::sprintf(cmd, "PGA%d.rsel", nInd);
        pDisp->Add(cmd, std::make_shared<CCmdSGHandler<unsigned>>(
            pPGA280,
            &CPGA280::GetSelectedReg,
            &CPGA280::SelectReg));
        std::sprintf(cmd, "PGA%d.rval", nInd);
        pDisp->Add(cmd, std::make_shared<CCmdSGHandler<int>>(
            pPGA280,
            &CPGA280::ReadSelectedReg,
            &CPGA280::WriteSelectedReg));
#endif

      }
    } else {
      for (int i{}; i < nChannels; ++i)
        node_control->AddMesChannel(std::make_shared<CIEPEchannel>(
            i, pADC[i], pDAC[i], static_cast<CView::vischan>(i), bVisEnabled));
    }

    //2 DAC PWMs:
    auto pPWM1=std::make_shared<CDacPWMht>(CDacPWMht::PWM1, pDAConPin);
    auto pPWM2=std::make_shared<CDacPWMht>(CDacPWMht::PWM2, pDAConPin);

    //PWM commands:
    pDisp->Add("PWM1", std::make_shared<CCmdSGHandler<bool>>(
        pPWM1,
        &CDacPWMht::IsStarted,
        &CDacPWMht::Start));
    pDisp->Add("PWM1.repeats", std::make_shared<CCmdSGHandler<unsigned>>(
        pPWM1,
        &CDacPWMht::GetRepeats,
        &CDacPWMht::SetRepeats));
    pDisp->Add("PWM1.duty", std::make_shared<CCmdSGHandler<float>>(
        pPWM1,
        &CDacPWMht::GetDutyCycle,
        &CDacPWMht::SetDutyCycle) );
    pDisp->Add("PWM1.freq", std::make_shared<CCmdSGHandler<unsigned>>(
        pPWM1,
        &CDacPWMht::GetFrequency,
        &CDacPWMht::SetFrequency));
    pDisp->Add("PWM1.high", std::make_shared<CCmdSGHandler<int>>(
        pPWM1,
        &CDacPWMht::GetHighLevel,
        &CDacPWMht::SetHighLevel));
    pDisp->Add("PWM1.low", std::make_shared<CCmdSGHandler<int>>(
        pPWM1,
        &CDacPWMht::GetLowLevel,
        &CDacPWMht::SetLowLevel));
    pDisp->Add("PWM2", std::make_shared<CCmdSGHandler<bool>>(
        pPWM2,
        &CDacPWMht::IsStarted,
        &CDacPWMht::Start));
    pDisp->Add("PWM2.repeats", std::make_shared<CCmdSGHandler<unsigned>>(
        pPWM2,
        &CDacPWMht::GetRepeats,
        &CDacPWMht::SetRepeats));
    pDisp->Add("PWM2.duty", std::make_shared<CCmdSGHandler<float>>(
        pPWM2,
        &CDacPWMht::GetDutyCycle,
        &CDacPWMht::SetDutyCycle));
    pDisp->Add("PWM2.freq", std::make_shared<CCmdSGHandler<unsigned>>(
        pPWM2,
        &CDacPWMht::GetFrequency,
        &CDacPWMht::SetFrequency));
    pDisp->Add("PWM2.high", std::make_shared<CCmdSGHandler<int>>(
        pPWM2,
        &CDacPWMht::GetHighLevel,
        &CDacPWMht::SetHighLevel));
    pDisp->Add("PWM2.low", std::make_shared<CCmdSGHandler<int>>(
        pPWM2,
        &CDacPWMht::GetLowLevel,
        &CDacPWMht::SetLowLevel));

    //temp sensor+ PIN PWM:
    auto pTempSens=std::make_shared<CSamTempSensor>(pSamADC0);
    auto pFanPWM=std::make_shared<CPinPWM>(Sam_pin::Group::a, Sam_pin::Number::p09);
    auto pFanControl=std::make_shared<CFanControl>(pTempSens, pFanPWM);

    //temp sens+fan control:
    pDisp->Add("Temp", std::make_shared<CCmdSGHandler<float>>(
        pTempSens,
        &CSamTempSensor::GetTempCD));
    pDisp->Add("Fan.duty", std::make_shared<CCmdSGHandler<float>>(
        pFanPWM,
        &CPinPWM::GetDutyCycle));
    pDisp->Add("Fan.freq", std::make_shared<CCmdSGHandler<unsigned>>(
        pFanPWM,
        &CPinPWM::GetFrequency,
        &CPinPWM::SetFrequency));
    pDisp->Add("Fan", std::make_shared<CCmdSGHandler<bool>>(
        pFanControl,
        &CFanControl::GetEnabled,
        &CFanControl::SetEnabled));

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
      const int nInd=i+1;
      const auto pCH = node_control->GetMesChannel(i);

      std::sprintf(cmd, "CH%d.mode", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<Measurement_mode>>(
          pCH,
          &Channel::measurement_mode,
          &Channel::set_measurement_mode));
      std::sprintf(cmd, "CH%d.gain", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<float>>(
          pCH,
          &Channel::amplification_gain,
          &Channel::set_amplification_gain));
      std::sprintf(cmd, "CH%d.iepe", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<bool>>(
          pCH,
          &Channel::is_iepe,
          &Channel::set_iepe));

#ifdef CALIBRATION_STATION
      std::sprintf(cmd, "CH%d.clr", nInd);
      pDisp->Add(cmd, std::make_shared<CCmdSGHandler<typeLEDcol>>(
          pCH,
          &Channel::color,
          &Channel::set_color));
#endif
    }

    pDisp->Add("Offset.errtol", std::make_shared<CCmdSGHandler<int>>(
        &CADpointSearch::GetTargErrTol,
        &CADpointSearch::SetTargErrTol));
    pDisp->Add("ARMID", std::make_shared<CCmdSGHandler<std::string>>(
        &CSamService::GetSerialString));
    pDisp->Add("fwVersion", std::make_shared<CCmdSGHandler<std::string>>(
        pVersion,
        &CSemVer::GetVersionString));

    //control commands:
    pDisp->Add("Gain", std::make_shared<CCmdSGHandler<int>>(
        node_control,
        &nodeControl::GetGain,
        &nodeControl::SetGain));
    pDisp->Add("Bridge", std::make_shared<CCmdSGHandler<bool>>(
        node_control,
        &nodeControl::GetBridge,
        &nodeControl::SetBridge));
    pDisp->Add("Record", std::make_shared<CCmdSGHandler<bool>>(
        node_control,
        &nodeControl::IsRecordStarted,
        &nodeControl::StartRecord));
    pDisp->Add("Offset", std::make_shared<CCmdSGHandler<int>>(
        node_control,
        &nodeControl::GetOffsetRunSt,
        &nodeControl::SetOffset));
    pDisp->Add("EnableADmes", std::make_shared<CCmdSGHandler<bool>>(
        node_control,
        &nodeControl::IsMeasurementsEnabled,
        &nodeControl::EnableMeasurements));
    pDisp->Add("Mode", std::make_shared<CCmdSGHandler<int>>(
        node_control,
        &nodeControl::GetMode,
        &nodeControl::SetMode));
    pDisp->Add("CalStatus", std::make_shared<CCmdSGHandler<bool>>(
        node_control,
        &nodeControl::GetCalStatus));
    pDisp->Add("Voltage", std::make_shared<CCmdSGHandler<float>>(
        node_control,
        &nodeControl::GetVoltage,
        &nodeControl::SetVoltage));
    pDisp->Add("Current", std::make_shared<CCmdSGHandler<float>>(
        node_control,
        &nodeControl::GetCurrent,
        &nodeControl::SetCurrent));
    pDisp->Add("MaxCurrent", std::make_shared<CCmdSGHandler<float>>(
        node_control,
        &nodeControl::GetMaxCurrent,
        &nodeControl::SetMaxCurrent));
    // pDisp->Add("Fan", std::make_shared<CCmdSGHandler<bool>>(
    //     node_control, &nodeControl::IsFanStarted,
    //     &nodeControl::StartFan));


    CView &view=CView::Instance();
#ifdef CALIBRATION_STATION
    pDisp->Add("UItest", std::make_shared<CCmdSGHandler<bool>>(
        pBtnHandler,
        &CCalFWbtnHandler::HasUItestBeenDone,
        &CCalFWbtnHandler::StartUItest));
    //testing Ext EEPROM:
    pDisp->Add("EEPROMTest", std::make_shared<CCmdSGHandler<bool>>(
        i2c_eeprom_master,
        &Sam_i2c_eeprom_master::self_test_result,
        &Sam_i2c_eeprom_master::run_self_test));

    pDisp->Add("CalEnable", std::make_shared<CCmdSGHandler<bool>>(
        node_control,
        &nodeControl::IsCalEnabled,
        &nodeControl::EnableCal));
#endif


    //--------------------JSON- ---------------------
    auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
    pDisp->Add("js", pJC);
    pJC->AddSubHandler("cAtom",
      [node_control](auto& req, auto& res, const auto ct)
      {
        node_control->procCAtom(req, res, ct);
      });

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
    // node_control->LoadSettings();
    node_control->SetMode(0); //set default mode
#ifndef CALIBRATION_STATION
    view.BlinkAtStart();
#endif

    // Enable calibration settings.
    node_control->EnableCal(true);

    // Loop endlessly.
    while (true) {
      button.update();
      node_control->Update();
      view.Update();

      pSPIsc2->Update();
      pSamADC0->Update();
      pFanControl->Update();
    }
  } catch (...) {
  }

  // main() must never return.
  while (true) os::wait(10);
}
