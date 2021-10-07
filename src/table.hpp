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

#include "error.hpp"

#include <algorithm>
#include <type_traits>
#include <vector>
#include <utility>

namespace panda::timeswipe {

/// Table.
template<typename T>
class Table final {
public:
  /// Alias of the value type.
  using Value_type = T;

  /// Alias of the column type.
  using Column_type = std::vector<Value_type>;

  /// Alias the size type.
  using Size_type = typename Column_type::size_type;

  /// Constructs empty table.
  Table() = default;

  /// Constructs table with given `column_count` and zero rows.
  explicit Table(const Size_type column_count)
    : columns_(column_count)
  {}

  /// @returns The number of columns whose data this table contains.
  Size_type column_count() const noexcept
  {
    return columns_.size();
  }

  /// @returns The number of rows whose data this table contains.
  Size_type row_count() const noexcept
  {
    return column_count() ? columns_.back().size() : 0;
  }

  /**
   * @returns `true` if this table is empty.
   *
   * @see clear().
   */
  bool is_empty() const noexcept
  {
    return !column_count();
  }

  /**
   * @returns The reference to the column at the given `index`.
   *
   * @par Requires
   * `index` in range `[0, column_count())`.
   */
  const Column_type& column(const Size_type index) const
  {
    if (!(index < column_count()))
      detail::throw_exception("cannot get column by invalid index");

    return columns_[index];
  }

  /**
   * @returns The reference to the value of the given `column` and `row`.
   *
   * @par Requires
   * `column` in range `[0, column_count())`.
   * `row` in range `[0, row_count())`.
   */
  const Value_type& value(const Size_type column, const Size_type row) const
  {
    if (!(column < column_count()))
      detail::throw_exception("cannot get value by invalid column index");
    else if (!(row < row_count()))
      detail::throw_exception("cannot get value by invalid row index");

    return columns_[column][row];
  }

  /// @overload
  Value_type& value(const Size_type column, const Size_type row)
  {
    return const_cast<Value_type&>(static_cast<const Table*>(this)->value(column, row));
  }

  /**
   * Appends row specified as `args` to the end of this table.
   *
   * @par Requires
   * `(column_count() == sizeof...(args))`.
   *
   * @par Effects
   * row_count() increased by one.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<typename ... Types>
  void append_emplaced_row(Types&& ... args)
  {
    if (column_count() != sizeof...(args))
      detail::throw_exception("cannot append table row with invalid number of columns");

    append_emplaced_row__(std::make_index_sequence<sizeof...(args)>{},
      std::forward<Types>(args)...);
  }

  /**
   * Appends row by using `make_value` to the end of this table.
   *
   * @param make_value Function with one parameter "column index" of type
   * Size_type that returns a value of type Value_type. This function will be
   * called column_count() times.
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
    const Size_type cc = column_count();
    for (Size_type i{}; i < cc; ++i)
      columns_[i].push_back(make_value(i));
  }

  /**
   * Appends no more than `count` rows of `other` to the end of this table.
   *
   * @par Requires
   * `(is_empty() || (column_count() == other.column_count()))`.
   *
   * @par Effects
   * `(column_count() == other.column_count())`.
   * row_count() increased by `std::min(other.row_count(), count)`.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  void append_rows(const Table& other, const Size_type count = -1)
  {
    if (is_empty()) {
      columns_ = {}; // prevent UB if instance was moved
      columns_.resize(other.column_count());
    } else if (!(column_count() == other.column_count()))
      detail::throw_exception("cannot append table rows from table"
        " with different column count");

    const Size_type cc = column_count();
    const Size_type in_size = std::min(other.row_count(), count);
    const Size_type out_offset = row_count();
    for (Size_type i{}; i < cc; ++i) {
      columns_[i].resize(out_offset + in_size);
      const auto b = other.columns_[i].begin();
      std::copy(b, b + in_size, columns_[i].begin() + out_offset);
    }
  }

  /**
   * Appends `column` to this table.
   *
   * @param column Column to append.
   *
   * @par Requires
   * `(is_empty() || (row_count() == column.size()))`.
   * Column must be of type Column_type.
   *
   * @par Effects
   * column_count() increased by one.
   *
   * @par Exception safety guarantee
   * Strong.
   */
  template<class Column>
  void append_column(Column&& column)
  {
    static_assert(std::is_same_v<std::decay_t<Column>, Column_type>);

    if (!(is_empty() || (row_count() == column.size())))
      detail::throw_exception("cannot append table column"
        " with different row count");

    columns_.push_back(std::forward<Column>(column));
  }

  /**
   * Transforms column of the given `index` by using `make_value`.
   *
   * @param make_value Value transformer with one parameter "column value"
   * of type Value_type which will be called row_count() times.
   *
   * @par Exception safety guarantee
   * Basic.
   */
  template<typename F>
  void transform_column(const Size_type index, const F& make_value)
  {
    transform(cbegin(columns_[index]), cend(columns_[index]),
      begin(columns_[index]), make_value);
  }

  /// Removes `std::min(row_count(), count))` rows from the begin of this table.
  void erase_begin_rows(Size_type count) noexcept
  {
    const Size_type cc = column_count();
    count = std::min(row_count(), count);
    for (Size_type i{}; i < cc; ++i)
      columns_[i].erase(columns_[i].begin(), columns_[i].begin() + count);
  }

  /// Removes `std::min(row_count(), count))` rows from the end of this table.
  void erase_end_rows(Size_type count) noexcept
  {
    const Size_type cc = column_count();
    const Size_type rc = row_count();
    count = std::min(rc, count);
    for (Size_type i{}; i < cc; ++i)
      columns_[i].resize(rc - count);
  }

  /// Reserves memory for `count` columns.
  void reserve_columns(const Size_type count)
  {
    columns_.reserve(count);
  }

  /// Reserves memory for `count` rows.
  void reserve_rows(const Size_type count)
  {
    const Size_type cc = column_count();
    for (Size_type i{}; i < cc; ++i)
      columns_[i].reserve(count);
  }

  /**
   * Clears this table.
   *
   * @par Effects
   * `is_empty()`.
   *
   * @see is_empty().
   */
  void clear() noexcept
  {
    columns_.clear();
  }

  /**
   * Clears rows of this table.
   *
   * @par Effects
   * `!row_count()`.
   *
   * @see row_count().
   */
  void clear_rows() noexcept
  {
    const Size_type cc = column_count();
    for (Size_type i{}; i < cc; ++i)
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
  std::vector<Column_type> columns_;

  template<std::size_t ... I, typename ... Types>
  void append_emplaced_row__(std::index_sequence<I...>, Types&& ... args)
  {
    (columns_[I].push_back(std::forward<Types>(args)), ...);
  }
};

} // namespace panda::timeswipe

#endif  // PANDA_TIMESWIPE_TABLE_HPP
