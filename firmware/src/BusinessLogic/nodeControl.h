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
#include "ADmux.h"
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

class nodeControl : public CJSONEvCP{
protected:

        /*!
         * \brief A pointer to board's digital multiplexer object
         */
        static std::shared_ptr<CADmux>  m_pMUX;

        /*!
         * \brief A pointer to the controller object of finding amplifier offsets routine.
         */
        static std::shared_ptr<CCalMan> m_pZeroCal;

        /*!
         * \brief A helper function for setting amplifier gain output
         * \param val The gain setpoint
         * \return The gain that was set
         */
        static int gain_out(int val);

        /*!
         * \brief Receives JSON events from other objects
         * \param key An object string name (key)
         * \param val A JSON event
         */


public:
        /*!
         * \brief Returns the reference to the created class object instance. (The object created only once)
         * \return
         */
        static nodeControl& Instance()
        {
            static nodeControl singleton;
            return singleton;
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
         * \brief Persistent storage controller
         */
        CRawBinStorage m_PersistStorage;

        /*!
         * \brief The possible values for IEPE measure modes
         */
        enum MesModes
        {
            IEPE=0,         //!<IEPE mode
            Normsignal,     //!<Normal signal
            Digital         //!<Digital mode
        };
        static MesModes m_OpMode;

        /*!
         * \brief Loads all settings from the persist storage. Should be called once at startup
         */
        void LoadSettings(){m_PersistStorage.Load();}


        /*!
         * \brief Brings all settings to their factory default values
         */
        void SetDefaultSettings(){ m_PersistStorage.SetDefaults(); }


        /*!
         * \brief Binds board's digital multiplexer and controller object of finding amplifier offsets routine
         * to this object
         * \param pMUX A pointer to the board's digital multiplexer
         * \param pZeroCal A pointer to the controller object of finding amplifier offsets routine
         */
        static void SetControlItems(std::shared_ptr<CADmux>  &pMUX, std::shared_ptr<CCalMan> &pZeroCal)
        {
            m_pMUX=pMUX;
            m_pZeroCal=pZeroCal;
            Instance().m_PersistStorage.AddItem(pMUX);
            Instance().m_PersistStorage.AddItem(pZeroCal);
        }


        /*!
         * \brief Creates a new data visualization object
         * \param pADC An ADC channel to bind with the new object
         * \param pLED A LED object to bind with the new object
         */
        static void CreateDataVis(const std::shared_ptr<CAdc> &pADC,  CView::vischan nCh);

        /*!
         * \brief Turns on/off the data visualization process
         * \param bHow true=start data visualization process, false - stop
         * \param nDelay_mS An optional parameter sets the delay before starting the visualization process
         */
        static void StartDataVis(bool bHow, unsigned long nDelay_mS=0);

        /*!
         * \brief Sets a random 32-bit colour value as a new record stamp
         * \param how Currently is not used. Kept just for compatibility with previous versions
         */
        static void StartRecord(const bool how);

        /*!
         * \brief Returns the value that was set by StartRecord
         * \return The value that was set by StartRecord
         * \deprecated Kept just for compatibility with previous versions
         */
        static bool IsRecordStarted();

        /*!
         * \brief Sets the board's amplifier gain.
         * \param val A gain to set
         */
        static void SetGain(const int val)
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
        static int IncGain(const int step)
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
        static int GetGain();

        /*!
         * \brief Sets bridge voltage
         * \param how true=turn bridge voltage ON, false=turn OFF
         */
        static void SetBridge(bool how);

        /*!
         * \brief Returns an actual bridge voltage state
         * \return true=bridge voltage is ON, false=bridge voltage is off
         */
        static bool GetBridge();

        /*!
         * \brief Sets the measurement mode
         * \param nMode: 0 = IEPE; 1 = Normsignal
         */
        static void SetSecondary(int nMode);

        /*!
         * \brief Gets current measurement mode
         * \return 0 = IEPE; 1 = Normsignal
         */
        static int GetSecondary();

        /*!
         * \brief Sets the board opearation mode
         * \param nMode: 0 = IEPE; 1 = Normsignal
         */
        static void SetMode(int nMode);

        /*!
         * \brief Gets current board operation mode
         * \return 0 = IEPE; 1 = Normsignal
         */
        static int GetMode();
	
        /*!
         * \brief Starts/stops finding amplifier offsets procedure
         * \param nOffs 0- stop/reset, 1- negative offset search, 2- zero offset search, 3- positive offset search
         */
        static void SetOffset(int nOffs);

        /*!
         * \brief Enables board ADC measurements
         * \param how true=enable,false=disable
         */
        static void EnableMeasurements(bool how)
        {
            m_pMUX->EnableADmes(how);
            CView::Instance().SetButtonHeartbeat(how);
        }

        /*!
         * \brief Returns board ADC measurements enabled flag
         * \return true=enabled, false=disabled
         */
        static bool IsMeasurementsEnabled()
        {
            return m_pMUX->IsADmesEnabled();
        }


        /*!
         * \brief Returns current finding amplifier offsets procedure state
         * \return true=the procedure is running, false=the procedure is finished
         */
        inline static int GetOffsetRunSt(){ return m_pZeroCal->IsStarted(); }

        /*!
         * \brief Returns board calibration status
         * \return true=board's EEPROM contains valid calibration data, false=board is not calibrated
         */
        inline static bool GetCalStatus(){ return false;}

protected:
        /*!
         * \brief Holds Voltage Setting (mockup)
         */
        static  float m_Voltage;

        /*!
         * \brief Holds Current Setting (mockup)
         */
        static  float m_Current;

        /*!
         * \brief Holds MaxCurrent Setting (mockup)
         */
        static  float m_MaxCurrent;  //mA

public:
        /*!
         * \brief Sets Voltage Setting
         * \param val - the voltage to set
         */
        static void SetVoltage(float val){m_Voltage=val;}

        /*!
         * \brief Returns actual Voltage Setting
         * \return the Voltage Setting
         */
        static float GetVoltage(){return m_Voltage;}

        /*!
         * \brief Sets Current Setting
         * \param val - the current to set
         */
        static void SetCurrent(float val){ if(val<0) val=0; if(val>m_MaxCurrent) val=m_MaxCurrent; m_Current=val;}

        /*!
         * \brief Returns actual Current Setting
         * \return the Current Setting
         */
        static float GetCurrent(){ return m_Current; }

        /*!
         * \brief Sets MaxCurrent (current limiter) Setting
         * \param val - the current value to set
         */
        static void SetMaxCurrent(float val){if(val<0) val=0; m_MaxCurrent=val; }

        /*!
         * \brief Returns actual MaxCurrent Setting
         * \return the MaxCurrent Setting
         */
        static float GetMaxCurrent(){return m_MaxCurrent; }

        inline void StartFan(bool bHow){ m_pMUX->StartFan(bHow); }

        /*!
         * \brief The object state update method
         * \details Gets the CPU time to update internal state of the object.
         *  Must be called from a "super loop" or from corresponding thread
         */
        static void Update();
};
