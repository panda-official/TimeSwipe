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

#include "basics.hpp"
#include "board_settings.hpp"
#include "debug.hpp"
#include "driver.hpp"
#include "exceptions.hpp"
#include "rajson.hpp"

#include <algorithm>
#include <cstdint>
#include <regex>
#include <type_traits>
#include <utility>

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe {
using namespace detail;

// -----------------------------------------------------------------------------
// class Board_settings::Rep
// -----------------------------------------------------------------------------

struct Board_settings::Rep final {
  Rep()
  {}

  explicit Rep(rapidjson::Document doc)
    : doc_{std::move(doc)}
  {
    // Ensure that the input is the object. (Convert to object if NULL passed.)
    if (doc_.IsNull())
      doc_.SetObject();
    else if (!doc_.IsObject())
      throw Exception{Errc::board_settings_invalid, "not a JSON object"};

    // Assert invariant.
    PANDA_TIMESWIPE_ASSERT(doc_.IsObject());
  }

  explicit Rep(const std::string_view json_text) try
    : Rep{rajson::to_document(json_text)}
  {
  } catch (const rajson::Parse_exception& e) {
    throw Exception{Errc::board_settings_invalid,
      std::string{"cannot parse board settings: error near position "}
        .append(std::to_string(e.parse_result().Offset())).append(": ")
        .append(e.what())};
  }

  Rep(const Rep& rhs)
  {
    doc_.CopyFrom(rhs.doc_, doc_.GetAllocator(), true);
  }

  Rep& operator=(const Rep& rhs)
  {
    Rep tmp{rhs};
    swap(tmp);
    return *this;
  }

  Rep(Rep&&) = default;

  Rep& operator=(Rep&&) = default;

  std::vector<std::string> names() const
  {
    std::vector<std::string> result;

    using std::to_string;

    // analogOut
    for (int i{3}; i <= 4; ++i)
      result.push_back(std::string{"analogOut"}.append(to_string(i)).append("DacRaw"));
    result.push_back("analogOutsDacEnabled");

    // calibration
    for (const char* const name : {"Data", "DataEnabled", "DataValid"})
      result.push_back(std::string{"calibration"}.append(name));

    // channel
    for (const char* const name : {"AdcRaw", "DacRaw", "Gain", "Iepe", "Mode",
        "Color"}) {
      for (int i{1}; i <= mcc_; ++i)
        result.push_back(std::string{"channel"}.append(to_string(i)).append(name));
    }
    result.push_back("channelsAdcEnabled");

    // fan
    for (const char* const name : {"Enabled", "DutyCycle", "Frequency"})
      result.push_back(std::string{"fan"}.append(name));

    // pwm
    for (const char* const name : {"Enabled", "DutyCycle", "Frequency",
        "HighBoundary", "LowBoundary", "RepeatCount"}) {
      for (int i{1}; i <= mpc_; ++i)
        result.push_back(std::string{"pwm"}.append(to_string(i)).append(name));
    }

    // voltageOut
    for (const char* const name : {"Raw", "Value", "Enabled"})
      result.push_back(std::string{"voltageOut"}.append(name));

    // misc
    for (const char* const name : {"armId", "eepromTest", "firmwareVersion",
        "temperature"})
      result.push_back(name);

    return result;
  }

  std::vector<std::string> inapplicable_names() const
  {
    return {"channelsAdcEnabled"};
  }

  void swap(Rep& rhs) noexcept
  {
    doc_.Swap(rhs.doc_);
  }

  void set(const Rep& other)
  {
    using rapidjson::Value;
    auto& alloc = doc_.GetAllocator();
    for (const auto& other_m : other.doc_.GetObject()) {
      if (auto mi = doc_.FindMember(other_m.name); mi == doc_.MemberEnd())
        doc_.AddMember(Value{other_m.name, alloc, true},
          Value{other_m.value, alloc, true}, alloc);
      else
        mi->value.CopyFrom(other_m.value, alloc, true);
    }
  }

  std::string to_json_text() const
  {
    return rajson::to_text(doc_);
  }

  bool is_empty() const
  {
    return doc_.ObjectEmpty();
  }

  // ---------------------------------------------------------------------------

  void set_value(const std::string_view name, std::any value)
  {
    using std::any_cast;
    if (const auto& type = value.type(); type == typeid(Measurement_mode))
      set_member(name, any_cast<Measurement_mode>(value));
    else if (type == typeid(bool))
      set_member(name, any_cast<bool>(value));
    else if (type == typeid(char) || type == typeid(std::int8_t))
      set_member(name, any_cast<std::int8_t>(value));
    else if (type == typeid(unsigned char) || type == typeid(std::uint8_t))
      set_member(name, any_cast<std::uint8_t>(value));
    else if (type == typeid(short) || type == typeid(std::int16_t))
      set_member(name, any_cast<std::int16_t>(value));
    else if (type == typeid(unsigned short) || type == typeid(std::uint16_t))
      set_member(name, any_cast<std::uint16_t>(value));
    else if (type == typeid(int) || type == typeid(std::int32_t))
      set_member(name, any_cast<std::int32_t>(value));
    else if (type == typeid(unsigned int) || type == typeid(std::uint32_t))
      set_member(name, any_cast<std::uint32_t>(value));
    else if (type == typeid(long) || type == typeid(std::int64_t))
      set_member(name, any_cast<std::int64_t>(value));
    else if (type == typeid(unsigned long) || type == typeid(std::uint64_t))
      set_member(name, any_cast<std::uint64_t>(value));
    else if (type == typeid(float))
      set_member(name, any_cast<float>(value));
    else if (type == typeid(double))
      set_member(name, any_cast<double>(value));
    else if (type == typeid(std::string))
      set_member(name, any_cast<std::string>(value));
    else if (type == typeid(std::string_view))
      set_member(name, any_cast<std::string_view>(value));
    else
      throw Exception{Errc::board_settings_invalid, "unsupported value type"};
  }

  std::any value(const std::string_view name)
  {
    if (const auto result = member(name)) {
      using rajson::to;
      if (const auto& val = result->value(); val.IsBool())
        return to<bool>(val);
      else if (val.IsInt()) {
        if (std::regex_match(std::string{name}, std::regex{R"(channel\dMode)"}))
          return to<Measurement_mode>(val);
        else
          return to<std::int32_t>(val);
      } else if (val.IsUint())
        return to<std::uint32_t>(val);
      else if (val.IsInt64())
        return to<std::int64_t>(val);
      else if (val.IsUint64())
        return to<std::uint64_t>(val);
      else if (val.IsFloat() || val.IsLosslessFloat())
        return to<float>(val);
      else if (val.IsDouble() || val.IsLosslessDouble())
        return to<double>(val);
      else if (val.IsString())
        return to<std::string>(val);
    }
    return {};
  }

  // ---------------------------------------------------------------------------

  const rapidjson::Document& doc() const noexcept
  {
    return doc_;
  }

  rapidjson::Document& doc() noexcept
  {
    return doc_;
  }

private:
  inline static const unsigned mcc_{Driver::instance().max_channel_count()};
  inline static const unsigned mpc_{Driver::instance().max_pwm_count()};
  rapidjson::Document doc_{rapidjson::Type::kObjectType};

  // ---------------------------------------------------------------------------
  // Helpers
  // ---------------------------------------------------------------------------

  /// Adds or modifies the member named by `name` by using the given `value`.
  template<typename T>
  void set_member(const std::string_view name, T&& value)
  {
    detail::set_member(doc_, doc_.GetAllocator(), name, std::forward<T>(value));
  }

  /// @returns The value of `name` variable.
  std::optional<const rajson::Value_view<const rapidjson::Value>>
  member(const std::string_view name) const
  {
    return rajson::Value_view{doc_}.optional(name);
  }
};

// -----------------------------------------------------------------------------
// class Board_settings
// -----------------------------------------------------------------------------

Board_settings::~Board_settings() = default;

Board_settings::Board_settings(const Board_settings& rhs)
  : rep_{std::make_unique<Rep>(*rhs.rep_)}
{}

Board_settings& Board_settings::operator=(const Board_settings& rhs)
{
  Board_settings tmp{rhs};
  swap(tmp);
  return *this;
}

Board_settings::Board_settings(Board_settings&& rhs)
  : rep_{std::move(rhs.rep_)}
{}

Board_settings& Board_settings::operator=(Board_settings&& rhs)
{
  Board_settings tmp{std::move(rhs)};
  swap(tmp);
  return *this;
}

Board_settings::Board_settings(std::unique_ptr<Rep> rep)
  : rep_{std::move(rep)}
{
  PANDA_TIMESWIPE_ASSERT(rep_);
}

Board_settings::Board_settings()
  : rep_{std::make_unique<Rep>()}
{}

Board_settings::Board_settings(const std::string_view stringified_json)
  : rep_{std::make_unique<Rep>(stringified_json)}
{}

std::vector<std::string> Board_settings::names() const
{
  return rep_->names();
}

std::vector<std::string> Board_settings::inapplicable_names() const
{
  return rep_->inapplicable_names();
}

void Board_settings::swap(Board_settings& other) noexcept
{
  using std::swap;
  swap(rep_, other.rep_);
}

Board_settings& Board_settings::set(const Board_settings& other)
{
  rep_->set(*other.rep_);
  return *this;
}

std::string Board_settings::to_json_text() const
{
  return rep_->to_json_text();
}

bool Board_settings::is_empty() const
{
  return rep_->is_empty();
}

Board_settings& Board_settings::set_value(const std::string_view name, std::any value)
{
  rep_->set_value(name, std::move(value));
  return *this;
}

std::any Board_settings::value(const std::string_view name) const
{
  return rep_->value(name);
}

} // namespace panda::timeswipe
