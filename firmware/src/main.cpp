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
#include "DACmax5715.h"
#include "DACdecor.h"

#include "menu_logic.h"
#include "SAMbutton.h"
#include "nodeLED.h"
#include "View.h"
#include "nodeControl.h"
#include "zerocal_man.h"

#include "cmd.h"
#include "std_port.h"
#include "jsondisp.h"

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
        unsigned long last_time_upd=os::get_tick_mS();
        sys_clock_init(); //->120MHz
        nodeLED::init();
        auto pLED1      =std::make_shared<CLED>(typeLED::LED1);
        auto pLED2      =std::make_shared<CLED>(typeLED::LED2);
        auto pLED3      =std::make_shared<CLED>(typeLED::LED3);
        auto pLED4      =std::make_shared<CLED>(typeLED::LED4);

        //step 1 - creating QSPI bus:
        CSamQSPI objQSPI;


        auto pSPIsc2    =std::make_shared<CSamSPIsc2>();
        pSPIsc2->EnableIRQs(true);  //no working in IRQ mode....


        //----------------creating I2C EEPROM-----------------------
        //creating shared mem buf:
        auto pEEPROM_MemBuf=std::make_shared<CFIFO>();
        pEEPROM_MemBuf->reserve(1024); //reserve 1kb for an EEPROM data

        //create 2 I2C slaves for Read-only EEPROM data from extension plugs and connect them to the bufer:
        auto pEEPROM_HAT=std::make_shared<CSamI2CmemHAT>();
        pEEPROM_HAT->SetMemBuf(pEEPROM_MemBuf);
        pEEPROM_HAT->EnableIRQs(true);

        //creating an I2C EEPROM master to operate with an external chip:
        auto pEEPROM_MasterBus= std::make_shared<CSamI2CeepromMaster>();
        pEEPROM_MasterBus->EnableIRQs(true);

        //request data from an external chip:
        pEEPROM_MasterBus->SetDataAddrAndCountLim(0, 1024);
        pEEPROM_MasterBus->SetDeviceAddr(0xA0);
        pEEPROM_MasterBus->receive(*pEEPROM_MemBuf);
        //----------------------------------------------------------


         //step 2 - creating ADC0:
#ifndef  USE_AD_CUSTOM_RANGES
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

        //23.05.2019
        auto pSamDAC0   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac0, 0.0f, 4095.0f);
        auto pSamDAC1   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac1, 0.0f, 4095.0f);
#else
         auto pSamADC0   =std::make_shared<CSamADCcntr>(typeSamADC::Adc0);
         auto pADC1      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN2, typeSamADCmuxneg::none, -2.5f, 2.5f);
         auto pADC2      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN3, typeSamADCmuxneg::none, -2.5f, 2.5f);
         auto pADC3      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN6, typeSamADCmuxneg::none, -2.5f, 2.5f);
         auto pADC4      =std::make_shared<CSamADCchan>(pSamADC0, typeSamADCmuxpos::AIN7, typeSamADCmuxneg::none, -2.5f, 2.5f);

        //step 3 - creating DAC channels:
        auto pDACA=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACA, -10.0f, +10.0f);
        auto pDACB=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACB, -10.0f, +10.0f);
        auto pDACC=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACC, -10.0f, +10.0f);
        auto pDACD=std::make_shared<CDac5715sa>(&objQSPI, typeDac5715chan::DACD, -10.0f, +10.0f);

        //23.05.2019
        auto pSamDAC0   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac0, -10, +10);
        auto pSamDAC1   =std::make_shared<CSamDACcntr>(typeSamDAC::Dac1, -10, +10);
#endif


        //step 4 - creating mux:
        auto pADmux    =std::make_shared< CADmux >();

        //calibrator:
        auto pZeroCal=std::make_shared<CCalMan>();
        pZeroCal->Add(pADC1, pDACA, pLED1);
        pZeroCal->Add(pADC2, pDACB, pLED2);
        pZeroCal->Add(pADC3, pDACC, pLED3);
        pZeroCal->Add(pADC4, pDACD, pLED4);

        nodeControl::SetControlItems(pADmux, pZeroCal);



        //Setup initial offset for the amplifier
        pDACA->SetRawOutput(2048);
        pDACB->SetRawOutput(2048);
        pDACC->SetRawOutput(2048);
        pDACD->SetRawOutput(2048);


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


        //LEDs control:
        pDisp->Add("LED1", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED1, nullptr,  &CLED::ON) );
        pDisp->Add("LED1.blink", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED1, nullptr,  &CLED::SetBlinkMode) );
        pDisp->Add("LED1.col", std::make_shared< CCmdSGHandler<CLED, typeLEDcol> >(pLED1, nullptr,  &CLED::SetColor) );

        pDisp->Add("LED2", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED2, nullptr,  &CLED::ON) );
        pDisp->Add("LED2.blink", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED2, nullptr,  &CLED::SetBlinkMode) );
        pDisp->Add("LED2.col", std::make_shared< CCmdSGHandler<CLED, typeLEDcol> >(pLED2, nullptr,  &CLED::SetColor) );

        pDisp->Add("LED3", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED3, nullptr,  &CLED::ON) );
        pDisp->Add("LED3.blink", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED3, nullptr,  &CLED::SetBlinkMode) );
        pDisp->Add("LED3.col", std::make_shared< CCmdSGHandler<CLED, typeLEDcol> >(pLED3, nullptr,  &CLED::SetColor) );

        pDisp->Add("LED4", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED4, nullptr,  &CLED::ON) );
        pDisp->Add("LED4.blink", std::make_shared< CCmdSGHandler<CLED, bool> >(pLED4, nullptr,  &CLED::SetBlinkMode) );
        pDisp->Add("LED4.col", std::make_shared< CCmdSGHandler<CLED, typeLEDcol> >(pLED4, nullptr,  &CLED::SetColor) );

        //Node control:
        pDisp->Add("Gain", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetGain, &nodeControl::SetGain) );
        pDisp->Add("SetSecondary", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetSecondary,  &nodeControl::SetSecondary) );
        pDisp->Add("Bridge", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::GetBridge,  &nodeControl::SetBridge) );
        pDisp->Add("Record", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::IsRecordStarted,  &nodeControl::StartRecord) );
        pDisp->Add("Zero", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::GetZeroRunSt,  &nodeControl::SetZero) );

        pDisp->Add("Zero.errtol", std::make_shared< CCmdSGHandlerF<int> >(&CADpointSearch::GetTargErrTol,  &CADpointSearch::SetTargErrTol) );
        pDisp->Add("EnableADmes", std::make_shared< CCmdSGHandler<CADmux, bool> >(pADmux,  &CADmux::IsADmesEnabled,  &CADmux::EnableADmes) ); //18.07.2019 - a getter is added
        pDisp->Add("DACsw", std::make_shared< CCmdSGHandler<CADmux, int> >(pADmux, &CADmux::getDACsw,  &CADmux::setDACsw) );
        pDisp->Add("Fan", std::make_shared< CCmdSGHandler<CADmux, bool> >(pADmux,  &CADmux::IsFanStarted,  &CADmux::StartFan) );

        //--------------------menu+button+detection of a master----------------
        auto pMenu=std::make_shared<CMenuLogic>();
        SAMButton button(*pMenu);

        nodeControl::CreateDataVis(pADC1, pLED1);
        nodeControl::CreateDataVis(pADC2, pLED2);
        nodeControl::CreateDataVis(pADC3, pLED3);
        nodeControl::CreateDataVis(pADC4, pLED4);
        nodeControl::StartDataVis(true, 1200);


        //--------------------JSON- ---------------------
        auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
        pDisp->Add("js", pJC);

        //------------------JSON EVENTS-------------------
        auto pJE=std::make_shared<CJSONEvDispatcher>(pDisp);
        pDisp->Add("je", pJE);
        button.AdviseSink(pJE);
        pMenu->AdviseSink(pJE);
        pMenu->AdviseSink(nodeControl::Instance().shared_from_this());
        nodeControl::Instance().AdviseSink(pJE);

        pZeroCal->AdviseSink(pJE);
        pZeroCal->AdviseSink(pMenu);
        pZeroCal->AdviseSink(nodeControl::Instance().shared_from_this());
        //--------------------------------------------------------------------------------------------------------------

        //setup bridge voltage
        pADmux->SetUBRvoltage(true);


        //blink at start:
        CView::Instance().BlinkAtStart();

        while(1) //endless loop ("super loop")
        {
             //update calproc:
             pZeroCal->Update();


             nodeControl::Instance().Update();

             //update LEDs:
             nodeLED::Update();

             //upd button:
             button.update();

             //upd menu object:
             if( (os::get_tick_mS()-last_time_upd)>=1000 )	//to do: add an event!!!
             {
                     last_time_upd=os::get_tick_mS();
                     pMenu->OnTimer(0);
             }

             pSPIsc2->Update();
        }
}
