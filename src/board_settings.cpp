// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH / Dmitry Igrishin

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
#include "driver.hpp"
#include "error_detail.hpp"
#include "gain.hpp"
#include "rajson.hpp"

#include <algorithm>
#include <utility>

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe {
using namespace detail;

// -----------------------------------------------------------------------------
// class Board_settings::Rep
// -----------------------------------------------------------------------------

struct Board_settings::Rep final {
  Rep()
  {}

  explicit Rep(const std::string_view json_text)
    : doc_{rajson::to_document(json_text)}
  {
    // Check channel-related settings.
    if (const auto gains = channel_gains()) {
      for (int i{}; i < mpc_; ++i)
        check_channel_gain((*gains)[i]);
    }

    // Check PWM-related settings.
    {
      static const auto apply = [](const auto& checker, const auto& values)
      {
        if (values) {
          const auto values_size = values->size();
          PANDA_TIMESWIPE_ASSERT(values_size == mpc_);
          for (int i{}; i < values_size; ++i)
            checker(values->at(i));
        }
      };
      apply(check_pwm_frequency, pwm_frequencies());
      apply(check_pwm_signal_level, pwm_signal_levels());
      apply(check_pwm_repeat_count, pwm_repeat_counts());
      apply(check_pwm_duty_cycle, pwm_duty_cycles());
    }
  }

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

  std::string to_json_text() const
  {
    return rajson::to_text(doc_);
  }

  void set(const Rep& other)
  {
    const auto apply = [this](const auto& setter, const auto& data)
    {
      if (data) (this->*setter)(*data);
    };
    apply(&Rep::set_signal_mode, other.signal_mode());
    apply(&Rep::set_channel_measurement_modes, other.channel_measurement_modes());
    apply(&Rep::set_channel_gains, other.channel_gains());
    apply(&Rep::set_channel_iepes, other.channel_iepes());
    apply(&Rep::set_pwms, other.pwms());
    apply(&Rep::set_pwm_frequencies, other.pwm_frequencies());
    apply(&Rep::set_pwm_signal_levels, other.pwm_signal_levels());
    apply(&Rep::set_pwm_repeat_counts, other.pwm_repeat_counts());
    apply(&Rep::set_pwm_duty_cycles, other.pwm_duty_cycles());
  }

  // ---------------------------------------------------------------------------

  void set_signal_mode(const Signal_mode value)
  {
    set_member("Mode", value);
  }

  std::optional<Signal_mode> signal_mode() const
  {
    return member<Signal_mode>("Mode");
  }

  // ---------------------------------------------------------------------------

  void set_channel_measurement_modes(const std::vector<Measurement_mode>& values)
  {
    set_channel_values(values, "mode", "measurement modes", [](auto){return true;});
  }

  std::optional<std::vector<Measurement_mode>> channel_measurement_modes() const
  {
    return channel_values<Measurement_mode>("mode");
  }

  void set_channel_gains(const std::vector<float>& values)
  {
    set_channel_values(values, "gain", "gains", [](auto){return true;});
  }

  std::optional<std::vector<float>> channel_gains() const
  {
    return channel_values<float>("gain");
  }

  void set_channel_iepes(const std::vector<bool>& values)
  {
    set_channel_values(values, "iepe", "IEPEs", [](auto){return true;});
  }

  std::optional<std::vector<bool>> channel_iepes() const
  {
    return channel_values<bool>("iepe");
  }

  // ---------------------------------------------------------------------------

  void set_pwms(const std::vector<bool>& values)
  {
    set_pwm_values(values, "", "start flags", [](auto){return true;});
  }

  std::optional<std::vector<bool>> pwms() const
  {
    return pwm_values<bool>("");
  }

  void set_pwm_frequencies(const std::vector<int>& values)
  {
    set_pwm_values(values, "freq", "frequencies", check_pwm_frequency);
  }

  std::optional<std::vector<int>> pwm_frequencies() const
  {
    return pwm_values<int>("freq");
  }

  void set_pwm_signal_levels(const std::vector<std::pair<int, int>>& values)
  {
    if (!(values.size() >= mpc_))
      throw Generic_exception{"cannot set PWM signal levels (not enough values)"};

    // Ensure all the values are ok before applying them.
    for (int i{}; i < mpc_; ++i)
      check_pwm_signal_level(values[i]);

    // Apply the values.
    for (int i{}; i < mpc_; ++i) {
      set_member("PWM", i + 1, "low", values[i].first);
      set_member("PWM", i + 1, "high", values[i].second);
    }
  }

  std::optional<std::vector<std::pair<int, int>>> pwm_signal_levels() const
  {
    std::vector<std::pair<int, int>> result;
    result.reserve(mpc_);
    for (int i{}; i < mpc_; ++i) {
      const auto low = member<int>("PWM", i + 1, "low");
      if (!low) return std::nullopt;
      const auto high = member<int>("PWM", i + 1, "high");
      if (!high) return std::nullopt;

      result.emplace_back(*low, *high);
    }
    return result;
  }

  void set_pwm_repeat_counts(const std::vector<int>& values)
  {
    set_pwm_values(values, "repeats", "repeat counts", check_pwm_repeat_count);
  }

  std::optional<std::vector<int>> pwm_repeat_counts() const
  {
    return pwm_values<int>("repeats");
  }

  void set_pwm_duty_cycles(const std::vector<float>& values)
  {
    set_pwm_values(values, "duty", "duty cycles", check_pwm_duty_cycle);
  }

  std::optional<std::vector<float>> pwm_duty_cycles() const
  {
    return pwm_values<float>("duty");
  }

private:
  inline static const int mpc_{Driver::instance().max_channel_count()};
  rapidjson::Document doc_{rapidjson::Type::kObjectType};

  // ---------------------------------------------------------------------------
  // Checkers
  // ---------------------------------------------------------------------------

  static void check_channel_gain(const float value)
  {
    if (!(gain::ogain_min <= value && value <= gain::ogain_max))
      throw Generic_exception{Generic_errc::board_settings_invalid,
        "cannot set invalid channel gain"};
  }

  static void check_pwm_frequency(const int value)
  {
    if (!(1 <= value && value <= 1000))
      throw Generic_exception{Generic_errc::board_settings_invalid,
        "cannot set invalid PWM frequency"};
  }

  static void check_pwm_signal_level(const std::pair<int, int>& value)
  {
    static const auto check_value = [](const int value)
    {
      if (!(0 <= value && value <= 4095))
        throw Generic_exception{Generic_errc::board_settings_invalid,
          "cannot set invalid PWM signal level"};
    };
    const auto low = value.first;
    const auto high = value.second;
    check_value(low);
    check_value(high);
    if (!(low <= high))
      throw Generic_exception{Generic_errc::board_settings_invalid,
        R"(cannot set invalid PWM signal level ("low" cannot be greater than "high"))"};
  }

  static void check_pwm_repeat_count(const int value)
  {
    if (!(value >= 0))
      throw Generic_exception{Generic_errc::board_settings_invalid,
        "cannot set invalid PWM repeat count"};
  }

  static void check_pwm_duty_cycle(const float value)
  {
    if (!(0 < value && value < 1))
      throw Generic_exception{Generic_errc::board_settings_invalid,
        "cannot set invalid PWM duty cycle"};
  }

  // ---------------------------------------------------------------------------
  // High-level helpers setters and getters
  // ---------------------------------------------------------------------------

  template<typename T, typename F>
  void set_values(const std::vector<T>& values, const std::size_t values_req_size,
    std::string root_name, const std::string_view sub_name,
    const std::string_view plural,
    const F& check_value)
  {
    if (!(values.size() >= values_req_size))
      throw Generic_exception{std::string{"cannot set "}.append(plural)
        .append(" (not enough values)")};

    for (int i{}; i < values_req_size; ++i)
      check_value(values[i]);

    for (int i{}; i < values_req_size; ++i)
      set_member(std::move(root_name), i + 1, sub_name, values[i]);
  }

  template<typename T>
  std::optional<std::vector<T>> values(std::string root_name,
    const std::string_view sub_name, const std::size_t result_size) const
  {
    std::vector<T> result;
    result.reserve(result_size);
    for (int i{}; i < result_size; ++i) {
      if (const auto mm = member<T>(std::move(root_name), i + 1, sub_name))
        result.push_back(*mm);
      else
        return std::nullopt;
    }
    return result;
  }

  template<typename T, typename F>
  void set_channel_values(const std::vector<T>& values,
    const std::string_view sub_name, const std::string_view plural,
    const F& check_value)
  {
    set_values(values, mpc_, "CH", sub_name,
      std::string{"channel "}.append(plural), check_value);
  }

  template<typename T>
  std::optional<std::vector<T>> channel_values(const std::string_view sub_name) const
  {
    return values<T>("CH", sub_name, mpc_);
  }

  template<typename T, typename F>
  void set_pwm_values(const std::vector<T>& values,
    const std::string_view sub_name, const std::string_view plural,
    const F& check_value)
  {
    set_values(values, mpc_, "PWM", sub_name,
      std::string{"PWM "}.append(plural), check_value);
  }

  template<typename T>
  std::optional<std::vector<T>> pwm_values(const std::string_view sub_name) const
  {
    return values<T>("PWM", sub_name, mpc_);
  }

  // ---------------------------------------------------------------------------
  // Low-level helpers setters and getters
  // ---------------------------------------------------------------------------

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
    if (sub_name.empty())
      set_member(member_name(std::move(root_name), index), std::forward<T>(value));
    else
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
    if (sub_name.empty())
      return rajson::Value_view{doc_}.optional<T>(member_name(std::move(root_name), index));
    else
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

std::string Board_settings::to_json_text() const
{
  return rep_->to_json_text();
}

void Board_settings::set(const Board_settings& other)
{
  rep_->set(*other.rep_);
}

// -----------------------------------------------------------------------------

Board_settings& Board_settings::set_signal_mode(const Signal_mode value)
{
  rep_->set_signal_mode(value);
  return *this;
}

std::optional<Signal_mode> Board_settings::signal_mode() const
{
  return rep_->signal_mode();
}

// -----------------------------------------------------------------------------

Board_settings& Board_settings::set_channel_measurement_modes(const
  std::vector<Measurement_mode>& values)
{
  rep_->set_channel_measurement_modes(values);
  return *this;
}

std::optional<std::vector<Measurement_mode>> Board_settings::channel_measurement_modes() const
{
  return rep_->channel_measurement_modes();
}

Board_settings& Board_settings::set_channel_gains(const std::vector<float>& values)
{
  rep_->set_channel_gains(values);
  return *this;
}

std::optional<std::vector<float>> Board_settings::channel_gains() const
{
  return rep_->channel_gains();
}

Board_settings& Board_settings::set_channel_iepes(const std::vector<bool>& values)
{
  rep_->set_channel_iepes(values);
  return *this;
}

std::optional<std::vector<bool>> Board_settings::channel_iepes() const
{
  return rep_->channel_iepes();
}

// -----------------------------------------------------------------------------

Board_settings& Board_settings::set_pwms(const std::vector<bool>& values)
{
  rep_->set_pwms(values);
  return *this;
}

std::optional<std::vector<bool>> Board_settings::pwms() const
{
  return rep_->pwms();
}

Board_settings& Board_settings::set_pwm_frequencies(const std::vector<int>& values)
{
  rep_->set_pwm_frequencies(values);
  return *this;
}

std::optional<std::vector<int>> Board_settings::pwm_frequencies() const
{
  return rep_->pwm_frequencies();
}

Board_settings& Board_settings::set_pwm_signal_levels(const std::vector<std::pair<int, int>>& values)
{
  rep_->set_pwm_signal_levels(values);
  return *this;
}

std::optional<std::vector<std::pair<int, int>>> Board_settings::pwm_signal_levels() const
{
  return rep_->pwm_signal_levels();
}

Board_settings& Board_settings::set_pwm_repeat_counts(const std::vector<int>& values)
{
  rep_->set_pwm_repeat_counts(values);
  return *this;
}

std::optional<std::vector<int>> Board_settings::pwm_repeat_counts() const
{
  return rep_->pwm_repeat_counts();
}

Board_settings& Board_settings::set_pwm_duty_cycles(const std::vector<float>& values)
{
  rep_->set_pwm_duty_cycles(values);
  return *this;
}

std::optional<std::vector<float>> Board_settings::pwm_duty_cycles() const
{
  return rep_->pwm_duty_cycles();
}

} // namespace panda::timeswipe
