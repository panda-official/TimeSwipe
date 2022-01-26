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

#ifndef PANDA_TIMESWIPE_FIRMWARE_JSON_HPP
#define PANDA_TIMESWIPE_FIRMWARE_JSON_HPP

#include <cstddef>
#ifndef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#endif
#ifndef RAPIDJSON_NO_SIZETYPEDEFINE
#define RAPIDJSON_NO_SIZETYPEDEFINE 1
#endif
#ifndef RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY
#define RAPIDJSON_ALLOCATOR_DEFAULT_CHUNK_CAPACITY 32
#endif

namespace rapidjson {
using SizeType = std::size_t;
} // namespace rapidjson

#include "../3rdparty/dmitigr/3rdparty/rapidjson/document.h"
#include "../3rdparty/dmitigr/3rdparty/rapidjson/stringbuffer.h"
#include "../3rdparty/dmitigr/3rdparty/rapidjson/writer.h"

#include "../debug.hpp"
#include "error.hpp"
using namespace panda::timeswipe; // FIXME: REMOVE

#include <optional>
#include <string>
#include <type_traits>

/**
 * @brief Resets `root` to empty object and adds the member "result" with value
 * `value`.
 *
 * @par Requires
 * `root` must be owned by `resp`.
 */
inline void set_result(rapidjson::Value& root,
  rapidjson::Value&& value, rapidjson::Document::AllocatorType& alloc) noexcept
{
  root.SetObject();
  root.AddMember("result", std::move(value), alloc);
}

/**
 * @brief Resets `root` to empty object and adds the member "error" with value
 * `error.errc()` and the member "what" with value `error.what()`.
 *
 * @par Requires
 * `root` must be owned by `resp`.
 */
inline void set_error(rapidjson::Value& root, const Error& error,
  rapidjson::Document::AllocatorType& alloc) noexcept
{
  using Value = rapidjson::Value;
  root.SetObject();
  root.AddMember("error", Value{static_cast<int>(error.errc())}, alloc);
  root.AddMember("what", Value{error.what(), alloc}, alloc);
}

/// @returns A text representation of `value`.
inline std::string to_text(const rapidjson::Value& value)
{
  rapidjson::StringBuffer buf;
  rapidjson::Writer<decltype(buf)> writer{buf}; // decltype() is a workaround for GCC 7
  writer.SetMaxDecimalPlaces(3);
  return value.Accept(writer) ? std::string{buf.GetString(), buf.GetSize()} :
    std::string{};
}

// -----------------------------------------------------------------------------
// Json_value_view
// -----------------------------------------------------------------------------

/// A JSON value view.
class Json_value_view final {
public:
  /// The constructor.
  Json_value_view(rapidjson::Value* const value = {},
    rapidjson::Document::AllocatorType* const alloc = {})
    : value_{value}
    , alloc_{alloc}
  {}

  /// @returns The underlying value.
  const rapidjson::Value* value() const noexcept
  {
    return value_;
  }

  /// @overload
  rapidjson::Value* value() noexcept
  {
    return const_cast<rapidjson::Value*>(
      static_cast<const Json_value_view*>(this)->value());
  }

  /**
   * @returns The underlying value.
   *
   * @par Requires
   * `value()`.
   */
  const rapidjson::Value& value_ref() const noexcept
  {
    PANDA_TIMESWIPE_ASSERT(value_);
    return *value();
  }

  /// @overload
  rapidjson::Value& value_ref() noexcept
  {
    return const_cast<rapidjson::Value&>(
      static_cast<const Json_value_view*>(this)->value_ref());
  }

  /// @returns The underlying allocator.
  rapidjson::Document::AllocatorType* alloc() const noexcept
  {
    return alloc_;
  }

  /**
   * @returns The underlying allocator.
   *
   * @par Requires
   * `alloc()`.
   */
  rapidjson::Document::AllocatorType& alloc_ref() const noexcept
  {
    PANDA_TIMESWIPE_ASSERT(alloc_);
    return *alloc();
  }

  /// @returns `true` if `T` is a supported type.
  template<typename T>
  static constexpr bool is_type_supported() noexcept
  {
    using std::is_same_v;
    return is_same_v<T, bool> || is_same_v<T, int> || is_same_v<T, unsigned> ||
      is_same_v<T, float> || is_same_v<T, std::string>;
  }

private:
  rapidjson::Value* value_{};
  rapidjson::Document::AllocatorType* alloc_{};
};

/**
 * @brief Gets a value of type T.
 *
 * @returns Error if this instance doesn't represents a value of type T.
 *
 * @par Requires
 * `view.value()`.
 */
template<typename T>
Error get(const Json_value_view& view, T& result)
{
  static_assert(view.is_type_supported<T>());
  PANDA_TIMESWIPE_ASSERT(view.value());
  using std::is_same_v;
  const char* what{};
  if constexpr (is_same_v<T, bool>) {
    if (view.value()->IsBool())
      result = view.value()->GetBool();
    else
      what = "value is not boolean";
  } else if constexpr (is_same_v<T, int>) {
    if (view.value()->IsInt())
      result = view.value()->GetInt();
    else
      what = "value is not integer";
  } else if constexpr (is_same_v<T, unsigned>) {
    if (view.value()->IsUint())
      result = view.value()->GetUint();
    else
      what = "value is not unsigned integer";
  } else if constexpr (is_same_v<T, float>) {
    if (view.value()->IsFloat() || view.value()->IsLosslessFloat())
      result = view.value()->GetFloat();
    else
      what = "value is not float";
  } else if constexpr (is_same_v<T, std::string>) {
    if (view.value()->IsString())
      result.assign(view.value()->GetString(), view.value()->GetStringLength());
    else
      what = "value is not string";
  } else
    return Errc::bug;
  return what ? Error{Errc::generic, what} : Error{};
}

/**
 * @brief Gets a value of type T or null.
 *
 * @returns Error if this instance does represents neither a value of type T
 * nor null.
 *
 * @par Requires
 * `view.value()`.
 */
template<typename T>
Error get(const Json_value_view& view, std::optional<T>& result)
{
  PANDA_TIMESWIPE_ASSERT(view.value());
  Error err;
  if (!view.value()->IsNull()) {
    if (T res; !(err = get(view, res)))
      result = std::move(res);
  } else
    result.reset();
  return err;
}

/**
 * Sets the null value.
 *
 * @par Requires
 * `view.value() && view.alloc()`.
 */
inline Error set(Json_value_view& view, std::nullopt_t)
{
  PANDA_TIMESWIPE_ASSERT(view.value() && view.alloc());
  view.value()->SetNull();
  return {};
}

/**
 * @brief Sets the value of type T.
 *
 * @par Requires
 * `view.value() && view.alloc()`.
 */
template<typename T>
Error set(Json_value_view& view, const T& value)
{
  static_assert(view.is_type_supported<T>());
  PANDA_TIMESWIPE_ASSERT(view.value() && view.alloc());
  using std::is_same_v;
  if constexpr (is_same_v<T, bool>) {
    view.value()->SetBool(value);
  } else if constexpr (is_same_v<T, int>) {
    view.value()->SetInt(value);
  } else if constexpr (is_same_v<T, unsigned>) {
    view.value()->SetUint(value);
  } else if constexpr (is_same_v<T, float>) {
    view.value()->SetFloat(value);
  } else if constexpr (is_same_v<T, std::string>) {
    view.value()->SetString(value, *view.alloc());
  } else
    return Errc::bug;
  return {};
}

/**
 * @brief Sets the value of type T, or null if `!value`.
 *
 * @par Requires
 * `view.value() && view.alloc()`.
 */
template<typename T>
Error set(Json_value_view& view, const std::optional<T>& value)
{
  if (value)
    return set(view, *value);
  else
    return set(view, std::nullopt);
}

// -----------------------------------------------------------------------------
// Conversions for Error_result
// -----------------------------------------------------------------------------

/// Gets `mode` from the `view`.
inline Error get(const Json_value_view& view, Error_result& error)
{
  return Errc::not_implemented;
}

/// Sets `error` to the `view`.
inline Error set(Json_value_view& view, const Error_result& error)
{
  set_error(view.value_ref(), error, view.alloc_ref());
  return {};
}

#endif  // PANDA_TIMESWIPE_FIRMWARE_JSON_HPP
