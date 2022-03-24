// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2022 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"

#include <iostream>

int main(const int argc, const char* const argv[])
{
  namespace ts = panda::timeswipe;
  auto& drv = ts::Driver::instance().initialize();
  std::string_view criteria;
  if (argc > 1)
    criteria = argv[1];
  std::cout << drv.board_settings(criteria).to_json_text() << std::endl;
}
