// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/resampler.hpp"
#include "../../src/debug.hpp"
#include "../../src/driver.hpp"
#include "../../src/3rdparty/dmitigr/filesystem.hpp"
#include "../../src/3rdparty/dmitigr/progpar.hpp"
#include "../../src/3rdparty/dmitigr/str.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ts = panda::timeswipe;
namespace progpar = dmitigr::progpar;
namespace str = dmitigr::str;
using Sensor_value = ts::Data_vector::value_type::value_type;
constexpr auto sensor_count = ts::max_channel_count;

namespace {

// -----------------------------------------------------------------------------
// Program parameters
// -----------------------------------------------------------------------------

inline progpar::Program_parameters params;

inline bool is_conversion_mode() noexcept
{
  const auto sz = params.arguments().size();
  return sz == 2 || sz == 3;
}

inline bool is_resampling_mode() noexcept
{
  const auto sz = params.arguments().size();
  return sz == 4 || sz == 5;
}

inline unsigned input_file_param_index() noexcept
{
  return is_conversion_mode() ? 1 : 3;
}

inline std::optional<unsigned> output_file_param_index() noexcept
{
  const auto sz = params.arguments().size();
  return sz == 3 || sz == 5 ? sz - 1 : std::optional<unsigned>{};
}

// -----------------------------------------------------------------------------
// Messaging
// -----------------------------------------------------------------------------

template<typename ... Types>
void message(Types&& ... parts)
{
  std::clog << params.path().string() << ": ";
  (std::clog << ... << parts) << std::endl;
}

template<typename ... Types>
[[noreturn]] void exit_message(const int code, Types&& ... messages)
{
  message(std::forward<decltype(messages)>(messages)...);
  std::exit(code);
}

template<typename ... Types>
[[noreturn]] void exit_usage()
{
  exit_message(1, "usage:\n",
    "Conversion mode syntax:\n"
    "  ", params.path().string(),
    " [--sensors=<comma-separated-non-negative-integers>]\n"
    "    [--output-format=bin|csv] <sample-rate> <input-file> [<output-file>]\n\n",

    "Resampling mode syntax:\n"
    "  ", params.path().string(),
    " [--sensors=<comma-separated-non-negative-integers>]\n"
    "    [--output-format=bin|csv]\n"
    "    [--extrapolation=zero|constant|symmetric|reflect|periodic|smooth|antisymmetric|antireflect]\n"
    "    [--no-crop-extra]\n"
    "    [--filter-length=<positive-integer>]\n"
    "    [--freq=<comma-separated> --ampl=<comma-separated>]\n"
    "    <sample-rate> <up-factor> <down-factor> <input-file> [<output-file>]\n\n",

    "Resampling mode defaults:\n"
    "  --extrapolation=zero\n"
    "  --filter-length=2*10*max(up-factor,down-factor) + 1\n"
    "  --freq=0,0.(9)/up-factor,0.(9)/up-factor,1\n"
    "  --ampl=1,1,0,0\n\n"

    "Common defaults:\n"
    "  --sensors=<comma-separated-list-of-X>, where X=[0,",sensor_count,"]\n"
    "  --output-format is determined automatically from the input\n\n"

    "Remarks:\n"
    "  Up to ",sensor_count," values can be specified in --sensors option.\n"
    "  The value of 0 in --sensors option means \"all-zero column\".\n"
    "  The --sensors option can be used to customize the output. For example, if\n"
    "  --sensors=1,3,0 the output contains 3 columns: 1, 3 columns (in that\n"
    "  order) of the resampled/converted input followed by the column of zeros; the\n"
    "  4rd column of the input is ignored.\n\n"
    "  --no-crop-extra can be specified to prevent the automatic crop of extra\n"
    "  samples at both the begin and end of the result.\n\n"

    "Warnings:\n"
    "  When the input format is binary and the number of columns is not equals to ",sensor_count,"\n"
    "  the --sensors option must be used in order to specify the both input column\n"
    "  count and the output layout.");
}

// -----------------------------------------------------------------------------
// IO
// -----------------------------------------------------------------------------

struct Input_file final {
  bool is_binary{};
  std::uintmax_t size{};
  std::ifstream stream;
};

inline Input_file open_input_file(const std::filesystem::path& path)
{
  Input_file result;

  // Determining the file size.
  result.size = std::filesystem::file_size(path);

  // Opening the file.
  using std::ios_base;
  result.stream = std::ifstream{path, ios_base::in | ios_base::binary};
  if (!result.stream)
    throw std::runtime_error{"could not open file " + path.string()};

  // Scan first bytes to determining the file format.
  constexpr decltype(result.size) scan_block_size = 8192;
  const auto sz = std::min(scan_block_size, result.size);
  for (auto i = 0*sz; i < sz; ++i) {
    if (result.stream) {
      const char ch = result.stream.get();
      if (!std::isdigit(ch) && !std::isspace(ch) && ch != '.' && ch != ',') {
        result.is_binary = true;
        break;
      }
    } else
      throw std::runtime_error{"could not read file " + path.string()};
  }
  result.stream.seekg(0);

  return result;
}

enum class Output_format { bin, csv };

inline void write_output(std::ostream& out, const Output_format format, const ts::Data_vector& records)
{
  const auto sample_rate = records.size();
  const auto columns_count = count_if(records.cbegin(), records.cend(), [](const auto& v){return !v.empty();});
  if (!columns_count)
    return;
  if (format == Output_format::bin) {
    std::vector<Sensor_value> buf;
    buf.reserve(columns_count);
    for (auto j = 0*sample_rate; j < sample_rate; ++j) {
      for (auto k = 0*sensor_count; k < sensor_count; ++k) {
        if (!records[k].empty())
          buf.push_back(records[k][j]);
      }
      if (!out.write(reinterpret_cast<char*>(buf.data()), buf.size() * sizeof(decltype(buf)::value_type)))
        throw std::runtime_error{"error upon writing the output"};
      buf.clear();
    }
  } else { // CSV
    constexpr char delim = ',';
    for (auto j = 0*sample_rate; j < sample_rate; ++j) {
      auto columns_processed = 0*columns_count;
      for (auto k = 0*sensor_count; k < sensor_count; ++k) {
        if (!records[k].empty())
          out << records[k][j];
        if (columns_processed < columns_count - 1) {
          out << delim;
          ++columns_processed;
        }
      }
      out << '\n';
      if (!out)
        throw std::runtime_error{"error upon writing the output"};
    }
  }
}

} // namespace

int main(int argc, char* argv[])
try {
  params = {argc, argv};
  if (!is_conversion_mode() && !is_resampling_mode())
    exit_usage();

  // Parse sample-rate argument.
  const auto sample_rate = [s = params[0]]
  {
    const auto result = [&s]
    {
      try {
        return std::stoi(s);
      } catch (...) {
        throw std::runtime_error{"invalid sample-rate"};
      }
    }();
    if (!(0 < result && result <= 48000))
      throw std::runtime_error{"sample-rate must be in range [1, 48000]"};
    return result;
  }();

  // Parse up-factor and down-factor arguments.
  const auto [up_factor, down_factor] = is_resampling_mode() ? [u = params[1], d = params[2]]
  {
    const auto [up, down] = [u, d]
    {
      try {
        const int up = std::stoi(u);
        const int down = std::stoi(d);
        return std::pair{up, down};
      } catch (...) {
        throw std::runtime_error{"invalid up-factor or down-factor"};
      }
    }();
    if (up <= 0 || down <= 0)
      throw std::runtime_error{"both up-factor and down-factor must be positive"};
    return std::pair<unsigned, unsigned>{up, down};
  }() : std::make_pair(0u, 0u);

  // Parse input-file argument and opening the input file.
  auto input_file = open_input_file(params[input_file_param_index()]);
  if (input_file.is_binary && (input_file.size % sample_rate))
    throw std::runtime_error{"either input file is likely corrupted or incorrect sample rate specified"};

  // Create the resampler options instance.
  ts::detail::Resampler_options r_opts{up_factor, down_factor};

  // Parse --sensors option.
  const auto sensors = []
  {
    static const auto parse = [](const std::string& v)
    {
      try {
        const auto vals = str::split(v, ",");
        std::vector<int> result(vals.size());
        transform(cbegin(vals), cend(vals), begin(result), [](const auto s){return std::stoi(s);});
        return result;
      } catch (...) {
        throw std::runtime_error{"invalid sensor number"};
      }
    };

    std::vector<int> result;
    if (const auto o = params["sensors"]) {
      if (const auto& v = o.value()) {
        result = parse(*v);

        // Check the size.
        if (result.empty() || result.size() > sensor_count)
          throw std::runtime_error{"invalid number of sensors specified"};

        // Check the content.
        if (const auto [mi, ma] = minmax_element(cbegin(result), cend(result));
          *mi < 0 || *ma > static_cast<int>(sensor_count))
          throw std::runtime_error{"invalid sensor number"};
      } else
        exit_usage();
    } else {
      result.resize(sensor_count);
      generate(begin(result), end(result), [s = 1]() mutable {return s++;});
    }
    return result;
  }();

  // Parse --output-format option.
  const auto output_format = [&input_file]
  {
    if (const auto o = params["output-format"]) {
      if (const auto& v = o.value()) {
        if (*v == "bin")
          return Output_format::bin;
        else if (*v == "csv")
          return Output_format::csv;
      }
      exit_usage();
    } else if (input_file.is_binary)
      return Output_format::bin;
    else
      return Output_format::csv;
  }();

  // Parse --extrapolation option.
  r_opts.set_extrapolation([]
  {
    using ts::detail::Signal_extrapolation;
    if (const auto o = params["extrapolation"]) {
      if (const auto& v = o.value()) {
        if (*v == "zero")
          return Signal_extrapolation::zero;
        else if (*v == "constant")
          return Signal_extrapolation::constant;
        else if (*v == "symmetric")
          return Signal_extrapolation::symmetric;
        else if (*v == "reflect")
          return Signal_extrapolation::reflect;
        else if (*v == "periodic")
          return Signal_extrapolation::periodic;
        else if (*v == "smooth")
          return Signal_extrapolation::smooth;
        else if (*v == "antisymmetric")
          return Signal_extrapolation::antisymmetric;
        else if (*v == "antireflect")
          return Signal_extrapolation::antireflect;
      }
      exit_usage();
    } else
      return Signal_extrapolation::zero;
  }());

  // Parse --no-crop-extra option.
  r_opts.set_crop_extra(not []
  {
    bool result{};
    if (const auto o = params["no-crop-extra"]) {
      if (o.value())
        exit_usage();
      else
        result = true;
    }
    return result;
  }());

  // Parse --filter-length option.
  r_opts.set_filter_length([]
  {
    if (const auto o = params["filter-length"]) {
      if (const auto& v = o.value()) {
        const auto result = [&v]
        {
          try {
            return std::stoi(*v);
          } catch (...) {
            throw std::runtime_error{"invalid filter length"};
          }
        }();
        if (result < 0)
          throw std::runtime_error{"filter length must be non-negative"};
        return static_cast<unsigned>(result);
      }
      exit_usage();
    } else
      return 0u;
  }());

  // Parse --freq and --ampl options.
  {
    const auto [freq, ampl] = []
    {
      static const auto parse = [](const std::string& v)
      {
        try {
          const auto vals = str::split(v, ",");
          std::vector<double> result(vals.size());
          transform(cbegin(vals), cend(vals), begin(result), [](const auto f){return std::stod(f);});
          return result;
        } catch (...) {
          throw std::runtime_error{"invalid freq or ampl"};
        }
      };

      auto freq = []
      {
        if (const auto o = params["freq"]) {
          if (const auto& v = o.value())
            return parse(*v);
          exit_usage();
        }
        return std::vector<double>{};
      }();

      auto ampl = []
      {
        if (const auto o = params["ampl"]) {
          if (const auto& v = o.value())
            return parse(*v);
          exit_usage();
        }
        return std::vector<double>{};
      }();

      if (freq.size() != ampl.size())
        throw std::runtime_error{"both freq and ampl must be equal length"};

      return std::make_pair(std::move(freq), std::move(ampl));
    }();
    r_opts.set_freq_ampl(freq, ampl);
  }

  // Parse optional output-file argument.
  auto output_file = [output_format]
  {
    std::ofstream result;
    if (const auto idx = output_file_param_index()) {
      using std::ios_base;
      const std::filesystem::path path = params[*idx];
      auto mode = ios_base::out;
      if (output_format == Output_format::bin)
        mode |= ios_base::binary;
      result = std::ofstream{path, mode};
      if (!result)
        throw std::runtime_error{"could not open file " + path.string()};
    }
    return result;
  }();

  // Set the output stream.
  auto& os = output_file.is_open() ? output_file : std::cout;
  os.precision(std::numeric_limits<Sensor_value>::max_digits10);

  // Define the convenient function for resampling and output.
  unsigned entry_count{};
  const auto process_records = [&](ts::Data_vector&& records)
  {
    static const auto proc_recs = [&]
    {
      std::function<void(ts::Data_vector&&, bool)> process_records;
      if (is_resampling_mode()) {
        const auto resampler = std::make_shared<ts::detail::Resampler>(r_opts);
        process_records = [resampler, &os, output_format](ts::Data_vector&& records, const bool end)
        {
          auto recs = resampler->apply(std::move(records));
          write_output(os, output_format, recs);
          if (end) {
            recs = resampler->flush();
            write_output(os, output_format, recs);
          }
          records.clear();
        };
      } else {
        process_records = [&os, output_format](ts::Data_vector&& records, const bool /*end*/)
        {
          write_output(os, output_format, records);
          records.clear();
        };
      }
      return process_records;
    }();
    PANDA_TIMESWIPE_ASSERT(proc_recs);

    const auto last_progress = entry_count % sample_rate;
    if (last_progress)
      message("warning: unaligned input: ", sample_rate - last_progress, " records are missing ",
        "(sample rate is ", sample_rate, ")");

    const bool end = last_progress || records.empty();
    proc_recs(std::move(records), end);
  };

  // Read the input and resample it.
  const auto fill_by_sensors = [&sensors, sz = sensors.size()](auto& records, const auto& buf)
  {
    for (auto j = 0*sensors.size(); j < sz; ++j) {
      if (const auto idx = sensors[j]; !idx)
        records[j].emplace_back();
      else
        records[j].push_back(buf[idx - 1]);
    }
  };
  ts::Data_vector records;
  records.reserve(sample_rate);
  std::array<Sensor_value, sensor_count> buf;
  if (input_file.is_binary) {
    while (input_file.stream) {
      for (auto i = 0*sample_rate; i < sample_rate; ++i) {
        input_file.stream.read(reinterpret_cast<char*>(buf.data()), buf.size() * sizeof(decltype(buf)::value_type));
        const auto gcount = input_file.stream.gcount();
        if (input_file.stream) {
          ++entry_count;
          fill_by_sensors(records, buf);
        } else if (gcount)
          throw std::runtime_error{"unable to read the record completely"};
      }
      process_records(std::move(records));
    }
  } else { // CSV
    const std::string separators{" \t,"};
    PANDA_TIMESWIPE_ASSERT(!separators.empty());
    std::string line(512, '\0');
    auto& in = input_file.stream;
    while (in) {
      for (auto i = 0*sample_rate; in && i < sample_rate; ++i) {
        // Parse next line (even if CSV file has no newline after the last line).
        in.getline(line.data(), line.size());
        using Size = std::string::size_type;
        if (const Size gcount = in.gcount(); in || (gcount && (gcount < line.size()))) {
          ++entry_count;
          std::size_t j{};
          Size offset{};
          while (offset < gcount) {
            if (j >= sensor_count)
              throw std::runtime_error{"too many fields at line " + std::to_string(entry_count)};
            const auto pos = line.find_first_of(separators, offset);
            PANDA_TIMESWIPE_ASSERT(offset < pos);
            const auto field_length = std::min(pos, gcount) - offset;
            PANDA_TIMESWIPE_ASSERT(field_length);
            const auto field = line.substr(offset, field_length);
            const Sensor_value value = stof(field); // stof() discards leading whitespaces
            buf[j] = value;
            offset += field_length + 1;
            ++j;
          }
          if (j < sensors.size())
            throw std::runtime_error{"too few fields at line " + std::to_string(entry_count)};
          fill_by_sensors(records, buf);
        }
      }
      process_records(std::move(records));
    }
  }
} catch (const std::exception& e) {
  exit_message(1, e.what());
} catch (...) {
  exit_message(2, "unknown error");
}
