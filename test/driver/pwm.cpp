// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH
*/

#include "../../src/driver.hpp"
#include "../../src/common/json.hpp"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include <unistd.h>

// #include <chrono>

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void usage(const char* name)
{
    std::cerr << "Usage: 'sudo " << name << " <pwm_index> <command> [--freq <freq>] [--high <high>] [--low <low>] [--repeats <repeats>] [--duty <duty>] [--trace-spi]'" << std::endl;
    std::cerr << "command is one of start stop get" << std::endl;
    std::cerr << "index is only valid for start or stop commands" << std::endl;
    std::cerr << "duty, freq, high, low, repeats, duty are valid for start command" << std::endl;
}

int main(int argc, char *argv[])
{
    namespace drv = panda::timeswipe::driver;
    auto& tswipe = drv::Timeswipe::instance();

    bool start = false;
    bool stop = false;
    bool get = false;
    int pwm_index{};
    drv::Timeswipe_state state;
    for (int i{}; i < 2; ++i) {
      state
        .set_pwm_frequency(i, 1)
        .set_pwm_high(i, 4095)
        .set_pwm_low(i, 0)
        .set_pwm_repeat_count(i, 0)
        .set_pwm_duty_cycle(i, .5);
    }

    bool both = true;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    std::string command = argv[1];
    if (command == "start") start = true;
    else if (command == "stop") stop = true;
    else if (command == "get") get = true;
    else {
      usage(argv[0]);
      return 1;
    }

    pwm_index = std::stoi(argv[2]);
    if (pwm_index < 0 || pwm_index > 1) {
      std::cerr << "index must be in range [0, 1]" << std::endl;
      usage(argv[0]);
      return 1;
    }
    both = false;

    for (int i{3}; i < argc; ++i) {
      if (!strcmp(argv[i],"--freq")) {
        if (i+1 > argc) {
          usage(argv[0]);
          return 1;
        }
        state.set_pwm_frequency(pwm_index, std::stoi(argv[i+1]));
        ++i;
      } else if (!strcmp(argv[i],"--high")) {
        if (i+1 > argc) {
          usage(argv[0]);
          return 1;
        }
        state.set_pwm_high(pwm_index, std::stoi(argv[i+1]));
        ++i;
      } else if (!strcmp(argv[i],"--low")) {
        if (i+1 > argc) {
          usage(argv[0]);
          return 1;
        }
        state.set_pwm_low(pwm_index, std::stoi(argv[i+1]));
        ++i;
      } else if (!strcmp(argv[i],"--repeats")) {
        if (i+1 > argc) {
          usage(argv[0]);
          return 1;
        }
        state.set_pwm_repeat_count(pwm_index, std::stoi(argv[i+1]));
        ++i;
      } else if (!strcmp(argv[i],"--duty")) {
        if (i+1 > argc) {
          usage(argv[0]);
          return 1;
        }
        state.set_pwm_duty_cycle(pwm_index, std::stof(argv[i+1]));
        ++i;
      } else {
        std::cerr << "unkown argument \"" << argv[i] << "\"" << std::endl;
        usage(argv[0]);
        return 1;
      }
    }

    for (int i{}; i < 2; ++i) {
      if (!both && pwm_index != i) continue;
      if (start) {
        std::cout << "start " << i
                  << " freq: " << *state.get_pwm_frequency(i)
                  << " high: " << *state.get_pwm_high(i)
                  << " low: " << *state.get_pwm_low(i)
                  << " repeats: " << *state.get_pwm_repeat_count(i)
                  << " duty: " << *state.get_pwm_duty_cycle(i)
                  << std::endl;
        state.set_pwm_start(i, true);
        tswipe.set_state(state);
        std::cout << "start " << i << " succeded" << std::endl;
      } else if (stop) {
        std::cout << "stop " << i << std::endl;
        state.set_pwm_start(i, false);
        tswipe.set_state(state);
        std::cout << "stop " << i << " succeded" << std::endl;
      } else if (get) {
        std::cout << "get " << i << std::endl;
        const auto& state = tswipe.get_state();
        std::cout << "get " << i
                  << " freq: " << *state.get_pwm_frequency(i)
                  << " high: " << *state.get_pwm_high(i)
                  << " low: " << *state.get_pwm_low(i)
                  << " repeats: " << *state.get_pwm_repeat_count(i)
                  << " duty: " << *state.get_pwm_duty_cycle(i)
                  << std::endl;
      }
    }

    return 0;
}
