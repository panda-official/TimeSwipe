// -*- C++ -*-
// Copyright (C) 2022 Dmitry Igrishin
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
// Dmitry Igrishin
// dmitigr@gmail.com

#ifndef DMITIGR_RAJSON_CONVERSIONS_HPP
#define DMITIGR_RAJSON_CONVERSIONS_HPP

#include "fwd.hpp"
#include "exceptions.hpp"
#include "../3rdparty/rapidjson/document.h"
#include "../3rdparty/rapidjson/schema.h"
#include "../3rdparty/rapidjson/stringbuffer.h"
#include "../3rdparty/rapidjson/writer.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace dmitigr::rajson {

/// The centralized "namespace" for conversion algorithms implementations.
template<typename, typename = void> struct Conversions;

/**
 * @returns The result of conversion of `value` to a JSON text. (Aka serialization.)
 *
 * @throws Exception on error.
 */
template<class Encoding, class Allocator>
std::string to_text(const rapidjson::GenericValue<Encoding, Allocator>& value)
{
  rapidjson::StringBuffer buf;
  rapidjson::Writer<decltype(buf)> writer{buf}; // decltype() required for GCC 7
  if (!value.Accept(writer))
    throw Exception{"cannot convert JSON value to text representation"};
  return std::string{buf.GetString(), buf.GetSize()};
}

/**
 * @returns The result of parsing a JSON text. (Aka deserialization.)
 *
 * @throw Parse_exception on parse error.
 */
inline rapidjson::Document to_document(const std::string_view input)
{
  rapidjson::Document result;
  const rapidjson::ParseResult pr{result.Parse(input.data(), input.size())};
  if (!pr)
    throw Parse_exception{pr};
  return result;
}

/// @returns The RapidJSON's string reference.
inline auto to_string_ref(const std::string_view value)
{
  return rapidjson::StringRef(value.data(), value.size());
}

/**
 * @returns The result of conversion of `value` to the value of type `Destination`
 * by using specializations of the template structure Conversions.
 */
template<typename Destination, class Encoding, class Allocator, typename ... Types>
Destination to(const rapidjson::GenericValue<Encoding, Allocator>& value, Types&& ... args)
{
  using Dst = std::decay_t<Destination>;
  return Conversions<Dst>::to_type(value, std::forward<Types>(args)...);
}

/// @returns The result of conversion of `value` to the RapidJSON generic value.
template<class Encoding, class Allocator, typename Source, typename ... Types>
rapidjson::GenericValue<Encoding, Allocator> to_value(Source&& value, Types&& ... args)
{
  using Src = std::decay_t<Source>;
  return Conversions<Src>::template to_value<Encoding,
    Allocator>(std::forward<Source>(value), std::forward<Types>(args)...);
}

/// @overload
template<typename Source, typename ... Types>
rapidjson::Value to_value(Source&& value, Types&& ... args)
{
  using E = rapidjson::Value::EncodingType;
  using A = rapidjson::Value::AllocatorType;
  return to_value<E, A>(std::forward<Source>(value), std::forward<Types>(args)...);
}

// -----------------------------------------------------------------------------
// Conversions specializations
// -----------------------------------------------------------------------------

/// Generic implementation of conversion routines for arithmetic types.
struct Arithmetic_generic_conversions {
  template<class Encoding, class Allocator, typename T>
  static auto to_value(const T value, Allocator&)
  {
    static_assert(std::is_arithmetic_v<std::decay_t<T>>);
    return rapidjson::GenericValue<Encoding, Allocator>{value};
  }
};

/// Full specialization for `bool`.
template<>
struct Conversions<bool> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsBool())
      return value.GetBool();

    throw Exception{"cannot convert JSON value to bool"};
  }
};

/// Full specialization for `std::uint8_t`.
template<>
struct Conversions<std::uint8_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsUint()) {
      const auto result = value.GetUint();
      if (result <= std::numeric_limits<std::uint8_t>::max())
        return static_cast<std::uint8_t>(result);
    }
    throw Exception{"cannot convert JSON value to std::uint8_t"};
  }
};

/// Full specialization for `std::uint16_t`.
template<>
struct Conversions<std::uint16_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsUint()) {
      const auto result = value.GetUint();
      if (result <= std::numeric_limits<std::uint16_t>::max())
        return static_cast<std::uint16_t>(result);
    }
    throw Exception{"cannot convert JSON value to std::uint16_t"};
  }
};

/// Full specialization for `std::uint32_t`.
template<>
struct Conversions<std::uint32_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsUint()) {
      const auto result = value.GetUint();
      if (result <= std::numeric_limits<std::uint32_t>::max())
        return static_cast<std::uint32_t>(result);
    }
    throw Exception{"cannot convert JSON value to std::uint32_t"};
  }
};

/// Full specialization for `std::uint64_t`.
template<>
struct Conversions<std::uint64_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsUint64())
      return static_cast<std::uint64_t>(value.GetUint64());

    throw Exception{"cannot convert JSON value to std::uint64_t"};
  }
};

/// Full specialization for `std::int8_t`.
template<>
struct Conversions<std::int8_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsInt()) {
      const auto result = value.GetInt();
      if (std::numeric_limits<std::int8_t>::min() <= result &&
        result <= std::numeric_limits<std::int8_t>::max())
        return static_cast<std::int8_t>(result);
    }
    throw Exception{"cannot convert JSON value to std::int8_t"};
  }
};

/// Full specialization for `std::int16_t`.
template<>
struct Conversions<std::int16_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsInt()) {
      const auto result = value.GetInt();
      if (std::numeric_limits<std::int16_t>::min() <= result &&
        result <= std::numeric_limits<std::int16_t>::max())
        return static_cast<std::int16_t>(result);
    }
    throw Exception{"cannot convert JSON value to std::int16_t"};
  }
};

/// Full specialization for `std::int32_t`.
template<>
struct Conversions<std::int32_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsInt()) {
      const auto result = value.GetInt();
      if (std::numeric_limits<std::int32_t>::min() <= result &&
        result <= std::numeric_limits<std::int32_t>::max())
        return static_cast<std::int32_t>(result);
    }
    throw Exception{"cannot convert JSON value to std::int32_t"};
  }
};

/// Full specialization for `std::int64_t`.
template<>
struct Conversions<std::int64_t> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsInt64())
      return static_cast<std::int64_t>(value.GetInt64());

    throw Exception{"cannot convert JSON value to std::int64_t"};
  }
};

/// Full specialization for `float`.
template<>
struct Conversions<float> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsFloat() || value.IsLosslessFloat())
      return value.GetFloat();

    throw Exception{"cannot convert JSON value to float"};
  }
};

/// Full specialization for `double`.
template<>
struct Conversions<double> final : Arithmetic_generic_conversions {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsDouble() || value.IsLosslessDouble())
      return value.GetDouble();

    throw Exception{"cannot convert JSON value to double"};
  }
};

/// Full specialization for `std::string`.
template<>
struct Conversions<std::string> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsString())
      return std::string{value.GetString(), value.GetStringLength()};

    throw Exception{"cannot convert JSON value to std::string"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::string& value, Allocator& alloc)
  {
    // Copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value, alloc};
  }
};

/// Full specialization for `std::string_view`.
template<>
struct Conversions<std::string_view> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsString())
      return std::string_view{value.GetString(), value.GetStringLength()};

    throw Exception{"cannot convert JSON value to std::string_view"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::string_view value, Allocator&)
  {
    // Don't copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value.data(), value.size()};
  }
};

/// Full specialization for `const char*`.
template<>
struct Conversions<const char*> final {
  template<class Encoding, class Allocator>
  static auto to_value(const char* const value, Allocator& alloc)
  {
    // Don't copy `value` to result.
    return rapidjson::GenericValue<Encoding, Allocator>{value, alloc};
  }
};

/// Partial specialization for enumeration types.
template<typename T>
struct Conversions<T, std::enable_if_t<std::is_enum_v<T>>> {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return static_cast<T>(to<std::underlying_type_t<T>>(value));
  }

  template<class Encoding, class Allocator>
  static auto to_value(const T value, Allocator&)
  {
    return rapidjson::GenericValue<Encoding, Allocator>(static_cast<std::underlying_type_t<T>>(value));
  }
};

/// Partial specialization for `std::vector<T>`.
template<typename T>
struct Conversions<std::vector<T>> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsArray()) {
      const auto arr = value.GetArray();
      std::vector<T> result;
      result.reserve(arr.Size());
      for (const auto& val : arr) {
        if (val.IsNull())
          throw Exception{"cannot emplace NULL value to std::vector<T>"};
        result.emplace_back(to<T>(val));
      }
      return result;
    }

    throw Exception{"cannot convert JSON value to std::vector<T>"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::vector<T>& value, Allocator& alloc)
  {
    rapidjson::GenericValue<Encoding, Allocator> result{rapidjson::kArrayType};
    result.Reserve(value.size(), alloc);
    for (const auto& val : value)
      result.PushBack(rajson::to_value<Encoding, Allocator>(val, alloc), alloc);
    return result;
  }
};

/// Partial specialization for `std::vector<std::optional<T>>`.
template<typename T>
struct Conversions<std::vector<std::optional<T>>> final {
  template<class Encoding, class Allocator>
  static auto to_type(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    if (value.IsArray()) {
      const auto arr = value.GetArray();
      std::vector<std::optional<T>> result;
      result.reserve(arr.Size());
      for (const auto& val : arr)
        result.emplace_back(!val.IsNull() ? to<T>(val) : std::optional<T>{});
      return result;
    }

    throw Exception{"cannot convert JSON value to std::vector<std::optional<T>>"};
  }

  template<class Encoding, class Allocator>
  static auto to_value(const std::vector<std::optional<T>>& value, Allocator& alloc)
  {
    using Value = rapidjson::GenericValue<Encoding, Allocator>;
    Value result{rapidjson::kArrayType};
    result.Reserve(value.size(), alloc);
    for (const auto& val : value)
      result.PushBack(val ? rajson::to_value<Encoding, Allocator>(*val, alloc) : Value{}, alloc);
    return result;
  }
};

/// Partial specialization for `rapidjson::GenericValue`.
template<class Encoding, class Allocator>
struct Conversions<rapidjson::GenericValue<Encoding, Allocator>> final {
  using Type = rapidjson::GenericValue<Encoding, Allocator>;

  template<class E, class A>
  static auto to_type(Type&& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return std::move(value);
  }

  template<class E, class A>
  static auto to_type(const Type& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    Type result;
    result.CopyFrom(value, result.GetAllocator(), true);
    return result;
  }

  template<class E, class A>
  static auto to_value(Type&& value, Allocator&)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return std::move(value);
  }

  template<class E, class A>
  static auto to_value(const Type& value, Allocator&)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return to_type(value);
  }
};

/// Partial specialization for `rapidjson::GenericDocument`.
template<class Encoding, class Allocator, class StackAllocator>
struct Conversions<rapidjson::GenericDocument<Encoding, Allocator, StackAllocator>> final {
  using Type = rapidjson::GenericDocument<Encoding, Allocator, StackAllocator>;
  using Value_type = rapidjson::GenericValue<Encoding, Allocator>;

  template<class E, class A>
  static auto to_type(Type&& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    return std::move(value);
  }

  template<class E, class A>
  static auto to_type(const Type& value)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    Type result;
    result.CopyFrom(value, result.GetAllocator(), true);
    return result;
  }

  template<class E, class A>
  static auto to_value(const Type& value, Allocator& alloc)
  {
    static_assert(std::is_same_v<E, Encoding> && std::is_same_v<A, Allocator>);
    Value_type result;
    result.CopyFrom(value, alloc, true);
    return result;
  }
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_CONVERSIONS_HPP
