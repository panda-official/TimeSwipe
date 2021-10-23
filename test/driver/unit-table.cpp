// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/table.hpp"
#include "../../src/error_detail.hpp"

#include <iostream>
#include <vector>

#define ASSERT PANDA_TIMESWIPE_ASSERT

int main()
try {
  namespace ts = panda::timeswipe;
  using Table = ts::Table<float>;

  Table tab;

  // Empty table.
  ASSERT(!tab.column_count());
  ASSERT(!tab.row_count());

  // Table with N columns.
  tab = Table(3);
  ASSERT(tab.column_count() == 3);
  ASSERT(!tab.row_count());

  // Add columns.
  tab = Table();
  tab.append_generated_column([](auto){return 0;});
  ASSERT(tab.column_count() == 1);
  tab.append_generated_column([](auto){return 0;});
  ASSERT(tab.column_count() == 2);
  tab.append_generated_column([](auto){return 0;});
  ASSERT(tab.column_count() == 3);

  // Add rows.
  tab.append_generated_row([](auto){return 1;});
  ASSERT(tab.row_count() == 1);
  ASSERT(tab.value(0, 0) == 1);

  // Set values.
  tab.value(0, 0) = 3;
  ASSERT(tab.value(0, 0) == 3);
  tab.value(1, 0) = 5;
  ASSERT(tab.value(1, 0) == 5);
  tab.value(2, 0) = 7;
  ASSERT(tab.value(2, 0) == 7);

  // Add row with emplaced values.
  tab.append_emplaced_row(2, 4, 6);
  ASSERT(tab.value(0, 1) == 2);
  ASSERT(tab.value(1, 1) == 4);
  ASSERT(tab.value(2, 1) == 6);
 } catch (const std::exception& e) {
  std::cerr << "error: " << e.what() << std::endl;
  return 1;
 } catch (...) {
  std::cerr << "unknown error\n";
  return 2;
 }
