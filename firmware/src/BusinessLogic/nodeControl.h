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

/*!
 * \brief Provides the basic functionality of the board
 * \details The class can be considered as somewhat usually called "controller" in MCV pattern.
 * The only one controller object can exist. This is a "singleton" class
 * The class object has an ability for receiving JSON events from other objects and generate own JSON events
 * when basic board settings are changed
 */

class nodeControl : public std::enable_shared_from_this<nodeControl>, public CJSONEvCP,  public IJSONEvent{
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
        virtual void on_event(const char *key, nlohmann::json &val);

public:
        /*!
         * \brief Returns the reference to the created class object instance. (The object created only once)
         * \return
         */
        static nodeControl& Instance()
        {
           static std::shared_ptr<nodeControl> ptr(new nodeControl);
           return *ptr;
        }
private:
        //! Forbid creating other instances of the class object
        nodeControl() {}

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
            Normsignal      //!<Normal signal
        };

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
        }


        /*!
         * \brief Creates a new data visualization object
         * \param pADC An ADC channel to bind with the new object
         * \param pLED A LED object to bind with the new object
         */
        static void CreateDataVis(const std::shared_ptr<CAdc> &pADC, const std::shared_ptr<CLED> &pLED);

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
         * \brief Starts/stops finding amplifier offsets procedure
         * \param how true=start the procedure, false=stop
         */
        static void SetZero(bool how);

        /*!
         * \brief Returns current finding amplifier offsets procedure state
         * \return true=the procedure is running, false=the procedure is finished
         */
        inline static bool GetZeroRunSt(){ return m_pZeroCal->IsStarted(); }

        /*!
         * \brief The object state update method
         * \details Gets the CPU time to update internal state of the object.
         *  Must be called from a "super loop" or from corresponding thread
         */
        static void Update();
};
