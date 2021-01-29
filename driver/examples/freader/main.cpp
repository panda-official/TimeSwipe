#include <iostream>

#include <csignal>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <functional>
#include <thread>
#include <chrono>
#include "timeswipe.hpp"


std::function<void(int)> shutdown_handler;
void signal_handler(int signal) { shutdown_handler(signal); }

void usage(const char* name)
{
    std::cerr << "Usage: 'sudo " << name << " [--output <outname>] " << std::endl;
}

int main(int argc, char *argv[])
{
    std::string dumpname;

    for (unsigned i = 1; i < argc; i++) {
         if (!strcmp(argv[i],"--output")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            dumpname = argv[i+1];
            ++i;

        }else {
            usage(argv[0]);
            return 1;
        }
    }


    TimeSwipe tswipe;
   // tswipe.TraceSPI(trace_spi);

    std::vector<uint8_t> file;
    std::string strErrMsg;
    if(!tswipe.readFile("MeasResult", file, strErrMsg))
    {
        std::cout << "Failed to read the file: " << strErrMsg << std::endl;
    }

    std::cout << "File size: "<<file.size()<<std::endl<<std::endl;
    /*for( auto ch : file)
    {
        std::cout<<ch;
    }*/
    std::ofstream FILE(dumpname, std::ios::out | std::ofstream::binary);
    std::copy(file.begin(), file.end(), std::ostreambuf_iterator<char>(FILE));
    std::copy(file.begin(), file.end(), std::ostreambuf_iterator<char>(std::cout));

    std::cout<<std::endl;



    // Board Shutdown on signals
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    shutdown_handler = [&](int signal) {
        tswipe.Stop();
        exit(1);
    };


    bool ret = tswipe.onEvent([&](TimeSwipeEvent&& event) {
        if (event.is<TimeSwipeEvent::Button>()) {
            auto button = event.get<TimeSwipeEvent::Button>();
            std::cout << "Button event: " <<  (button.pressed() ? "pressed":"released") << " counter: " << button.count() << std::endl;
        } else if(event.is<TimeSwipeEvent::Gain>()) {
            auto val = event.get<TimeSwipeEvent::Gain>();
            std::cout << "Gain event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::SetSecondary>()) {
            auto val = event.get<TimeSwipeEvent::SetSecondary>();
            std::cout << "SetSecondary event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Bridge>()) {
            auto val = event.get<TimeSwipeEvent::Bridge>();
            std::cout << "Bridge event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Record>()) {
            auto val = event.get<TimeSwipeEvent::Record>();
            std::cout << "Record event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Offset>()) {
            auto val = event.get<TimeSwipeEvent::Offset>();
            std::cout << "Offset event: " <<  val.value() << std::endl;
        } else if(event.is<TimeSwipeEvent::Mode>()) {
            auto val = event.get<TimeSwipeEvent::Mode>();
            std::cout << "Mode event: " <<  val.value() << std::endl;
        }
    });
    if (!ret) {
        std::cerr << "onEvent init failed" << std::endl;
        return 1;
    }

    ret = tswipe.onError([&](uint64_t errors) {
        std::cout << "Got errors: " << errors << std::endl;
    });
    if (!ret) {
        std::cerr << "onError init failed" << std::endl;
        return 1;
    }

    return 0;
}
