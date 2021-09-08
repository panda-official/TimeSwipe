// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#ifndef PANDA_TIMESWIPE_VERSION_HPP
#define PANDA_TIMESWIPE_VERSION_HPP

namespace panda::timeswipe {

/// Current Timeswipe major version.
constexpr int version_major{0};
static_assert(0 <= version_major && version_major <= 99);

/// Current Timeswipe minor version.
constexpr int version_minor{1};
static_assert(0 <= version_minor && version_minor <= 99);

/// Current Timeswipe patch version.
constexpr int version_patch{1};
static_assert(0 <= version_patch && version_patch <= 99);

/// Current Timeswipe version.
constexpr int version{version_major*10000 + version_minor*100 + version_patch};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_VERSION_HPP
