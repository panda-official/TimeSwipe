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

#include "driver.hpp"
#include "driver_settings.hpp"
#include "rajson.hpp"

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe {

// -----------------------------------------------------------------------------
// class Driver_settings::Rep
// -----------------------------------------------------------------------------

struct Driver_settings::Rep final {
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

  static constexpr int default_sample_rate{48000};
  static constexpr std::size_t default_burst_buffer_size{};
  static constexpr int default_data_translation_offset{};
  static constexpr float default_data_translation_slope{1};

  // ---------------------------------------------------------------------------

  void set_sample_rate(const int rate)
  {
    if (!(Driver::instance().min_sample_rate() <= rate &&
        rate <= Driver::instance().max_sample_rate()))
      throw Exception{Errc::out_of_range};

    set_member("SampleRate", rate);
  }

  int sample_rate() const
  {
    const auto result = member<int>("SampleRate");
    return result ? *result : default_sample_rate;
  }

  void set_burst_buffer_size(const std::size_t size)
  {
    const int sz = static_cast<int>(size);
    if (!(Driver::instance().min_sample_rate() <= sz &&
        sz <= Driver::instance().max_sample_rate()))
      throw Exception{Errc::out_of_range};

    set_member("BurstBufferSize", size);
  }

  std::size_t burst_buffer_size() const
  {
    const auto result = member<std::size_t>("BurstBufferSize");
    return result ? *result : default_burst_buffer_size;
  }

  void set_frequency(const int frequency)
  {
    const auto srate = sample_rate();
    if (!(1 <= frequency && frequency <= srate))
      throw Exception{Errc::invalid_argument};

    set_burst_buffer_size(srate / frequency);
  }

  int frequency() const
  {
    return get_frequency<int>();
  }

  float frequency_precise() const
  {
    return get_frequency<float>();
  }

  template<typename T>
  T get_frequency() const
  {
    if (const T bbs = burst_buffer_size())
      return static_cast<T>(sample_rate()) / bbs;
    else
      return 1;
  }

  void set_data_translation_offset(const int index, const int value)
  {
    set_array_element("TranslationOffsets", index, value, default_data_translation_offset);
  }

  int data_translation_offset(const int index) const
  {
    const auto result = array_element<int>("TranslationOffsets", index);
    return result ? *result : default_data_translation_offset;
  }

  void set_data_translation_slope(const int index, const float value)
  {
    set_array_element("TranslationSlopes", index, value, default_data_translation_slope);
  }

  float data_translation_slope(const int index) const
  {
    const auto result = array_element<float>("TranslationSlopes", index);
    return result ? *result : default_data_translation_slope;
  }

private:
  rapidjson::Document doc_{rapidjson::Type::kObjectType};

  template<typename T>
  void set_member(const std::string_view name, T&& value)
  {
    detail::set_member(doc_, doc_.GetAllocator(), name, std::forward<T>(value));
  }

  template<typename T>
  std::optional<T> member(const std::string_view root_name) const
  {
    return rajson::Value_view{doc_}.optional<T>(root_name);
  }

  template<typename T>
  void set_array_element(const std::string_view name, const std::size_t index, T&& value,
    T&& default_value = T{})
  {
    detail::set_array_element(doc_, doc_.GetAllocator(), name, index,
      std::forward<T>(value), std::forward<T>(default_value));
  }

  template<typename T>
  std::optional<T> array_element(const std::string_view name, const std::size_t index) const
  {
    return detail::array_element<T>(doc_, name, index);
  }
};

// -----------------------------------------------------------------------------
// class Driver_settings
// -----------------------------------------------------------------------------

Driver_settings::~Driver_settings() = default;

Driver_settings::Driver_settings(const Driver_settings& rhs)
  : rep_{std::make_unique<Rep>(*rhs.rep_)}
{}

Driver_settings& Driver_settings::operator=(const Driver_settings& rhs)
{
  Driver_settings tmp{rhs};
  swap(tmp);
  return *this;
}

Driver_settings::Driver_settings(Driver_settings&& rhs)
  : rep_{std::move(rhs.rep_)}
{}

Driver_settings& Driver_settings::operator=(Driver_settings&& rhs)
{
  Driver_settings tmp{std::move(rhs)};
  swap(tmp);
  return *this;
}

Driver_settings::Driver_settings()
  : rep_{std::make_unique<Rep>()}
{}

Driver_settings::Driver_settings(const std::string_view stringified_json)
  : rep_{std::make_unique<Rep>(stringified_json)}
{}

void Driver_settings::swap(Driver_settings& other) noexcept
{
  using std::swap;
  swap(rep_, other.rep_);
}

std::string Driver_settings::to_stringified_json() const
{
  return rep_->to_stringified_json();
}

Driver_settings& Driver_settings::set_sample_rate(const int rate)
{
  rep_->set_sample_rate(rate);
  return *this;
}

int Driver_settings::sample_rate() const
{
  return rep_->sample_rate();
}

Driver_settings& Driver_settings::set_burst_buffer_size(const std::size_t size)
{
  rep_->set_burst_buffer_size(size);
  return *this;
}

std::size_t Driver_settings::burst_buffer_size() const
{
  return rep_->burst_buffer_size();
}

Driver_settings& Driver_settings::set_frequency(const int frequency)
{
  rep_->set_frequency(frequency);
  return *this;
}

int Driver_settings::frequency() const
{
  return rep_->frequency();
}

Driver_settings& Driver_settings::set_data_translation_offset(const int index, const int value)
{
  rep_->set_data_translation_offset(index, value);
  return *this;
}

int Driver_settings::data_translation_offset(const int index) const
{
  return rep_->data_translation_offset(index);
}

Driver_settings& Driver_settings::set_data_translation_slope(const int index, const float value)
{
  rep_->set_data_translation_slope(index, value);
  return *this;
}

float Driver_settings::data_translation_slope(const int index) const
{
  return rep_->data_translation_slope(index);
}

} // namespace panda::timeswipe
