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

#include <system_error>

namespace dmitigr {

/**
 * @ingroup errors
 *
 * Generic error codes (or conditions).
 */
enum class Generic_errc {
  /// Generic error.
  generic = 1,
};

/**
 * @ingroup errors
 *
 * @returns The literal representation of the `errc`, or `nullptr`
 * if `errc` does not corresponds to any value defined by Generic_errc.
 */
constexpr const char* to_literal(const Generic_errc errc) noexcept
{
  switch (errc) {
  case Generic_errc::generic: return "generic";
  }
  return nullptr;
}

/**
 * @ingroup errors
 *
 * @returns The literal returned by `to_literal(errc)`, or literal
 * "unknown error" if `to_literal(errc)` returned `nullptr`.
 */
constexpr const char* to_literal_anyway(const Generic_errc errc) noexcept
{
  constexpr const char* unknown{"unknown error"};
  const char* const literal{to_literal(errc)};
  return literal ? literal : unknown;
}

} // namespace dmitigr

namespace std {

/**
 * @ingroup errors
 *
 * @brief The full specialization for integration with `<system_error>`.
 */
template<>
struct is_error_condition_enum<dmitigr::Generic_errc> final : true_type {};

} // namespace std

#endif  // DMITIGR_ERROR_ERRC_HPP
