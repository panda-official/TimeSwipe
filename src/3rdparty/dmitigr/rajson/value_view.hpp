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

#ifndef DMITIGR_RAJSON_VALUE_VIEW_HPP
#define DMITIGR_RAJSON_VALUE_VIEW_HPP

#include "conversions.hpp"
#include "exceptions.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace dmitigr::rajson {

/// A value view.
template<class GenericValue>
class Value_view final {
public:
  /// An alias of underlying type.
  using Underlying_type = GenericValue;

  /// The constructor.
  Value_view(GenericValue& value)
    : value_{value}
  {}

  /// @returns The JSON value.
  const auto& value() const
  {
    return value_;
  }

  /// @overload
  auto& value()
  {
    return value_;
  }

  /**
   * @returns The value of member named by `name` converted to type `R` by
   * using rajson::Conversions, or `std::nullopt` if either `!value().IsObject()`,
   * or no such a member presents, or if the member is `null`.
   */
  template<typename R>
  std::optional<R> optional(const std::string_view name) const
  {
    return optional__<R>(*this, name);
  }

  /// @overload
  template<typename R>
  std::optional<R> optional(const std::string_view name)
  {
    return optional__<R>(*this, name);
  }

  /**
   * @returns The instance of `std::optional<Value_view>` bound to member
   * named by `name`, or `std::nullopt` if either `!value().IsObject()`,
   * or no such a member presents, or if the member is `null`.
   */
  auto optional(const std::string_view name) const
  {
    return optional__(*this, name);
  }

  /// @overload
  auto optional(const std::string_view name)
  {
    return optional__(*this, name);
  }

  /**
   * @returns The instance of Value_view bound to member named by `name`.
   *
   * @par Requires
   * `optional(name)`.
   */
  auto mandatory(const std::string_view name) const
  {
    return mandatory__(*this, name);
  }

  /// @overload
  auto mandatory(const std::string_view name)
  {
    return mandatory__(*this, name);
  }

  /**
   * @returns The value of member named by `name` converted to type `R` by
   * using rajson::Conversions.
   *
   * @par Requires
   * `optional(name)`.
   */
  template<typename R>
  R mandatory(const std::string_view name) const
  {
    return rajson::to<R>(mandatory(name).value());
  }

private:
  Underlying_type& value_;

  template<class Value>
  static auto optional_iterator__(Value& value, const std::string_view name)
  {
    if (const auto e = value.MemberEnd(); name.empty())
      return e;
    else if (const auto m = value.FindMember(rajson::to_string_ref(name)); m != e)
      return m;
    else
      return e;
  }

  template<class ValueView>
  static auto optional__(ValueView& view, const std::string_view name)
  {
    using Value = decltype(optional_iterator__(view.value_, name)->value);
    using Result = std::conditional_t<
      std::is_const_v<typename std::remove_reference_t<decltype(view.value_)>>,
      std::optional<Value_view<std::add_const_t<Value>>>,
      std::optional<Value_view<Value>>
      >;
    if (!view.value_.IsObject())
      return Result{};
    else if (const auto i = optional_iterator__(view.value_, name);
      i != view.value_.MemberEnd() && !i->value.IsNull())
      return Result{i->value};
    else
      return Result{};
  }

  template<typename R, class ValueView>
  static std::optional<R> optional__(ValueView& view, const std::string_view name)
  {
    if (auto result = optional__(view, name))
      return rajson::to<R>(std::move(result->value()));
    else
      return std::nullopt;
  }

  template<class ValueView>
  static auto mandatory__(ValueView& view, const std::string_view name)
  {
    if (auto result = optional__(view, name); result)
      return *result;
    else
      throw Exception{std::string{"JSON member \""}.append(name).append("\"")
        .append(" not found")};
  }
};

} // namespace dmitigr::rajson

#endif // DMITIGR_RAJSON_VALUE_VIEW_HPP
