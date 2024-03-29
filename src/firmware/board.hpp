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

#ifndef PANDA_TIMESWIPE_FIRMWARE_BOARD_HPP
#define PANDA_TIMESWIPE_FIRMWARE_BOARD_HPP

#include "../hat.hpp"
#include "basics.hpp"
#include "channel.hpp"
#include "error.hpp"
#include "json.hpp"
#include "pin.hpp"
#include "base/RawBinStorage.h"
#include "control/zerocal_man.h"

#include <memory>

/// FIXME: remove after placing the entire code base in the namespace
using namespace panda::timeswipe::detail;

/**
 * @brief Controls the basic behavior of the board.
 *
 * @details This class follows the Singleton design pattern.
 */
class Board final : public ISerialize
                  , public std::enable_shared_from_this<Board> {
public:
  /// The possible values for IEPE measure modes.
  enum MesModes {
    IEPE,
    Normsignal,
    Digital
  };

  /// @see ISerialize::Serialize(CStorage&).
  void Serialize(CStorage& st) override;

  /// @returns The reference to the singleton instance.
  static Board& instance() noexcept
  {
    if (!instance_) {
      instance_.reset(new(std::nothrow) Board);
      PANDA_TIMESWIPE_ASSERT(instance_);
    }
    return *instance_;
  }

  /// Non copy-constructible.
  Board(const Board&) = delete;
  /// Non copy-assignable.
  Board& operator=(const Board&) = delete;
  /// Non move-constructible.
  Board(Board&&) = delete;
  /// Non move-assignable.
  Board& operator=(Board&&) = delete;

  /// @returns `true` if the calibration data enabled.
  bool is_calibration_data_enabled() const noexcept
  {
    return is_calibration_data_enabled_;
  }

  /**
   * @brief Enables or disables the calibration data.
   *
   * @par Effects
   * `is_calibration_data_enabled()` on success.
   */
  Error enable_calibration_data(const bool enabled) noexcept
  {
    is_calibration_data_enabled_ = enabled;
    return apply_calibration_data(true);
  }

  /// Sets the board type.
  void set_board_type(const Board_type type)
  {
    board_type_ = type;
  }

  /// Sets the UBR switch (bridge voltage).
  void set_ubr_pin(const std::shared_ptr<Pin>& pin)
  {
    ubr_pin_ = pin;
  }

  /**
   * @brief Sets the DAC mode switch.
   *
   * @details When set, both the AOUT3 and AOUT4 are enabled.
   */
  void set_dac_mode_pin(const std::shared_ptr<Pin>& pin)
  {
    dac_mode_pin_ = pin;
  }

  /// Sets the ADC measurements enable switch.
  void set_adc_measurement_enable_pin(const std::shared_ptr<Pin>& pin)
  {
    adc_measurement_enable_pin_ = pin;
  }

  /// Sets the fan switch.
  void set_fan_pin(const std::shared_ptr<Pin>& pin)
  {
    fan_pin_ = pin;
  }

  /**
   * @brief Sets the gain pins of the IEPE board
   *
   * @param gain0_pin The LSB gain select pin.
   * @param gain1_pin The MSB gain select pin.
   */
  void set_iepe_gain_pins(const std::shared_ptr<Pin>& gain0_pin,
    const std::shared_ptr<Pin>& gain1_pin)
  {
    gain0_pin_ = gain0_pin;
    gain1_pin_ = gain1_pin;
  }

  /// Sets the Voltage DAC controlled by the set_voltage().
  void set_voltage_dac(const std::shared_ptr<Calibratable_dac>& dac)
  {
    voltage_dac_ = dac;
    apply_calibration_data(true);
  }

  /// Adds measurement channel to the tracking list.
  void add_channel(const std::shared_ptr<Channel>& channel)
  {
    channel->set_board(this);
    channels_.emplace_back(channel);
    offset_search_.Add(channel->adc(),
      channel->dac(),
      channel->visualization_index().GetVisChannel());
  }

  /// @returns The measurement channel by the given index.
  std::shared_ptr<Channel> channel(const std::size_t index) const noexcept
  {
    PANDA_TIMESWIPE_ASSERT(index < channels_.size());
    return channels_[index];
  }

  /**
   * @brief Imports all the settings from the persist storage.
   *
   * @remarks Can be called only once. (At startup.)
   */
  void import_settings()
  {
    if (!is_settings_imported_) {
      raw_bin_storage_.AddItem(this->shared_from_this());
      raw_bin_storage_.Import();
      is_settings_imported_ = true;
    }
  }

  /// Resets the settings to their defaults.
  void reset_settings()
  {
    raw_bin_storage_.SetDefaults();
  }

  /// Sets the board's amplifier gain.
  [[deprecated]] void set_gain(int value)
  {
    if (value < 1)
      value = 1;
    else if (value > 4)
      value = 4;
    set_gain_out(value);
  }

  /**
   * @brief Increments the board's amplifier gain.
   *
   * @param value The increment.
   *
   * @returns The gain that was set
   *
   * @remarks Swithes to minimun value on overflow.
   */
  [[deprecated]] int increment_gain(int value)
  {
    value = gain() + value;
    if (value > 4)
      value = 1;
    return set_gain_out(value);
  }

  /// @returns The gain setting.
  [[deprecated]] int gain() const noexcept
  {
    return gain_;
  }

  /// Enables or disables the bridge voltage mode.
  void enable_bridge(bool enabled);

  /// @returns `true` if bridge mode is enabled.
  bool is_bridge_enabled() const noexcept
  {
    return is_bridge_enabled_;
  }

  /**
   * @brief Sets the secondary measurement mode.
   *
   * @param mode: 0 = IEPE; 1 = Normsignal.
   */
  [[deprecated]] void set_secondary_measurement_mode(const int mode)
  {
    secondary_ = mode & 1;
  }

  /// @returns Current secondary measurement mode: 0 = IEPE; 1 = Normsignal.
  [[deprecated]] int secondary_measurement_mode() const noexcept
  {
    return secondary_;
  }

  /**
   * @brief Sets the measurement mode.
   *
   * @param mode: 0 = IEPE; 1 = Normsignal.
   */
  [[deprecated]] void set_measurement_mode(int mode);

  /// @returns Current measurement mode: 0 = IEPE; 1 = Normsignal.
  [[deprecated]] int measurement_mode() const noexcept
  {
    return measurement_mode_;
  }

  /**
   * @brief Starts or stops finding amplifier offsets procedure.
   *
   * @param how Meanings:
   *   - 0 stop/reset;
   *   - 1 negative offset search;
   *   - 2 zero offset search;
   *   - 3 positive offset search.
   */
  [[deprecated]] void start_offset_search(int how);

  /// @returns `true` if the procedure of finding amplifier offsets is started.
  [[deprecated]] int is_offset_search_started() const noexcept
  {
    return offset_search_.IsStarted();
  }

  /// Enables or disables channels ADC measurement.
  Error enable_channels_adc(const bool enabled)
  {
    // Return if there is a channel with either mode or gain unset.
    if (enabled) {
      for (auto& channel : channels_)
        if (!channel->measurement_mode() || !channel->amplification_gain())
          return Errc::board_settings_insufficient;
    }
    PANDA_TIMESWIPE_ASSERT(adc_measurement_enable_pin_);
    adc_measurement_enable_pin_->write(enabled);
    CView::Instance().SetButtonHeartbeat(enabled);
    return {};
  }

  /// @returns `true` if board ADC measurement enabled.
  bool is_channels_adc_enabled() const noexcept
  {
    PANDA_TIMESWIPE_ASSERT(adc_measurement_enable_pin_);
    return adc_measurement_enable_pin_->read_back();
  }

  /// @returns The error of last application of a calibration data.
  Error_result calibration_data_apply_error() const noexcept
  {
    return Error_result{calibration_data_apply_error_};
  }

  /// @returns The error of last interaction with EEPROM.
  Error_result calibration_data_eeprom_error() const noexcept
  {
    return Error_result{calibration_data_eeprom_error_};
  }

  /**
   * @brief Sets the handles for working with external EEPROM chip.
   *
   * @param bus The EEPROM bus to send/receive EEPROM image.
   * @param buf The pre-allocated RAM storage for the EEPROM image.
   */
  void set_eeprom_handles(const std::shared_ptr<ISerial>& bus,
    const std::shared_ptr<CFIFO>& buf);

  /**
   * @brief Updates the both cache and persistent storage of EEPROM.
   *
   * @par Effects
   * Updates the state of all objects that depend on the calibration data.
   */
  Error set_calibration_data(const hat::Calibration_map& map) noexcept;

  /// @returns The RAM-cached calibration data.
  Error_or<hat::Calibration_map> calibration_data() const noexcept;

  /// Enables or disables the board cooler. (Deprecated by CPinPWM.)
  [[deprecated]] void enable_fan(const bool enabled)
  {
    PANDA_TIMESWIPE_ASSERT(fan_pin_);
    fan_pin_->write(enabled);
  }

  /// @returns `true` if the board cooler is enabled. (Deprecated by CPinPWM.)
  [[deprecated]] bool is_fan_enabled() const noexcept
  {
    PANDA_TIMESWIPE_ASSERT(fan_pin_);
    return fan_pin_->read_back();
  }

  /// Sets `voltageOutValue` setting.
  void set_voltage(const float value)
  {
    if (voltage_dac_)
      voltage_dac_->set_value(value);
    else
      voltage_ = value;
  }

  /// @returns `voltageOutValue` Setting.
  float voltage() const noexcept
  {
    if (voltage_dac_)
      return voltage_dac_->value();
    else
      return voltage_;
  }

  /// Sets Current setting.
  [[deprecated]] void set_current(float val)
  {
    if (val < 0)
      val = 0;
    else if (val > max_current_)
      val = max_current_;
    current_ = val;
  }

  /// @returns Current setting.
  [[deprecated]] float current() const noexcept
  {
    return current_;
  }

  /// Sets MaxCurrent (current limit) setting.
  [[deprecated]] void set_max_current(float val)
  {
    if (val < 0)
      val = 0;
    max_current_ = val;
  }

  /// @returns MaxCurrent (current limit) setting.
  [[deprecated]] float max_current() const noexcept
  {
    return max_current_;
  }

  /**
   * @brief Updates the state of the instance.
   *
   * @details Gets the CPU time to update internal state of the object.
   *
   * @remarks Must be called from a "super loop" or from corresponding thread.
   */
  void update();

private:
  inline static std::shared_ptr<Board> instance_;
  Board_type board_type_{Board_type::iepe};
  std::shared_ptr<Pin> ubr_pin_;
  std::shared_ptr<Pin> dac_mode_pin_;
  std::shared_ptr<Pin> adc_measurement_enable_pin_;
  std::shared_ptr<Pin> fan_pin_;
  std::shared_ptr<Pin> gain0_pin_;
  std::shared_ptr<Pin> gain1_pin_;
  std::shared_ptr<Calibratable_dac> voltage_dac_;
  std::vector<std::shared_ptr<Channel>> channels_;
  CCalMan offset_search_;
  CRawBinStorage raw_bin_storage_;
  unsigned record_count_{1};
  hat::Manager eeprom_cache_; // EEPROM cache
  std::shared_ptr<ISerial> eeprom_bus_; // for read/write external EEPROM
  bool is_settings_imported_{};
  bool is_calibration_data_enabled_{};
  Error calibration_data_apply_error_;
  Error calibration_data_eeprom_error_;
  bool is_bridge_enabled_{}; // persistent
  int gain_{1}; // persistent
  int secondary_{}; // persistent
  float voltage_{}; // mockup for IEPE board
  float current_{}; // mockup
  float max_current_{1000}; // mockup, mA
  MesModes measurement_mode_{Board::IEPE};

  /// Default-constructible.
  Board();

  /**
   * @brief A helper function for setting amplifier gain output.
   *
   * @param val The gain setpoint.
   *
   * @returns The gain that was set.
   */
  int set_gain_out(int val);

  /**
   * @brief Applies the current calibration map to board ADCs/DACs.
   *
   * @param is_fallback If `true` then when `!calibration_data()`, the hardcoded
   * default calibration data will be applied instead of returning an Error.
   */
  Error apply_calibration_data(bool is_fallback) noexcept;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_BOARD_HPP
