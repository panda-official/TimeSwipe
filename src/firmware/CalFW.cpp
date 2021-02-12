/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//build for ADCs-DACs:

#include "os.h"
#include "SamQSPI.h"
#include "SamSPIsc2.h"
#include "SamSPIsc7.h"
#include "I2CmemHAT.h"
#include "I2Cmem8Pin.h"
#include "SamI2CeepromMaster.h"
#include "ADmux.h"
#include "SamADCcntr.h"
#include "SamDACcntr.h"
#include "SamService.h"
#include "DACmax5715.h"
#include "DACdecor.h"
#include "DACPWMht.h"

//#include "menu_logic.h"
//#include "NewMenu.h"
#include "CalFWbtnHandler.h"
#include "SAMbutton.h"
#include "nodeLED.h"
#include "View.h"
#include "nodeControl.h"
#include "zerocal_man.h"

#include "cmd.h"
#include "std_port.h"
#include "jsondisp.h"

#include "HatsMemMan.h"
#include "RawBinStorage.h"

#include "FanControlSimple.h"
#include "SamNVMCTRL.h"
#include "SemVer.h"
#include "View.h"

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
        auto pVersion=std::make_shared<CSemVer>(0,0,11);

        CSamNVMCTRL::Instance(); //check/setup SmartEEPROM before clock init

        //step 0: clock init:
        sys_clock_init(); //->120MHz

        //----------------creating I2C EEPROM-----------------------
        //creating shared mem buf:
        auto pEEPROM_MemBuf=std::make_shared<CFIFO>();
        pEEPROM_MemBuf->reserve(1024); //reserve 1kb for an EEPROM data

        //creating an I2C EEPROM master to operate with an external chip:
        auto pEEPROM_MasterBus= std::make_shared<CSamI2CeepromMaster>();
        pEEPROM_MasterBus->EnableIRQs(true);

        //request data from an external chip:
        pEEPROM_MasterBus->SetDataAddrAndCountLim(0, 1024);
        pEEPROM_MasterBus->SetDeviceAddr(0xA0);
        pEEPROM_MasterBus->receive(*pEEPROM_MemBuf);

        //verifing the image:
        CHatsMemMan HatMan(pEEPROM_MemBuf);
        if(CHatsMemMan::op_result::OK!=HatMan.Verify()) //image is corrupted
        {
            //make default image:
            HatMan.Reset();

            CHatAtomVendorInfo vinf;

            vinf.m_uuid=CSamService::GetSerial();
            vinf.m_PID=0;
            vinf.m_pver=2;
            vinf.m_vstr="PANDA";
            vinf.m_pstr="TimeSwipe";

            HatMan.Store(vinf); //storage is ready
        }

        //create 2 I2C slaves for Read-only EEPROM data from extension plugs and connect them to the bufer:
        auto pEEPROM_HAT=std::make_shared<CSamI2CmemHAT>();
        pEEPROM_HAT->SetMemBuf(pEEPROM_MemBuf);
        pEEPROM_HAT->EnableIRQs(true);
        //----------------------------------------------------------


        //step 1 - creating QSPI bus:
        CSamQSPI objQSPI;
        auto pSPIsc2    =std::make_shared<CSamSPIsc2>();
        pSPIsc2->EnableIRQs(true);



         //step 2 - creating ADC0:
        auto pSamADC0   =std::make_shared<CSamADCcntr>(typeSamADC::Adc0);
        auto pADC1      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN2, typeSamADCmuxneg::none, 0.0f, 4095.0f);
        auto pADC2      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN3, typeSamADCmuxneg::none, 0.0f, 4095.0f);
        auto pADC3      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN6, typeSamADCmuxneg::none, 0.0f, 4095.0f);
        auto pADC4      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN7, typeSamADCmuxneg::none, 0.0f, 4095.0f);

        //step 3 - creating DAC channels:
        auto pDACA=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACA, 0.0f, 4095.0f);
        auto pDACB=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACB, 0.0f, 4095.0f);
        auto pDACC=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACC, 0.0f, 4095.0f);
        auto pDACD=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACD, 0.0f, 4095.0f);

        auto pSamDAC0   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac0, 0.0f, 4095.0f);
        auto pSamDAC1   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac1, 0.0f, 4095.0f);

        //set default level:
        pSamDAC0->SetRawBinVal(2048);
        pSamDAC1->SetRawBinVal(2048);


        //step 4 - creating mux:
        auto pADmux    =std::make_shared< CADmux >();

        //calibrator:
        auto pZeroCal=std::make_shared<CCalMan>();
        pZeroCal->Add(pADC1, pDACA, CView::ch1);
        pZeroCal->Add(pADC2, pDACB, CView::ch2);
        pZeroCal->Add(pADC3, pDACC, CView::ch3);
        pZeroCal->Add(pADC4, pDACD, CView::ch4);

        nodeControl::SetControlItems(pADmux, pZeroCal);


        //2 DAC PWMs:
        auto pPWM1=std::make_shared<CDacPWMht>(CDacPWMht::PWM1, pADmux);
        auto pPWM2=std::make_shared<CDacPWMht>(CDacPWMht::PWM2, pADmux);

        //temp sens+fan control:
        auto pTempSens=std::make_shared<CSamTempSensor>(pSamADC0);
        auto pFanControl=std::make_shared<CFanControlSimple>(pTempSens, CSamPORT::group::A, CSamPORT::pin::P09);


        //---------------------------------------------------command system------------------------------------------------------
        auto pDisp=         std::make_shared<CCmdDispatcher>();
        auto pStdPort=      std::make_shared<CStdPort>(pDisp, pSPIsc2);
        pSPIsc2->AdviseSink(pStdPort);

        // adding offsets control commands DAC 1-4
        pDisp->Add("DAC1.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACA, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DAC2.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACB, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DAC3.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACC, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DAC4.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACD, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );

        // adding commands for analog outputs 3-4 control
        pDisp->Add("AOUT3.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pSamDAC0, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("AOUT4.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pSamDAC1, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );


        pDisp->Add("ADC1.raw", std::make_shared< CCmdSGHandler<CAdc, int> >(pADC1, &CAdc::DirectMeasure) );
        pDisp->Add("ADC2.raw", std::make_shared< CCmdSGHandler<CAdc, int> >(pADC2, &CAdc::DirectMeasure) );
        pDisp->Add("ADC3.raw", std::make_shared< CCmdSGHandler<CAdc, int> >(pADC3, &CAdc::DirectMeasure) );
        pDisp->Add("ADC4.raw", std::make_shared< CCmdSGHandler<CAdc, int> >(pADC4, &CAdc::DirectMeasure) );


        //Node control:
        pDisp->Add("Gain", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetGain, &nodeControl::SetGain) );
        pDisp->Add("SetSecondary", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetSecondary,  &nodeControl::SetSecondary) );
        pDisp->Add("Bridge", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::GetBridge,  &nodeControl::SetBridge) );
        pDisp->Add("Record", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::IsRecordStarted,  &nodeControl::StartRecord) );
        pDisp->Add("Offset", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetOffsetRunSt,  &nodeControl::SetOffset) );
        pDisp->Add("EnableADmes", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::IsMeasurementsEnabled,  &nodeControl::EnableMeasurements) );
        pDisp->Add("Mode", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetMode,  &nodeControl::SetMode) );


        pDisp->Add("Offset.errtol", std::make_shared< CCmdSGHandlerF<int> >(&CADpointSearch::GetTargErrTol,  &CADpointSearch::SetTargErrTol) );
        pDisp->Add("DACsw", std::make_shared< CCmdSGHandler<CADmux, int> >(pADmux, &CADmux::getDACsw,  &CADmux::setDACsw) );
        pDisp->Add("Fan", std::make_shared< CCmdSGHandler<CADmux, bool> >(pADmux,  &CADmux::IsFanStarted,  &CADmux::StartFan) );

        pDisp->Add("Temp", std::make_shared< CCmdSGHandler<CSamTempSensor, float> >(pTempSens,  &CSamTempSensor::GetTempCD) );


        //PWM:
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

        //chip serial:
        pDisp->Add("ARMID", std::make_shared< CCmdSGHandlerF<std::string> >(&CSamService::GetSerialString) );
        pDisp->Add("fwVersion", std::make_shared< CCmdSGHandler<CSemVer, std::string> >(pVersion, &CSemVer::GetVersionString) );

        //cal status:
        pDisp->Add("CalStatus", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::GetCalStatus) );

        //extended settings:
        pDisp->Add("Voltage", std::make_shared< CCmdSGHandlerF<float> >(&nodeControl::GetVoltage, &nodeControl::SetVoltage) );
        pDisp->Add("Current", std::make_shared< CCmdSGHandlerF<float> >(&nodeControl::GetCurrent, &nodeControl::SetCurrent) );
        pDisp->Add("MaxCurrent", std::make_shared< CCmdSGHandlerF<float> >(&nodeControl::GetMaxCurrent, &nodeControl::SetMaxCurrent) );


        auto pBtnHandler=std::make_shared<CCalFWbtnHandler>();
        SAMButton &button=SAMButton::Instance();
        button.AdviseSink(pBtnHandler);

        pDisp->Add("UItest", std::make_shared< CCmdSGHandler<CCalFWbtnHandler, bool> >(pBtnHandler,
                                                                                       &CCalFWbtnHandler::HasUItestBeenDone,
                                                                                       &CCalFWbtnHandler::StartUItest) );

        //--------------------JSON- ---------------------
        auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
        pDisp->Add("js", pJC);

        //------------------JSON EVENTS-------------------
        auto pJE=std::make_shared<CJSONEvDispatcher>(pDisp);
        pDisp->Add("je", pJE);
        button.CJSONEvCP::AdviseSink(pJE);
        nodeControl::Instance().AdviseSink(pJE);
        //--------------------------------------------------------------------------------------------------------------


        nodeControl &nc=nodeControl::Instance();
        CView &view=CView::Instance();
        nc.LoadSettings();
        //view.CalUItest(); //dbg

        while(1) //endless loop ("super loop")
        {
             button.update();
             nc.Update();
             view.Update();

             pSPIsc2->Update();
             pSamADC0->Update();
             pFanControl->Update();
        }
}
