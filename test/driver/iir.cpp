// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2022 PANDA GmbH / Dmitry Igrishin
*/

#include "common/read.hpp"
#include "../../src/3rdparty/dmitigr/progpar/progpar.hpp"

namespace progpar = dmitigr::progpar;

inline progpar::Program_parameters params;

[[noreturn]] inline void usage()
{
  std::cerr << "usage: iir --target-sample-rate=<int> --source-sample-rate=<int> "
            << "[--cutoff-freq=<float>] file.csv\n";
  std::exit(1);
}

int main(const int argc, const char* const argv[])
{
  // Parse arguments.
  params = progpar::Program_parameters{argc, argv};
  try {
    const auto target_sample_rate =
      stoi(params["target-sample-rate"].not_empty_value());
    const auto source_sample_rate =
      stoi(params["source-sample-rate"].not_empty_value());
    const auto cutoff_freq =
      stof(params["cutoff-freq"].value_or("0.25"));
    const auto& file = params[0];
  } catch (...) {
    usage();
  }

  // Set output precision.
  std::clog.precision(std::numeric_limits<double>::max_digits10);
}
