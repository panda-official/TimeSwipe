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

#ifndef PANDA_TIMESWIPE_FIRMWARE_JSON_STREAM_HPP
#define PANDA_TIMESWIPE_FIRMWARE_JSON_STREAM_HPP

#include "../io_stream.hpp"
#include "../error.hpp"
#include "../json.hpp"

/// A JSON stream.
class Json_stream final : public Io_stream {
public:
  /// The constructor.
  Json_stream(rapidjson::Value& value,
    rapidjson::Document::AllocatorType* const alloc)
    : value_{value}
    , alloc_{alloc}
  {}

  /// Non copy-constructible;
  Json_stream(const Json_stream&) = delete;
  /// Non copy-assignable;
  Json_stream& operator=(const Json_stream&) = delete;
  /// Non move-constructible;
  Json_stream(Json_stream&&) = delete;
  /// Non move-assignable;
  Json_stream& operator=(Json_stream&&) = delete;

  bool is_good() const noexcept override
  {
    return !is_error_;
  }

  /// @see Io_stream::write(std::nullopt_t).
  void write(std::nullopt_t) override
  {
    value_.SetNull();
    is_error_ = false;
  }

  /// @see Io_stream::write(bool).
  void write(const bool value) override
  {
    value_.SetBool(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<bool>&).
  void read(std::optional<bool>& value) override
  {
    if (value_.IsBool()) value = value_.GetBool();
    else is_error_ = true;
  }

  /// @see Io_stream::write(int).
  void write(const int value) override
  {
    value_.SetInt(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<int>&).
  void read(std::optional<int>& value) override
  {
    if (value_.IsInt()) value = value_.GetInt();
    else is_error_ = true;
  }

  /// @see Io_stream::write(unsigned int).
  void write(const unsigned int value) override
  {
    value_.SetUint(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<unsigned int>&).
  void read(std::optional<unsigned int>& value) override
  {
    if (value_.IsUint()) value = value_.GetUint();
    else is_error_ = true;
  }

  /// @see Io_stream::write(float).
  void write(const float value) override
  {
    value_.SetFloat(value);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<float>&).
  void read(std::optional<float>& value) override
  {
    if (value_.IsFloat() || value_.IsLosslessFloat()) value = value_.GetFloat();
    else is_error_ = true;
  }

  /// @see Io_stream::write(const std::string&).
  void write(const std::string& value) override
  {
    PANDA_TIMESWIPE_ASSERT(alloc_);
    value_.SetString(value, *alloc_);
    is_error_ = false;
  }

  /// @see Io_stream::read(std::optional<std::string>&).
  void read(std::optional<std::string>& value) override
  {
    if (value_.IsString()) value = value_.GetString();
    else is_error_ = true;
  }

private:
  bool is_error_{};
  rapidjson::Value& value_; // value holder
  rapidjson::Document::AllocatorType* alloc_{};
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_JSON_STREAM_HPP
