// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_DRIVER_ERROR_HPP
#define PANDA_TIMESWIPE_DRIVER_ERROR_HPP

#include <stdexcept>
#include <system_error>

namespace panda::timeswipe::driver {

// -----------------------------------------------------------------------------
// Errc
// -----------------------------------------------------------------------------

/// Error codes.
enum class Errc {
  /// No error.
  kOk = 0,

  /// Generic error.
  kGeneric = 1,

  /// Pid file lock failed.
  kPidFileLockFailed = 2,

  /// Board is busy.
  kBoardIsBusy = 1001,

  /// Invalid drift reference.
  kInvalidDriftReference = 2001,

  /// No drift references calculated.
  kNoDriftReferences = 2002,

  /// Insufficient drift reference count.
  kInsufficientDriftReferences = 2003,

  /// Excessive drift reference count.
  kExcessiveDriftReferences = 2004
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
  case Errc::kPidFileLockFailed: return "PID file lock failed";
  case Errc::kBoardIsBusy: return "board is busy";
  case Errc::kInvalidDriftReference: return "invalid drift reference";
  case Errc::kNoDriftReferences: return "no drift references";
  case Errc::kInsufficientDriftReferences: return "insufficient drift references";
  case Errc::kExcessiveDriftReferences: return "excessive drift references";
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
  /// @returns The literal `panda_timeswipe_driver_error`.
  const char* name() const noexcept override
  {
    return "panda_timeswipe_driver_error";
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
// std::error_condition maker
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), error_category())`.
 */
inline std::error_condition make_error_condition(const Errc errc) noexcept
{
  return std::error_condition{static_cast<int>(errc), error_category()};
}

} // namespace panda::timeswipe::driver

// -----------------------------------------------------------------------------
// Integration with std::system_error
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<> struct is_error_condition_enum<panda::timeswipe::driver::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe::driver {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception.
 */
class Exception final : public std::exception {
public:
  /// The constructor.
  explicit Exception(const Errc errc, std::string what = {})
    : condition_{errc}
    , what_holder_{what.empty() ? to_literal(errc) :
        what.append(" (").append(to_literal(errc)).append(")")}
  {}

  /// @returns Error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

  /// @returns A "what string".
  const char* what() const noexcept override
  {
    return what_holder_.what();
  }

private:
  std::error_condition condition_;
  std::runtime_error what_holder_;
};

} // namespace panda::timeswipe::driver

#endif  // PANDA_TIMESWIPE_DRIVER_ERROR_HPP
