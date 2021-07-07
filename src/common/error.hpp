// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_ERROR_HPP
#define PANDA_TIMESWIPE_ERROR_HPP

#include <cstring>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace panda::timeswipe {

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
constexpr bool IsError(const Errc errc) noexcept
{
  return errc != Errc::kOk;
}

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* ToLiteral(const Errc errc) noexcept
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

/**
 * @returns The literal returned by `ToLiteral(errc)`, or literal `unknown error`
 * if `ToLiteral(errc)` returned `nullptr`.
 */
constexpr const char* ToLiteralAnyway(const Errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{ToLiteral(errc)};
  return literal ? literal : unknown;
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
  /// @returns The literal `panda_timeswipe_error`.
  const char* name() const noexcept override
  {
    return "panda_timeswipe_error";
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
    const char* const desc{ToLiteralAnyway(static_cast<Errc>(ev))};
    constexpr const char* const sep{": "};
    std::string result;
    result.reserve(std::strlen(name()) + std::strlen(sep) + std::strlen(desc));
    return result.append(name()).append(sep).append(desc);
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type ErrorCategory.
 */
inline const ErrorCategory& ErrorCategoryInstance() noexcept
{
  static const ErrorCategory instance;
  return instance;
}

// -----------------------------------------------------------------------------
// std::error_condition maker
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @returns `std::error_condition(int(errc), ErrorCategoryInstance())`.
 */
inline std::error_condition make_error_condition(const Errc errc) noexcept
{
  return std::error_condition{static_cast<int>(errc), ErrorCategoryInstance()};
}

} // namespace panda::timeswipe

// -----------------------------------------------------------------------------
// Integration with std::system_error
// -----------------------------------------------------------------------------

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<> struct is_error_condition_enum<panda::timeswipe::Errc> final : true_type {};

} // namespace std

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception base class.
 *
 * @tparam StdError Must be either `std::logic_error` or `std::runtime_error`.
 */
template<class StdError>
class BasicExceptionBase : public StdError {
public:
  /// @returns Error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

protected:
  static_assert(std::is_same_v<std::logic_error, StdError> ||
    std::is_same_v<std::runtime_error, StdError>);

  /**
   * The constructor.
   *
   * @param errc The error condition.
   * @param what The custom what-string. If ommitted, the value returned by
   * `ToLiteral(errc)` will be used as a what-string.
   */
  explicit BasicExceptionBase(const Errc errc, std::string what = {})
    : StdError{what.empty() ? ToLiteralAnyway(errc) : what}
    , condition_{errc}
  {}

private:
  std::error_condition condition_;
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_ERROR_HPP
