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

#include "timeswipe_state.hpp"

#include "../common/error.hpp"
#include "../3rdparty/dmitigr/assert.hpp"
#include "../3rdparty/dmitigr/rajson.hpp"

#include <utility>

namespace rajson = dmitigr::rajson;

namespace dmitigr::rajson {
template<>
struct Conversions<panda::timeswipe::Measurement_mode> final {
  template<class Encoding, class Allocator>
  static auto from(const rapidjson::GenericValue<Encoding, Allocator>& value)
  {
    return static_cast<panda::timeswipe::Measurement_mode>(to<int>(value));
  }
};
} // namespace dmitigr::rajson

namespace panda::timeswipe::driver {

// -----------------------------------------------------------------------------
// class Timeswipe_state::Rep
// -----------------------------------------------------------------------------

struct Timeswipe_state::Rep final {
  Rep()
  {}

  explicit Rep(const std::string_view stringified_json)
    : doc_{rajson::to_document(stringified_json)}
  {}

  std::string to_stringified_json() const
  {
    return rajson::to_stringified(doc_);
  }

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
    auto& alloc = doc_.GetAllocator();
    auto val = rajson::to<rapidjson::Value>(std::forward<T>(value), alloc);
    const auto name_ref = rajson::to<rapidjson::Value::StringRefType>(name);
    if (const auto i = doc_.FindMember(name_ref); i != doc_.MemberEnd())
      i->value = std::move(val);
    else
      doc_.AddMember(rapidjson::Value{name.data(), name.size(), alloc},
        std::move(val), alloc);
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

  /// Sets PWM at `index` to `value`.
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

  /// @returns The value of PWM at `index`.
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
// class Timeswipe_state
// -----------------------------------------------------------------------------

Timeswipe_state::~Timeswipe_state() = default;

Timeswipe_state::Timeswipe_state()
  : rep_{std::make_unique<Rep>()}
{}

std::string Timeswipe_state::to_stringified_json() const
{
  return rep_->to_stringified_json();
}

Timeswipe_state::Timeswipe_state(const std::string_view stringified_json)
  : rep_{std::make_unique<Rep>(stringified_json)}
{}

Timeswipe_state& Timeswipe_state::set_channel_measurement_mode(const int index,
  const Measurement_mode value)
{
  rep_->set_channel_measurement_mode(index, value);
  return *this;
}

std::optional<Measurement_mode> Timeswipe_state::channel_measurement_mode(const int index) const
{
  return rep_->channel_measurement_mode(index);
}

Timeswipe_state& Timeswipe_state::set_channel_gain(const int index, const float value)
{
  rep_->set_channel_gain(index, value);
  return *this;
}

std::optional<float> Timeswipe_state::channel_gain(const int index) const
{
  return rep_->channel_gain(index);
}

Timeswipe_state& Timeswipe_state::set_channel_iepe(const int index, const bool value)
{
  rep_->set_channel_iepe(index, value);
  return *this;
}

std::optional<bool> Timeswipe_state::channel_iepe(const int index) const
{
  return rep_->channel_iepe(index);
}

// -----------------------------------------------------------------------------

Timeswipe_state& Timeswipe_state::set_pwm_start(const int index, const bool value)
{
  rep_->set_pwm_start(index, value);
  return *this;
}

std::optional<bool> Timeswipe_state::pwm_start(const int index) const
{
  return rep_->pwm_start(index);
}

Timeswipe_state& Timeswipe_state::set_pwm_frequency(const int index, const int value)
{
  rep_->set_pwm_frequency(index, value);
  return *this;
}

std::optional<int> Timeswipe_state::pwm_frequency(const int index) const
{
  return rep_->pwm_frequency(index);
}

Timeswipe_state& Timeswipe_state::set_pwm_low(const int index, const int value)
{
  rep_->set_pwm_low(index, value);
  return *this;
}

std::optional<int> Timeswipe_state::pwm_low(const int index) const
{
  return rep_->pwm_low(index);
}

Timeswipe_state& Timeswipe_state::set_pwm_high(const int index, const int value)
{
  rep_->set_pwm_high(index, value);
  return *this;
}

std::optional<int> Timeswipe_state::pwm_high(const int index) const
{
  return rep_->pwm_high(index);
}

Timeswipe_state& Timeswipe_state::set_pwm_repeat_count(const int index, const int value)
{
  rep_->set_pwm_repeat_count(index, value);
  return *this;
}

std::optional<int> Timeswipe_state::pwm_repeat_count(const int index) const
{
  return rep_->pwm_repeat_count(index);
}

Timeswipe_state& Timeswipe_state::set_pwm_duty_cycle(const int index, const float value)
{
  rep_->set_pwm_duty_cycle(index, value);
  return *this;
}

std::optional<float> Timeswipe_state::pwm_duty_cycle(const int index) const
{
  return rep_->pwm_duty_cycle(index);
}

} // namespace panda::timeswipe::driver
