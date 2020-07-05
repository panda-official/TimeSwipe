/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for
*   nodeControl
*/

#include <memory>
#include "BaseMesChannel.h"
#include "zerocal_man.h"
#include "json_evsys.h"
#include "RawBinStorage.h"


/*!
 * \brief Provides the basic functionality of the board
 * \details The class can be considered as somewhat usually called "controller"
 * The only one controller object can exist. This is a "singleton" class
 * The class object has an ability for receiving JSON events from other objects and generate own JSON events
 * when basic board settings are changed
 */

class nodeControl : public CJSONEvCP, public ISerialize, public std::enable_shared_from_this<nodeControl>{
protected:
        typeBoard m_BoardType=typeBoard::IEPEBoard;


        std::shared_ptr<CPin> m_pUBRswitch;
        std::shared_ptr<CPin> m_pDACon;
        std::shared_ptr<CPin> m_pEnableMes;
        std::shared_ptr<CPin> m_pFanOn;

        std::shared_ptr<CPin> m_pGain0pin;
        std::shared_ptr<CPin> m_pGain1pin;

        std::shared_ptr<CDac> m_pVoltageDAC;

        CCalMan m_OffsetSearch;

        std::vector<std::shared_ptr<CMesChannel>>  m_pMesChans;

        /*!
         * \brief Persistent storage controller
         */
        bool           m_bSettingsLoaded=false;
        CRawBinStorage m_PersistStorage;

        int m_GainSetting=0;

        bool m_BridgeSetting=false;
        int  m_SecondarySetting=0;

        /*!
         * \brief Holds Current Setting (mockup)
         */
        float m_Current;

        /*!
         * \brief Holds MaxCurrent Setting (mockup)
         */
        float m_MaxCurrent=1000.0f;  //mA


        /*!
         * \brief A helper function for setting amplifier gain output
         * \param val The gain setpoint
         * \return The gain that was set
         */
        int gain_out(int val);

public:
        /*!
         * \brief Provides the serialization of the object content
         * \param st A reference to the storage from which the object content is downloading or uploading to
         */
         virtual void Serialize(CStorage &st);

        /*!
         * \brief Returns the reference to the created class object instance. (The object created only once)
         * \return
         */
        static nodeControl& Instance()
        {
            //static nodeControl singleton;
            //return singleton;
           static std::shared_ptr<nodeControl> pThis(new nodeControl);
           return *pThis;

        }
private:
        //! Forbid creating other instances of the class object
        nodeControl();

        //! Forbid copy constructor
        nodeControl(const nodeControl&)=delete;

        //! Forbid copying
        nodeControl& operator=(const nodeControl&)=delete;
	
public:
        /*!
         * \brief The possible values for IEPE measure modes
         */
        enum MesModes
        {
            IEPE=0,         //!<IEPE mode
            Normsignal,     //!<Normal signal
            Digital         //!<Digital mode
        };
protected:
        MesModes m_OpMode=nodeControl::IEPE;

public:
        inline void SetBoardType(typeBoard BoardType){
            m_BoardType=BoardType;
        }


        inline void SetUBRpin(const std::shared_ptr<CPin> &pUBRswitch){
            m_pUBRswitch=pUBRswitch;
        }
        inline void SetDAConPin(const std::shared_ptr<CPin> &pDACon){
            m_pDACon=pDACon;
        }
        inline void SetEnableMesPin(const std::shared_ptr<CPin> &pEnableMes){
            m_pEnableMes=pEnableMes;
        }
        inline void SetFanPin(const std::shared_ptr<CPin> &pFanOn){
            m_pFanOn=pFanOn;
        }
        inline void SetIEPEboardGainSwitches(const std::shared_ptr<CPin> &pGain0pin, const std::shared_ptr<CPin> &pGain1pin){
            m_pGain0pin=pGain0pin;
            m_pGain1pin=pGain1pin;
        }
        inline void SetVoltageDAC(const std::shared_ptr<CDac> &pVoltageDAC){
            m_pVoltageDAC=pVoltageDAC;
        }


        void AddMesChannel(const std::shared_ptr<CMesChannel> &pChan)
        {
            m_pMesChans.emplace_back(pChan);
            m_OffsetSearch.Add(pChan->m_pADC, pChan->m_pDAC, pChan->m_VisChan.GetVisChannel());
        }

        std::shared_ptr<CMesChannel> GetMesChannel(size_t nCh)
        {
            if(nCh<m_pMesChans.size())
                return m_pMesChans[nCh];
            else
                return nullptr;
        }


        /*!
         * \brief Loads all settings from the persist storage. Should be called once at startup
         */
        void LoadSettings(){

            if(!m_bSettingsLoaded)
            {
                m_PersistStorage.AddItem(this->shared_from_this());
                m_PersistStorage.Load();
                m_bSettingsLoaded=true;
            }
        }


        /*!
         * \brief Brings all settings to their factory default values
         */
        void SetDefaultSettings(){ m_PersistStorage.SetDefaults(); }


        /*!
         * \brief Sets a random 32-bit colour value as a new record stamp
         * \param how Currently is not used. Kept just for compatibility with previous versions
         */
        void StartRecord(bool how);

        /*!
         * \brief Returns the value that was set by StartRecord
         * \return The value that was set by StartRecord
         * \deprecated Kept just for compatibility with previous versions
         */
        bool IsRecordStarted(){

            return false;
        }

        /*!
         * \brief Sets the board's amplifier gain.
         * \param val A gain to set
         */
        void SetGain(int val)
        {
            int outp=val;
            if(outp<1)
                outp=1;
            if(outp>4)
                outp=4;
            gain_out(outp);
        }

        /*!
         * \brief Increments the board's amplifier gain.
         * \param step An incrementation step
         * \return The gain that was set
         */
        int IncGain(int step)
        {
            int outp=GetGain()+step;
            if(outp>4)
                outp=1;
			
            return gain_out(outp);
        }

        /*!
         * \brief Returns an actual gain setpoint
         * \return An actual gain setpoint
         */
        int GetGain(){

            return m_GainSetting;
        }

        /*!
         * \brief Sets bridge voltage
         * \param how true=turn bridge voltage ON, false=turn OFF
         */
        void SetBridge(bool how);

        /*!
         * \brief Returns an actual bridge voltage state
         * \return true=bridge voltage is ON, false=bridge voltage is off
         */
        bool GetBridge();

        /*!
         * \brief Sets the measurement mode
         * \param nMode: 0 = IEPE; 1 = Normsignal
         */
        void SetSecondary(int nMode);

        /*!
         * \brief Gets current measurement mode
         * \return 0 = IEPE; 1 = Normsignal
         */
        int GetSecondary();

        /*!
         * \brief Sets the board opearation mode
         * \param nMode: 0 = IEPE; 1 = Normsignal
         */
        void SetMode(int nMode);

        /*!
         * \brief Gets current board operation mode
         * \return 0 = IEPE; 1 = Normsignal
         */
        int GetMode();
	
        /*!
         * \brief Starts/stops finding amplifier offsets procedure
         * \param nOffs 0- stop/reset, 1- negative offset search, 2- zero offset search, 3- positive offset search
         */
        void SetOffset(int nOffs);

        /*!
         * \brief Enables board ADC measurements
         * \param how true=enable,false=disable
         */
        void EnableMeasurements(bool bHow)
        {
            assert(m_pEnableMes);
            m_pEnableMes->Set(bHow);
            CView::Instance().SetButtonHeartbeat(bHow);
        }

        /*!
         * \brief Returns board ADC measurements enabled flag
         * \return true=enabled, false=disabled
         */
        bool IsMeasurementsEnabled()
        {
            assert(m_pEnableMes);
            return m_pEnableMes->RbSet();
        }


        /*!
         * \brief Returns current finding amplifier offsets procedure state
         * \return true=the procedure is running, false=the procedure is finished
         */
        inline int GetOffsetRunSt()
        {
            return m_OffsetSearch.IsStarted();
        }

        /*!
         * \brief Returns board calibration status
         * \return true=board's EEPROM contains valid calibration data, false=board is not calibrated
         */
        inline bool GetCalStatus(){ return false;}

        inline void StartFan(bool bHow)
        {
            assert(m_pFanOn);
            m_pFanOn->Set(bHow);
        }
        inline bool IsFanStarted()
        {
            assert(m_pFanOn);
            return m_pFanOn->RbSet();
        }

        /*!
         * \brief Sets Voltage Setting
         * \param val - the voltage to set
         */
        void SetVoltage(float val)
        {
            assert(m_pVoltageDAC);
            return m_pVoltageDAC->SetVal(val);
        }

        /*!
         * \brief Returns actual Voltage Setting
         * \return the Voltage Setting
         */
        float GetVoltage()
        {
            assert(m_pVoltageDAC);
            return m_pVoltageDAC->GetRealVal();
        }

        /*!
         * \brief Sets Current Setting
         * \param val - the current to set
         */
        void SetCurrent(float val){ if(val<0) val=0; if(val>m_MaxCurrent) val=m_MaxCurrent; m_Current=val;}

        /*!
         * \brief Returns actual Current Setting
         * \return the Current Setting
         */
        float GetCurrent(){ return m_Current; }

        /*!
         * \brief Sets MaxCurrent (current limiter) Setting
         * \param val - the current value to set
         */
        void SetMaxCurrent(float val){if(val<0) val=0; m_MaxCurrent=val; }

        /*!
         * \brief Returns actual MaxCurrent Setting
         * \return the MaxCurrent Setting
         */
        float GetMaxCurrent(){return m_MaxCurrent; }

        /*!
         * \brief The object state update method
         * \details Gets the CPU time to update internal state of the object.
         *  Must be called from a "super loop" or from corresponding thread
         */
        void Update();
};
