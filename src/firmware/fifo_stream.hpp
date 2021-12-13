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

#ifndef PANDA_TIMESWIPE_FIRMWARE_FIFO_STREAM_HPP
#define PANDA_TIMESWIPE_FIRMWARE_FIFO_STREAM_HPP

#include "io_stream.hpp"
#include "../serial.hpp"

/// A FIFO stream.
class Fifo_stream final : public Io_stream {
public:
  /// The constructor.
  explicit Fifo_stream(CFIFO* const fifo)
    : fifo_{fifo}
  {}

  /// Non copy-constructible;
  Fifo_stream(const Fifo_stream&) = delete;
  /// Non copy-assignable;
  Fifo_stream& operator=(const Fifo_stream&) = delete;
  /// Non move-constructible;
  Fifo_stream(Fifo_stream&&) = delete;
  /// Non move-assignable;
  Fifo_stream& operator=(Fifo_stream&&) = delete;

  /// @returns `true` if the last operation was successful.
  bool is_good() const noexcept override
  {
    return !is_error_;
  }

  /// @see Io_stream::write(std::nullopt_t).
  void write(std::nullopt_t) override
  {
    is_error_ = true;
  }

  /// @see Io_stream::write(bool).
  void write(const bool value) override
  {
    *fifo_ += std::to_string(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<bool>&).
  void read(std::optional<bool>& value) override
  {
    if (const auto str = read_string(); !str.empty()) {
      value = std::isdigit(str[0]) ?
        str[0] - 0x30 : // first '0' considered as a whole `false`
        (str == "True" || str == "true");
    }
  }

  /// @see Io_stream::write(int).
  void write(const int value) override
  {
    *fifo_ += std::to_string(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<int>&).
  void read(std::optional<int>& value) override
  {
    try {
      if (const auto str = read_string(); !str.empty()) {
        std::size_t pos{};
        value = std::stoi(str, &pos, 0);
      }
    } catch (...) {}
  }

  /// @see Io_stream::write(unsigned int).
  void write(const unsigned int value) override
  {
    *fifo_ += std::to_string(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<unsigned int>&).
  void read(std::optional<unsigned int>& value) override
  {
    try {
      if (const auto str = read_string(); !str.empty()) {
        std::size_t pos{};
        value = static_cast<unsigned int>(std::stoul(str, &pos, 0));
      }
    } catch (...) {}
  }

  /// @see Io_stream::write(float).
  void write(const float value) override
  {
    *fifo_ += std::to_string(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<float>&).
  void read(std::optional<float>& value) override
  {
    try {
      if (const auto str = read_string(); !str.empty()) {
        std::size_t pos{};
        value = std::stof(str, &pos);
      }
    } catch (...) {}
  }

  /// @see Io_stream::write(const std::string&).
  void write(const std::string& value) override
  {
    *fifo_ += value;
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<std::string>&).
  void read(std::optional<std::string>& result) override
  {
    if (auto str = read_string(); !str.empty()) result = std::move(str);
  }

private:
  /// A pointer to FIFO buffer used as stream-buffer.
  CFIFO* fifo_{};
  /// `true` if the last operation failed.
  bool is_error_{};

  /**
   * @returns Extracted string from this stream.
   *
   * @par Effects
   * `!is_good()` on failure.
   */
  std::string read_string()
  {
    // Start token used for string extraction.
    constexpr Character start_token{' '};
    // End token used for string extraction.
    constexpr Character end_token{'\0'};

    std::string result;
    bool is_extracted{};
    Character token{start_token};
    while (fifo_->in_avail()) {
      Character ch;
      *fifo_ >> ch;
      if (ch == token) {
        if (is_extracted)
          break;
      } else {
        token = end_token;
        is_extracted = true;
        result += static_cast<char>(ch);
      }
    }

    if (!is_extracted) {
      is_error_ = true;
      return {std::string{}, false};
    } else
      is_error_ = false;

    return result;
  }
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_FIFO_STREAM_HPP
