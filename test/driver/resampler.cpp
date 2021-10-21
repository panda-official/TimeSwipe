// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"
#include "../../src/error_detail.hpp"
#include "../../src/resampler.hpp"
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
#include <type_traits>
#include <utility>
#include <vector>

namespace ts = panda::timeswipe;
namespace progpar = dmitigr::progpar;
namespace str = dmitigr::str;
using Table = ts::Driver::Data;
using Cell = Table::Value_type;
constexpr unsigned max_column_count{4};

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
    " [--columns=<comma-separated-non-negative-integers>]\n"
    "    [--output-format=bin|csv] <sample-rate> <input-file> [<output-file>]\n\n",

    "Resampling mode syntax:\n"
    "  ", params.path().string(),
    " [--columns=<comma-separated-non-negative-integers>]\n"
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
    "  --columns=<comma-separated-list-of-X>, where X=[0,",max_column_count,"]\n"
    "  --output-format is determined automatically from the input\n\n"

    "Remarks:\n"
    "  Up to ",max_column_count," values can be specified in --columns option.\n"
    "  The value of 0 in --columns option means \"all-zero column\".\n"
    "  The --columns option can be used to customize the output. For example, if\n"
    "  --columns=1,3,0 the output contains 3 columns: 1, 3 columns (in that\n"
    "  order) of the resampled/converted input followed by the column of zeros; the\n"
    "  4rd column of the input is ignored.\n\n"
    "  --no-crop-extra can be specified to prevent the automatic crop of extra\n"
    "  samples at both the begin and end of the result.\n\n"

    "Warnings:\n"
    "  When the input format is binary and the number of columns is not equals to ",max_column_count,"\n"
    "  the --columns option must be used in order to specify the both input column\n"
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

inline void write_output(std::ostream& out, const Output_format format, const Table& table)
{
  const auto row_count = table.row_count(); // same as sample rate
  const auto column_count = table.column_count();
  using Row_count = std::decay_t<decltype(row_count)>;
  using Column_count = std::decay_t<decltype(column_count)>;

  if (!column_count)
    return;

  if (format == Output_format::bin) {
    std::vector<Cell> row;
    row.reserve(column_count);
    for (Row_count ri{}; ri < row_count; ++ri) {
      for (Column_count ci{}; ci < column_count; ++ci)
        row.push_back(table.value(ci, ri));
      auto* const data = reinterpret_cast<char*>(row.data());
      const auto data_size = row.size() * sizeof(decltype(row)::value_type);
      if (!out.write(data, data_size))
        throw std::runtime_error{"error upon writing the output"};
      row.clear();
    }
  } else { // CSV
    constexpr char delim{','};
    for (Row_count ri{}; ri < row_count; ++ri) {
      Column_count columns_processed{};
      for (Column_count ci{}; ci < column_count; ++ci) {
        out << table.value(ci, ri);
        if (columns_processed < column_count - 1) {
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

  // Create the resampler options instance.
  ts::detail::Resampler_options r_opts;

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
  const auto [up_factor, down_factor] = is_resampling_mode() ?
  [u = params[1], d = params[2]]
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
    return std::make_pair(up, down);
  }() : std::make_pair(0, 0);

  // Parse input-file argument and opening the input file.
  auto input_file = open_input_file(params[input_file_param_index()]);
  if (input_file.is_binary && (input_file.size % sample_rate))
    throw std::runtime_error{"either input file is likely corrupted or"
      " incorrect sample rate specified"};

  // Set up and down factors.
  r_opts.set_up_down(up_factor, down_factor);

  // Parse --columns option.
  const auto columns = []
  {
    static const auto parse = [](const std::string& v)
    {
      try {
        const auto vals = str::split(v, ",");
        std::vector<unsigned> result(vals.size());
        transform(cbegin(vals), cend(vals), begin(result),
          [](const auto s){return std::stoi(s);});
        return result;
      } catch (...) {
        throw std::runtime_error{"invalid column number"};
      }
    };

    std::vector<unsigned> result;
    if (const auto o = params["columns"]) {
      if (const auto& v = o.value()) {
        result = parse(*v);

        // Check the size.
        if (result.empty() || result.size() > max_column_count)
          throw std::runtime_error{"invalid number of columns specified"};

        // Check the content.
        const auto ma = max_element(cbegin(result), cend(result));
        static_assert(!std::is_signed_v<decltype(*ma)>);
        if (*ma > max_column_count)
          throw std::runtime_error{"invalid column number"};
      } else
        exit_usage();
    } else {
      result.resize(max_column_count);
      generate(begin(result), end(result), [s = 1]() mutable {return s++;});
    }
    return result;
  }();

  // Set column count resampler option.
  r_opts.set_channel_count(columns.size());

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

  // Parse --extrapolation option and set the corresponding resampler option.
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

  // Parse --no-crop-extra option and set the corresponding resampler option.
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

  // Parse --filter-length option and set the corresponding resampler option.
  r_opts.set_filter_length([up_factor, down_factor]
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
        return result;
      }
      exit_usage();
    } else
      return ts::detail::Resampler_options::default_filter_length(up_factor, down_factor);
  }());

  // Parse --freq and --ampl options and set the corresponding resampler options.
  r_opts.set_freq_ampl([up_factor]
  {
    static const auto parse = [](const std::string& v)
    {
      try {
        const auto vals = str::split(v, ",");
        std::vector<double> result(vals.size());
        transform(cbegin(vals), cend(vals), begin(result),
          [](const auto f){return std::stod(f);});
        return result;
      } catch (...) {
        throw std::runtime_error{"invalid freq or ampl"};
      }
    };

    auto freq = [up_factor]
    {
      if (const auto o = params["freq"]) {
        if (const auto& v = o.value())
          return parse(*v);
        exit_usage();
      }
      return ts::detail::Resampler_options::default_freq(up_factor);
    }();

    auto ampl = []
    {
      if (const auto o = params["ampl"]) {
        if (const auto& v = o.value())
          return parse(*v);
        exit_usage();
      }
      return ts::detail::Resampler_options::default_ampl();
    }();

    if (freq.size() != ampl.size())
      throw std::runtime_error{"both freq and ampl must be equal length"};

    return std::make_pair(std::move(freq), std::move(ampl));
  }());

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
  os.precision(std::numeric_limits<Cell>::max_digits10);

  // Define the convenient function for resampling and output.
  unsigned entry_count{};
  const auto process = [&](Table& table)
  {
    static const auto proc = [&]
    {
      std::function<void(const Table&, bool)> result;
      if (is_resampling_mode()) {
        const auto resampler = std::make_shared<ts::detail::Resampler<float>>(r_opts);
        result = [resampler, &os, output_format](const Table& table, const bool end)
        {
          auto recs = resampler->apply(table);
          write_output(os, output_format, recs);
          if (end) {
            recs = resampler->flush();
            write_output(os, output_format, recs);
          }
        };
      } else {
        result = [&os, output_format](const Table& table, const bool /*end*/)
        {
          write_output(os, output_format, table);
        };
      }
      return result;
    }();
    PANDA_TIMESWIPE_ASSERT(proc);

    const auto last_progress = entry_count % sample_rate;
    if (last_progress)
      message("warning: unaligned input: ", sample_rate - last_progress,
        " rows are missing (sample rate is ", sample_rate, ")");

    const bool end{last_progress || table.is_empty()};
    proc(table, end);
    table.clear_rows();
  };

  // Read the input and resample it.
  const auto append_row = [&columns, sz = columns.size()]
    (auto& table, const auto& row)
  {
    PANDA_TIMESWIPE_ASSERT(table.column_count());
    table.append_generated_row([&](const auto i)
    {
      const auto idx = columns[i];
      return idx ? row[idx - 1] : 0;
    });
  };
  Table table(columns.size());
  table.reserve_rows(sample_rate);
  std::vector<Cell> row(columns.size());
  if (input_file.is_binary) {
    while (input_file.stream) {
      for (int i{}; i < sample_rate; ++i) {
        input_file.stream.read(reinterpret_cast<char*>(row.data()),
          row.size() * sizeof(decltype(row)::value_type));
        const auto gcount = input_file.stream.gcount();
        if (input_file.stream) {
          ++entry_count;
          append_row(table, row);
        } else if (gcount)
          throw std::runtime_error{"unable to read the row completely"};
      }
      process(table);
    }
  } else { // CSV
    const std::string separators{" \t,"};
    PANDA_TIMESWIPE_ASSERT(!separators.empty());
    std::string line(max_column_count * 128, '\0');
    auto& in = input_file.stream;
    while (in) {
      for (int ri{}; in && ri < sample_rate; ++ri) {
        // Parse next line (even if CSV file has no newline after the last line).
        in.getline(line.data(), line.size());
        using Size = std::string::size_type;
        if (const Size gcount = in.gcount(); in || (gcount && (gcount < line.size()))) {
          ++entry_count;
          unsigned ci{};
          Size offset{};
          while (offset < gcount) {
            if (ci >= max_column_count)
              throw std::runtime_error{"too many fields at line "
                + std::to_string(entry_count)};
            const auto pos = line.find_first_of(separators, offset);
            PANDA_TIMESWIPE_ASSERT(offset < pos);
            const auto field_length = std::min(pos, gcount) - offset;
            PANDA_TIMESWIPE_ASSERT(field_length);
            const auto field = line.substr(offset, field_length);
            const Cell value = stof(field); // stof() discards leading whitespaces
            row[ci] = value;
            offset += field_length + 1;
            ++ci;
          }
          if (ci < columns.size())
            throw std::runtime_error{"too few fields at line "
              + std::to_string(entry_count)};
          append_row(table, row);
        }
      }
      process(table);
    }
  }
} catch (const std::exception& e) {
  exit_message(1, e.what());
} catch (...) {
  exit_message(2, "unknown error");
}
