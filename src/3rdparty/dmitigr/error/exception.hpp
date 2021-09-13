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

#ifndef DMITIGR_ERROR_EXCEPTION_HPP
#define DMITIGR_ERROR_EXCEPTION_HPP

#include "errc.hpp"
#include "exception_base.hpp"
#include "std_system_error.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace dmitigr {

/**
 * @ingroup errors
 *
 * An exception class.
 */
class Exception : public Exception_base {
public:
  /**
   * The constructor of instance which represents the generic error.
   *
   * @param what The custom what-string. If ommitted, the value returned by
   * `to_literal(errc)` will be used as a what-string.
   */
  Exception(std::string what = {})
    : Exception{Errc::generic, std::move(what)}
  {}

  /**
   * The constructor.
   *
   * @param errc The error condition.
   * @param what The custom what-string. If ommitted, the value returned by
   * `to_literal(errc)` will be used as a what-string.
   */
  explicit Exception(const Errc errc, std::string what = {})
    : condition_{errc}
    , what_holder_{what.empty() ? to_literal_anyway(errc) : what}
  {}

  /// @see std::exception::what().
  const char* what() const noexcept override
  {
    return what_holder_.what();
  }

  /// @returns Error condition.
  std::error_condition condition() const noexcept
  {
    return condition_;
  }

private:
  std::error_condition condition_;
  std::runtime_error what_holder_;
};

} // namespace dmitigr

#endif  // DMITIGR_ERROR_EXCEPTION_HPP
