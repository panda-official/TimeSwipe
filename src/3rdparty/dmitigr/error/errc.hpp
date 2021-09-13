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

#ifndef DMITIGR_ERROR_ERRC_HPP
#define DMITIGR_ERROR_ERRC_HPP

namespace dmitigr {

/// Error codes.
enum class Errc {
  ok = 0,

  /** Generic section **/

  generic = 1,
  invalid_argument = 2,
  out_of_range = 3,
  length_error = 4
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
constexpr const char* to_literal(const Errc errc) noexcept
{
  switch (errc) {
  case Errc::ok: return "ok";

  case Errc::generic: return "generic";
  case Errc::invalid_argument: return "invalid_argument";
  case Errc::out_of_range: return "out_of_range";
  case Errc::length_error: return "length_error";
  }
  return nullptr;
}

/**
 * @returns The literal returned by `to_literal(errc)`, or literal
 * `unknown error` if `to_literal(errc)` returned `nullptr`.
 */
constexpr const char* to_literal_anyway(const Errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

} // namespace dmitigr

#endif  // DMITIGR_ERROR_ERRC_HPP
