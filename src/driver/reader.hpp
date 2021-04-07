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

#include <cstdint>
#include <type_traits>
#include <vector>

#ifdef PANDA_BUILD_FIRMWARE_EMU
#include <cmath>
#endif

struct GPIOData final {
  std::uint8_t byte{};
  unsigned int tco{};
  bool piOk{};

  static GPIOData read() noexcept
  {
    setGPIOHigh(CLOCK);
    sleep55ns();
    sleep55ns();

    setGPIOLow(CLOCK);
    sleep55ns();
    sleep55ns();

    const unsigned int allGPIO{readAllGPIO()};
    const std::uint8_t byte =
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
};

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

constexpr std::size_t kBlocksPerChunk{8u};
constexpr std::size_t kChunkSizeInByte{kBlocksPerChunk};
constexpr std::size_t kTcoSize{256};

inline void convertChunkToRecord(SensorsData& data,
  const std::array<std::uint8_t, kChunkSizeInByte>& chunk,
  const std::array<std::uint16_t, 4>& offsets,
  const std::array<float, 4>& mfactors)
{
  std::array<std::uint16_t, 4> sensors{};
  static_assert(data.SensorsSize() == 4); // KLUDGE
  using OffsetValue = std::decay_t<decltype(offsets)>::value_type;
  using SensorValue = std::decay_t<decltype(sensors)>::value_type;
  static_assert(sizeof(OffsetValue) == sizeof(SensorValue));

  constexpr auto setBit = [](std::uint16_t& word, const std::uint8_t N, const bool bit) noexcept
  {
    word = (word & ~(1UL << N)) | (bit << N);
  };
  constexpr auto getBit = [](const std::uint8_t byte, const std::uint8_t N) noexcept -> bool
  {
    return (byte & (1UL << N));
  };
  for (std::size_t i{}, count{}; i < kChunkSizeInByte; ++i) {
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

  auto& underlying_data{data.data()};
  for (std::size_t i{}; i < 4; ++i)
    underlying_data[i].push_back(static_cast<float>(sensors[i] - offsets[i]) * mfactors[i]);
}

class RecordReader final {
private:
  friend class TimeSwipeImpl;

  // read records from hardware buffer
  SensorsData Read()
  {
#ifndef PANDA_BUILD_FIRMWARE_EMU
    SensorsData out;
    out.reserve(last_read_*2);
    int lastTCO{};
    int currentTCO{};

    int count{};
    bool run{true};

    // I.
    WaitForPiOk();

    // II.
    bool dry_run{true};
    while (run) {
      auto res{GPIOData::read()};
      currentTCO = res.tco;

      count++;
      current_chunk_[bytes_read_] = res.byte;
      bytes_read_++;

      if (bytes_read_ == kChunkSizeInByte) {
        convertChunkToRecord(out, current_chunk_, offsets_, mfactors_);
        bytes_read_ = 0;
      }

      run = dry_run || !(currentTCO - lastTCO == 16384);
      // run = !isRisingFlank(lastTCO,currentTCO);
      lastTCO = currentTCO;
      dry_run = false;
    }

    // discard first read - thats any old data in RAM!
    if (is_first_) {
      is_first_ = false;
      out.clear();
    }

    // III.
    sleep55ns();
    sleep55ns();

    last_read_ = out.DataSize();
    return out;
#else
    return readEmulated();
#endif
  }

  void WaitForPiOk()
  {
    // for 12MHz Quartz
    std::this_thread::sleep_for(std::chrono::microseconds(700));
  }

  void Setup()
  {
#ifndef PANDA_BUILD_FIRMWARE_EMU
    setup_io();
#endif
  }

  void Start()
  {
    for (std::size_t i{}; i < mfactors_.size(); ++i)
      mfactors_[i] = gains_[i] * transmissions_[i];
#ifdef PANDA_BUILD_FIRMWARE_EMU
    emul_point_begin_ = std::chrono::steady_clock::now();
    emul_sent_ = 0;
#else
    init(mode_);
#endif
  }

  void Stop()
  {
#ifndef PANDA_BUILD_FIRMWARE_EMU
    shutdown();
#endif
  }

#ifdef PANDA_BUILD_FIRMWARE_EMU
  double angle{};
  SensorsData ReadEmulated()
  {
    namespace chrono = std::chrono;
    SensorsData out;
    auto& data{out.data()};
    while (true) {
      emul_point_end_ = chrono::steady_clock::now();
      const std::uint64_t diff_us{chrono::duration_cast<chrono::microseconds>
        (emul_point_end_ - emul_point_begin_).count()};
      const std::uint64_t wouldSent{diff_us * emul_rate_ / 1000 / 1000};
      if (wouldSent > emul_sent_) {
        while (emul_sent_++ < wouldSent) {
          const auto val{int(3276 * std::sin(angle) + 32767)};
          angle += (2.0 * M_PI) / emul_rate_;
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

  const std::array<std::uint16_t, 4>& Offsets() const noexcept
  {
    return offsets_;
  }

  std::array<std::uint16_t, 4>& Offsets() noexcept
  {
    return offsets_;
  }

  const std::array<float, 4>& Gains() const noexcept
  {
    return gains_;
  }

  std::array<float, 4>& Gains() noexcept
  {
    return gains_;
  }

  const std::array<float, 4>& Transmissions() const noexcept
  {
    return transmissions_;
  }

  std::array<float, 4>& Transmissions() noexcept
  {
    return transmissions_;
  }

  int Mode() const noexcept
  {
    return mode_;
  }

  void SetMode(const int mode) noexcept
  {
    mode_ = mode;
  }

private:
  std::array<std::uint8_t, kChunkSizeInByte> current_chunk_{};
  std::size_t bytes_read_{};
  bool is_first_{true};
  std::size_t last_read_{};

  int mode_{};
  std::array<std::uint16_t, 4> offsets_{0, 0, 0, 0};
  std::array<float, 4> gains_{1, 1, 1, 1};
  std::array<float, 4> transmissions_{1, 1, 1, 1};
  std::array<float, 4> mfactors_{};

#ifdef PANDA_BUILD_FIRMWARE_EMU
  std::chrono::steady_clock::time_point emul_point_begin_;
  std::chrono::steady_clock::time_point emul_point_end_;
  std::uint64_t emul_sent_{};
  static constexpr std::size_t emul_rate_{48000};
#endif

  static constexpr bool isRisingFlank(const bool last, const bool now) noexcept
  {
    return !last && now;
  }
};

#endif  // PANDA_TIMESWIPE_DRIVER_READER_HPP
