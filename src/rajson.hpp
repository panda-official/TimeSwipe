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

#ifndef PANDA_TIMESWIPE_RAJSON_HPP
#define PANDA_TIMESWIPE_RAJSON_HPP

#include "board_settings.hpp"
#include "error.hpp"

#include "3rdparty/dmitigr/rajson.hpp"

#include <optional>
#include <string_view>

namespace panda::timeswipe::detail {

/// Traits structure for enumerations.
template<class> struct Enum_traits;

/// Full specialization for Measurement_mode.
template<> struct Enum_traits<Measurement_mode> final {
  static constexpr const char* singular_name() noexcept
  {
    return "measurement mode";
  }
};

/// Generic enum/JSON conversions.
template<class E>
struct Enum_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    const auto result = static_cast<E>(dmitigr::rajson::to<std::underlying_type_t<E>>(value));
    if (!to_literal(result))
      throw Exception{std::string{"invalid JSON representation of "}
        .append(Enum_traits<E>::singular_name())};
    return result;
  }

  template<class Encoding, class Allocator>
  static auto to_value(const E value, Allocator&)
  {
    if (!to_literal(value))
      throw Exception{std::string{"cannot represent "}
        .append(Enum_traits<E>::singular_name()).append(" as JSON value")};
    return rapidjson::GenericValue<Encoding, Allocator>(static_cast<int>(value));
  }
};

} // namespace panda::timeswipe::detail

namespace dmitigr::rajson {

/// Full specialization for `panda::timeswipe::Measurement_mode`.
template<>
struct Conversions<panda::timeswipe::Measurement_mode> final :
  panda::timeswipe::detail::Enum_conversions<panda::timeswipe::Measurement_mode>{};

} // dmitigr::rajson

namespace panda::timeswipe::detail {

/// Adds or modifies the member named by `name` by using the given `value`.
template<typename Encoding, typename Allocator, typename T>
void set_member(rapidjson::GenericValue<Encoding, Allocator>& json, Allocator& alloc,
  const std::string_view name, T&& value)
{
  namespace rajson = dmitigr::rajson;
  auto val = rajson::to_value(std::forward<T>(value), alloc);
  const auto name_ref = rajson::to_string_ref(name);
  if (const auto i = json.FindMember(name_ref); i != json.MemberEnd())
    i->value = std::move(val);
  else
    json.AddMember(rapidjson::Value{name.data(), name.size(), alloc},
      std::move(val), alloc);
}

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_RAJSON_HPP
