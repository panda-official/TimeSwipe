/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for CMesChannel class aka CIEPEchannel
*/

#pragma once

#include "../adcdac.hpp"
#include "../control/DataVis.h"
#include "../pin.hpp"

#include <memory>

class nodeControl;

/*!
 * \brief The basic class representing board measurement channel functionality
 * \details Defines the basic interface of the board measurement channel.
 *  Must be overriden in the concrete implementation of the measurement channel for IEPE and DMS boards
 */
class CMesChannel {
public:

    /*!
     * \brief The possible measurement modes
     */
    enum mes_mode{

        Voltage=0,  //!<Voltage mode
        Current     //!<Current mode
    };


  /// @returns The measurement mode.
  virtual mes_mode measurement_mode() const noexcept = 0;

  /// Sets The measurement mode.
  virtual void set_measurement_mode(mes_mode mode) = 0;

  /// @returns IEPE mode indicator.
  virtual bool is_iepe() const noexcept = 0;

  /// Sets IEPE mode indicator.
  virtual void set_iepe(bool enable) = 0;

  /// @returns The amplification gain.
  virtual float amplification_gain() const noexcept = 0;

  /// Sets the amplification gain.
  virtual void set_amplification_gain(float gain) = 0;

  /// @returns The zero-based channel index.
  virtual int channel_index() const noexcept = 0;

  /**
   * @brief The visualization index of the channel.
   *
   * @details Used to bind the channel with the visualization LED.
   */
  virtual const CDataVis& visualization_index() const noexcept = 0;

  /// @overload
  virtual CDataVis& visualization_index() noexcept = 0;

  /// @returns `true` if visualization enabled.
  virtual bool is_visualization_enabled() const noexcept = 0;

  /// @returns The pointer to the channel's ADC.
  virtual std::shared_ptr<const CAdc> adc() const noexcept = 0;

  /// @overload
  virtual std::shared_ptr<CAdc> adc() noexcept = 0;

  /// @returns The pointer to the channel's DAC.
  virtual std::shared_ptr<const CDac> dac() const noexcept = 0;

  /// @overload
  virtual std::shared_ptr<CDac> dac() noexcept = 0;

  /// Update channel offset values.
  virtual void update_offsets() = 0;

  /// @returns The color of the corresponding LED.
  typeLEDcol color() const noexcept
  {
    return CView::Instance().GetChannel(visualization_index().GetVisChannel()).GetColor();
  }

  /// Sets the color of the corresponding LED.
  void set_color(const typeLEDcol color)
  {
    CView::Instance().GetChannel(visualization_index().GetVisChannel()).SetColor(color);
  }

  /**
   * @returns Measurement mode.
   *
   * @see CCmdSGHandler::Getter.
   *
   * @todo Remove.
   */
  unsigned int CmGetMesMode() const noexcept
  {
    return static_cast<int>(measurement_mode());
  }

  /**
   * @brief Sets measurement mode.
   *
   * @see CCmdSGHandler::Setter.
   *
   * @todo Remove.
   */
  void CmSetMesMode(unsigned int nMode)
  {
    if (nMode > mes_mode::Current)
      nMode = mes_mode::Current;

    set_measurement_mode(static_cast<mes_mode>(nMode));
  }

  /// @returns The pointer to the control instance containing this channel.
  nodeControl* node_control() const noexcept
  {
    return node_control_;
  }

  /// Associates the control instance with this channel.
  void set_node_control(nodeControl* const node_control)
  {
    node_control_ = node_control;
  }

  /**
   * @brief The object state update method.
   *
   * @details Gets the CPU time to update internal state of the object.
   */
  void update()
  {
    if (is_visualization_enabled())
      visualization_index().Update(adc()->GetRawBinVal());
  }

private:
  nodeControl* node_control_{};
};

class CIEPEchannel final : public CMesChannel {
public:
  CIEPEchannel(const int channel_index,
    const std::shared_ptr<CAdc>& adc,
    const std::shared_ptr<CDac>& dac,
    const CView::vischan visualization_index,
    const bool is_visualization_enabled)
    : channel_index_{channel_index}
    , visualization_index_{visualization_index}
    , is_visualization_enabled_{is_visualization_enabled}
    , adc_{adc}
    , dac_{dac}
  {}

  mes_mode measurement_mode() const noexcept override
  {
    return measurement_mode_;
  }

  void set_measurement_mode(const mes_mode mode) override
  {
    measurement_mode_ = mode;
  }

  bool is_iepe() const noexcept override
  {
    return is_iepe_;
  }

  void set_iepe(bool value) override
  {
    is_iepe_ = value;
  }

  float amplification_gain() const noexcept override
  {
    return amplification_gain_;
  }

  void set_amplification_gain(const float gain) override
  {
    amplification_gain_ = gain;
  }

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

  void update_offsets() override
  {}

private:
  bool is_iepe_{};
  mes_mode measurement_mode_{mes_mode::Voltage};
  float amplification_gain_{1};
  int channel_index_{-1};
  CDataVis visualization_index_;
  bool is_visualization_enabled_{};
  std::shared_ptr<CAdc> adc_;
  std::shared_ptr<CDac> dac_;
};
