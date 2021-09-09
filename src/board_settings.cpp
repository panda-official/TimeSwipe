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

#include "board_settings.hpp"

#include "error.hpp"
#include "rajson.hpp"

#include <algorithm>
#include <utility>

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// class Board_settings::Rep
// -----------------------------------------------------------------------------

struct Board_settings::Rep final {
  Rep()
  {}

  explicit Rep(const std::string_view stringified_json)
    : doc_{rajson::to_document(stringified_json)}
  {}

  Rep(const Rep& rhs)
  {
    doc_.CopyFrom(rhs.doc_, doc_.GetAllocator(), true);
  }

  Rep& operator=(const Rep& rhs)
  {
    Rep tmp{rhs};
    swap(tmp);
    return *this;
  }

  Rep(Rep&&) = default;

  Rep& operator=(Rep&&) = default;

  void swap(Rep& rhs) noexcept
  {
    doc_.Swap(rhs.doc_);
  }

  std::string to_stringified_json() const
  {
    return rajson::to_stringified(doc_);
  }

  // ---------------------------------------------------------------------------

  void set_signal_mode(const Signal_mode mode)
  {
    set_member("Mode", static_cast<int>(mode));
  }

  std::optional<Signal_mode> signal_mode() const
  {
    return member<Signal_mode>("Mode");
  }

  // ---------------------------------------------------------------------------

  void set_channel_measurement_mode(const int index, const Measurement_mode value)
  {
    set_member("CH", index + 1, "mode", static_cast<int>(value));
  }

  std::optional<Measurement_mode> channel_measurement_mode(const int index) const
  {
    return member<Measurement_mode>("CH", index + 1, "mode");
  }

  void set_channel_gain(const int index, const float value)
  {
    set_member("CH", index + 1, "gain", value);
  }

  std::optional<float> channel_gain(const int index) const
  {
    return member<float>("CH", index + 1, "gain");
  }

  void set_channel_iepe(const int index, const bool value)
  {
    set_member("CH", index + 1, "iepe", value);
  }

  std::optional<bool> channel_iepe(const int index) const
  {
    return member<bool>("CH", index + 1, "iepe");
  }

  // ---------------------------------------------------------------------------

  void set_pwm_start(const int index, const bool value)
  {
    set_member("PWM", index + 1, value);
  }

  std::optional<bool> pwm_start(const int index)
  {
    return member<bool>("PWM", index + 1);
  }

  void set_pwm_frequency(const int index, const int value)
  {
    set_member("PWM", index + 1, "freq", value);
  }

  std::optional<int> pwm_frequency(const int index) const
  {
    return member<int>("PWM", index + 1, "freq");
  }

  void set_pwm_low(const int index, const int value)
  {
    set_member("PWM", index + 1, "low", value);
  }

  std::optional<int> pwm_low(int index) const
  {
    return member<int>("PWM", index + 1, "low");
  }

  void set_pwm_high(const int index, const int value)
  {
    set_member("PWM", index + 1, "high", value);
  }

  std::optional<int> pwm_high(const int index) const
  {
    return member<int>("PWM", index + 1, "high");
  }

  void set_pwm_repeat_count(const int index, const int value)
  {
    set_member("PWM", index + 1, "repeats", value);
  }

  std::optional<int> pwm_repeat_count(const int index) const
  {
    return member<int>("PWM", index + 1, "repeats");
  }

  void set_pwm_duty_cycle(const int index, const float value)
  {
    set_member("PWM", index + 1, "duty", value);
  }

  std::optional<float> pwm_duty_cycle(const int index) const
  {
    return member<float>("PWM", index + 1, "duty");
  }

private:
  rapidjson::Document doc_{rapidjson::Type::kObjectType};

  /// Adds or modifies the member named by `name` by using the given `value`.
  template<typename T>
  void set_member(const std::string_view name, T&& value)
  {
    detail::set_member(doc_, doc_.GetAllocator(), name, std::forward<T>(value));
  }

  /// @returns The full name of access point at the given `index`.
  std::string member_name(std::string root_name, const int index) const
  {
    return root_name.append(std::to_string(index));
  }

  /// @overload
  std::string member_name(std::string root_name, const int index,
    const std::string_view sub_name) const
  {
    return root_name.append(std::to_string(index)).append(".").append(sub_name);
  }

  /// Sets `root_name` variable with index `index` to `value`.
  template<typename T>
  void set_member(std::string root_name, const int index, T&& value)
  {
    set_member(member_name(std::move(root_name), index), std::forward<T>(value));
  }

  /// @overload
  template<typename T>
  void set_member(std::string root_name, const int index, const std::string_view sub_name, T&& value)
  {
    set_member(member_name(std::move(root_name), index, sub_name), std::forward<T>(value));
  }

  /// @returns The value of `root_name` variable.
  template<typename T>
  std::optional<T> member(const std::string_view root_name) const
  {
    return rajson::Value_view{doc_}.optional<T>(root_name);
  }

  /// @returns The variable at `index`.
  template<typename T>
  std::optional<T> member(std::string root_name, const int index) const
  {
    return rajson::Value_view{doc_}.optional<T>(member_name(std::move(root_name), index));
  }

  /// @overload
  template<typename T>
  std::optional<T> member(std::string root_name, const int index, const std::string_view sub_name) const
  {
    return rajson::Value_view{doc_}.optional<T>(member_name(std::move(root_name), index, sub_name));
  }
};

// -----------------------------------------------------------------------------
// class Board_settings
// -----------------------------------------------------------------------------

Board_settings::~Board_settings() = default;

Board_settings::Board_settings(const Board_settings& rhs)
  : rep_{std::make_unique<Rep>(*rhs.rep_)}
{}

Board_settings& Board_settings::operator=(const Board_settings& rhs)
{
  Board_settings tmp{rhs};
  swap(tmp);
  return *this;
}

Board_settings::Board_settings(Board_settings&& rhs)
  : rep_{std::move(rhs.rep_)}
{}

Board_settings& Board_settings::operator=(Board_settings&& rhs)
{
  Board_settings tmp{std::move(rhs)};
  swap(tmp);
  return *this;
}

Board_settings::Board_settings()
  : rep_{std::make_unique<Rep>()}
{}

Board_settings::Board_settings(const std::string_view stringified_json)
  : rep_{std::make_unique<Rep>(stringified_json)}
{}

void Board_settings::swap(Board_settings& other) noexcept
{
  using std::swap;
  swap(rep_, other.rep_);
}

std::string Board_settings::to_stringified_json() const
{
  return rep_->to_stringified_json();
}

// -----------------------------------------------------------------------------

Board_settings& Board_settings::set_signal_mode(const Signal_mode mode)
{
  rep_->set_signal_mode(mode);
  return *this;
}

std::optional<Signal_mode> Board_settings::signal_mode() const
{
  return rep_->signal_mode();
}

// -----------------------------------------------------------------------------

Board_settings& Board_settings::set_channel_measurement_mode(const int index,
  const Measurement_mode value)
{
  rep_->set_channel_measurement_mode(index, value);
  return *this;
}

std::optional<Measurement_mode> Board_settings::channel_measurement_mode(const int index) const
{
  return rep_->channel_measurement_mode(index);
}

Board_settings& Board_settings::set_channel_gain(const int index, const float value)
{
  rep_->set_channel_gain(index, value);
  return *this;
}

std::optional<float> Board_settings::channel_gain(const int index) const
{
  return rep_->channel_gain(index);
}

Board_settings& Board_settings::set_channel_iepe(const int index, const bool value)
{
  rep_->set_channel_iepe(index, value);
  return *this;
}

std::optional<bool> Board_settings::channel_iepe(const int index) const
{
  return rep_->channel_iepe(index);
}

// -----------------------------------------------------------------------------

Board_settings& Board_settings::set_pwm_start(const int index, const bool value)
{
  rep_->set_pwm_start(index, value);
  return *this;
}

std::optional<bool> Board_settings::pwm_start(const int index) const
{
  return rep_->pwm_start(index);
}

Board_settings& Board_settings::set_pwm_frequency(const int index, const int value)
{
  rep_->set_pwm_frequency(index, value);
  return *this;
}

std::optional<int> Board_settings::pwm_frequency(const int index) const
{
  return rep_->pwm_frequency(index);
}

Board_settings& Board_settings::set_pwm_low(const int index, const int value)
{
  rep_->set_pwm_low(index, value);
  return *this;
}

std::optional<int> Board_settings::pwm_low(const int index) const
{
  return rep_->pwm_low(index);
}

Board_settings& Board_settings::set_pwm_high(const int index, const int value)
{
  rep_->set_pwm_high(index, value);
  return *this;
}

std::optional<int> Board_settings::pwm_high(const int index) const
{
  return rep_->pwm_high(index);
}

Board_settings& Board_settings::set_pwm_repeat_count(const int index, const int value)
{
  rep_->set_pwm_repeat_count(index, value);
  return *this;
}

std::optional<int> Board_settings::pwm_repeat_count(const int index) const
{
  return rep_->pwm_repeat_count(index);
}

Board_settings& Board_settings::set_pwm_duty_cycle(const int index, const float value)
{
  rep_->set_pwm_duty_cycle(index, value);
  return *this;
}

std::optional<float> Board_settings::pwm_duty_cycle(const int index) const
{
  return rep_->pwm_duty_cycle(index);
}

} // namespace panda::timeswipe
