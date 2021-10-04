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
#include "error_detail.hpp"

#include "3rdparty/dmitigr/rajson.hpp"

#include <optional>
#include <string_view>

namespace panda::timeswipe::detail {

template<class> struct Enum_traits;

template<> struct Enum_traits<Measurement_mode> final {
  static constexpr const char* singular_name() noexcept
  {
    return "measurement mode";
  }
};
template<> struct Enum_traits<Signal_mode> final {
  static constexpr const char* singular_name() noexcept
  {
    return "signal mode";
  }
};

template<class E>
struct Enum_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    const auto result = static_cast<E>(dmitigr::rajson::to<std::underlying_type_t<E>>(value));
    if (!to_literal(result))
      throw Generic_exception{std::string{"cannot use JSON value that doesn't match any "}
        .append(Enum_traits<E>::singular_name())};
    return result;
  }

  template<class Encoding, class Allocator>
  static auto to_value(const E value, Allocator&)
  {
    if (!to_literal(value))
      throw Generic_exception{std::string{"cannot convert invalid "}
        .append(Enum_traits<E>::singular_name()).append(" to JSON value")};
    return rapidjson::GenericValue<Encoding, Allocator>(static_cast<int>(value));
  }
};

} // namespace panda::timeswipe::detail

namespace dmitigr::rajson {

/// Full specialization for `panda::timeswipe::Measurement_mode`.
template<>
struct Conversions<panda::timeswipe::Measurement_mode> final :
  panda::timeswipe::detail::Enum_conversions<panda::timeswipe::Measurement_mode>{};

/// Full specialization for `panda::timeswipe::Signal_mode`.
template<>
struct Conversions<panda::timeswipe::Signal_mode> final :
  panda::timeswipe::detail::Enum_conversions<panda::timeswipe::Signal_mode>{};

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

/// Adds or modifies the element of array named by `name` at the given `index`.
template<typename T, typename Encoding, typename Allocator>
void set_array_element(rapidjson::GenericValue<Encoding, Allocator>& json, Allocator& alloc,
  const std::string_view name, const std::size_t index, T&& value,
  T&& default_value = T{})
{
  namespace rajson = dmitigr::rajson;
  const auto name_ref = rajson::to_string_ref(name);
  auto i = json.FindMember(name_ref);
  if (i == json.MemberEnd()) {
    json.AddMember(rapidjson::Value{name.data(), name.size(), alloc},
      rapidjson::Value{rapidjson::kArrayType}, alloc);
    i = json.FindMember(name_ref);
  } else if (!i->value.IsArray())
    i->value.SetArray();

  PANDA_TIMESWIPE_ASSERT(i != json.MemberEnd());
  PANDA_TIMESWIPE_ASSERT(i->value.IsArray());

  auto& array = i->value;
  if (index <= array.Size()) {
    const std::size_t extra_size = array.Size() - index + 1;
    for (std::size_t i{}; i < extra_size; ++i)
      array.PushBack(std::forward<T>(default_value), alloc);
  }
  array[index] = rajson::to_value(std::forward<T>(value), alloc);
}

template<typename T, typename Encoding, typename Allocator>
std::optional<T> array_element(const rapidjson::GenericValue<Encoding, Allocator>& json,
  const std::string_view name, const std::size_t index)
{
  namespace rajson = dmitigr::rajson;
  const auto name_ref = rajson::to_string_ref(name);
  if (const auto i = json.FindMember(name_ref); i != json.MemberEnd()) {
    if (i->value.IsArray() && index < i->value.Size())
      return rajson::to<T>(i->value[index]);
  }
  return {};
}

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_RAJSON_HPP
