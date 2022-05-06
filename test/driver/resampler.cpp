// -*- C++ -*-
/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Copyright (c) 2021 PANDA GmbH / Dmitry Igrishin
*/

#include "../../src/driver.hpp"
#include "../../src/error.hpp"
#include "../../src/resampler.hpp"
#include "../../src/3rdparty/dmitigr/fs/filesystem.hpp"
#include "../../src/3rdparty/dmitigr/progpar/progpar.hpp"
#include "../../src/3rdparty/dmitigr/str/transform.hpp"

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
using Value = Table::Value;

namespace {

// -----------------------------------------------------------------------------
// Program parameters
// -----------------------------------------------------------------------------

inline progpar::Program_parameters params;

inline bool is_common_mandatory_params_specified() noexcept
{
  const auto sz = params.arguments().size();
  return (sz == 1 || sz == 2) && params["columns"] && params["sample-rate"];
}

inline bool is_conversion_mode() noexcept
{
  return is_common_mandatory_params_specified() &&
    !params["up-factor"] && !params["down-factor"];
}

inline bool is_resampling_mode() noexcept
{
  return is_common_mandatory_params_specified() &&
    params["up-factor"] && params["down-factor"];
}

inline unsigned input_file_param_index() noexcept
{
  return 0;
}

inline std::optional<unsigned> output_file_param_index() noexcept
{
  const auto sz = params.arguments().size();
  PANDA_TIMESWIPE_ASSERT(sz <= 2);
  return sz == 2 ? sz - 1 : std::optional<unsigned>{};
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
    "Conversion mode synopsis:\n"
    "  ", params.path().string(),
    " [--output-format=bin|csv]\n"
    "    --columns=<comma-separated-non-negative-integers>\n"
    "    --sample-rate=<positive-integer>\n"
    "    <input-file> [<output-file>]\n\n",

    "Resampling mode synopsis:\n"
    "  ", params.path().string(),
    " [--output-format=bin|csv]\n"
    "    [--extrapolation=zero|constant|symmetric|reflect|periodic|smooth|antisymmetric|antireflect]\n"
    "    [--no-crop-extra]\n"
    "    [--filter-length=<positive-integer>]\n"
    "    [--freq=<comma-separated> --ampl=<comma-separated>]\n"
    "    --columns=<comma-separated-non-negative-integers>\n"
    "    --sample-rate=<positive-integer>\n"
    "    --up-factor=<positive-integer>\n"
    "    --down-factor=<positive-integer>\n"
    "    <input-file> [<output-file>]\n\n",

    "Resampling mode defaults:\n"
    "  --extrapolation=zero\n"
    "  --filter-length=2*10*max(up-factor,down-factor) + 1\n"
    "  --freq=0,0.(9)/up-factor,0.(9)/up-factor,1\n"
    "  --ampl=1,1,0,0\n\n"

    "Common defaults:\n"
    "  --output-format is determined automatically from the input\n\n"

    "Remarks:\n"
    "  The value of 0 in --columns option means \"all-zero column\".\n"
    "  The --columns option can be used to customize the output. For example, if\n"
    "  --columns=1,3,0 the output contains 3 columns: 1, 3 columns (in that\n"
    "  order) of the resampled/converted input followed by the column of zeros; the\n"
    "  4rd column of the input is ignored.\n\n"
    "  --no-crop-extra can be specified to prevent the automatic crop of extra\n"
    "  samples at both the begin and end of the result.\n\n"

    "Warnings:\n"
    "  When the input format is binary the input column count is defined as the\n"
    "  maximum column number specified with the --columns option. For example,\n"
    "  --columns=1,6 assumes that the input contains exactly 6 columns and 2 output\n"
    "  columns (resampled/converted 1 and 6 input columns) required.\n"
    "  When the input format is CSV the input column count is determined\n"
    "  automatically, but the maximum column number, specified with the --columns\n"
    "  option, defines the minimum column count the input must contains.");
}

// -----------------------------------------------------------------------------
// IO
// -----------------------------------------------------------------------------

/// Output format.
enum class Output_format { bin, csv };

/// Open file info.
struct Input_file final {
  bool is_binary{};
  std::uintmax_t size{};
  std::ifstream stream;
};

/// @returns The open file info.
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
  for (std::decay_t<decltype(sz)> i{}; i < sz; ++i) {
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

/**
 * @brief Outputs the table in the specified format and according to the
 * specified columns layout.
 *
 * @param out Output stream.
 * @param format Output format.
 * @param table Table for output.
 * @param output_columns Output columns layout.
 */
inline void write_output(std::ostream& out, const Output_format format,
  const Table& table, const std::vector<int>& output_columns)
{
  const auto row_count = table.row_count(); // same as sample rate
  const auto output_column_count = output_columns.size();
  using Row_count = std::decay_t<decltype(row_count)>;
  using Column_count = std::decay_t<decltype(output_column_count)>;

  PANDA_TIMESWIPE_ASSERT(table.column_count() <= output_columns.size());

  if (format == Output_format::bin) {
    std::vector<Value> row;
    row.reserve(output_column_count);
    for (Row_count ri{}; ri < row_count; ++ri) {
      for (Column_count ci{}; ci < output_column_count; ++ci) {
        const auto idx = output_columns[ci];
        const auto val = (idx >= 0) ? table.value(idx, ri) : Value{};
        row.push_back(val);
      }
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
      for (Column_count ci{}; ci < output_column_count; ++ci) {
        const auto idx = output_columns[ci];
        const auto val = (idx >= 0) ? table.value(idx, ri) : Value{};
        out << val;
        if (columns_processed < output_column_count - 1) {
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

  // Process --columns.
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

    // Parse columns vector.
    const auto o = params["columns"];
    PANDA_TIMESWIPE_ASSERT(o);
    auto result = parse(o.not_empty_value());

    // Check the size.
    if (result.empty())
      throw std::runtime_error{"invalid number of columns"};

    // Check the content.
    static_assert(!std::is_signed_v<decltype(result)::value_type>);

    return result;
  }();

  // Calculate output columns.
  const auto output_columns = [&columns]
  {
    std::vector<int> result(columns.size());
    transform(cbegin(columns), cend(columns), begin(result),
      [i=0](const auto v)mutable{return v ? i++ : -1;});
    return result;
  }();

  // Calculate real columns.
  const auto real_columns = [&columns]
  {
    std::vector<unsigned> result;
    for (const auto ci : columns)
      if (ci) result.push_back(ci);
    return result;
  }();

  // Calculate max input column number. (real_columns can be used.)
  const auto max_input_column_number = *max_element(cbegin(real_columns),
    cend(real_columns));

  // Process --sample-rate.
  const auto sample_rate = [s = params["sample-rate"]]
  {
    PANDA_TIMESWIPE_ASSERT(s);

    const auto result = [&s]
    {
      try {
        return std::stoi(s.not_empty_value());
      } catch (...) {
        throw std::runtime_error{"invalid sample-rate"};
      }
    }();
    if (!(0 < result && result <= 48000))
      throw std::runtime_error{"invalid sample-rate - out of range [1, 48000]"};

    return result;
  }();

  // Process resampler options.
  ts::detail::Resampler_options r_opts;
  if (is_resampling_mode()) {
    // Set column count resampler option.
    r_opts.set_channel_count(real_columns.size());

    // Process --up-factor, --down-factor.
    const auto [up_factor, down_factor] = [
      u = params["up-factor"].not_empty_value(),
      d = params["down-factor"].not_empty_value()]
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
        throw std::runtime_error{"non positive up-factor or down-factor"};

      return std::make_pair(up, down);
    }();
    r_opts.set_up_down(up_factor, down_factor);

    // Process --extrapolation.
    r_opts.set_extrapolation([]
    {
      using ts::detail::Signal_extrapolation;
      if (const auto o = params["extrapolation"]) {
        const auto& v = o.not_empty_value();
        if (v == "zero")
          return Signal_extrapolation::zero;
        else if (v == "constant")
          return Signal_extrapolation::constant;
        else if (v == "symmetric")
          return Signal_extrapolation::symmetric;
        else if (v == "reflect")
          return Signal_extrapolation::reflect;
        else if (v == "periodic")
          return Signal_extrapolation::periodic;
        else if (v == "smooth")
          return Signal_extrapolation::smooth;
        else if (v == "antisymmetric")
          return Signal_extrapolation::antisymmetric;
        else if (v == "antireflect")
          return Signal_extrapolation::antireflect;
        else
          throw std::runtime_error{"invalid extrapolation"};
      } else
        return Signal_extrapolation::zero;
    }());

    // Process --no-crop-extra.
    r_opts.set_crop_extra(!params["no-crop-extra"].is_valid_throw_if_value());

    // Process --filter-length.
    r_opts.set_filter_length([up_factor, down_factor]
    {
      if (const auto o = params["filter-length"]) {
        const auto result = [v = o.not_empty_value()]
        {
          try {
            return std::stoi(v);
          } catch (...) {
            throw std::runtime_error{"invalid filter length"};
          }
        }();
        if (result < 0)
          throw std::runtime_error{"negative filter length"};

        return result;
      } else
        return ts::detail::Resampler_options::default_filter_length(up_factor, down_factor);
    }());

    // Process --freq, --ampl.
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
        if (const auto o = params["freq"])
          return parse(o.not_empty_value());
        else
          return ts::detail::Resampler_options::default_freq(up_factor);
      }();

      auto ampl = []
      {
        if (const auto o = params["ampl"])
          return parse(o.not_empty_value());
        else
          return ts::detail::Resampler_options::default_ampl();
      }();

      if (freq.size() != ampl.size())
        throw std::runtime_error{"freq and ampl of different sizes"};

      return std::make_pair(std::move(freq), std::move(ampl));
    }());
  }

  // Process input-file argument and open the input file.
  auto input_file = open_input_file(params[input_file_param_index()]);
  if (input_file.is_binary && (input_file.size % sample_rate))
    throw std::runtime_error{"input file is corrupted or incorrect sample rate"};

  // Process --output-format.
  const auto output_format = [&input_file]
  {
    if (const auto o = params["output-format"]) {
      const auto& v = o.not_empty_value();
      if (v == "bin")
        return Output_format::bin;
      else if (v == "csv")
        return Output_format::csv;
      else
        throw std::runtime_error{"invalid output format"};
    } else if (input_file.is_binary)
      return Output_format::bin;
    else
      return Output_format::csv;
  }();

  // Process optional output-file argument.
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
        throw std::runtime_error{"cannot open file " + path.string()};
    }
    return result;
  }();

  // Set the output stream.
  auto& os = output_file.is_open() ? output_file : std::cout;
  os.precision(std::numeric_limits<Value>::max_digits10);

  // Make the data processing function.
  unsigned entry_count{};
  const auto process = [&](Table& table)
  {
    static const auto proc = [&]
    {
      std::function<void(const Table&, bool)> result;
      if (is_resampling_mode()) {
        const auto resampler =
          std::make_shared<ts::detail::Fir_table_resampler<float>>(r_opts);
        result = [resampler, &os, output_format, &output_columns]
          (const Table& table, const bool end)
        {
          auto tab = resampler->apply(table);
          write_output(os, output_format, tab, output_columns);
          if (end) {
            tab = resampler->flush();
            write_output(os, output_format, tab, output_columns);
          }
        };
      } else {
        result = [&os, output_format, &output_columns]
          (const Table& table, const bool /*end*/)
        {
          write_output(os, output_format, table, output_columns);
        };
      }
      return result;
    }();
    PANDA_TIMESWIPE_ASSERT(proc);

    const auto last_progress = entry_count % sample_rate;
    if (last_progress)
      message("warning: unaligned input: ", sample_rate - last_progress,
        " rows are missing (sample rate is ", sample_rate, ")");

    const bool end{last_progress || !table.row_count()};
    proc(table, end);
    table.clear_rows();
  };

  // Make the data appending function.
  const auto append = [&real_columns](auto& table, const auto& row)
  {
    PANDA_TIMESWIPE_ASSERT(table.column_count());
    PANDA_TIMESWIPE_ASSERT(table.column_count() == real_columns.size());
    table.append_generated_row([&real_columns, &row](const auto ci)
    {
      const auto idx = real_columns[ci];
      PANDA_TIMESWIPE_ASSERT((idx - 1) < row.size());
      return row[idx - 1];
    });
  };

  /*
   * Process the input data.
   *
   * `table` contains only real columns from input to process.
   * `row` contains all the next cells read from input for appending to `table`.
   */
  Table table(real_columns.size());
  table.reserve_rows(sample_rate);
  std::vector<Value> row;
  row.reserve(max_input_column_number);
  if (input_file.is_binary) {
    row.resize(max_input_column_number);
    while (input_file.stream) {
      for (int i{}; i < sample_rate; ++i) {
        input_file.stream.read(reinterpret_cast<char*>(row.data()),
          row.size() * sizeof(decltype(row)::value_type));
        const auto gcount = input_file.stream.gcount();
        if (input_file.stream) {
          ++entry_count;
          append(table, row);
        } else if (gcount)
          throw std::runtime_error{"cannot read row completely"};
      }
      process(table);
    }
  } else { // CSV
    static const std::string separators{" \t,"};
    PANDA_TIMESWIPE_ASSERT(!separators.empty());
    std::string line(max_input_column_number * 128, '\0');
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
            const auto pos = line.find_first_of(separators, offset);
            PANDA_TIMESWIPE_ASSERT(offset < pos);
            const auto field_length = std::min(pos, gcount) - offset;
            PANDA_TIMESWIPE_ASSERT(field_length);
            const auto field = line.substr(offset, field_length);
            const Value value = stof(field); // stof() discards leading whitespaces
            row.push_back(value);
            offset += field_length + 1;
            ++ci;
          }
          if (ci < max_input_column_number)
            throw std::runtime_error{"too few fields at line "
              + std::to_string(entry_count)};
          append(table, row);
          row.clear();
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
