// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH
*/

#include "../../src/common/json.hpp"
#include "../../src/driver/timeswipe.hpp"

#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>

#include <unistd.h>

// #include <chrono>

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void usage(const char* name)
{
    std::cerr << "Usage: 'sudo " << name << " <command> [--index <index>] [--freq <freq>] [--high <high>] [--low <low>] [--repeats <repeats>] [--duty <duty>] [--trace-spi]'" << std::endl;
    std::cerr << "command is one of start stop get" << std::endl;
    std::cerr << "index is only valid for start or stop commands" << std::endl;
    std::cerr << "duty, freq, high, low, repeats, duty are valid for start command" << std::endl;
}

int main(int argc, char *argv[])
{
    namespace drv = panda::timeswipe::driver;
    auto& tswipe = drv::TimeSwipe::instance();

    bool start = false;
    bool stop = false;
    bool get = false;
    int index = 0;
    drv::Pwm_state state;
    state.frequency(1).high(4095).low(0).repeat_count(0).duty_cycle(.5);

    bool both = true;

    if (argc < 2) {
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

    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i],"--num")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            index = std::stoi(argv[i+1]);
            ++i;
            if (index > 1) {
                std::cerr << "index can be 0 or 1 only" << std::endl;
                usage(argv[0]);
                return 1;
            }
            both = false;
        } else if (!strcmp(argv[i],"--freq")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            state.frequency(std::stoi(argv[i+1]));
            ++i;
        } else if (!strcmp(argv[i],"--high")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            state.high(std::stoi(argv[i+1]));
            ++i;
        } else if (!strcmp(argv[i],"--low")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            state.low(std::stoi(argv[i+1]));
            ++i;
        } else if (!strcmp(argv[i],"--repeats")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            state.repeat_count(std::stoi(argv[i+1]));
            ++i;
        } else if (!strcmp(argv[i],"--duty")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            state.duty_cycle(std::stof(argv[i+1]));
            ++i;
        } else if (!strcmp(argv[i],"--trace-spi")) {
            tswipe.TraceSPI(true);
        } else {
            std::cerr << "unkown argument \"" << argv[i] << "\"" << std::endl;
            usage(argv[0]);
            return 1;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (!both && index != i) continue;
        if (start) {
          std::cout << "start " << i
                    << " freq: " << state.frequency()
                    << " high: " << state.high()
                    << " low: " << state.low()
                    << " repeats: " << state.repeat_count()
                    << " duty: " << state.duty_cycle()
                    << std::endl;
            if (!tswipe.start_pwm(i, state)) {
                std::cout << "start " << i << " failed" << std::endl;
            } else {
                std::cout << "start " << i << " succeded" << std::endl;
            }
        } else if (stop) {
            std::cout << "stop " << i << std::endl;
            if (!tswipe.stop_pwm(i)) {
                std::cout << "stop " << i << " failed" << std::endl;
            } else {
                std::cout << "stop " << i << " succeded" << std::endl;
            }
        } else if (get) {
            std::cout << "get " << i << std::endl;
            if (auto pwm = tswipe.pwm_state(i)) {
              std::cout << "get " << i
                        << " freq: " << pwm->frequency()
                        << " high: " << pwm->high()
                        << " low: " << pwm->low()
                        << " repeats: " << pwm->repeat_count()
                        << " duty: " << pwm->duty_cycle()
                        << std::endl;
            } else
              std::cout << "get " << i << " failed" << std::endl;
        }
    }
    return 0;
}
