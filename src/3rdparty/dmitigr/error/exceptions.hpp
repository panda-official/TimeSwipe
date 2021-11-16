// -*- C++ -*-
// Copyright (C) 2021 Dmitry Igrishin
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

#ifndef DMITIGR_ERROR_EXCEPTIONS_HPP
#define DMITIGR_ERROR_EXCEPTIONS_HPP

#include "errctg.hpp"
#include "exception.hpp"

#include <stdexcept> // std::runtime_error
#include <string>
#include <type_traits>

namespace dmitigr {

// -----------------------------------------------------------------------------
// Basic_generic_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * The basic generic exception class template.
 */
template<class Base = Exception>
class Basic_generic_exception : public Base {
  static_assert(std::is_base_of_v<std::exception, Base>);
public:
  /**
   * The constructor.
   *
   * @param errc The error condition.
   * @param what The what-string.
   */
  explicit Basic_generic_exception(const std::error_condition& errc,
    const std::string& what)
    : condition_{errc}
    , what_holder_{what}
  {}

  /// @overload
  explicit Basic_generic_exception(const std::string& what)
    : Basic_generic_exception{Generic_errc::generic, what}
  {}

  /// @see Exception::what().
  const char* what() const noexcept override
  {
    return what_holder_.what();
  }

  /// @see Exception::condition().
  std::error_condition condition() const noexcept override
  {
    return condition_;
  }

private:
  std::error_condition condition_;
  std::runtime_error what_holder_;
};

// -----------------------------------------------------------------------------
// Basic_debug_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * The basic debug exception class template.
 *
 * The purpose of this template is to provide the diagnostic information such as
 * the source file name and line from where the exception was thrown.
 */
template<class Base = Exception>
class Basic_debug_exception : public Basic_generic_exception<Base> {
public:
  /**
   * The constructor.
   *
   * @param file The name of file from where the exception thrown.
   * @param line The line of file from where the exception thrown.
   * @param errc The error condition.
   * @param what The what-string.
   */
  Basic_debug_exception(const char* const file, const int line,
    const std::error_condition& errc, const std::string& what)
    : Basic_generic_exception<Base>{errc, what}
    , file_{file}
    , line_{line}
  {}

  /// @overload
  Basic_debug_exception(const char* const file, const int line,
    const std::string& what)
    : Basic_debug_exception{file, line, Generic_errc::generic, what}
  {}

  /// @returns The name of file from where the exception thrown.
  const char* file() const noexcept
  {
    return file_;
  }

  /// @returns The line of file from where the exception thrown.
  int line() const noexcept
  {
    return line_;
  }

private:
  const char* file_{};
  int line_{};
};

} // namespace dmitigr

// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

/// Throws exception with what-string `what`.
#define DMITIGR_THROW(what)                     \
  do {                                          \
    throw Generic_exception{what};              \
  } while (false)

/// Throws exception with code `errc`, what-string `what`.
#define DMITIGR_THROW2(errc, what)              \
  do {                                          \
    throw Generic_exception{errc, what};        \
  } while (false)

/// Throws exception with what-string `what` and the debug information.
#define DMITIGR_THROW_DEBUG(what)                       \
  do {                                                  \
    throw Debug_exception{__FILE__, __LINE__, what};    \
  } while (false)

/// Throws exception with code `errc`, what-string `what` and the debug information.
#define DMITIGR_THROW_DEBUG2(errc, what)                    \
  do {                                                      \
    throw Debug_exception{__FILE__, __LINE__, errc, what};  \
  } while (false)

#endif  // DMITIGR_ERROR_EXCEPTIONS_HPP
