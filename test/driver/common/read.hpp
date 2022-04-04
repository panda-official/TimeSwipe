// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2022 PANDA GmbH / Dmitry Igrishin
*/

#ifndef COMMON_READ_HPP
#define COMMON_READ_HPP

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

inline std::vector<double> read_whole_column(const std::string& path)
{
  std::ifstream in{path, std::ios_base::in | std::ios_base::binary};
  if (!in) {
    std::cerr << "cannot open file " << path << "\n";
    std::exit(1);
  }
  std::string line(128, '\0');
  std::vector<double> result;
  while (in) {
    in.getline(line.data(), line.size());
    using Size = std::string::size_type;
    if (const Size gcount = in.gcount(); in || (gcount && (gcount < line.size()))) {
      const auto value = stod(line); // stod() discards leading whitespaces
      result.push_back(value);
    }
  }
  return result;
}

#endif  // COMMON_READ_HPP
