#include "../../src/driver.hpp"
#include "../../src/3rdparty/dmitigr/fs/filesystem.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main()
{
  namespace fs = std::filesystem;
  namespace ts = panda::timeswipe;
  auto& drv = ts::Driver::instance().initialize();
  drv.start_measurement([](auto, auto){});
  const auto stop_flag = fs::current_path()/"stopts";
  while (!fs::exists(stop_flag)) {
    // std::cout << stop_flag << " don't exists" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds{1});
  }
  drv.stop_measurement();
}
