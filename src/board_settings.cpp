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

#include "basics.hpp"
#include "board_settings.hpp"
#include "debug.hpp"
#include "driver.hpp"
#include "exceptions.hpp"
#include "rajson.hpp"

#include <algorithm>
#include <utility>
#include <type_traits>

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe {
using namespace detail;

// -----------------------------------------------------------------------------
// class Board_settings::Rep
// -----------------------------------------------------------------------------

struct Board_settings::Rep final {
  Rep()
  {}

  explicit Rep(const std::string_view json_text) try
    : doc_{rajson::to_document(json_text)}
  {
    // Convert to object if NULL passed.
    if (doc_.IsNull())
      doc_.SetObject();
    else if (!doc_.IsObject())
      throw Exception{Errc::board_settings_invalid, "not a JSON object"};

    // Assert invariant.
    PANDA_TIMESWIPE_ASSERT(doc_.IsObject());

    // Check measurement modes.
    if (const auto modes = channel_measurement_modes()) {
      for (std::decay_t<decltype(mcc_)> i{}; i < mcc_; ++i)
        check_channel_measurement_mode((*modes)[i]);
    }

    // Check channel gains settings.
    if (const auto gains = channel_gains()) {
      for (std::decay_t<decltype(mcc_)> i{}; i < mcc_; ++i)
        check_channel_gain((*gains)[i]);
    }

    // Check channel IEPEs settings.
    channel_iepes();

    // Check PWM-related settings.
    pwm_enabled();
    {
      static const auto apply = [](const auto& checker, const auto& values)
      {
        if (values) {
          const auto values_size = values->size();
          PANDA_TIMESWIPE_ASSERT(values_size == mpc_);
          for (std::decay_t<decltype(values_size)> i{}; i < values_size; ++i)
            checker(values->at(i));
        }
      };
      apply(check_pwm_frequency, pwm_frequencies());
      apply(check_pwm_boundary, pwm_boundaries());
      apply(check_pwm_repeat_count, pwm_repeat_counts());
      apply(check_pwm_duty_cycle, pwm_duty_cycles());
    }
  } catch (const rajson::Parse_exception& e) {
    throw Exception{Errc::board_settings_invalid,
      std::string{"cannot parse board settings: error near position "}
        .append(std::to_string(e.parse_result().Offset())).append(": ")
        .append(e.what())};
  } catch (const std::exception& e) {
    throw Exception{Errc::board_settings_invalid,
      std::string{"invalid board settings: "}.append(e.what())};
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

  void set(const Rep& other)
  {
    const auto apply = [this](const auto& setter, const auto& data)
    {
      if (data) (this->*setter)(*data);
    };
    apply(&Rep::set_channel_measurement_modes, other.channel_measurement_modes());
    apply(&Rep::set_channel_gains, other.channel_gains());
    apply(&Rep::set_channel_iepes, other.channel_iepes());
    apply(&Rep::set_pwm_enabled, other.pwm_enabled());
    apply(&Rep::set_pwm_frequencies, other.pwm_frequencies());
    apply(&Rep::set_pwm_boundaries, other.pwm_boundaries());
    apply(&Rep::set_pwm_repeat_counts, other.pwm_repeat_counts());
    apply(&Rep::set_pwm_duty_cycles, other.pwm_duty_cycles());
  }

  std::string to_json_text() const
  {
    return rajson::to_text(doc_);
  }

  bool is_empty() const
  {
    return doc_.ObjectEmpty() ||
      !(channel_measurement_modes() ||
        channel_gains() ||
        channel_iepes() ||
        pwm_enabled() ||
        pwm_frequencies() ||
        pwm_boundaries() ||
        pwm_repeat_counts() ||
        pwm_duty_cycles());
  }

  // ---------------------------------------------------------------------------

  void set_channel_measurement_modes(const std::vector<Measurement_mode>& values)
  {
    set_channel_values(values, "Mode", "measurement modes", [](auto){return true;});
  }

  std::optional<std::vector<Measurement_mode>> channel_measurement_modes() const
  {
    return channel_values<Measurement_mode>("Mode");
  }

  void set_channel_gains(const std::vector<float>& values)
  {
    set_channel_values(values, "Gain", "gains", [](auto){return true;});
  }

  std::optional<std::vector<float>> channel_gains() const
  {
    return channel_values<float>("Gain");
  }

  void set_channel_iepes(const std::vector<bool>& values)
  {
    set_channel_values(values, "Iepe", "IEPEs", [](auto){return true;});
  }

  std::optional<std::vector<bool>> channel_iepes() const
  {
    return channel_values<bool>("Iepe");
  }

  // ---------------------------------------------------------------------------

  void set_pwm_enabled(const std::vector<bool>& values)
  {
    set_pwm_values(values, "", "enable flags", [](auto){return true;});
  }

  std::optional<std::vector<bool>> pwm_enabled() const
  {
    return pwm_values<bool>("");
  }

  void set_pwm_frequencies(const std::vector<int>& values)
  {
    set_pwm_values(values, "Frequency", "frequencies", check_pwm_frequency);
  }

  std::optional<std::vector<int>> pwm_frequencies() const
  {
    return pwm_values<int>("Frequency");
  }

  void set_pwm_boundaries(const std::vector<std::pair<int, int>>& values)
  {
    if (!(values.size() == mpc_))
      throw Exception{"invalid number of PWM boundaries"};

    // Ensure all the values are ok before applying them.
    for (std::decay_t<decltype(mpc_)> i{}; i < mpc_; ++i)
      check_pwm_boundary(values[i]);

    // Apply the values.
    for (std::decay_t<decltype(mpc_)> i{}; i < mpc_; ++i) {
      set_member("pwm", i + 1, "LowBoundary", values[i].first);
      set_member("pwm", i + 1, "HighBoundary", values[i].second);
    }
  }

  std::optional<std::vector<std::pair<int, int>>> pwm_boundaries() const
  {
    std::vector<std::pair<int, int>> result;
    result.reserve(mpc_);
    for (std::decay_t<decltype(mpc_)> i{}; i < mpc_; ++i) {
      const auto low = member<int>("pwm", i + 1, "LowBoundary");
      if (!low) return std::nullopt;
      const auto high = member<int>("pwm", i + 1, "HighBoundary");
      if (!high) return std::nullopt;

      result.emplace_back(*low, *high);
    }
    return result;
  }

  void set_pwm_repeat_counts(const std::vector<int>& values)
  {
    set_pwm_values(values, "RepeatCount", "repeat counts", check_pwm_repeat_count);
  }

  std::optional<std::vector<int>> pwm_repeat_counts() const
  {
    return pwm_values<int>("RepeatCount");
  }

  void set_pwm_duty_cycles(const std::vector<float>& values)
  {
    set_pwm_values(values, "DutyCycle", "duty cycles", check_pwm_duty_cycle);
  }

  std::optional<std::vector<float>> pwm_duty_cycles() const
  {
    return pwm_values<float>("DutyCycle");
  }

  const rapidjson::Document& doc() const noexcept
  {
    return doc_;
  }

  rapidjson::Document& doc() noexcept
  {
    return doc_;
  }

private:
  inline static const unsigned mcc_{Driver::instance().max_channel_count()};
  inline static const unsigned mpc_{Driver::instance().max_pwm_count()};
  rapidjson::Document doc_{rapidjson::Type::kObjectType};

  // ---------------------------------------------------------------------------
  // Checkers
  // ---------------------------------------------------------------------------

  static void check_channel_measurement_mode(const Measurement_mode mode)
  {
    if (!to_literal(mode))
      throw Exception{Errc::board_settings_invalid, "invalid measurement mode"};
  }

  static void check_channel_gain(const float value)
  {
    if (!(Driver::instance().min_channel_gain() <= value &&
        value <= Driver::instance().max_channel_gain()))
      throw Exception{Errc::board_settings_invalid, "invalid channel gain"};
  }

  static void check_pwm_frequency(const int value)
  {
    if (!(1 <= value && value <= 1000))
      throw Exception{Errc::board_settings_invalid, "invalid PWM frequency"};
  }

  static void check_pwm_boundary(const std::pair<int, int>& value)
  {
    static const auto check_value = [](const int value)
    {
      if (!(0 <= value && value <= 4095))
        throw Exception{Errc::board_settings_invalid, "invalid PWM boundary"};
    };
    const auto low = value.first;
    const auto high = value.second;
    check_value(low);
    check_value(high);
    if (!(low <= high))
      throw Exception{Errc::board_settings_invalid, "invalid range of PWM boundaries"};
  }

  static void check_pwm_repeat_count(const int value)
  {
    if (!(value >= 0))
      throw Exception{Errc::board_settings_invalid, "invalid PWM repeat count"};
  }

  static void check_pwm_duty_cycle(const float value)
  {
    if (!(0 < value && value < 1))
      throw Exception{Errc::board_settings_invalid, "invalid PWM duty cycle"};
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
    if (!(values.size() == values_req_size))
      throw Exception{std::string{"invalid number of "}.append(plural)};

    for (std::size_t i{}; i < values_req_size; ++i)
      check_value(values[i]);

    for (std::size_t i{}; i < values_req_size; ++i)
      set_member(root_name, i + 1, sub_name, values[i]);
  }

  template<typename T>
  std::optional<std::vector<T>> values(std::string root_name,
    const std::string_view sub_name, const std::size_t result_size) const
  {
    std::vector<T> result;
    result.reserve(result_size);
    for (std::size_t i{}; i < result_size; ++i) {
      if (const auto mm = member<T>(root_name, i + 1, sub_name))
        result.push_back(*mm);
    }
    if (result.empty())
      return std::nullopt;
    else if (result.size() == result_size)
      return result;
    else
      throw Exception{Errc::board_settings_invalid,
        std::string{"invalid number of "}.append(root_name)};
  }

  template<typename T, typename F>
  void set_channel_values(const std::vector<T>& values,
    const std::string_view sub_name, const std::string_view plural,
    const F& check_value)
  {
    set_values(values, mcc_, "channel", sub_name,
      std::string{"channel "}.append(plural), check_value);
  }

  template<typename T>
  std::optional<std::vector<T>> channel_values(const std::string_view sub_name) const
  {
    return values<T>("channel", sub_name, mcc_);
  }

  template<typename T, typename F>
  void set_pwm_values(const std::vector<T>& values,
    const std::string_view sub_name, const std::string_view plural,
    const F& check_value)
  {
    set_values(values, mpc_, "pwm", sub_name,
      std::string{"PWM "}.append(plural), check_value);
  }

  template<typename T>
  std::optional<std::vector<T>> pwm_values(const std::string_view sub_name) const
  {
    return values<T>("pwm", sub_name, mpc_);
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
  std::string member_name(std::string root_name, const std::size_t index) const
  {
    return root_name.append(std::to_string(index));
  }

  /// @overload
  std::string member_name(std::string root_name, const std::size_t index,
    const std::string_view sub_name) const
  {
    return sub_name.empty() ? member_name(root_name, index) :
      root_name.append(std::to_string(index)).append(sub_name);
  }

  /// Sets `root_name` variable with index `index` to `value`.
  template<typename T>
  void set_member(std::string root_name, const std::size_t index, T&& value)
  {
    set_member(member_name(std::move(root_name), index), std::forward<T>(value));
  }

  /// @overload
  template<typename T>
  void set_member(std::string root_name, const std::size_t index,
    const std::string_view sub_name, T&& value)
  {
    set_member(member_name(std::move(root_name), index, sub_name),
      std::forward<T>(value));
  }

  /// @returns The value of `root_name` variable.
  template<typename T>
  std::optional<T> member(const std::string_view root_name) const
  {
    return rajson::Value_view{doc_}.optional<T>(root_name);
  }

  /// @returns The variable at `index`.
  template<typename T>
  std::optional<T> member(std::string root_name, const std::size_t index) const
  {
    return rajson::Value_view{doc_}.optional<T>(member_name(std::move(root_name), index));
  }

  /// @overload
  template<typename T>
  std::optional<T> member(std::string root_name, const std::size_t index,
    const std::string_view sub_name) const
  {
    return rajson::Value_view{doc_}.optional<T>(member_name(std::move(root_name),
        index, sub_name));
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

void Board_settings::set(const Board_settings& other)
{
  rep_->set(*other.rep_);
}

std::string Board_settings::to_json_text() const
{
  return rep_->to_json_text();
}

bool Board_settings::is_empty() const
{
  return rep_->is_empty();
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

Board_settings& Board_settings::set_pwm_enabled(const std::vector<bool>& values)
{
  rep_->set_pwm_enabled(values);
  return *this;
}

std::optional<std::vector<bool>> Board_settings::pwm_enabled() const
{
  return rep_->pwm_enabled();
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

Board_settings&
Board_settings::set_pwm_boundaries(const std::vector<std::pair<int, int>>& values)
{
  rep_->set_pwm_boundaries(values);
  return *this;
}

std::optional<std::vector<std::pair<int, int>>> Board_settings::pwm_boundaries() const
{
  return rep_->pwm_boundaries();
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
