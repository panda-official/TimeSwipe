// -*- C++ -*-

// PANDA TimeSwipe Project
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

#ifndef PANDA_TIMESWIPE_DRIVER_READER_HPP
#define PANDA_TIMESWIPE_DRIVER_READER_HPP

#include "board.hpp"

#include <vector>

#ifdef PANDA_BUILD_FIRMWARE_EMU
#include <cmath>
#endif

struct GPIOData final {
  uint8_t byte{};
  unsigned int tco{};
  bool piOK{};
};

inline GPIOData readByteAndStatusFromGPIO()
{
  setGPIOHigh(CLOCK);
  sleep55ns();
  sleep55ns();

  setGPIOLow(CLOCK);
  sleep55ns();
  sleep55ns();

  unsigned int allGPIO = readAllGPIO();
  uint8_t byte =
    ((allGPIO & DATA_POSITION[0]) >> 17) |  // Bit 7
    ((allGPIO & DATA_POSITION[1]) >> 19) |  //     6
    ((allGPIO & DATA_POSITION[2]) >> 2) |   //     5
    ((allGPIO & DATA_POSITION[3]) >> 1) |   //     4
    ((allGPIO & DATA_POSITION[4]) >> 3) |   //     3
    ((allGPIO & DATA_POSITION[5]) >> 10) |  //     2
    ((allGPIO & DATA_POSITION[6]) >> 12) |  //     1
    ((allGPIO & DATA_POSITION[7]) >> 16);   //     0

  sleep55ns();
  sleep55ns();

  return {byte, (allGPIO & TCO_POSITION), (allGPIO & PI_STATUS_POSITION) != 0};
}

constexpr bool isRisingFlank(const bool last, const bool now) noexcept
{
  return !last && now;
}

// chunk-Layout:
// ------+----------------------------+---------------------------
//  Byte | Bit7   Bit6   Bit5   Bit4  | Bit3   Bit2   Bit1   Bit0
// ------+----------------------------+---------------------------
//     0 | 1-14   2-14   3-14   4-14  | 1-15   2-15   3-15   4-15
//     1 | 1-12   2-12   3-12   4-12  | 1-13   2-13   3-13   4-13
//     2 | 1-10   2-10   3-10   4-10  | 1-11   2-11   3-11   4-11
//     3 |  1-8    2-8    3-8    4-8  |  1-9    2-9    3-9    4-9
//     4 |  1-6    2-6    3-6    4-6  |  1-7    2-7    3-7    4-7
//     5 |  1-4    2-4    3-4    4-4  |  1-5    2-5    3-5    4-5
//     6 |  1-2    2-2    3-2    4-2  |  1-3    2-3    3-3    4-3
//     7 |  1-0    2-0    3-0    4-0  |  1-1    2-1    3-1    4-1

constexpr size_t BLOCKS_PER_CHUNK = 8;
constexpr size_t CHUNK_SIZE_IN_BYTE = BLOCKS_PER_CHUNK;
constexpr size_t TCO_SIZE = 256;

constexpr void setBit(uint16_t &word, uint8_t N, bool bit) noexcept
{
  word = (word & ~(1UL << N)) | (bit << N);
}

constexpr bool getBit(uint8_t byte, uint8_t N) noexcept
{
  return (byte & (1UL << N));
}

template <class T>
void convertChunkToRecord(const std::array<uint8_t, CHUNK_SIZE_IN_BYTE>& chunk,
  const std::array<int, 4>& offset, const std::array<float, 4>& mfactor, T& data)
{
  size_t count{};
  std::vector<uint16_t> sensors(4);
  static std::vector<uint16_t> sensorOld(4, 32768);

  for (size_t i{}; i < CHUNK_SIZE_IN_BYTE; ++i) {
    setBit(sensors[0], 15 - count, getBit(chunk[i], 3));
    setBit(sensors[1], 15 - count, getBit(chunk[i], 2));
    setBit(sensors[2], 15 - count, getBit(chunk[i], 1));
    setBit(sensors[3], 15 - count, getBit(chunk[i], 0));
    count++;

    setBit(sensors[0], 15 - count, getBit(chunk[i], 7));
    setBit(sensors[1], 15 - count, getBit(chunk[i], 6));
    setBit(sensors[2], 15 - count, getBit(chunk[i], 5));
    setBit(sensors[3], 15 - count, getBit(chunk[i], 4));
    count++;
  }

  for (size_t i{}; i < 4; ++i) {
    //##########################//
    //TBD: Dirty fix for clippings
    //##########################//
    if (sensors[i] % 64 == 7 || sensors[i] % 64 == 56)
      sensors[i] = sensorOld[i];

    sensorOld[i] = sensors[i];
    //##########################//

    data[i].push_back((float)(sensors[i] - offset[i]) * mfactor[i]);
  }
}

struct RecordReader final {
  std::array<uint8_t, CHUNK_SIZE_IN_BYTE> currentChunk{};
  size_t bytesRead{};
  bool isFirst{true};
  size_t lastRead{};

  int mode{};
  std::array<int, 4> offset{0, 0, 0, 0};
  std::array<float, 4> gain{1, 1, 1, 1};
  std::array<float, 4> transmission{1, 1, 1, 1};
  std::array<float, 4> mfactor{};

#ifdef PANDA_BUILD_FIRMWARE_EMU
  std::chrono::steady_clock::time_point emulPointBegin;
  std::chrono::steady_clock::time_point emulPointEnd;
  uint64_t emulSent{};
  static constexpr size_t emulRate{48000};
#endif

  // read records from hardware buffer
  SensorsData read()
  {
#ifndef PANDA_BUILD_FIRMWARE_EMU
    SensorsData out;
    out.reserve(lastRead*2);
    int lastTCO{};
    int currentTCO{};

    int count{};
    bool run{true};

    // I.
    waitForPiOk();

    // II.
    bool dry_run{true};
    while (run) {
      auto res = readByteAndStatusFromGPIO();
      currentTCO = res.tco;

      count++;
      currentChunk[bytesRead] = res.byte;
      bytesRead++;

      if (bytesRead == CHUNK_SIZE_IN_BYTE) {
        convertChunkToRecord(currentChunk, offset, mfactor, out.data());
        bytesRead = 0;
      }

      run = dry_run || !(currentTCO - lastTCO == 16384);
      // run = !isRisingFlank(lastTCO,currentTCO);
      lastTCO = currentTCO;
      dry_run = false;
    }

    // discard first read - thats any old data in RAM!
    if (isFirst) {
      isFirst = false;
      out.clear();
    }

    // III.
    sleep55ns();
    sleep55ns();

    lastRead = out.DataSize();
    return out;
#else
    return readEmulated();
#endif
  }

  void waitForPiOk()
  {
    // for 12MHz Quartz
    std::this_thread::sleep_for(std::chrono::microseconds(700));
  }

  void setup()
  {
#ifndef PANDA_BUILD_FIRMWARE_EMU
    setup_io();
#endif
  }

  void start()
  {
    for (size_t i{}; i < mfactor.size(); i++)
      mfactor[i] = gain[i] * transmission[i];
#ifdef PANDA_BUILD_FIRMWARE_EMU
    emulPointBegin = std::chrono::steady_clock::now();
    emulSent = 0;
#else
    init(mode);
#endif
  }

  void stop()
  {
#ifndef PANDA_BUILD_FIRMWARE_EMU
    shutdown();
#endif
  }

#ifdef PANDA_BUILD_FIRMWARE_EMU
  double angle{};
  SensorsData readEmulated()
  {
    namespace chrono = std::chrono;
    SensorsData out;
    auto& data{out.data()};
    while (true) {
      emulPointEnd = chrono::steady_clock::now();
      const uint64_t diff_us{chrono::duration_cast<chrono::microseconds>(emulPointEnd - emulPointBegin).count()};
      const uint64_t wouldSent{diff_us * emulRate / 1000 / 1000};
      if (wouldSent > emulSent) {
        while (emulSent++ < wouldSent) {
          constexpr int NB_OF_SAMPLES{emulRate};
          auto val = int(3276 * sin(angle) + 32767);
          angle += (2.0 * M_PI) / NB_OF_SAMPLES;
          data[0].push_back(val);
          data[1].push_back(val);
          data[2].push_back(val);
          data[3].push_back(val);
        }
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds{2});
    }
    return out;
  }
#endif
};

#endif  // PANDA_TIMESWIPE_DRIVER_READER_HPP
