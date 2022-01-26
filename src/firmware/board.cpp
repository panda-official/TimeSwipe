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

#include "board.hpp"
#include "control/DataVis.h"
#include "sam/SamService.h"

Board::Board()
{
  channels_.reserve(4);
}

void Board::set_eeprom_handles(const std::shared_ptr<ISerial>& bus,
  const std::shared_ptr<CFIFO>& buf)
{
  // Initialize EEPROM cache.
  calibration_data_eeprom_error_ = eeprom_cache_.set_buf(buf);
  if (calibration_data_eeprom_error_ && buf) {
    const auto err = eeprom_cache_.reset();
    PANDA_TIMESWIPE_ASSERT(!err);
  }

  // Initialize bus.
  eeprom_bus_ = bus;

  // Add or overwrite vendor info.
  eeprom_cache_.set(hat::atom::Vendor_info{
    CSamService::GetSerial(), 0, 2, "Panda", "Timeswipe"});

  // Add or overwrite the stubs.
  for (int i{eeprom_cache_.atom_count()}; i < 3; ++i)
    eeprom_cache_.set(hat::atom::Stub{i});
}

Error Board::apply_calibration_data(const bool is_fallback) noexcept
{
  // Update voltage DAC.
  if (voltage_dac_) {
    using Ct = hat::atom::Calibration::Type;
    hat::atom::Calibration atom{Ct::v_supply, 1};
    if (is_calibration_data_enabled_) {
      if (const auto [err, map] = calibration_data(); err) {
        calibration_data_apply_error_ = err;
        if (!is_fallback)
          return err;
      } else
        atom = map.atom(Ct::v_supply);
    }
    if (atom.entry_count() != 1) // exactly 1 entry per specification
      return calibration_data_apply_error_ = Errc::hat_eeprom_data_corrupted;
    const auto& entry = atom.entry(0);
    voltage_dac_->set_linear_factors(entry.slope(), entry.offset());
  }

  // Update channels.
  for (auto& channel : channels_)
    channel->update_offsets(); // is_fallback is always `true` here

  return calibration_data_apply_error_ = {};
}

Error Board::set_calibration_data(const hat::Calibration_map& map) noexcept
{
  // Update the cache.
  if (const auto err = eeprom_cache_.set(map))
    return err;

  // Update the state of all dependent objects.
  if (const auto err = apply_calibration_data(false); err)
    return err;

  // Update EEPROM.
  if (!eeprom_bus_->send(*eeprom_cache_.buf()))
    return calibration_data_eeprom_error_ = Errc::hat_eeprom_unavailable;

  return calibration_data_eeprom_error_ = {};
}

Error_or<hat::Calibration_map> Board::calibration_data() const noexcept
{
  hat::Calibration_map result;
  if (const auto err = eeprom_cache_.get(result))
    return err;
  return result;
}

void Board::Serialize(CStorage &st)
{
  offset_search_.Serialize(st);
  if (st.IsDefaultSettingsOrder()) {
    set_gain(1);
    enable_bridge(false);
    set_secondary_measurement_mode(0);
  }

  st.ser(gain_).ser(is_bridge_enabled_).ser(secondary_);

  if (st.IsImporting()) {
    set_gain(gain_);
    enable_bridge(is_bridge_enabled_);
    set_secondary_measurement_mode(secondary_);
  }
}

void Board::update()
{
  for (auto& channel : channels_)
    channel->update();
  raw_bin_storage_.Update();
  offset_search_.Update();
}

int Board::set_gain_out(const int val)
{
  // Update gains for all channels.
  gain_ = val;
  for (auto& channel : channels_)
    channel->set_amplification_gain(val);

  // Set old IEPE gain.
  if (board_type_ == Board_type::iepe) {
    const int gset{val - 1};
    gain1_pin_->write(gset>>1);
    gain0_pin_->write(gset&1);
  }

  return val;
}

void Board::enable_bridge(const bool enabled)
{
  is_bridge_enabled_ = enabled;

  if (board_type_ != Board_type::iepe) {
    PANDA_TIMESWIPE_ASSERT(ubr_pin_);
    ubr_pin_->write(enabled);
  }
}

void Board::set_measurement_mode(const int mode)
{
  measurement_mode_ = static_cast<MesModes>(mode);
  if (measurement_mode_ < MesModes::IEPE)
    measurement_mode_ = MesModes::IEPE;
  else if (measurement_mode_ > MesModes::Normsignal)
    measurement_mode_ = MesModes::Normsignal;

  if (board_type_ == Board_type::iepe) { // old IEPE board setting
    //enable_bridge(measurement_mode_);
    PANDA_TIMESWIPE_ASSERT(ubr_pin_);
    ubr_pin_->write(measurement_mode_ == MesModes::IEPE);
  }

  // Switch all channels to IEPE.
  for (auto& channel : channels_)
    channel->set_iepe(measurement_mode_ == MesModes::IEPE);

  set_secondary_measurement_mode(measurement_mode_);
}

void Board::start_offset_search(int how)
{
  switch (how) {
  case 1: // negative
    offset_search_.Start(4000);
    break;
  case 2: // zero
    offset_search_.Start();
    break;
  case 3: // positive
    offset_search_.Start(100);
    break;
  default:
    how = 0;
    offset_search_.StopReset();
    return;
  }
}
