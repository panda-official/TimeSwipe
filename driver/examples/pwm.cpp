#include <nlohmann/json.hpp>

#include <iostream>

#include <csignal>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <functional>
#include <thread>
#include <chrono>
#include "timeswipe.hpp"

// #include <chrono>

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void usage(const char* name)
{
    std::cerr << "Usage: 'sudo " << name << " <command> [--num <num>] [--freq <freq>] [--high <high>] [--low <low>] [--repeats <repeats>] [--duty <duty>] [--trace-spi]'" << std::endl;
    std::cerr << "command is one of start stop get" << std::endl;
    std::cerr << "num is only valid for start or stop commands" << std::endl;
    std::cerr << "duty, freq, high, low, repeats, duty are valid for start command" << std::endl;
}

int main(int argc, char *argv[])
{
    TimeSwipe tswipe;

    bool start = false;
    bool stop = false;
    bool get = false;
    uint8_t num = 0;
    uint32_t freq = 1;
    uint32_t high = 4095;
    uint32_t low = 0;
    uint32_t repeats = 0;
    float duty = 0.5;

    bool num_both = true;

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

    for (unsigned i = 2; i < argc; i++) {
        if (!strcmp(argv[i],"--num")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            num = atoi(argv[i+1]);
            ++i;
            if (num > 1) {
                std::cerr << "num can be 0 or 1 only" << std::endl;
                usage(argv[0]);
                return 1;
            }
            num_both = false;
        } else if (!strcmp(argv[i],"--freq")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            freq = atoi(argv[i+1]);
            ++i;
        } else if (!strcmp(argv[i],"--high")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            high = atoi(argv[i+1]);
            ++i;
        } else if (!strcmp(argv[i],"--low")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            low = atoi(argv[i+1]);
            ++i;
        } else if (!strcmp(argv[i],"--repeats")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            repeats = atoi(argv[i+1]);
            ++i;
        } else if (!strcmp(argv[i],"--duty")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            duty = atof(argv[i+1]);
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
        if (!num_both && num != i) continue;
        if (start) {
            std::cout << "start " << i <<" freq: " << freq << " high: " << high << " low: " << low << " repeats: " << repeats << " duty: " << duty << std::endl;
            if (!tswipe.StartPWM(i, freq, high, low, repeats, duty)) {
                std::cout << "start " << i << " failed" << std::endl;
            } else {
                std::cout << "start " << i << " succeded" << std::endl;
            }
        } else if (stop) {
            std::cout << "stop " << i << std::endl;
            if (!tswipe.StopPWM(i)) {
                std::cout << "stop " << i << " failed" << std::endl;
            } else {
                std::cout << "stop " << i << " succeded" << std::endl;
            }
        } else if (get) {
            std::cout << "get " << i << std::endl;
            bool active;
            if (!tswipe.GetPWM(i, active, freq, high, low, repeats, duty)) {
                std::cout << "get " << i << " failed" << std::endl;
            } else {
                if (active) {
                    std::cout << "get " << i << " active: " << active <<" freq: " << freq << " high: " << high << " low: " << low << " repeats: " << repeats << " duty: " << duty << std::endl;
                } else {
                    std::cout << "get " << i << " active: " << active << std::endl;
                }
            }
        }
    }
    return 0;
}
