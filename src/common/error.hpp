// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_ERROR_HPP
#define PANDA_TIMESWIPE_ERROR_HPP

#include "../3rdparty/dmitigr/assert.hpp"

#include <cstring>
#include <stdexcept>
#include <system_error>
#include <type_traits>

namespace panda::timeswipe {

/// `true` if `NDEBUG` is not set.
constexpr bool is_debug{dmitigr::is_debug};

// -----------------------------------------------------------------------------
// Errc
// -----------------------------------------------------------------------------

/// Error codes.
enum class Errc {
  ok = 0,

  /** Generic section **/

  generic = 1,
  out_of_range = 2,

  /** PID file section **/

  pid_file = 1001,
  pid_file_lock_failed = 1002,

  /** Board section **/

  board = 2001,
  board_is_busy = 2002,
  invalid_board_state = 2003,

  /** Drift reference section **/

  drift_reference = 3001,
  invalid_drift_reference = 3002,
  no_drift_references = 3003,
  insufficient_drift_references = 3004,
  excessive_drift_references = 3005,

  /** Calibration ATOM section **/

  calibration_atom = 4001,
  invalid_calibration_atom_type = 4002,
  invalid_calibration_atom_entry_index = 4003,

  /** SPI section **/

  spi = 5001,
  spi_send = 5002,
  spi_receive = 5003,

  /** Communication protocol section **/

  com_proto = 6001,
  com_proto_invalid_request = 6002,
  com_proto_bus = 6003,
  com_proto_timeout = 6004,
  com_proto_object_not_found = 6005,
  com_proto_get_unsupported = 6006,
  com_proto_set_unsupported = 6007,
  com_proto_access_point_disabled = 6008
};

/// @returns `true` if `errc` indicates an error.
constexpr bool is_error(const Errc errc) noexcept
{
  return errc != Errc::ok;
}

/**
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Errc.
 */
constexpr const char* make_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::ok: return "ok";

  case Errc::generic: return "generic";
  case Errc::out_of_range: return "out_of_range";

  case Errc::pid_file: return "pid_file";
  case Errc::pid_file_lock_failed: return "pid_file_lock_failed";

  case Errc::board: return "board";
  case Errc::board_is_busy: return "board_is_busy";
  case Errc::invalid_board_state: return "invalid_board_state";

  case Errc::drift_reference: return "drift_reference";
  case Errc::invalid_drift_reference: return "invalid_drift_reference";
  case Errc::no_drift_references: return "no_drift_references";
  case Errc::insufficient_drift_references: return "insufficient_drift_references";
  case Errc::excessive_drift_references: return "excessive_drift_references";

  case Errc::calibration_atom: return "calibration_atom";
  case Errc::invalid_calibration_atom_type: return "invalid_calibration_atom_type";
  case Errc::invalid_calibration_atom_entry_index: return "invalid_calibration_atom_entry_index";

  case Errc::spi: return "spi";
  case Errc::spi_send: return "spi_send";
  case Errc::spi_receive: return "spi_receive";

  case Errc::com_proto: return "com_proto";
  case Errc::com_proto_invalid_request: return "com_proto_invalid_request";
  case Errc::com_proto_bus: return "com_proto_bus";
  case Errc::com_proto_timeout: return "com_proto_timeout";
  case Errc::com_proto_object_not_found: return "com_proto_object_not_found";
  case Errc::com_proto_get_unsupported: return "com_proto_get_unsupported";
  case Errc::com_proto_set_unsupported: return "com_proto_set_unsupported";
  case Errc::com_proto_access_point_disabled: return "com_proto_access_point_disabled";
  }
  return nullptr;
}

/**
 * @returns The literal returned by `make_literal(errc)`, or literal
 * `unknown error` if `make_literal(errc)` returned `nullptr`.
 */
constexpr const char* make_literal_anyway(const Errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{make_literal(errc)};
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
class Error_category final : public std::error_category {
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
    const char* const desc{make_literal_anyway(static_cast<Errc>(ev))};
    constexpr const char* const sep{": "};
    std::string result;
    result.reserve(std::strlen(name()) + std::strlen(sep) + std::strlen(desc));
    return result.append(name()).append(sep).append(desc);
  }
};

/**
 * @ingroup errors
 *
 * @returns The reference to the instance of type Error_category.
 */
inline const Error_category& error_category() noexcept
{
  static const Error_category instance;
  return instance;
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
// Basic_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * @brief An exception base class.
 *
 * @tparam StdError Must be either `std::logic_error` or `std::runtime_error`.
 */
template<class StdError>
class Basic_exception : public StdError {
  static_assert(std::is_same_v<std::logic_error, StdError> ||
    std::is_same_v<std::runtime_error, StdError>);
public:
  /**
   * The constructor of instance which represents the generic error.
   *
   * @param what The custom what-string. If ommitted, the value returned by
   * `make_literal(errc)` will be used as a what-string.
   */
  Basic_exception(std::string what = {})
    : Basic_exception{Errc::generic, std::move(what)}
  {}

  /**
   * The constructor.
   *
   * @param errc The error condition.
   * @param what The custom what-string. If ommitted, the value returned by
   * `make_literal(errc)` will be used as a what-string.
   */
  explicit Basic_exception(const Errc errc, std::string what = {})
    : StdError{what.empty() ? make_literal_anyway(errc) : what}
    , condition_{errc}
  {}

  /// @returns Error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

private:
  std::error_condition condition_;
};

// -----------------------------------------------------------------------------
// Exception_with_info
// -----------------------------------------------------------------------------

/// Exception with source info.
template<class StdError>
class Exception_with_info :
    public dmitigr::Exception_with_info<Basic_exception<StdError>> {
  using Super = dmitigr::Exception_with_info<Basic_exception<StdError>>;
  using Super::Super;
};

// -----------------------------------------------------------------------------
// Runtime_exception
// -----------------------------------------------------------------------------

using Runtime_exception = Basic_exception<std::runtime_error>;

} // namespace panda::timeswipe

/*
 * CHECK macros are for logic errors debugging and should be used in
 * implementation details only.
 */
#define PANDA_TIMESWIPE_CHECK_GENERIC(a, Base)                          \
  DMITIGR_CHECK_GENERIC(a, panda::timeswipe::Exception_with_info<Base>)
#define PANDA_TIMESWIPE_CHECK(a)                        \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::logic_error)
#define PANDA_TIMESWIPE_CHECK_ARG(a)                        \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::invalid_argument)
#define PANDA_TIMESWIPE_CHECK_DOMAIN(a)                 \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::domain_error)
#define PANDA_TIMESWIPE_CHECK_LENGTH(a)                 \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::length_error)
#define PANDA_TIMESWIPE_CHECK_RANGE(a)                  \
  PANDA_TIMESWIPE_CHECK_GENERIC(a, std::out_of_range)

// THROW macro is for runtime errors.
#define PANDA_TIMESWIPE_THROW(errc)                                     \
  do {                                                                  \
    using E = panda::timeswipe::Exception_with_info<std::runtime_error>; \
    throw E{__FILE__, __LINE__, errc};                                  \
  } while(true)

#endif  // PANDA_TIMESWIPE_ERROR_HPP
