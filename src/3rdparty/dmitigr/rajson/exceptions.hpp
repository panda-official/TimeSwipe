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

#ifndef DMITIGR_RAJSON_EXCEPTIONS_HPP
#define DMITIGR_RAJSON_EXCEPTIONS_HPP

#include "errctg.hpp"
#include "../error/exceptions.hpp"

#include <stdexcept> // std::runtime_error

namespace dmitigr::rajson {

// -----------------------------------------------------------------------------
// Exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * The base exception class.
 */
class Exception : public dmitigr::Exception {};

// -----------------------------------------------------------------------------
// Generic_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * The generic exception class.
 */
class Generic_exception final : public Basic_generic_exception<Exception> {
  using Basic_generic_exception::Basic_generic_exception;
};

// -----------------------------------------------------------------------------
// Parse_exception
// -----------------------------------------------------------------------------

/**
 * @ingroup errors
 *
 * The exception denotes parse error.
 */
class Parse_exception final : public Exception {
public:
  /// The constructor.
  explicit Parse_exception(const rapidjson::ParseResult pr)
    : pr_{pr}
    , what_holder_{rapidjson::GetParseError_En(pr.Code())}
  {}

  /// @see Exception::what().
  const char* what() const noexcept override
  {
    return what_holder_.what();
  }

  /// @see Exception::condition().
  std::error_condition condition() const noexcept override
  {
    return make_error_condition(pr_.Code());
  }

  /// @returns A parse result.
  const rapidjson::ParseResult& parse_result() const noexcept
  {
    return pr_;
  }

private:
  rapidjson::ParseResult pr_;
  std::runtime_error what_holder_;
};

} // namespace dmitigr::rajson

#endif  // DMITIGR_RAJSON_EXCEPTIONS_HPP
