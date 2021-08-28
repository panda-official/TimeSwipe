// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver/resampler.hpp"

#include <thread>
#include <vector>

int main()
{
  using namespace panda::timeswipe::driver::detail;
  const unsigned thread_count{std::thread::hardware_concurrency()};
  std::vector<std::thread> threads(thread_count);
  constexpr unsigned max_factor{500};
  const auto rest{max_factor % thread_count};
  const auto step{max_factor / thread_count};
  for (unsigned i{}; i < thread_count; ++i) {
    const auto r{(i == thread_count - 1) ? rest : 0};
    threads[i] = std::thread{[up_from = step*i + 1, up_to = step * (i + 1) + r]
    {
      for (unsigned up{up_from}; up <= up_to; ++up) {
        for (unsigned down{1}; down <= max_factor; ++down) {
          Resampler_options opts{up, down};
          Resampler resampler{std::move(opts)};
        }
      }
    }};
  }
  for (auto& t : threads)
    t.join();
}
