/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include "../os.h"
#include "adcdac.hpp"
#include "clock_generator.hpp"

#include <list>
#include <memory>

/// A possible SAME5x ADC device.
enum class typeSamADC { Adc0, Adc1 };

/**
 * @brief A possible positive ADC inputs.
 *
 * @see Manual at page 1638.
 */
enum class typeSamADCmuxpos {
  AIN0,
  AIN1,
  AIN2,
  AIN3,
  AIN4,
  AIN5,
  AIN6,
  AIN7,
  SCALEDCOREVCC = 0x18,
  PTAT = 0x1C,
  CTAT = 0x1D
};

/**
 * @brief A possible negative ADC inputs.
 *
 * @see Manual at page 1637.
 */
enum class typeSamADCmuxneg {
  none = -1,
  AIN0,
  AIN1,
  AIN2,
  AIN3,
  AIN4,
  AIN5,
  AIN6,
  AIN7
};

class CSamADCcntr;

/**
 * @brief An SAME5x ADC channel.
 *
 * @details The class object should be used in conjunction with CSamADCcntr - an
 * ADC "board" virtual device that holds a collection of channels and can poll them.
 */
class CSamADCchan final : public Adc_channel {
private:
  friend CSamADCcntr;

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
    float filtered_raw_{};

    /*!
     * \brief Unfiltered raw binary value of last ADC conversion
     */
    // int           m_UnfilteredRawVal=0;

    /*!
     * \brief A 1st order digital filter time constant, milliseconds
     */
    constexpr static float filter_time_ms_{50.0f};

    /*!
     * \brief Returns the age of last ADC conversion.
     * \return The age of last ADC conversion, milliseconds
     */
    unsigned long data_age(){ return (os::get_tick_mS()-m_MesTStamp); }

    void handle_measurement(short raw);

public:
    /*!
     * \brief The class constructor
     * \param pCont A pointer to the ADC board (channel container)
     * \param posIN A positive input for this channel
     * \param negIN A negative input for this channel (can be none=only positive mode)
     * \param AutoUpd auto updation flag: true=channel will be updated(measured) by the container Update function, false=channel is responsible for its updation
     */
    CSamADCchan(std::shared_ptr<CSamADCcntr> &pCont, typeSamADCmuxpos posIN, typeSamADCmuxneg negIN, bool bAutoUpd=true);

    /*!
     * \brief The class virtual destructor
     */
    virtual ~CSamADCchan();

    /// @see Adcdac_channel::GetRawBinVal().
    int GetRawBinVal() const noexcept override
    {
      return filtered_raw_;
    }

    /// @see Adc_channel::GetRawBinValDirectly().
    int GetRawBinValDirectly() const noexcept override
    {
      return DirectMeasure(50, 0.8f);
    }

    /*!
     * \brief The own implementation of "Direct Measure"
     * \param nMesCnt A number of samples to average
     * \param alpha A weight factor
     * \return An averaged ADC raw binary conversion result.
     * \details The averaging method used for the methode is: Result=alpha*Result +(1.0f-alpha)*ADC_conversion_result
     */
    int DirectMeasure(int nMesCnt, float alpha) const noexcept;
};

/**
 * @brief A virtual "ADC board" class.
 *
 * @details The class object holds a collection of channels and can poll them by
 * using SAME5x AdcX facility. Also it is possible to perform a "Direct measure"
 * operation for a single channel to avoid queueing.
 */
class CSamADCcntr final {
  friend class CSamADCchan;
protected:
    /// SAME5x real ADC index using for measurements.
    typeSamADC m_nADC;

    /// A list of channels.
    std::list<CSamADCchan *> m_Chans;

    /// An associated clock generator: must be provided to perform conversions.
    std::shared_ptr<Sam_clock_generator> m_pCLK;

public:
    /**
     * @brief The constructor.
     *
     * @details The constructor does the following:
     *   -# setups corresponding PINs and its multiplexing;
     *   -# enables communication bus with SAME5x ADC;
     *   -# loads calibration settings from the NVMscpage;
     *   -# connects available clock generator via CSom CLK service;
     *   -# performs final tuning and enables SAME54 ADC.
     *
     * @param nADC SAME54's real ADC index using for measurements
     */
    CSamADCcntr(typeSamADC nADC);

    /**
     * @brief Selects 2 analog inputs for subsequent ADC conversion operations
     * on them via CSamADCcntr::SingleConv().
     *
     * @param nPos A positive input for this channel.
     * @param nNeg A negative input for this channel (can be none=only positive mode).
     */
    void SelectInput(typeSamADCmuxpos nPos,
      typeSamADCmuxneg nNeg=typeSamADCmuxneg::none);

    /**
     * @brief Performs a single conversion operation for selected input pair.
     *
     * @returns An ADC conversion result.
     */
    short SingleConv();

    /**
     * @brief The object state update method.
     *
     * @returns `true` on success.
     *
     * @details Gets the CPU time to update the internal state of the object.
     * Traverses the list of ADC channels and polls them by performing an ADC
     * conversion. Must be called from a "super loop" or from the corresponding
     * thread.
     */
    bool Update();
};
