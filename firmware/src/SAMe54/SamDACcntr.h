/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/
/*!
*   \file
*   \brief A definition file for
*   typeSamDAC, CSamDACcntr
*/

#pragma once

/*!
 * \brief An enumeration of possible SAME54 DAC devices
 */
enum class typeSamDAC{Dac0, Dac1};


#include "DAC.h"
#include "SamCLK.h"

/*!
 * \brief The class implements a single SAME54 DAC channel
 * \details The control functionality of a channel is implemented directly via
 *  overridden CDac::DriverSetVal().
 */
class CSamDACcntr : public CDac
{
protected:

    /*!
     * \brief The channel ID
     */
    typeSamDAC m_chan;

    /*!
     * \brief The DAC subsystem is initialized.
     * \details SAME54 DACs share a lot of common components: like one clock generator used for both DACs
     * Thus the common components have to be initialized only once
     */
    static bool       m_bInitialized;

    /*!
     * \brief An associated clock generator: must be provided to perform conversions
     */
    std::shared_ptr<CSamCLK> m_pCLK;


    /*!
     * \brief Initializes the DAC subsystem
     */
    void common_init();

    /*!
     * \brief Override of CDac::DriverSetVal(...) for the class
     * \param val Ignored
     * \param out_bin A value to set in a raw-binary format
     */
    virtual void DriverSetVal(float val, int out_bin);

public:
    /*!
     * \brief The class constructor
     * \param nChan The channel ID(index) of SAME54 DAC
     * \param RangeMin Minimum range in the real units (V, A, etc)
     * \param RangeMax Maximum range in the real measurement units (V, A, etc)
     * \details The constructor does the following:
     * 1) setups corresponding PINs and its multiplexing
     * 2) enables communication bus with SAME54 DACs
     * 3) connects available clock generator via CSom CLK service
     * 4) performs final tuning and enables the DAC
     */
    CSamDACcntr(typeSamDAC nChan, float RangeMin, float RangeMax);
};
