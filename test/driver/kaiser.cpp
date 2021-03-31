// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "resampler.hpp"

int main()
{
  constexpr unsigned max_down_factor{500};
  for (unsigned down{1}; down <= max_down_factor; ++down) {
    TimeSwipeResamplerOptions opts{1, down};
    TimeSwipeResampler resampler{std::move(opts)};
  }
}
