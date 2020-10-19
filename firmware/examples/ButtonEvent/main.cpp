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
#include "SamI2Cmem.h"  //19.08.2019
#include "ADmux.h"
#include "SamADCcntr.h"
#include "SamDACcntr.h"
#include "DACmax5715.h"

//#include "menu_logic.h"
#include "button_logic.h" //another controller 15.10.2019
#include "SAMbutton.h"
#include "nodeLED.h"
//#include "zerocal_man.h"

#include "cmd.h"
#include "std_port.h"
#include "jsondisp.h"


//GPIO 24 pull-up:
/*const char PandaHat[]=
{
0x52, 0x2D, 0x50, 0x69, 0x01, 0x00, 0x02, 0x00, 0x6D, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
0x31, 0x00, 0x00, 0x00, 0xDF, 0x5D, 0x24, 0xB1, 0x59, 0xC3, 0xE3, 0x99, 0x75, 0x45, 0xD9, 0x6A,
0x95, 0x28, 0xD2, 0x6C, 0x00, 0x00, 0x01, 0x00, 0x0A, 0x0F, 0x50, 0x61, 0x6E, 0x64, 0x61, 0x20,
0x47, 0x6D, 0x42, 0x48, 0x54, 0x69, 0x6D, 0x65, 0x53, 0x77, 0x69, 0x70, 0x65, 0x20, 0x42, 0x6F,
0x61, 0x72, 0x64, 0xCD, 0xD2, 0x02, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA0, 0x00, 0x00, 0x00, 0xCF, 0x6E
};*/

int sys_clock_init(void);

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
        //nodeLED::blinkLED(typeLED::LED1, CMenuLogic::RESET_COLOR);

        //step 1 - creating QSPI bus:
        CSamQSPI objQSPI;

        //10.05.2019: creating SC2 SPI:
      //  pSPIsc7    =std::make_shared<CSamSPIsc7>(true); //it will be a master 03.06.2019
        auto pSPIsc2    =std::make_shared<CSamSPIsc2>();
        pSPIsc2->EnableIRQs(true);  //no working in IRQ mode....


         //step 2 - creating ADC0:
#ifndef  USE_AD_CUSTOM_RANGES   //if not using custom ranges range is absolute 17.06.2019
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
   /*     auto pZeroCal=std::make_shared<CCalMan>();
        pZeroCal->Add(pADC1, pDACA, pLED1);
        pZeroCal->Add(pADC2, pDACB, pLED2);
        pZeroCal->Add(pADC3, pDACC, pLED3);
        pZeroCal->Add(pADC4, pDACD, pLED4);

        nodeControl::SetControlItems(pADmux, pZeroCal);


        //17.07.2019: DataVis:
        nodeControl::CreateDataVis(pADC1, pLED1);
        nodeControl::CreateDataVis(pADC2, pLED2);
        nodeControl::CreateDataVis(pADC3, pLED3);
        nodeControl::CreateDataVis(pADC4, pLED4);

        //18.07.2019: preparing and start:
        pDACA->SetRawOutput(2048);
        pDACB->SetRawOutput(2048);
        pDACC->SetRawOutput(2048);
        pDACD->SetRawOutput(2048);
        nodeControl::StartDataVis(true, 1000);*/



        //---------------------------11.05.2019: some commands:---------------------------------------------------------
        auto pDisp=         std::make_shared<CCmdDispatcher>();
        auto pStdPort=      std::make_shared<CStdPort>(pDisp, pSPIsc2);
        pSPIsc2->AdviseSink(pStdPort);

       // auto pStdPort2=      std::make_shared<CStdPort>(pDisp, pSPIsc7);
       // pSPIsc7->AdviseSink(pStdPort2);

        //DACs control:
        pDisp->Add("DACA", std::make_shared< CCmdSGHandler<CDac, float> >(pDACA, &CDac::GetRealVal, &CDac::SetVal ) );
        pDisp->Add("DACB", std::make_shared< CCmdSGHandler<CDac, float> >(pDACB, &CDac::GetRealVal, &CDac::SetVal ) );
        pDisp->Add("DACC", std::make_shared< CCmdSGHandler<CDac, float> >(pDACC, &CDac::GetRealVal, &CDac::SetVal ) );
        pDisp->Add("DACD", std::make_shared< CCmdSGHandler<CDac, float> >(pDACD, &CDac::GetRealVal, &CDac::SetVal ) );

        //22.05.2019 DACs raw control:
        pDisp->Add("DACA.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACA, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DACB.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACB, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DACC.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACC, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DACD.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pDACD, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );

        //27.05.2019: SAMs dac control:
        pDisp->Add("DAC0.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pSamDAC0, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );
        pDisp->Add("DAC1.raw", std::make_shared< CCmdSGHandler<CDac, int> >(pSamDAC1, &CDac::GetRawBinVal, &CDac::SetRawOutput ) );


        //ADC direct: 27.05.2019:
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
   /*     pDisp->Add("Gain", std::make_shared< CCmdSGHandlerF<int> >(&nodeControl::GetGain, &nodeControl::SetGain) );
        pDisp->Add("Bridge", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::GetBridge,  &nodeControl::SetBridge) );
        pDisp->Add("Record", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::IsRecordStarted,  &nodeControl::StartRecord) );
        pDisp->Add("Zero", std::make_shared< CCmdSGHandlerF<bool> >(&nodeControl::GetZeroRunSt,  &nodeControl::SetZero) );

        //15.07.2019 adding m_TargErrTolerance getter/setter:
        pDisp->Add("Zero.errtol", std::make_shared< CCmdSGHandlerF<int> >(&CADpointSearch::GetTargErrTol,  &CADpointSearch::SetTargErrTol) );*/


        //ADMUX:
        pDisp->Add("EnableADmes", std::make_shared< CCmdSGHandler<CADmux, bool> >(pADmux,  &CADmux::IsADmesEnabled,  &CADmux::EnableADmes) ); //18.07.2019 - a getter is added
        //pDisp->Add("DACmode", std::make_shared< CCmdSGHandler<CADmux, int> >(pADmux, nullptr,  &CADmux::SetDACmode) );

        //27.05.2019:
        pDisp->Add("DACsw", std::make_shared< CCmdSGHandler<CADmux, int> >(pADmux, &CADmux::getDACsw,  &CADmux::setDACsw) );
        //pDisp->Add("EE.ind", std::make_shared< CCmdSGHandler<CSamI2Cmem, unsigned int> >(pEEPROM, &CSamI2Cmem::GetCurMemInd, &CSamI2Cmem::SetCurMemInd ) );


        //--------------------menu+button-----------------------
        auto pMenu=std::make_shared<CButtonLogic>();
        //SAMButton button;
        //button.AdviseSink(pMenu);

        SAMButton::Instance().AdviseSink(pMenu);

        //------------------JSON 10.06.2019---------------------
        auto pJC=std::make_shared<CJSONDispatcher>(pDisp);
        pDisp->Add("js", pJC);

        //------------------EVENTS 14.06.2019-------------------
        auto pJE=std::make_shared<CJSONEvDispatcher>(pDisp); //14.08.2019 - removed event pin
        pDisp->Add("je", pJE);
   //     button.AdviseSink(pJE);
        pMenu->AdviseSink(pJE);
        /*pMenu->AdviseSink(nodeControl::Instance().shared_from_this()); //18.07.2019
        nodeControl::Instance().AdviseSink(pJE);

        pZeroCal->AdviseSink(pJE);
        pZeroCal->AdviseSink(pMenu);
        pZeroCal->AdviseSink(nodeControl::Instance().shared_from_this()); //18.07.2019*/
        //--------------------------------------------------------------------------------------------------------------


        while(1) //endless loop
        {
             //update calproc:
             //pZeroCal->Update();

             //17.07.2019: update control
             //nodeControl::Instance().Update();

             //update LEDs:
             nodeLED::Update();

             //upd button:
             //button.update();
             SAMButton::Instance().update();

             pSPIsc2->Update();
        }
}
