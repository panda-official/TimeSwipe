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
#ifndef CALIBRATION_STATION
  : is_calibration_data_enabled_{true};
#endif
{
  channels_.reserve(4);
}

void Board::set_eeprom_handles(const std::shared_ptr<ISerial>& bus,
  const std::shared_ptr<CFIFO>& buf)
{
  eeprom_cache_.set_buf(buf);
  eeprom_bus_ = bus;

  if (eeprom_cache_.verify() != hat::Manager::Op_result::ok) {
    eeprom_cache_.reset();
    hat::atom::Vendor_info vinf{CSamService::GetSerial(), 0, 2, "Panda", "Timeswipe"};
    eeprom_cache_.set(vinf); //storage is ready
  }

  // Fill blank atoms with the stubs.
  for (int i{eeprom_cache_.atom_count()}; i < 3; ++i) {
    hat::atom::Stub stub{i};
    eeprom_cache_.set(stub);
  }

  hat::Calibration_map map;
  calibration_status_ = eeprom_cache_.get(map);
  if (calibration_status_ == hat::Manager::Op_result::ok) {
    std::string err;
    apply_calibration_data(map, err);
  }
}

void Board::apply_calibration_data(const hat::Calibration_map& map, std::string& err)
{
  if (!is_calibration_data_enabled_) {
    err = "calibration settings are disabled";
    return;
  }

  if (voltage_dac_) {
    const auto& atom = map.atom(hat::atom::Calibration::Type::v_supply);
    if (atom.entry_count() != 1) { // exactly 1 entry per specification
      err = "invalid v_supply calibration atom";
      return;
    }
    const auto& entry = atom.entry(0);
    voltage_dac_->SetLinearFactors(entry.slope(), entry.offset());
    voltage_dac_->SetVal();
  }

  // Update channels.
  for (auto& channel : channels_)
    channel->update_offsets();
}

bool Board::set_calibration_data(const hat::Calibration_map& map, std::string& err)
{
  calibration_status_ = eeprom_cache_.set(map);
  if (calibration_status_ != hat::Manager::Op_result::ok) {
    err = "invalid calibration map";
    return false;
  }

  apply_calibration_data(map, err);
  if (!err.empty()) return false;

  if (!eeprom_bus_->send(*eeprom_cache_.buf())) {
    err = "failed to write EEPROM";
    return false;
  }

  return true;
}

bool Board::get_calibration_data(hat::Calibration_map& data, std::string& err) const
{
  using Op_result = hat::Manager::Op_result;
  const auto r = eeprom_cache_.get(data);
  if (r == Op_result::ok || r == Op_result::atom_not_found)
    return true;

  err = "EEPROM image is corrupted";
  return false;
}

void Board::handle_catom(rapidjson::Value& req, rapidjson::Document& res,
  const CCmdCallDescr::ctype ct)
{
  const auto handle = [this](auto& req, auto& res, const auto ct, auto& err)
  {
    hat::Calibration_map map;

    //load existing atom
    if (!get_calibration_data(map, err))
      return false;

    const auto catom = req["cAtom"].GetUint();
    const auto type = hat::atom::Calibration::to_type(catom, err);
    if (!err.empty()) return false;

    const auto cal_entry_count = map.atom(type).entry_count();

    if(CCmdCallDescr::ctype::ctSet==ct)
      {
#ifndef CALIBRATION_STATION
        err="calibration setting is prohibited!";
        return false;
#endif

        auto& data = req["data"];
        const auto data_size = data.Size();
        if (data_size > cal_entry_count) {
          err = "wrong data count";
          return false;
        }

        for (std::size_t i{}; i < data_size; ++i) {
          const auto& el = data[i];
          auto entry = map.atom(type).entry(i);
          if (const auto it = el.FindMember("m"); it != el.MemberEnd())
            entry.set_slope(it->value.GetFloat());
          if (const auto it = el.FindMember("b"); it != el.MemberEnd())
            entry.set_offset(it->value.GetInt());
          map.atom(type).set_entry(i, std::move(entry));
        }

        // Save the calibration map.
        if (!set_calibration_data(map, err)) return false;
      }

    // Generate data member of the response.
    auto& alloc = res.GetAllocator();
    rapidjson::Value data{rapidjson::kArrayType};
    data.Reserve(cal_entry_count, alloc);
    for (std::size_t i{}; i < cal_entry_count; ++i) {
      const auto& entry = map.atom(type).entry(i);
      const auto m = entry.slope();
      const auto b = entry.offset();
      data.PushBack(std::move(rapidjson::Value{rapidjson::kObjectType}
          .AddMember("m", m, alloc)
          .AddMember("b", b, alloc)), alloc);
    }

    // Fill the response.
    res.RemoveAllMembers();
    res.AddMember("cAtom", catom, alloc);
    res.AddMember("data", std::move(data), alloc);

    return true;
  };

  std::string err;
  if (!handle(req, res, ct, err)) {
    using Value = rapidjson::Value;
    auto& alloc = res.GetAllocator();
    res.AddMember("cAtom", Value{}, alloc);
    set_error(res, res["cAtom"], err);
  }
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
    PANDA_TIMESWIPE_FIRMWARE_ASSERT(ubr_pin_);
    ubr_pin_->write(enabled);
  }

  // Emit the event.
  rapidjson::Value v{enabled};
  Fire_on_event("Bridge", v);
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
    PANDA_TIMESWIPE_FIRMWARE_ASSERT(ubr_pin_);
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
