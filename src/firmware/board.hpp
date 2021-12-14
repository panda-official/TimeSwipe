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
#include "pin.hpp"
#include "json.hpp"
#include "base/RawBinStorage.h"
#include "control/zerocal_man.h"
#include "json/json_evsys.h"

#include <memory>

/// FIXME: remove after placing the entire code base in the namespace
using namespace panda::timeswipe::detail;

/**
 * @brief Controls the basic behavior of the board.
 *
 * @details This class follows the Singleton design pattern. Emits JSON event
 * on each change of the board settings and receives such events from others.
 */
class Board final : public CJSONEvCP
                  , public ISerialize
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
  static Board& instance()
  {
    static std::shared_ptr<Board> pThis(new Board);
    return *pThis;
  }

  /// Non copy-constructible.
  Board(const Board&) = delete;
  /// Non copy-assignable.
  Board& operator=(const Board&) = delete;
  /// Non move-constructible.
  Board(Board&&) = delete;
  /// Non move-assignable.
  Board& operator=(Board&&) = delete;

  /**
   * @brief JSON handler wrapper to store/retrieve calibration atoms.
   *
   * @param req JSON request.
   * @param res JSON response.
   * @param ct Call type.
   */
  void handle_catom(rapidjson::Value& req, rapidjson::Document& res,
    const CCmdCallDescr::ctype ct);

  /// @returns `true` if the calibration data enabled.
  bool is_calibration_data_enabled() const noexcept
  {
    return is_calibration_data_enabled_;
  }

  /// Enables or disables the calibration data.
  void enable_calibration_data(const bool enabled)
  {
    is_calibration_data_enabled_ = enabled;
    if (is_calibration_data_enabled_) {
      hat::Calibration_map data;
      calibration_status_ = eeprom_cache_.get(data);
      std::string err;
      apply_calibration_data(data, err);
    }
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
  void set_voltage_dac(const std::shared_ptr<CDac>& dac)
  {
    voltage_dac_ = dac;
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
    assert(index < channels_.size());
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

  /// Sets a random 32-bit colour value as a new record stamp.
  void start_record(bool unused = true);

  /**
   * @returns The value that was set by start_record().
   *
   * @deprecated Kept just for compatibility with previous versions.
   */
  bool is_record_started() const noexcept
  {
    return false;
  }

  /// Sets the board's amplifier gain.
  void set_gain(int value)
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
  int increment_gain(int value)
  {
    value = gain() + value;
    if (value > 4)
      value = 1;
    return set_gain_out(value);
  }

  /// @returns The gain setting.
  int gain() const noexcept
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
  void set_secondary_measurement_mode(const int mode)
  {
    secondary_ = mode & 1;
  }

  /// @returns Current secondary measurement mode: 0 = IEPE; 1 = Normsignal.
  int secondary_measurement_mode() const noexcept
  {
    return secondary_;
  }

  /**
   * @brief Sets the measurement mode.
   *
   * @param mode: 0 = IEPE; 1 = Normsignal.
   */
  void set_measurement_mode(int mode);

  /// @returns Current measurement mode: 0 = IEPE; 1 = Normsignal.
  int measurement_mode() const noexcept
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
  void start_offset_search(int how);

  /// @returns `true` if the procedure of finding amplifier offsets is started.
  int is_offset_search_started() const noexcept
  {
    return offset_search_.IsStarted();
  }

  /// Enables or disables board ADC measurement.
  void enable_measurement(const bool enabled)
  {
    assert(adc_measurement_enable_pin_);
    adc_measurement_enable_pin_->write(enabled);
    CView::Instance().SetButtonHeartbeat(enabled);
  }

  /// @returns `true` if board ADC measurement enabled.
  bool is_measurement_enabled() const noexcept
  {
    assert(adc_measurement_enable_pin_);
    return adc_measurement_enable_pin_->read_back();
  }

  /// @returns `true` if EEPROM stores a valid calibration data.
  bool is_calibrated() const noexcept
  {
    return calibration_status_ == hat::Manager::Op_result::ok;
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
   * @brief Applies the calibration data to all the board items and overwrites
   * it in the EEPROM.
   *
   * @param[in] map The calibration map.
   * @param[out] err The error message which is set if the function returns `false`.
   *
   * @returns `false` on error.
   */
  bool set_calibration_data(hat::Calibration_map& map, std::string& err);

  /**
   * @brief Gets the calibration data from the RAM cache.
   *
   * @param[out] map The calibration map.
   * @param[out] err The error message which is set if the function returns `false`.
   *
   * @returns `false` on error.
   */
  bool get_calibration_data(hat::Calibration_map& map, std::string& err);

  /// Enables or disables the board cooler.
  void enable_fan(const bool enabled)
  {
    assert(fan_pin_);
    fan_pin_->write(enabled);
  }

  /// @returns `true` if the board cooler is enabled.
  bool is_fan_enabled() const noexcept
  {
    assert(fan_pin_);
    return fan_pin_->read_back();
  }

  /// Sets Voltage setting.
  void set_voltage(const float val)
  {
    if (voltage_dac_)
      voltage_dac_->SetVal(val);
    else
      voltage_ = val;
  }

  /// @returns Voltage Setting.
  float voltage() const noexcept
  {
    if (voltage_dac_)
      return voltage_dac_->GetRealVal();
    else
      return voltage_;
  }

  /// Sets Current setting.
  void set_current(float val)
  {
    if (val < 0)
      val = 0;
    else if (val > max_current_)
      val = max_current_;
    current_ = val;
  }

  /// @returns Current setting.
  float current() const noexcept
  {
    return current_;
  }

  /// Sets MaxCurrent (current limit) setting.
  void set_max_current(float val)
  {
    if (val < 0)
      val = 0;
    max_current_ = val;
  }

  /// @returns MaxCurrent (current limit) setting.
  float max_current() const noexcept
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
  Board_type board_type_{Board_type::iepe};
  std::shared_ptr<Pin> ubr_pin_;
  std::shared_ptr<Pin> dac_mode_pin_;
  std::shared_ptr<Pin> adc_measurement_enable_pin_;
  std::shared_ptr<Pin> fan_pin_;
  std::shared_ptr<Pin> gain0_pin_;
  std::shared_ptr<Pin> gain1_pin_;
  std::shared_ptr<CDac> voltage_dac_;
  std::vector<std::shared_ptr<Channel>> channels_;
  CCalMan offset_search_;
  CRawBinStorage raw_bin_storage_;
  unsigned record_count_{1};
  hat::Manager eeprom_cache_; // EEPROM cache
  std::shared_ptr<ISerial> eeprom_bus_; // for read/write external EEPROM
  hat::Manager::Op_result calibration_status_;
  bool is_settings_imported_{};
  bool is_calibration_data_enabled_{};
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

  /// Applies the given calibration map to board ADCs/DACs.
  void apply_calibration_data(const hat::Calibration_map& map, std::string& err);
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_BOARD_HPP
