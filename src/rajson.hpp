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

#ifndef PANDA_TIMESWIPE_RAJSON_HPP
#define PANDA_TIMESWIPE_RAJSON_HPP

#include "basics.hpp"

#include "3rdparty/dmitigr/assert.hpp"
#include "3rdparty/dmitigr/rajson.hpp"

#include <optional>
#include <string_view>

namespace dmitigr::rajson {
template<>
struct Conversions<panda::timeswipe::Measurement_mode> final {
  template<class Encoding, class Allocator>
  static auto from(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return static_cast<panda::timeswipe::Measurement_mode>(to<int>(value));
  }
};

template<>
struct Conversions<panda::timeswipe::Signal_mode> final {
  template<class Encoding, class Allocator>
  static auto from(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return static_cast<panda::timeswipe::Signal_mode>(to<int>(value));
  }
};
} // namespace dmitigr::rajson

namespace panda::timeswipe::detail {

/// Adds or modifies the member named by `name` by using the given `value`.
template<typename Encoding, typename Allocator, typename T>
void set_member(rapidjson::GenericValue<Encoding, Allocator>& json, Allocator& alloc,
  const std::string_view name, T&& value)
{
  namespace rajson = dmitigr::rajson;
  auto val = rajson::to<rapidjson::Value>(std::forward<T>(value), alloc);
  const auto name_ref = rajson::to<rapidjson::Value::StringRefType>(name);
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
  const auto name_ref = rajson::to<rapidjson::Value::StringRefType>(name);
  auto i = json.FindMember(name_ref);
  if (i == json.MemberEnd()) {
    json.AddMember(rapidjson::Value{name.data(), name.size(), alloc},
      rapidjson::Value{rapidjson::kArrayType}, alloc);
    i = json.FindMember(name_ref);
  } else if (!i->value.IsArray())
    i->value.SetArray();

  DMITIGR_ASSERT(i != json.MemberEnd());
  DMITIGR_ASSERT(i->value.IsArray());

  auto& array = i->value;
  if (index <= array.Size()) {
    const std::size_t extra_size = array.Size() - index + 1;
    for (std::size_t i{}; i < extra_size; ++i)
      array.PushBack(std::forward<T>(default_value), alloc);
  }
  array[index] = rajson::to<rapidjson::Value>(std::forward<T>(value), alloc);
}

template<typename T, typename Encoding, typename Allocator>
std::optional<T> array_element(const rapidjson::GenericValue<Encoding, Allocator>& json,
  const std::string_view name, const std::size_t index)
{
  namespace rajson = dmitigr::rajson;
  const auto name_ref = rajson::to<rapidjson::Value::StringRefType>(name);
  if (const auto i = json.FindMember(name_ref); i != json.MemberEnd()) {
    if (i->value.IsArray() && index < i->value.Size())
      return rajson::to<T>(i->value[index]);
  }
  return {};
}

} // namespace panda::timeswipe::detail

#endif  // PANDA_TIMESWIPE_RAJSON_HPP
