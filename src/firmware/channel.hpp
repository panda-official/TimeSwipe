// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_FIRMWARE_CHANNEL_HPP
#define PANDA_TIMESWIPE_FIRMWARE_CHANNEL_HPP

#include "adcdac.hpp"
#include "control/DataVis.h"

#include "io_stream.hpp"

#include "../basics.hpp"
using namespace panda::timeswipe; // FIXME: remove

#include <memory>
#include <optional>

class Board;

/// A board measurement channel.
class Channel {
public:
  /// @returns The measurement mode.
  virtual std::optional<Measurement_mode> measurement_mode() const noexcept = 0;

  /// Sets The measurement mode.
  virtual void set_measurement_mode(Measurement_mode mode) = 0;

  /// @returns IEPE mode indicator.
  virtual bool is_iepe() const noexcept = 0;

  /// Sets IEPE mode indicator.
  virtual void set_iepe(bool enable) = 0;

  /// @returns The amplification gain.
  virtual std::optional<float> amplification_gain() const noexcept = 0;

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
    return CView::Instance()
      .GetChannel(visualization_index().GetVisChannel()).GetColor();
  }

  /// Sets the color of the corresponding LED.
  void set_color(const typeLEDcol color)
  {
    CView::Instance()
      .GetChannel(visualization_index().GetVisChannel()).SetColor(color);
  }

  /// @returns The board which controls this channel.
  Board* board() const noexcept
  {
    return board_;
  }

  /// Associates the board with this channel.
  void set_board(Board* const board)
  {
    board_ = board;
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
  Board* board_{};
};

/// Writes `mode` to the `os` stream.
inline Io_stream& operator<<(Io_stream& os, const Measurement_mode mode)
{
  os << static_cast<unsigned>(mode);
  return os;
}

/// Reads `mode` from the `is` stream.
inline Io_stream& operator>>(Io_stream& is, Measurement_mode& mode)
{
  unsigned umode;
  is >> umode;
  if (is.is_good()) {
    constexpr auto cur = static_cast<unsigned>(Measurement_mode::current);
    if (umode > cur)
      umode = cur;
    mode = static_cast<Measurement_mode>(umode);
  }
  return is;
}

class CIEPEchannel final : public Channel {
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

  std::optional<Measurement_mode> measurement_mode() const noexcept override
  {
    return measurement_mode_;
  }

  void set_measurement_mode(const Measurement_mode mode) override
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

  std::optional<float> amplification_gain() const noexcept override
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
  std::optional<Measurement_mode> measurement_mode_;
  std::optional<float> amplification_gain_;
  int channel_index_{-1};
  CDataVis visualization_index_;
  bool is_visualization_enabled_{};
  std::shared_ptr<CAdc> adc_;
  std::shared_ptr<CDac> dac_;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_CHANNEL_HPP
