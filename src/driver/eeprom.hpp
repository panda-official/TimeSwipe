// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_DRIVER_EEPROM_HPP
#define PANDA_TIMESWIPE_DRIVER_EEPROM_HPP

#include "../common/hat.hpp"

#include <fstream>
#include <memory>
#include <string>

namespace panda::timeswipe::driver::detail {

struct Eeprom final {
  static bool read(std::string& error)
  {
#ifndef PANDA_TIMESWIPE_FIRMWARE_EMU
    const char* i2c_file = "/sys/class/i2c-adapter/i2c-0/0-0050/eeprom";
    std::ifstream i2c(i2c_file);
    if (!i2c.is_open()) {
      //echo "24c32 0x50" > /sys/class/i2c-adapter/i2c-0/new_device
      std::ofstream ofs("/sys/class/i2c-adapter/i2c-0/new_device");
      if (!ofs.is_open()) {
        error = "Can not access i2c subsystem. Check drivers are properly loaded";
        return false;
      }
      std::string cmd_buf = "24c32 0x50\n";
      ofs.write(cmd_buf.c_str(), cmd_buf.length());
      ofs.close();
      if (!ofs) {
        error = "Create i2c failed. Check permissions";
        return false;
      }
      i2c.open(i2c_file, std::ifstream::in);
      if (!i2c.is_open()) {
        error = "Can not access i2c subsystem. Check drivers are properly loaded";
        return false;
      }
    }
    std::string str((std::istreambuf_iterator<char>(i2c)),
      std::istreambuf_iterator<char>());
    if (str.length() > 127) str.resize(127);
    auto buf = std::make_shared<CFIFO>();
    *buf += str;
    const hat::Manager manager{buf};

    // Verify EEPROM image.
    if (manager.verify() != hat::Manager::Op_result::ok) {
      error = "EEPROM verify failed";
      return false;
    }

    // The number of atoms can be obtained as following (optional)
    const auto atom_count = manager.get_atom_count();

    //Then obligatory atoms can be obtained:
    hat::atom::Vendor_info vi;
    hat::atom::Gpio_map gpio;
    (void)manager.get(vi);
    (void)manager.get(gpio);

    /*
      printf("uuid: %x-%x-%x-%x\n", vi.m_uuid[0], vi.m_uuid[1], vi.m_uuid[2], vi.m_uuid[3]);
      printf("pid: 0x%x\n", vi.m_PID);
      printf("vstr: %s\n", vi.m_vstr.c_str());
      printf("pstr: %s\n", vi.m_pstr.c_str());
      printf("GPIO:\n");
      for (int i = 0; i < 28; i++) {
      const auto& g = gpio.m_GPIO[i];
      printf("%d: func_sel: %u reserved: %u pulltype: %u is_used: %u\n", i + 2, g.func_sel, g.reserved, g.pulltype, g.is_used);
      }
    */
#endif
    return true;
  }
};

} // namespace panda::timeswipe::driver::detail

#endif  // PANDA_TIMESWIPE_DRIVER_EEPROM_HPP
