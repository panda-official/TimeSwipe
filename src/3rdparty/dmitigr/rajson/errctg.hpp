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

#ifndef DMITIGR_RAJSON_ERRCTG_HPP
#define DMITIGR_RAJSON_ERRCTG_HPP

#include "errc.hpp"

#include <string>

namespace dmitigr::rajson {

// -----------------------------------------------------------------------------
// Parse error category
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief The parse error category.
 *
 * @see Parse_exception.
 */
class Parse_error_category final : public std::error_category {
public:
  /// @returns The literal `dmitigr_rajson_parse_error`.
  const char* name() const noexcept override
  {
    return "dmitigr_rajson_parse_error";
  }

  /**
   * @returns The string that describes the error condition denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of `rapidjson::ParseErrorCode`.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    std::string result{name()};
    result += ' ';
    result += std::to_string(ev);
    if (const auto* const literal =
      rapidjson::GetParseError_En(static_cast<rapidjson::ParseErrorCode>(ev))) {
      result += ' ';
      result += literal;
    }
    return result;
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Parse_error_category.
 */
inline const Parse_error_category& parse_error_category() noexcept
{
  static const Parse_error_category result;
  return result;
}

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), parse_error_category())`.
 */
inline std::error_condition
make_error_condition(const rapidjson::ParseErrorCode errc) noexcept
{
  return std::error_condition{static_cast<int>(errc), parse_error_category()};
}

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_ERRCTG_HPP
