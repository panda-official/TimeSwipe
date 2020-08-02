/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

//build for ADCs-DACs:

#include "os.h"
#include "SamSPIbase.h"
#include "SamQSPI.h"
#include "SPIcomm.h"

#include "I2CmemHAT.h"
#include "I2Cmem8Pin.h"
#include "SamI2CeepromMaster.h"
#include "SamADCcntr.h"
#include "SamDACcntr.h"
#include "SamService.h"
#include "DACmax5715.h"
#include "DACPWMht.h"
#include "ShiftReg.h"
#include "PGA280.h"

#include "NewMenu.h"
#include "SAMbutton.h"
#include "nodeLED.h"
#include "View.h"
#include "nodeControl.h"
#include "zerocal_man.h"
#include "DMSchannel.h"

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
        auto pVersion=std::make_shared<CSemVer>(0,0,15);

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


        //communication bus:
        auto pSPIsc2    =std::make_shared<CSPIcomm>(typeSamSercoms::Sercom2, CSamPORT::pxy::PA12, CSamPORT::pxy::PA15, CSamPORT::pxy::PA13, CSamPORT::pxy::PA14);
        pSPIsc2->EnableIRQs(true);
        auto pDisp=         std::make_shared<CCmdDispatcher>();
        auto pStdPort=      std::make_shared<CStdPort>(pDisp, pSPIsc2);
        pSPIsc2->AdviseSink(pStdPort);


        //setup pins:
#ifdef DMS_BOARD
        typeBoard ThisBoard=typeBoard::DMSBoard;
#else
        typeBoard ThisBoard=typeBoard::IEPEBoard;
#endif
        const int nChannels=4;

        nodeControl &nc=nodeControl::Instance();
        nc.SetBoardType(ThisBoard);
        std::shared_ptr<IPin> pDAConPin;
        std::shared_ptr<IPin> pUB1onPin;
        std::shared_ptr<IPin> pQSPICS0Pin;
        std::shared_ptr<CDMSsr> pDMSsr;


        //1st step:
        if(typeBoard::DMSBoard==ThisBoard)
        {
            pDMSsr=std::make_shared<CDMSsr>(
                        CSamPORT::FactoryPin(CSamPORT::group::C, CSamPORT::pin::P05, true),
                        CSamPORT::FactoryPin(CSamPORT::group::C, CSamPORT::pin::P06, true),
                        CSamPORT::FactoryPin(CSamPORT::group::C, CSamPORT::pin::P07, true) );

            pDAConPin=pDMSsr->FactoryPin(CDMSsr::pins::DAC_On);
            pUB1onPin=pDMSsr->FactoryPin(CDMSsr::pins::UB1_On);

            auto pCS0=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS0); pCS0->SetInvertedBehaviour(true);  pQSPICS0Pin=pCS0; pCS0->Set(false);

#ifdef DMS_TEST_MODE
           pDisp->Add("SR", std::make_shared< CCmdSGHandler<CDMSsr, unsigned int> >(pDMSsr, &CDMSsr::GetShiftReg, &CDMSsr::SetShiftReg) );
#endif

        }
        else
        {
            pDAConPin=CSamPORT::FactoryPin(CSamPORT::group::B, CSamPORT::pin::P04, true);
            pUB1onPin=CSamPORT::FactoryPin(CSamPORT::group::C, CSamPORT::pin::P07, true); //pUB1onPin->SetInvertedBehaviour(true); pUB1onPin->Set(false);
            pQSPICS0Pin=CSamPORT::FactoryPin(CSamPORT::group::B, CSamPORT::pin::P11, true);

            //old IEPE gain switches:
            auto pGain0=CSamPORT::FactoryPin(CSamPORT::group::B, CSamPORT::pin::P15, true);
            auto pGain1=CSamPORT::FactoryPin(CSamPORT::group::B, CSamPORT::pin::P14, true);
            nc.SetIEPEboardGainSwitches(pGain0, pGain1);

        }
        auto pEnableMesPin=CSamPORT::FactoryPin(CSamPORT::group::B, CSamPORT::pin::P13, true);
        auto pFanPin=CSamPORT::FactoryPin(CSamPORT::group::A, CSamPORT::pin::P09, true);

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
        pDisp->Add("DACsw", std::make_shared< CCmdSGHandler<CPin, bool> >(pDAConPin, &CPin::RbSet,  &CPin::Set) );



        //2nd step:
        if(typeBoard::DMSBoard==ThisBoard)
        {
            auto pCS1=pDMSsr->FactoryPin(CDMSsr::pins::QSPI_CS1); pCS1->SetInvertedBehaviour(true);  pCS1->Set(false);

            //create PGA280 extension bus:
            auto pInaSpi=std::make_shared<CSamSPIbase>(true, typeSamSercoms::Sercom5,
                                                       CSamPORT::pxy::PB16, CSamPORT::pxy::PB19, CSamPORT::pxy::PB17, CSamPORT::none);


            auto pInaSpiCSpin=CSamPORT::FactoryPin(CSamPORT::group::B, CSamPORT::pin::P18, true);
            pInaSpiCSpin->SetInvertedBehaviour(true);
            pInaSpiCSpin->Set(false);

            auto pDAC2A=std::make_shared<CDac5715sa>(&objQSPI, pCS1, typeDac5715chan::DACA, 2.5f, 24.0f);
            pDAC2A->SetLinearFactors(-0.005786666f, 25.2f);
            pDAC2A->SetVal(0);
            nc.SetVoltageDAC(pDAC2A);


            //create 4 PGAs:
            CDMSsr::pins IEPEpins[]={CDMSsr::pins::IEPE1_On, CDMSsr::pins::IEPE2_On, CDMSsr::pins::IEPE3_On, CDMSsr::pins::IEPE4_On};
            for(int i=0; i<nChannels; i++)
            {
                auto pPGA_CS=std::make_shared<CPGA_CS>(static_cast<CDMSsr::pga_sel>(i), pDMSsr, pInaSpiCSpin);
                auto pIEPEon=pDMSsr->FactoryPin(IEPEpins[i]);
                auto pPGA280=std::make_shared<CPGA280>(pInaSpi, pPGA_CS);

                nc.AddMesChannel( std::make_shared<CDMSchannel>(pADC[i], pDAC[i], static_cast<CView::vischan>(i), pIEPEon, pPGA280) );
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
                nc.AddMesChannel( std::make_shared<CIEPEchannel>(pADC[i], pDAC[i], static_cast<CView::vischan>(i)) );
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


        //temp sens+fan control:
        auto pTempSens=std::make_shared<CSamTempSensor>(pSamADC0);
        pDisp->Add("Temp", std::make_shared< CCmdSGHandler<CSamTempSensor, float> >(pTempSens,  &CSamTempSensor::GetTempCD) );
        auto pFanControl=std::make_shared<CFanControlSimple>(pTempSens, CSamPORT::group::A, CSamPORT::pin::P09);


        //---------------------------------------------------command system------------------------------------------------------
        //channel commands:
        for(int i=0; i<nChannels; i++)
        {
            char cmd[64];
            int nInd=i+1;
            auto pCH=nc.GetMesChannel(i);

            std::sprintf(cmd, "CH%d.mode", nInd);
            pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CMesChannel, unsigned int> >(pCH, &CMesChannel::CmGetMesMode, &CMesChannel::CmSetMesMode) );
            std::sprintf(cmd, "CH%d.gain", nInd);
            pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CMesChannel, float> >(pCH, &CMesChannel::GetActualAmpGain, &CMesChannel::SetAmpGain) );
            std::sprintf(cmd, "CH%d.iepe", nInd);
            pDisp->Add(cmd, std::make_shared< CCmdSGHandler<CMesChannel, bool> >(pCH, &CMesChannel::IsIEPEon, &CMesChannel::IEPEon) );
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
        pDisp->Add("Fan", std::make_shared< CCmdSGHandler<nodeControl, bool> >(pNC,  &nodeControl::IsFanStarted,  &nodeControl::StartFan) );


        SAMButton &button=SAMButton::Instance();
        button.AdviseSink( std::make_shared<CNewMenu>() );

        //--------------------JSON- ---------------------
        auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
        pDisp->Add("js", pJC);

        //------------------JSON EVENTS-------------------
        auto pJE=std::make_shared<CJSONEvDispatcher>(pDisp);
        pDisp->Add("je", pJE);
        button.CJSONEvCP::AdviseSink(pJE);
        nodeControl::Instance().AdviseSink(pJE);
        //--------------------------------------------------------------------------------------------------------------



        CView &view=CView::Instance();
        nc.LoadSettings();
        nc.SetMode(0); //set default mode
        view.BlinkAtStart();

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
