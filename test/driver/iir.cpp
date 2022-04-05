// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2022 PANDA GmbH / Dmitry Igrishin
*/

#include "common/read.hpp"
#include "../../src/3rdparty/dmitigr/progpar/progpar.hpp"
#include "../../src/iir_filter.hpp"

#include <iostream>
#include <stdexcept>
#include <tuple>

namespace progpar = dmitigr::progpar;
namespace ts = panda::timeswipe;
namespace test = ts::test;

inline progpar::Program_parameters params;

[[noreturn]] inline void usage()
{
  std::cerr << "usage: iir --input-freq=<int> --target-freq=<int> "
            << "[--cutoff-freq=<float>] [file.csv]\n";
  std::exit(1);
}

int main(const int argc, const char* const argv[])
{
  // Parse arguments.
  params = progpar::Program_parameters{argc, argv};
  const auto [input_freq, target_freq, cutoff_freq, file] =
    []
    {
      try {
        const auto inf = stoi(params["input-freq"].not_empty_value());
        const auto taf = stoi(params["target-freq"].not_empty_value());
        const auto cuf = stod(params["cutoff-freq"].value_or("0.25"));

        const auto param_count = params.arguments().size();
        if (param_count > 1)
          throw std::runtime_error{""};
        const auto& file = param_count && !params[0].empty() ? params[0] : "-";
        return std::make_tuple(inf, taf, cuf, file);
      } catch (...) {
        usage();
      }
    }();

  // Make IIR filter.
  using ts::detail::Iir_filter;
  Iir_filter filter{input_freq, target_freq, cutoff_freq};

  // Read the input from either the specified file or standard input.
  auto data = file == "-" ?
    test::read_whole_column(std::cin) : test::read_whole_column(file);

  // Filter the input and write the standard output.
  std::cout.precision(std::numeric_limits<double>::max_digits10);
  for (auto& d : data)
    std::cout << filter(d) << '\n';
}
