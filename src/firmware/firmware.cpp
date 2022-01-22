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
#include "board.hpp"
#include "os.h"
#include "settings.hpp"
#include "base/SPIcomm.h"
#include "dms_channel.hpp"
#include "pga280.hpp"
#include "shiftreg.hpp"
#include "base/I2CmemHAT.h"
#include "base/I2Cmem8Pin.h"
#include "base/dac_max5715.hpp"
#include "base/DACPWMht.h"
#include "base/RawBinStorage.h"
#include "base/FanControl.h"
#include "control/NewMenu.h"
#include "control/CalFWbtnHandler.h"
#include "control/View.h"
#include "control/zerocal_man.h"
#include "control/SemVer.h"
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
  namespace ts = panda::timeswipe;
  namespace detail = panda::timeswipe::detail;

  constexpr int channel_count{detail::max_channel_count};
  constexpr std::size_t max_eeprom_size{2*1024};

#ifdef CALIBRATION_STATION
  constexpr auto is_visualization_enabled = false;
#else
  constexpr auto is_visualization_enabled = true;
#endif

#ifdef DMS_BOARD
  constexpr auto board_type = Board_type::dms;
#else
  constexpr auto board_type = Board_type::iepe;
#endif

  // Check/setup SmartEEPROM before clock init.
  CSamNVMCTRL::Instance();

  // Initialize the system clock: 120 MHz.
  initialize_system_clock();

  // Create the control instance.
  const auto board = Board::instance().shared_from_this();

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

  // Set handles.
  board->set_eeprom_handles(i2c_eeprom_master, eeprom_buffer);

  // -------------------------------------------------------------------------
  // Setup communication bus
  // -------------------------------------------------------------------------

  const auto sercom2_spi = std::make_shared<CSPIcomm>(Sam_sercom::Id::sercom2,
    Sam_pin::Id::pa12,
    Sam_pin::Id::pa15,
    Sam_pin::Id::pa13,
    Sam_pin::Id::pa14);
  sercom2_spi->EnableIRQs(true);
  const auto setting_dispatcher = std::make_shared<Setting_dispatcher>();
  const auto setting_parser = std::make_shared<Setting_parser>(setting_dispatcher,
    sercom2_spi);
  sercom2_spi->AdviseSink(setting_parser);

  board->set_board_type(board_type);
  std::shared_ptr<Pin> pDAConPin;
  std::shared_ptr<Pin> pUB1onPin;
  std::shared_ptr<Pin> pQSPICS0Pin;
  std::shared_ptr<CDMSsr> pDMSsr;

  //1st step:
  if constexpr (board_type == Board_type::dms) {
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
  } else {
    pDAConPin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p04, true);
    pUB1onPin=std::make_shared<Sam_pin>(Sam_pin::Group::c, Sam_pin::Number::p07, true);
    // pUB1onPin->set_inverted(true);
    // pUB1onPin->set(false);
    pQSPICS0Pin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p11, true);

    //old IEPE gain switches:
    auto pGain0=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p15, true);
    auto pGain1=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p14, true);
    board->set_iepe_gain_pins(pGain0, pGain1);
  }
  const auto pEnableMesPin = std::make_shared<Sam_pin>(
    Sam_pin::Group::b,
    Sam_pin::Number::p13,
    true);
  const auto pFanPin = std::make_shared<Sam_pin>(
    Sam_pin::Group::a,
    Sam_pin::Number::p09,
    true);

  //setup control:
  board->set_ubr_pin(pUB1onPin);
  board->set_dac_mode_pin(pDAConPin);
  board->set_adc_measurement_enable_pin(pEnableMesPin);
  board->set_fan_pin(pFanPin);

  auto pSamADC0 = std::make_shared<CSamADCcntr>(typeSamADC::Adc0);
  std::shared_ptr<CSamADCchan> pADC[]={
    std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN2, typeSamADCmuxneg::none),
    std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN3, typeSamADCmuxneg::none),
    std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN6, typeSamADCmuxneg::none),
    std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN7, typeSamADCmuxneg::none)};

  CSamQSPI objQSPI;
  std::shared_ptr<Dac_max5715> pDAC[]={
    std::make_shared<Dac_max5715>(&objQSPI, pQSPICS0Pin, Dac_max5715::Channel::a,
      50, 4045),
    std::make_shared<Dac_max5715>(&objQSPI, pQSPICS0Pin, Dac_max5715::Channel::b,
      50, 4045),
    std::make_shared<Dac_max5715>(&objQSPI, pQSPICS0Pin, Dac_max5715::Channel::c,
      50, 4045),
    std::make_shared<Dac_max5715>(&objQSPI, pQSPICS0Pin, Dac_max5715::Channel::d,
      50, 4045)};
  for (const auto& dac : pDAC)
    dac->set_raw(dac->raw_range().second);

  auto pSamDAC0=std::make_shared<CSamDACcntr>(typeSamDAC::Dac0);
  auto pSamDAC1=std::make_shared<CSamDACcntr>(typeSamDAC::Dac1);
  pSamDAC0->set_raw(2048);
  pSamDAC1->set_raw(2048);

  // channel%dAdcRaw, channel%dDacRaw
  for (int i{}; i < channel_count; ++i) {
    char cmd[64];
    const int nInd=i+1;
    std::sprintf(cmd, "channel%dAdcRaw", nInd);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<int>>(
        pADC[i],
        &Adc_channel::GetRawBinVal));
    std::sprintf(cmd, "channel%dDacRaw", nInd);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<int>>(
        pDAC[i],
        &Dac_channel::GetRawBinVal,
        &Dac_channel::set_raw));
  }
  for (int i{3}; i <= 4; ++i) {
    char cmd[64];
    std::sprintf(cmd, "analogOut%dDacRaw", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<int>>(
        pSamDAC0,
        &Dac_channel::GetRawBinVal,
        &Dac_channel::set_raw));
  }
  setting_dispatcher->add("analogOutsDacEnabled", std::make_shared<Setting_generic_handler<bool>>(
      pDAConPin,
      &Pin::read_back,
      &Pin::write));

  //2nd step:
  std::shared_ptr<Calibratable_dac> voltage_dac;
  if constexpr (board_type == Board_type::dms) {
    auto pCS1=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS1);
    pCS1->set_inverted(true);
    pCS1->write(false);

    //create PGA280 extension bus:
    auto pInaSpi=std::make_shared<CSamSPIbase>(true, Sam_sercom::Id::sercom5,
      Sam_pin::Id::pb16, Sam_pin::Id::pb19, Sam_pin::Id::pb17, std::nullopt, nullptr);

    auto pInaSpiCSpin=std::make_shared<Sam_pin>(Sam_pin::Group::b, Sam_pin::Number::p18, true);
    pInaSpiCSpin->set_inverted(true);
    pInaSpiCSpin->write(false);

    voltage_dac = std::make_shared<Calibratable_dac>(
      std::make_shared<Dac_max5715>(&objQSPI, pCS1, Dac_max5715::Channel::a,
        120, 3904), 2.5, 24);
    voltage_dac->set_value(voltage_dac->value_range().first);
    board->set_voltage_dac(voltage_dac);

#ifdef CALIBRATION_STATION
    //ability to control Voltage DAC's raw value:
    setting_dispatcher->add("voltageOutRaw", std::make_shared<Setting_generic_handler<int>>(
        voltage_dac,
        &Calibratable_dac::GetRawBinVal,
        &Calibratable_dac::set_raw));
#endif

    //create 4 PGAs:
    CDMSsr::pins IEPEpins[]={CDMSsr::pins::IEPE1_On, CDMSsr::pins::IEPE2_On, CDMSsr::pins::IEPE3_On, CDMSsr::pins::IEPE4_On};
    for (int i{}; i < channel_count; ++i) {
      auto pPGA_CS=std::make_shared<CPGA_CS>(static_cast<CDMSsr::pga_sel>(i), pDMSsr, pInaSpiCSpin);
      auto pIEPEon=pDMSsr->FactoryPin(IEPEpins[i]);
      auto pPGA280=std::make_shared<CPGA280>(pInaSpi, pPGA_CS);

      board->add_channel(std::make_shared<Dms_channel>(i, pADC[i], pDAC[i], static_cast<CView::vischan>(i), pIEPEon, pPGA280, is_visualization_enabled));
    }
  } else {
    for (int i{}; i < channel_count; ++i)
      board->add_channel(std::make_shared<CIEPEchannel>(
          i, pADC[i], pDAC[i], static_cast<CView::vischan>(i), is_visualization_enabled));
  }

  //2 DAC PWMs:
  auto pPWM1=std::make_shared<CDacPWMht>(CDacPWMht::PWM1, pDAConPin);
  auto pPWM2=std::make_shared<CDacPWMht>(CDacPWMht::PWM2, pDAConPin);

  //PWM commands:
  for (int i{1}; i <= 2; ++i) {
    char cmd[64];
    std::sprintf(cmd, "pwm%dEnabled", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<bool>>(
        pPWM1,
        &CDacPWMht::IsStarted,
        &CDacPWMht::Start));
    std::sprintf(cmd, "pwm%dRepeatCount", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<unsigned>>(
        pPWM1,
        &CDacPWMht::GetRepeats,
        &CDacPWMht::SetRepeats));
    std::sprintf(cmd, "pwm%dDutyCycle", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<float>>(
        pPWM1,
        &CDacPWMht::GetDutyCycle,
        &CDacPWMht::SetDutyCycle));
    std::sprintf(cmd, "pwm%dFrequency", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<unsigned>>(
        pPWM1,
        &CDacPWMht::GetFrequency,
        &CDacPWMht::SetFrequency));
    std::sprintf(cmd, "pwm%dHighBoundary", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<int>>(
        pPWM1,
        &CDacPWMht::GetHighLevel,
        &CDacPWMht::SetHighLevel));
    std::sprintf(cmd, "pwm%dLowBoundary", i);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<int>>(
        pPWM1,
        &CDacPWMht::GetLowLevel,
        &CDacPWMht::SetLowLevel));
  }

  //temp sensor+ PIN PWM:
  auto pTempSens=std::make_shared<CSamTempSensor>(pSamADC0);
  auto pFanPWM=std::make_shared<CPinPWM>(Sam_pin::Group::a, Sam_pin::Number::p09);
  auto pFanControl=std::make_shared<CFanControl>(pTempSens, pFanPWM);

  //temp sens+fan control:
  setting_dispatcher->add("temperature", std::make_shared<Setting_generic_handler<float>>(
      pTempSens,
      &CSamTempSensor::GetTempCD));
  setting_dispatcher->add("fanDutyCycle", std::make_shared<Setting_generic_handler<float>>(
      pFanPWM,
      &CPinPWM::GetDutyCycle));
  setting_dispatcher->add("fanFrequency", std::make_shared<Setting_generic_handler<unsigned>>(
      pFanPWM,
      &CPinPWM::GetFrequency,
      &CPinPWM::SetFrequency));
  setting_dispatcher->add("fanEnabled", std::make_shared<Setting_generic_handler<bool>>(
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
  for (int i{}; i < channel_count; ++i) {
    char cmd[64];
    const int nInd=i+1;
    const auto channel = board->channel(i);

    std::sprintf(cmd, "channel%dMode", nInd);
    using Ch_mode_handler = Setting_generic_handler<std::optional<Measurement_mode>,
      Measurement_mode>;
    setting_dispatcher->add(cmd, std::make_shared<Ch_mode_handler>(
        channel,
        &Channel::measurement_mode,
        &Channel::set_measurement_mode));
    std::sprintf(cmd, "channel%dGain", nInd);
    using Ch_gain_handler = Setting_generic_handler<std::optional<float>, float>;
    setting_dispatcher->add(cmd, std::make_shared<Ch_gain_handler>(
        channel,
        &Channel::amplification_gain,
        &Channel::set_amplification_gain));
    std::sprintf(cmd, "channel%dIepe", nInd);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<bool>>(
        channel,
        &Channel::is_iepe,
        &Channel::set_iepe));

#ifdef CALIBRATION_STATION
    std::sprintf(cmd, "channel%dColor", nInd);
    setting_dispatcher->add(cmd, std::make_shared<Setting_generic_handler<typeLEDcol>>(
        channel,
        &Channel::color,
        &Channel::set_color));
#endif
  }

  // [[deprecated]]
  // setting_dispatcher->add("fanEnabled", std::make_shared<Setting_generic_handler<bool>>(
  //     board,
  //     &Board::is_fan_enabled,
  //     &Board::enable_fan));
  // setting_dispatcher->add("Offset", std::make_shared<Setting_generic_handler<int>>(
  //     board,
  //     &Board::is_offset_search_started,
  //     &Board::start_offset_search));
  // setting_dispatcher->add("Offset.errtol", std::make_shared<Setting_generic_handler<int>>(
  //     &CADpointSearch::GetTargErrTol,
  //     &CADpointSearch::SetTargErrTol));
  // setting_dispatcher->add("Gain", std::make_shared<Setting_generic_handler<int>>(
  //     board,
  //     &Board::gain,
  //     &Board::set_gain));
  // setting_dispatcher->add("Mode", std::make_shared<Setting_generic_handler<int>>(
  //     board,
  //     &Board::measurement_mode,
  //     &Board::set_measurement_mode));
  // setting_dispatcher->add("Record", std::make_shared<Setting_generic_handler<bool>>(
  //     board,
  //     &Board::is_record_started,
  //     &Board::start_record));
  // setting_dispatcher->add("Current", std::make_shared<Setting_generic_handler<float>>(
  //     board,
  //     &Board::current,
  //     &Board::set_current));
  // setting_dispatcher->add("MaxCurrent", std::make_shared<Setting_generic_handler<float>>(
  //     board,
  //     &Board::max_current,
  //     &Board::set_max_current));
  setting_dispatcher->add("armId", std::make_shared<Setting_generic_handler<std::string>>(
      &CSamService::GetSerialString));

  const auto version = std::make_shared<CSemVer>(detail::version_major,
    detail::version_minor, detail::version_patch);
  setting_dispatcher->add("firmwareVersion", std::make_shared<Setting_generic_handler<std::string>>(
      version,
      &CSemVer::GetVersionString));

  //control commands:
  setting_dispatcher->add("voltageOutEnabled", std::make_shared<Setting_generic_handler<bool>>(
      board,
      &Board::is_bridge_enabled,
      &Board::enable_bridge));
  setting_dispatcher->add("channelsAdcEnabled", std::make_shared<Setting_generic_handler<bool>>(
      board,
      &Board::is_measurement_enabled,
      &Board::enable_measurement));
  setting_dispatcher->add("channelsCalibrationValid", std::make_shared<Setting_generic_handler<bool>>(
      board,
      &Board::is_calibration_data_valid));
  setting_dispatcher->add("voltageOutValue", std::make_shared<Setting_generic_handler<float>>(
      board,
      &Board::voltage,
      &Board::set_voltage));

  CView &view=CView::Instance();
#ifdef CALIBRATION_STATION
  // [[deprecated]]
  // setting_dispatcher->add("UItest", std::make_shared<Setting_generic_handler<bool>>(
  //     pBtnHandler,
  //     &CCalFWbtnHandler::HasUItestBeenDone,
  //     &CCalFWbtnHandler::StartUItest));

  //testing Ext EEPROM:
  setting_dispatcher->add("eepromTest", std::make_shared<Setting_generic_handler<bool>>(
      i2c_eeprom_master,
      &Sam_i2c_eeprom_master::self_test_result,
      &Sam_i2c_eeprom_master::run_self_test));

  setting_dispatcher->add("channelsCalibrationEnabled",
    std::make_shared<Setting_generic_handler<bool>>(
      board,
      &Board::is_calibration_data_enabled,
      &Board::enable_calibration_data));
#endif

  //------------------JSON EVENTS-------------------
  auto pJE=std::make_shared<CJSONEvDispatcher>(setting_dispatcher);
  setting_dispatcher->add("je", pJE);
  button.CJSONEvCP::AdviseSink(pJE);
  Board::instance().AdviseSink(pJE);
  //--------------------------------------------------------------------------------------------------------------


  /*
   * Board::import_settings() activates the persistent
   * storage handling which is currently broken!
   */
  // board->import_settings();
  // board->set_measurement_mode(0); FIXME: remove. This is obsolete.
#ifndef CALIBRATION_STATION
  view.BlinkAtStart();
#endif

  board->enable_calibration_data(true);

  // Loop endlessly. (main() must never return!)
  while (true) {
    button.update();
    board->update();
    view.Update();

    sercom2_spi->Update();
    pSamADC0->Update();
    pFanControl->Update();
  }
}
