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
// std::ofstream data_log;

std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

int main(int argc, char *argv[])
{
    nlohmann::json config;

    if (argc < 3)
    {
        std::cerr << "Wrong usage: 'sudo " << argv[0] << " [config-path (config as json file - see example)] [--dump] [input type: NORM/IEPE]'" << std::endl;
        return 1;
    }
    bool dump = false;
    std::ifstream i(argv[1]);
    i >> config;
    std::string input{argv[2]};
    if (input == "--dump") {
        dump = true;
        input = argv[3];
    }

    TimeSwipe tswipe;

    // Board Preparation

    tswipe.SetBridge(config[input]["U_BRIDGE"]);

    const auto& offs = config[input]["SENSOR_OFFSET"];
    tswipe.SetSensorOffsets(offs[0], offs[1], offs[2], offs[3]);

    const auto& gains = config[input]["SENSOR_GAIN"];
    tswipe.SetSensorGains(gains[0], gains[1], gains[2], gains[3]);

    const auto& trans = config[input]["SENSOR_TRANSMISSION"];
    tswipe.SetSensorTransmissions(trans[0], trans[1], trans[2], trans[3]);


    // Board Shutdown on signals

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    shutdown_handler = [&](int signal) {
        tswipe.Stop();
        exit(1);
    };

    // Board Start

    bool ret = tswipe.Start([&](auto&& records) {
            for (const auto& rec: records) {
                std::cout << rec.Sensors[0] << "\t" << rec.Sensors[1] << "\t" << rec.Sensors[2] << "\t" << rec.Sensors[3] << "\n";
                if (!dump) break; // print only first
            }
            // It is possible to read as fast as possible and get small amonut of data
            // if this callback function delays more than 500ms - some data will be loosed
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    if (!ret) {
        std::cerr << "timeswipe start failed" << std::endl;
        return -1;
    }

    // Board Stop

    if (!tswipe.Stop()) {
        std::cerr << "timeswipe stop failed" << std::endl;
        return -1;
    }

    return 0;
}
