/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for Channel class aka CIEPEchannel
*   CDac5715sa
*
*/

/*!
*   \file
*   \brief A definition file for CDMSchannel
*/


#pragma once

#include "channel.hpp"
#include "pga280.hpp"
#include "shiftreg.hpp"

/// The DMS measurement channel.
class CDMSchannel final : public Channel {
public:
  bool is_iepe() const noexcept
  {
    return is_iepe_;
  }

  void set_iepe(const bool value) override
  {
    is_iepe_ = value;
    iepe_switch_->write(value);
  }

  Measurement_mode measurement_mode() const noexcept override
  {
    return measurement_mode_;
  }

  void set_measurement_mode(const Measurement_mode mode) override
  {
    measurement_mode_ = mode;
    pga_->SetMode(static_cast<CPGA280::mode>(mode));
    update_offsets();
  }

  float amplification_gain() const noexcept override
  {
    return amplification_gain_;
  }

  void set_amplification_gain(float GainValue) override;

  int channel_index() const noexcept override
  {
    return channel_index_;
  }

  const CDataVis& visualization_index() const noexcept override
  {
    return visualization_index_;
  }

  CDataVis& visualization_index() noexcept override
  {
    return visualization_index_;
  }

  bool is_visualization_enabled() const noexcept override
  {
    return is_visualization_enabled_;
  }

  std::shared_ptr<const CAdc> adc() const noexcept override
  {
    return adc_;
  }

  std::shared_ptr<CAdc> adc() noexcept
  {
    return adc_;
  }

  std::shared_ptr<const CDac> dac() const noexcept
  {
    return dac_;
  }

  std::shared_ptr<CDac> dac() noexcept
  {
    return dac_;
  }

  void update_offsets() override;

    /*!
     * \brief The class constructor
     * \param pADC - the pointer to the channel's ADC
     * \param pDAC - the pointer to channel's offset control DAC
     * \param nCh  - the visualization index of the channel
     * \param pIEPEswitch - the pointer to the IEPE switch pin
     * \param pPGA - the pointer to the PGA280 amplifier control instance
     * \param bVisEnabled - The visualisation enable flag
     */
  CDMSchannel(const int channel_index,
    const std::shared_ptr<CAdc>& adc,
    const std::shared_ptr<CDac>& dac,
    const CView::vischan visualization_index,
    const std::shared_ptr<Pin>& pIEPEswitch,
    const std::shared_ptr<CPGA280>& pPGA,
    const bool is_visualization_enabled)
    : channel_index_{channel_index}
    , visualization_index_{visualization_index}
    , is_visualization_enabled_{is_visualization_enabled}
    , adc_{adc}
    , dac_{dac}
    , iepe_switch_{pIEPEswitch}
    , pga_{pPGA}
    {}

private:
  bool is_iepe_{};
  Measurement_mode measurement_mode_{Measurement_mode::voltage};
  float amplification_gain_{1};
  std::size_t gain_index_{};
  int channel_index_{-1};

  CDataVis visualization_index_;
  bool is_visualization_enabled_{};

  std::shared_ptr<CAdc> adc_;
  std::shared_ptr<CDac> dac_;

  /// The pointer to the IEPE switch pin.
  std::shared_ptr<Pin> iepe_switch_;

  /// The pointer to the PGA280 amplifier control instance.
  std::shared_ptr<CPGA280> pga_;
};
