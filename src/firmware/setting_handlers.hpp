// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH / Dmitry Igrishin

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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SETTING_HANDLERS_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SETTING_HANDLERS_HPP

#include "board.hpp"
#include "settings.hpp"

#include <limits>

// -----------------------------------------------------------------------------
// Calibration_data_handler
// -----------------------------------------------------------------------------

class Calibration_data_handler final : public Setting_handler {
public:
  Error handle(Setting_request& request) override
  {
    using hat::atom::Calibration;

    static const auto error = [](std::string msg) noexcept
    {
      return Error{Errc::board_settings_invalid, std::move(msg)};
    };

    static const auto to_type = [](const rapidjson::Value& v)
    {
      return Calibration::to_type(static_cast<std::uint16_t>(v.GetUint()));
    };

    const auto& input_v = request.input.value_ref();

    // Get the current calibration data.
    auto [err1, map] = Board::instance().calibration_data();
    if (err1 && err1 != Errc::hat_eeprom_atom_missed)
      return err1;

    std::vector<Calibration::Type> requested_types;
    requested_types.reserve(map.atoms().size());
    switch (request.type) {
    case Setting_request_type::write:
#ifndef CALIBRATION_STATION
      return Error{Errc::board_settings_write_forbidden,
        "device is not calibration station"};
#endif
      // Precheck the input calibration data.
      if (!input_v.IsArray() || input_v.Empty())
        return error("invalid calibration data");

      /*
       * Check the input calibration data and copy it to the `map`.
       * "calibrationData":[{"type":%, "data":[{"slope":%, "offset":%},...]},...]
       */
      for (const auto& catom_v : input_v.GetArray()) {
        if (!catom_v.IsObject())
          return error("invalid calibration atom");

        const auto type_m = catom_v.FindMember("type");
        if (type_m == catom_v.MemberEnd())
          return error("invalid calibration atom");
        else if (!type_m->value.IsUint())
          return error("invalid calibration atom");
        const auto [err2, type] = to_type(type_m->value);
        if (err2)
          return err2;
        const auto entry_count = map.atom(type).entry_count();

        const auto data_m = catom_v.FindMember("data");
        if (data_m == catom_v.MemberEnd())
          return error("invalid calibration atom");
        else if (!data_m->value.IsArray())
          return error("invalid calibration atom");
        const auto data_size = data_m->value.Size();
        if (data_size > entry_count)
          return error("invalid calibration atom");

        for (std::size_t i{}; i < data_size; ++i) {
          const auto& entry_v =  data_m->value[i];
          if (!entry_v.IsObject())
            return error("invalid calibration atom");

          const auto slope_m = entry_v.FindMember("slope");
          if (slope_m == entry_v.MemberEnd())
            return error("invalid calibration atom");
          else if (!slope_m->value.IsFloat() || !slope_m->value.IsLosslessFloat())
            return error("invalid calibration atom slope");
          const auto slope = slope_m->value.GetFloat();

          const auto offset_m = entry_v.FindMember("offset");
          if (offset_m == entry_v.MemberEnd())
            return error("invalid calibration atom");
          else if (!offset_m->value.IsInt())
            return error("invalid calibration atom offset");
          const auto offset = offset_m->value.GetInt();
          if (!(std::numeric_limits<std::int16_t>::min() <= offset &&
              offset <= std::numeric_limits<std::int16_t>::max()))
            return error("invalid calibration atom offset");

          // Update the map.
          map.atom(type).set_entry(i, Calibration::Entry{slope,
              static_cast<std::int16_t>(offset)});
        }

        requested_types.push_back(type);
      }

      // Update the calibration data.
      PANDA_TIMESWIPE_ASSERT(!requested_types.empty());
      if (const auto err = Board::instance().set_calibration_data(map); err)
        return err;

      break;

    case Setting_request_type::read:
      // Precheck the input calibration data.
      if (input_v.IsNull()) {
        for (const auto& atom : map.atoms())
          requested_types.push_back(atom.type());
      } else if (input_v.IsArray()) {
        for (const auto& type_v : input_v.GetArray()) {
          if (!type_v.IsUint())
            return error("invalid request");
          else if (const auto [err, type] = to_type(type_v); err)
            return err;
          else
            requested_types.push_back(type);
        }
      } else if (input_v.IsUint()) {
        if (const auto [err, type] = to_type(input_v); err)
          return err;
        else
          requested_types = {type};
      } else
        return error("invalid request");

      break;
    } // switch (request.type)

    // Generate the result.
    auto& result = request.output.value_ref();
    auto& alloc = request.output.alloc_ref();
    result.SetArray();
    for (const auto type : requested_types) {
      using rapidjson::Value;
      const auto entry_count = map.atom(type).entry_count();
      Value data{rapidjson::kArrayType};
      data.Reserve(entry_count, alloc);
      for (std::size_t i{}; i < entry_count; ++i) {
        const auto& entry = map.atom(type).entry(i);
        data.PushBack(
          std::move(Value{rapidjson::kObjectType}
            .AddMember("slope", entry.slope(), alloc)
            .AddMember("offset", entry.offset(), alloc)), alloc);
      }
      result.PushBack(
        std::move(Value{rapidjson::kObjectType}
          .AddMember("type", static_cast<std::uint16_t>(type), alloc)
          .AddMember("data", std::move(data), alloc)) , alloc);
    }

    return {};
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SETTING_HANDLERS_HPP
