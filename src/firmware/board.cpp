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
  auto err = eeprom_cache_.set_buf(buf);
  if (err && buf)
    err = eeprom_cache_.reset();
  PANDA_TIMESWIPE_ASSERT(!err);

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
        if (!is_fallback)
          return err;
      } else
        atom = map.atom(Ct::v_supply);
    }
    if (atom.entry_count() != 1) // exactly 1 entry per specification
      return Errc::hat_eeprom_data_corrupted;
    const auto& entry = atom.entry(0);
    voltage_dac_->set_linear_factors(entry.slope(), entry.offset());
  }

  // Update channels.
  for (auto& channel : channels_)
    channel->update_offsets(); // is_fallback is always `true` here

  return {};
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
    return Errc::hat_eeprom_unavailable;

  return {};
}

Error_or<hat::Calibration_map> Board::calibration_data() const noexcept
{
  hat::Calibration_map result;
  if (const auto err = eeprom_cache_.get(result))
    return err;
  return result;
}

Error Board::handle_catom(const rapidjson::Value& req, rapidjson::Document& res,
  const CCmdCallDescr::ctype ct) noexcept
{
  using Value = rapidjson::Value;

  const auto handle = [this](auto& req, auto& res, const auto ct) noexcept -> Error
  {
    auto [err1, map] = calibration_data();
    if (err1 && err1 != Errc::hat_eeprom_atom_missed)
      return err1;

    if (!req.IsObject() || req.FindMember("cAtom") == req.MemberEnd())
      return Error{Errc::spi_request_invalid};

    const auto catom = req["cAtom"].GetUint();
    const auto [err2, type] = hat::atom::Calibration::to_type(catom);
    if (err2)
      return err2;

    const auto cal_entry_count = map.atom(type).entry_count();

    if (ct == CCmdCallDescr::ctype::ctSet) {
#ifndef CALIBRATION_STATION
      return Error{Errc::board_settings_calibration_not_permitted};
#endif

      if (req.FindMember("data") == req.MemberEnd())
        return Error{Errc::spi_request_invalid};

      auto& data = req["data"];
      const auto data_size = data.Size();
      if (data_size > cal_entry_count)
        return Error{Errc::spi_request_invalid};

      for (std::size_t i{}; i < data_size; ++i) {
        const auto& el = data[i];
        auto entry = map.atom(type).entry(i);
        if (const auto it = el.FindMember("slope"); it != el.MemberEnd())
          entry.set_slope(it->value.GetFloat());
        if (const auto it = el.FindMember("offset"); it != el.MemberEnd())
          entry.set_offset(it->value.GetInt());
        map.atom(type).set_entry(i, std::move(entry));
      }

      // Update objects.
      if (const auto err = set_calibration_data(map); err)
        return err;
    }

    // Generate data member of the response.
    auto& alloc = res.GetAllocator();
    rapidjson::Value data{rapidjson::kArrayType};
    data.Reserve(cal_entry_count, alloc);
    for (std::size_t i{}; i < cal_entry_count; ++i) {
      const auto& entry = map.atom(type).entry(i);
      const auto slope = entry.slope();
      const auto offset = entry.offset();
      data.PushBack(
        std::move(Value{rapidjson::kObjectType}
          .AddMember("slope", slope, alloc)
          .AddMember("offset", offset, alloc)), alloc);
    }

    // Fill the response.
    res.RemoveAllMembers();
    res.AddMember("cAtom", catom, alloc);
    res.AddMember("data", std::move(data), alloc);

    return {};
  };

  const auto err = handle(req, res, ct);
  if (err) {
    auto& alloc = res.GetAllocator();
    res.AddMember("cAtom", Value{}, alloc);
    set_error(res, res["cAtom"], to_literal_anyway(err.errc()));
  }
  return err;
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

void Board::start_record(bool)
{
  rapidjson::Value v{record_count_++};
  Fire_on_event("Record", v);
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

  // Emit the event.
  rapidjson::Value v{val};
  Fire_on_event("Gain", v);

  return val;
}

void Board::enable_bridge(const bool enabled)
{
  is_bridge_enabled_ = enabled;

  if (board_type_ != Board_type::iepe) {
    PANDA_TIMESWIPE_ASSERT(ubr_pin_);
    ubr_pin_->write(enabled);
  }

  // Emit the event.
  rapidjson::Value v{enabled};
  Fire_on_event("voltageOutEnabled", v);
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

  // Emit the event.
  rapidjson::Value v{mode};
  Fire_on_event("Mode", v);
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
  rapidjson::Value v{how};
  Fire_on_event("Offset", v);
}
