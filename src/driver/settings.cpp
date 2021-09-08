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

#include "settings.hpp"

#include "../common/rajson.hpp"

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe::driver {

// -----------------------------------------------------------------------------
// class Settings::Rep
// -----------------------------------------------------------------------------

struct Settings::Rep final {
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
    set_member("SampleRate", rate);
  }

  int get_sample_rate() const
  {
    const auto result = get_member<int>("SampleRate");
    return result ? *result : default_sample_rate;
  }

  void set_burst_buffer_size(const std::size_t size)
  {
    set_member("BurstBufferSize", size);
  }

  std::size_t get_burst_buffer_size() const
  {
    const auto result = get_member<std::size_t>("BurstBufferSize");
    return result ? *result : default_burst_buffer_size;
  }

  void set_data_translation_offset(const int index, const int value)
  {
    set_array("TranslationOffsets", index, value, default_data_translation_offset);
  }

  int get_data_translation_offset(const int index) const
  {
    const auto result = get_array<int>("TranslationOffsets", index);
    return result ? *result : default_data_translation_offset;
  }

  void set_data_translation_slope(const int index, const float value)
  {
    set_array("TranslationSlopes", index, value, default_data_translation_slope);
  }

  float get_data_translation_slope(const int index) const
  {
    const auto result = get_array<float>("TranslationSlopes", index);
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
  std::optional<T> get_member(const std::string_view root_name) const
  {
    return rajson::Value_view{doc_}.optional<T>(root_name);
  }

  template<typename T>
  void set_array(const std::string_view name, const std::size_t index, T&& value,
    T&& default_value = T{})
  {
    detail::set_array(doc_, doc_.GetAllocator(), name, index,
      std::forward<T>(value), std::forward<T>(default_value));
  }

  template<typename T>
  std::optional<T> get_array(const std::string_view name, const std::size_t index) const
  {
    return detail::get_array<T>(doc_, name, index);
  }
};

// -----------------------------------------------------------------------------
// class Settings
// -----------------------------------------------------------------------------

Settings::~Settings() = default;

Settings::Settings(const Settings& rhs)
  : rep_{std::make_unique<Rep>(*rhs.rep_)}
{}

Settings& Settings::operator=(const Settings& rhs)
{
  Settings tmp{rhs};
  swap(tmp);
  return *this;
}

Settings::Settings(Settings&& rhs)
  : rep_{std::move(rhs.rep_)}
{}

Settings& Settings::operator=(Settings&& rhs)
{
  Settings tmp{std::move(rhs)};
  swap(tmp);
  return *this;
}

Settings::Settings()
  : rep_{std::make_unique<Rep>()}
{}

Settings::Settings(const std::string_view stringified_json)
  : rep_{std::make_unique<Rep>(stringified_json)}
{}

void Settings::swap(Settings& other) noexcept
{
  using std::swap;
  swap(rep_, other.rep_);
}

std::string Settings::to_stringified_json() const
{
  return rep_->to_stringified_json();
}

Settings& Settings::set_sample_rate(const int rate)
{
  rep_->set_sample_rate(rate);
  return *this;
}

int Settings::get_sample_rate() const
{
  return rep_->get_sample_rate();
}

Settings& Settings::set_burst_buffer_size(const std::size_t size)
{
  rep_->set_burst_buffer_size(size);
  return *this;
}

std::size_t Settings::get_burst_buffer_size() const
{
  return rep_->get_burst_buffer_size();
}

Settings& Settings::set_data_translation_offset(const int index, const int value)
{
  rep_->set_data_translation_offset(index, value);
  return *this;
}

int Settings::get_data_translation_offset(const int index) const
{
  return rep_->get_data_translation_offset(index);
}

Settings& Settings::set_data_translation_slope(const int index, const float value)
{
  rep_->set_data_translation_slope(index, value);
  return *this;
}

float Settings::get_data_translation_slope(const int index) const
{
  return rep_->get_data_translation_slope(index);
}

} // namespace panda::timeswipe::driver
