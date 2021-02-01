/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   typeSamADC, typeSamADCmuxpos, typeSamADCmuxneg, CSamADCchan, CSamADCcntr
*/


#pragma once
#include  <memory>
#include <list>

#include "os.h"
#include "ADC.h"
#include "SamCLK.h"

/*!
 * \brief An enumeration of possible SAME54 ADC devices
 */
enum class typeSamADC{Adc0, Adc1};

/*!
 * \brief An enumeration of possible positive ADC inputs (manual's page 1638)
 */
enum class typeSamADCmuxpos : int {AIN0=0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7, SCALEDCOREVCC=0x18, PTAT=0x1C, CTAT=0x1D};

/*!
 * \brief An enumeration of possible negative ADC inputs (manual's page 1637)
 */
enum class typeSamADCmuxneg : int {none=-1, AIN0=0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7};


class CSamADCcntr;

/*!
 * \brief An implementation of SAME54 ADC channel class
 * \details The class object should be used in conjunction with CSamADCcntr - an ADC "board" virtual device
 *  that holds a collection of channels and can poll them in a queue
 */
class CSamADCchan : public CAdc
{
friend class CSamADCcntr;
protected:

    /*!
     * \brief A pointer to the ADC board (channel container)
     */
    std::shared_ptr<CSamADCcntr> m_pCont;

    /*!
     * \brief A positive input for this channel
     */
    typeSamADCmuxpos m_posIN;

    /*!
     * \brief A negative input for this channel (can be none=only positive mode)
     */
    typeSamADCmuxneg m_negIN;

    /*!
     * \brief A time stamp when last ADC conversion was made
     */
    unsigned long m_MesTStamp=os::get_tick_mS();

    /*!
     * \brief A filtered raw binary value of last ADC convertion
     */
    float         m_FilteredRawVal=0;

    /*!
     * \brief Unfiltered raw binary value of last ADC conversion
     */
    int           m_UnfilteredRawVal=0;

    /*!
     * \brief A 1st order digital filter time constant, milliseconds
     */
    float         m_filter_t_mSec=50.0f;

    /*!
     * \brief Returns the age of last ADC conversion.
     * \return The age of last ADC conversion, milliseconds
     */
    unsigned long data_age(){ return (os::get_tick_mS()-m_MesTStamp); }

    /*!
     * \brief Overrides the method ADchan::SetRawBinVal(...)
     * \param A conversion result to be set
     */
    void SetRawBinVal(int RawVal);


public:
    /*!
     * \brief The class constructor
     * \param pCont A pointer to the ADC board (channel container)
     * \param posIN A positive input for this channel
     * \param negIN A negative input for this channel (can be none=only positive mode)
     * \param RangeMin Minimum range in the real measurement units (V, A, etc)
     * \param RangeMax Maximum range in the real measurement units (V, A, etc)
     * \param AutoUpd auto updation flag: true=channel will be updated(measured) by the container Update function, false=channel is responsible for its updation
     */
    CSamADCchan(std::shared_ptr<CSamADCcntr> &pCont, typeSamADCmuxpos posIN, typeSamADCmuxneg negIN, float RangeMin, float RangeMax, bool bAutoUpd=true);

    /*!
     * \brief The class virtual destructor
     */
    virtual ~CSamADCchan();

    /*!
     * \brief Override "DirectMeasure" methode
     * \return
     */
    virtual int DirectMeasure()
    {
        return CSamADCchan::DirectMeasure(50, 0.8f);
    }

    /*!
     * \brief The own implementation of "Direct Measure"
     * \param nMesCnt A number of samples to average
     * \param alpha A weight factor
     * \return An averaged ADC raw binary conversion result.
     * \details The averaging method used for the methode is: Result=alpha*Result +(1.0f-alpha)*ADC_conversion_result
     */
    int DirectMeasure(int nMesCnt, float alpha);
};

/*!
 * \brief A virtual "ADC board" class.
 * \details The class object holds a collection of channels and can poll them in a queue using SAME54 AdcX facility.
 * Also it is possible to perform a "Direct measure" operation for a single channel avoid queueing.
 */
 class CSamADCcntr
{
friend class CSamADCchan;
protected:

    /*!
     * \brief SAME54's real ADC index using for measurements
     */
	typeSamADC m_nADC;

    /*!
     * \brief A collection of channels
     */
    std::list<CSamADCchan *> m_Chans;

    /*!
     * \brief An associated clock generator: must be provided to perform conversions
     */
    std::shared_ptr<CSamCLK> m_pCLK;
	
public:
    /*!
     * \brief The class constructor
     * \param nADC SAME54's real ADC index using for measurements
     * \details The constructor does the following:
     * 1) setups corresponding PINs and its multiplexing
     * 2) enables communication bus with SAME54 ADC
     * 3) loads calibration settings from the NVMscpage
     * 4) connects available clock generator via CSom CLK service
     * 5) performs final tuning and enables SAME54 ADC
     */
	CSamADCcntr(typeSamADC nADC);

    /*!
     * \brief Selects 2 analog inputs for subsequent ADC conversion operations on them via CSamADCcntr::SingleConv()
     * \param nPos A positive input for this channel
     * \param nNeg A negative input for this channel (can be none=only positive mode)
     */
    void SelectInput(typeSamADCmuxpos nPos, typeSamADCmuxneg nNeg=typeSamADCmuxneg::none);

    /*!
     * \brief Performs a single conversion operation for selected input pair.
     * \return An ADC conversion result
     */
    short SingleConv();

    /*!
     * \brief The object state update method.
     * \return true=update is successful
     * \details Gets the CPU time to update internal state of the object.
     *  Polling each ADC channel from the collection by performing an ADC conversion for it in a queue
     *  Must be called from a "super loop" or from corresponding thread
     */
    bool Update();
};
