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
#include "error_detail.hpp"
#include "rajson.hpp"

namespace rajson = dmitigr::rajson;

namespace panda::timeswipe {
using namespace detail;

// -----------------------------------------------------------------------------
// class Driver_settings::Rep
// -----------------------------------------------------------------------------

struct Driver_settings::Rep final {
  Rep()
  {}

  explicit Rep(const std::string_view json_text) try
    : doc_{rajson::to_document(json_text)}
  {
    const auto srate = sample_rate();
    check_sample_rate(srate);

    const auto bbs = burst_buffer_size();
    const auto freq = frequency();
    if (bbs && freq)
      throw Generic_exception{Generic_errc::driver_settings_invalid,
        "cannot set mutually exclusive settings: burstBufferSize, frequency"};

    check_burst_buffer_size(bbs);
    check_frequency(freq, srate);

    // Translation offsets and translation slopes doesn't need to be checked.
  } catch (const rajson::Parse_exception& e) {
    throw Generic_exception{Generic_errc::driver_settings_invalid,
      std::string{"cannot create driver settings from JSON text (error near"}
        .append(" position ").append(std::to_string(e.parse_result().Offset()))
        .append("): ").append(e.what())};
  } catch (const std::exception& e) {
    throw Generic_exception{Generic_errc::driver_settings_invalid,
      std::string{"cannot create driver settings from JSON text: "}.append(e.what())};
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
    apply(&Rep::set_sample_rate, other.sample_rate());
    apply(&Rep::set_burst_buffer_size, other.burst_buffer_size());
    apply(&Rep::set_frequency, other.frequency());
    apply(&Rep::set_translation_offsets, other.translation_offsets());
    apply(&Rep::set_translation_slopes, other.translation_slopes());
  }

  // ---------------------------------------------------------------------------

  void set_sample_rate(const int rate)
  {
    check_sample_rate(rate);
    set_member("sampleRate", rate);
  }

  std::optional<int> sample_rate() const
  {
    return member<int>("sampleRate");
  }

  void set_burst_buffer_size(const std::size_t size)
  {
    check_burst_buffer_size(static_cast<int>(size));
    set_member("burstBufferSize", size);
    doc_.EraseMember("frequency");
  }

  std::optional<std::size_t> burst_buffer_size() const
  {
    return member<std::size_t>("burstBufferSize");
  }

  void set_frequency(const int frequency)
  {
    check_frequency(frequency, sample_rate());
    set_member("frequency", frequency);
    doc_.EraseMember("burstBufferSize");
  }

  std::optional<int> frequency() const
  {
    return member<int>("frequency");
  }

  void set_translation_offsets(const std::vector<int>& values)
  {
    if (!(values.size() == Driver::instance().max_channel_count()))
      throw Generic_exception{Generic_errc::driver_settings_invalid,
        "cannot set invalid translation offsets"};

    set_member("translationOffsets", values);
  }

  std::optional<std::vector<int>> translation_offsets() const
  {
    return member<std::vector<int>>("translationOffsets");
  }

  void set_translation_slopes(const std::vector<float>& values)
  {
    if (!(values.size() == Driver::instance().max_channel_count()))
      throw Generic_exception{Generic_errc::driver_settings_invalid,
        "cannot set invalid translation slopes"};

    set_member("translationSlopes", values);
  }

  std::optional<std::vector<float>> translation_slopes() const
  {
    return member<std::vector<float>>("translationSlopes");
  }

private:
  rapidjson::Document doc_{rapidjson::Type::kObjectType};

  // ---------------------------------------------------------------------------
  // Checkers
  // ---------------------------------------------------------------------------

  static void check_sample_rate(const std::optional<int> rate)
  {
    if (rate) {
      if (!(Driver::instance().min_sample_rate() <= *rate &&
          *rate <= Driver::instance().max_sample_rate()))
        throw Generic_exception{Generic_errc::driver_settings_invalid,
          "cannot set invalid sample rate"};
    }
  }

  static void check_burst_buffer_size(const std::optional<std::size_t> size)
  {
    if (size) {
      const auto sz = static_cast<int>(*size);
      if (!(Driver::instance().min_sample_rate() <= sz &&
          sz <= Driver::instance().max_sample_rate()))
        throw Generic_exception{Generic_errc::driver_settings_invalid,
          "cannot set invalid burst buffer size"};
    }
  }

  static void check_frequency(const std::optional<int> frequency,
    const std::optional<int> srate)
  {
    if (frequency) {
      if (!srate)
        throw Generic_exception{Generic_errc::driver_settings_invalid,
          "cannot set frequency without sample rate"};

      if (!(1 <= *frequency && *frequency <= *srate))
        throw Generic_exception{Generic_errc::driver_settings_invalid,
          "cannot set invalid frequency"};
    }
  }

  // ---------------------------------------------------------------------------
  // Low-level setters and getters
  // ---------------------------------------------------------------------------

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

Driver_settings::Driver_settings(const std::string_view json_text)
  : rep_{std::make_unique<Rep>(json_text)}
{}

void Driver_settings::swap(Driver_settings& other) noexcept
{
  using std::swap;
  swap(rep_, other.rep_);
}

std::string Driver_settings::to_json_text() const
{
  return rep_->to_json_text();
}

void Driver_settings::set(const Driver_settings& other)
{
  rep_->set(*other.rep_);
}

Driver_settings& Driver_settings::set_sample_rate(const int rate)
{
  rep_->set_sample_rate(rate);
  return *this;
}

std::optional<int> Driver_settings::sample_rate() const
{
  return rep_->sample_rate();
}

Driver_settings& Driver_settings::set_burst_buffer_size(const std::size_t size)
{
  rep_->set_burst_buffer_size(size);
  return *this;
}

std::optional<std::size_t> Driver_settings::burst_buffer_size() const
{
  return rep_->burst_buffer_size();
}

Driver_settings& Driver_settings::set_frequency(const int frequency)
{
  rep_->set_frequency(frequency);
  return *this;
}

std::optional<int> Driver_settings::frequency() const
{
  return rep_->frequency();
}

Driver_settings& Driver_settings::set_translation_offsets(const std::vector<int>& values)
{
  rep_->set_translation_offsets(values);
  return *this;
}

std::optional<std::vector<int>> Driver_settings::translation_offsets() const
{
  return rep_->translation_offsets();
}

Driver_settings& Driver_settings::set_translation_slopes(const std::vector<float>& values)
{
  rep_->set_translation_slopes(values);
  return *this;
}

std::optional<std::vector<float>> Driver_settings::translation_slopes() const
{
  return rep_->translation_slopes();
}

} // namespace panda::timeswipe
