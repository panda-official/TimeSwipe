// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_FIRMWARE_ERROR_HPP
#define PANDA_TIMESWIPE_FIRMWARE_ERROR_HPP

#include <stdexcept>
#include <system_error>

namespace panda::timeswipe::firmware {

// -----------------------------------------------------------------------------
// Errc
// -----------------------------------------------------------------------------

/// Error codes.
enum class Errc {
  /// No error.
  kOk = 0,

  /// Generic error.
  kGeneric = 1

  /// First error.
  // kFirstError = 1001
};

/// @returns `true` if `errc` indicates an error.
constexpr bool is_error(const Errc errc) noexcept
{
  return errc != Errc::kOk;
}

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::kOk: return "ok";
  case Errc::kGeneric: return "generic error";
  }
  return nullptr;
}

// -----------------------------------------------------------------------------
// ErrorCategory
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief A category of driver errors.
 */
class ErrorCategory final : public std::error_category {
public:
  /// @returns The literal `panda_timeswipe_firmware_error`.
  const char* name() const noexcept override
  {
    return "panda_timeswipe_firmware_error";
  }

  /**
   * @returns The string that describes the error condition denoted by `ev`.
   *
   * @par Requires
   * `ev` must corresponds to the value of Errc.
   *
   * @remarks The caller should not rely on the return value as it is a
   * subject to change.
   */
  std::string message(const int ev) const override
  {
    std::string result{name()};
    result += ' ';
    result += std::to_string(ev);
    if (const char* const literal = to_literal(static_cast<Errc>(ev))) {
      result += ' ';
      result += literal;
    }
    return result;
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type ErrorCategory.
 */
inline const ErrorCategory& error_category() noexcept
{
  static const ErrorCategory result;
  return result;
}

// -----------------------------------------------------------------------------
// std::error_code maker
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @returns `std::error_code(int(errc), error_category())`.
 */
inline std::error_code make_error_code(const Errc errc) noexcept
{
  return std::error_code{static_cast<int>(errc), error_category()};
}

} // namespace panda::timeswipe::firmware

// -----------------------------------------------------------------------------
// Integration with std::system_error
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<> struct is_error_code_enum<panda::timeswipe::firmware::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe::firmware {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception.
 */
class Exception final : public std::system_error {
public:
  /// The constructor.
  explicit Exception(const Errc errc)
    : system_error{errc}
  {}
};

} // namespace panda::timeswipe::firmware

// FIXME: remove when entire code base will be placed into the namespace.
using namespace panda::timeswipe::firmware;

#endif  // PANDA_TIMESWIPE_FIRMWARE_ERROR_HPP
