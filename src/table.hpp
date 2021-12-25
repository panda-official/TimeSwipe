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

#ifndef PANDA_TIMESWIPE_TABLE_HPP
#define PANDA_TIMESWIPE_TABLE_HPP

#include "exceptions.hpp"

#include <algorithm>
#include <limits>
#include <type_traits>
#include <vector>
#include <utility>

namespace panda::timeswipe {

/// Table.
template<typename T>
class Table final {
public:
  /// Alias of the value type.
  using Value = T;

  /// Alias of the column type.
  using Column = std::vector<Value>;

  /// Alias the size type.
  using Size = typename Column::size_type;

  /// Constructs table with zero number of columns and zero number of rows.
  Table() = default;

  /// Constructs table with given number of columns and zero number of rows.
  explicit Table(const Size column_count)
    : columns_(column_count)
  {}

  /// Constructs table with given number of columns and rows.
  Table(const Size column_count, const Size row_count)
    : Table{column_count}
  {
    for (auto& column : columns_) column.resize(row_count);
  }

  /// @returns The number of columns whose data this table contains.
  Size column_count() const noexcept
  {
    return columns_.size();
  }

  /// @returns The number of rows whose data this table contains.
  Size row_count() const noexcept
  {
    return column_count() ? columns_.back().size() : 0;
  }

  /**
   * @returns The reference to the column at the given `index`.
   *
   * @par Requires
   * `index` in range `[0, column_count())`.
   */
  const Column& column(const Size index) const
  {
    if (!(index < column_count()))
      throw Exception{"cannot get table column by invalid index"};

    return columns_[index];
  }

  /**
   * @returns The reference to the value of the given `column` and `row`.
   *
   * @par Requires
   * `column` in range `[0, column_count())`.
   * `row` in range `[0, row_count())`.
   */
  const Value& value(const Size column, const Size row) const
  {
    if (!(column < column_count()))
      throw Exception{"cannot get table value by invalid column index"};
    else if (!(row < row_count()))
      throw Exception{"cannot get table value by invalid row index"};

    return columns_[column][row];
  }

  /// @overload
  Value& value(const Size column, const Size row)
  {
    return const_cast<Value&>(static_cast<const Table*>(this)->value(column, row));
  }

  /**
   * @brief Appends row specified as `args` to the end of this table.
   *
   * @par Requires
   * `(column_count() == sizeof...(args))`.
   *
   * @par Effects
   * row_count() increased by one.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  template<typename ... Types>
  void append_emplaced_row(Types&& ... args)
  {
    if (column_count() != sizeof...(args))
      throw Exception{"cannot append table row with invalid number of columns"};

    append_emplaced_row__(std::make_index_sequence<sizeof...(args)>{},
      std::forward<Types>(args)...);
  }

  /**
   * @brief Appends row filled by using `make_value`.
   *
   * @param make_value Function with parameter "current column index" of type
   * Size that returns a value of type Value. This function will be called
   * column_count() times.
   *
   * @par Effects
   * row_count() increased by one.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  template<typename F>
  void append_generated_row(const F& make_value)
  {
    const Size cc = column_count();
    for (Size i{}; i < cc; ++i)
      columns_[i].push_back(make_value(i));
  }

  /**
   * @brief Appends no more than `count` rows of `other` to the end of this
   * table.
   *
   * @par Requires
   * `(!column_count() || (column_count() == other.column_count()))`.
   *
   * @par Effects
   * `(column_count() == other.column_count())`.
   * row_count() increased by `std::min(other.row_count(), count)`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  template<class Tab>
  void append_rows(Tab&& other,
    const Size count = std::numeric_limits<Size>::max())
  {
    static_assert(std::is_same_v<std::decay_t<Tab>, Table>);

    if (!column_count()) {
      columns_ = {}; // prevent UB if instance was moved
      columns_.resize(other.column_count());
    } else if (!(column_count() == other.column_count()))
      throw Exception{"cannot append table rows from table with different "
        "column count"};

    const Size cc = column_count();
    const Size in_size = std::min(other.row_count(), count);
    const Size out_offset = row_count();
    for (Size i{}; i < cc; ++i) {
      columns_[i].resize(out_offset + in_size);
      const auto b = other.columns_[i].begin();
      const auto e = b + in_size;
      const auto o = columns_[i].begin() + out_offset;
      if constexpr (std::is_rvalue_reference_v<decltype(std::forward<Tab>(other))>)
        std::move(b, e, o);
      else
        std::copy(b, e, o);
    }
  }

  /**
   * @brief Appends column filled by using `make_value`.
   *
   * @param make_value Function with parameter "current row index" of type
   * Size that returns a value of type Value. This function will be called
   * row_count() times.
   *
   * @par Effects
   * column_count() increased by one.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<typename F>
  void append_generated_column(const F& make_value)
  {
    Column column(row_count());
    generate(begin(column), end(column),
      [&make_value, i=0]()mutable{return make_value(i++);});
    columns_.push_back(std::move(column));
  }

  /**
   * @brief Appends `column` to this table.
   *
   * @param column Column to append.
   *
   * @par Requires
   * `(!column_count() || (row_count() == column.size()))`.
   * Column must be of type Column.
   *
   * @par Effects
   * column_count() increased by one.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<class C>
  void append_column(C&& column)
  {
    static_assert(std::is_same_v<std::decay_t<C>, Column>);

    if (!(!column_count() || (row_count() == column.size())))
      throw Exception{"cannot append table column with different row count"};

    columns_.push_back(std::forward<C>(column));
  }

  /**
   * @brief Transforms column of the given `index` by using `make_value`.
   *
   * @param make_value Value transformer with one parameter "column value"
   * of type Value which will be called row_count() times.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  template<typename F>
  void transform_column(const Size index, const F& make_value)
  {
    transform(cbegin(columns_[index]), cend(columns_[index]),
      begin(columns_[index]), make_value);
  }

  /// Removes `std::min(row_count(), count))` rows from the begin of this table.
  void remove_begin_rows(Size count) noexcept
  {
    const Size cc = column_count();
    count = std::min(row_count(), count);
    for (Size i{}; i < cc; ++i)
      columns_[i].erase(columns_[i].begin(), columns_[i].begin() + count);
  }

  /// Removes `std::min(row_count(), count))` rows from the end of this table.
  void remove_end_rows(Size count) noexcept
  {
    const Size cc = column_count();
    const Size rc = row_count();
    count = std::min(rc, count);
    for (Size i{}; i < cc; ++i)
      columns_[i].resize(rc - count);
  }

  /// Reserves memory for `count` columns.
  void reserve_columns(const Size count)
  {
    columns_.reserve(count);
  }

  /// Reserves memory for `count` rows.
  void reserve_rows(const Size count)
  {
    const Size cc = column_count();
    for (Size i{}; i < cc; ++i)
      columns_[i].reserve(count);
  }

  /**
   * @brief Clears columns of this table.
   *
   * @par Effects
   * `(!column_count() && !row_count())`.
   *
   * @see column_count(), row_count().
   */
  void clear_columns() noexcept
  {
    columns_.clear();
  }

  /**
   * @brief Clears rows of this table.
   *
   * @par Effects
   * `!row_count()`.
   *
   * @see row_count().
   */
  void clear_rows() noexcept
  {
    const Size cc = column_count();
    for (Size i{}; i < cc; ++i)
      columns_[i].clear();
  }

  /// @name Iterators
  /// @{

  /// @returns Constant iterator that points to a first column.
  auto columns_cbegin() const noexcept
  {
    return columns_.cbegin();
  }

  /// @returns Constant iterator that points to an one-past-the-last channel.
  auto columns_cend() const noexcept
  {
    return columns_.cend();
  }

  /// @}

private:
  std::vector<Column> columns_;

  template<std::size_t ... I, typename ... Types>
  void append_emplaced_row__(std::index_sequence<I...>, Types&& ... args)
  {
    (columns_[I].push_back(std::forward<Types>(args)), ...);
  }
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_TABLE_HPP
