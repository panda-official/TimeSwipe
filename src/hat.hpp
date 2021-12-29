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

#ifndef PANDA_TIMESWIPE_HAT_HPP
#define PANDA_TIMESWIPE_HAT_HPP

#include "debug.hpp"
#include "error.hpp"
#include "serial.hpp"
#include "3rdparty/dmitigr/crc.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace panda::timeswipe::detail::hat {

/// Forward declaration of the manager.
class Manager;

/// Forward declaration of the calibration map.
class Calibration_map;

/// EEPROM header.
struct Eeprom_header final {
  std::uint32_t signature{};
  std::uint8_t ver{};
  std::uint8_t res{};
  std::uint16_t numatoms{};
  std::uint32_t eeplen{};
};

namespace atom {

/**
 * @brief Atom type.
 *
 * @warning Must be std::uint16_t.
 */
enum class Type : std::uint16_t {
  invalid = 0x0000,
  vendor_info = 0x0001,
  gpio_map = 0x0002,
  linux_device_tree_blob = 0x0003,
  custom = 0x0004,
  invalid2 = 0xFFFF
};

/// Atom stub for unimplemented EEPROM atoms.
class Stub final {
public:
  /// The constructor.
  explicit Stub(const int index) noexcept
    : index_{index}
  {}

private:
  friend hat::Manager;

  static constexpr Type type_{Type::custom};
  int index_{};

  /**
   * @brief Simulates successful data reset.
   *
   * @returns `true`.
   */
  bool reset(CFIFO&) noexcept
  {
    return true;
  }

  /**
   * @brief Simulates successful data dump.
   *
   * @returns `true`.
   */
  bool dump(CFIFO&) const noexcept
  {
    return true;
  }
};

/// Vendor info atom.
class Vendor_info final {
public:
  /// Alias for Uuid.
  using Uuid = std::array<std::uint32_t, 4>;

  /// The default constructor.
  Vendor_info() = default;

  /// The constructor.
  Vendor_info(const Uuid uuid,
    const std::uint16_t pid,
    const std::uint16_t pver,
    std::string vstr,
    std::string pstr) noexcept
    : uuid_{uuid}
    , pid_{pid}
    , pver_{pver}
    , vstr_{std::move(vstr)}
    , pstr_{std::move(pstr)}
  {}

  /// @returns UUID.
  const Uuid& uuid() const noexcept
  {
    return uuid_;
  }

  /// @returns Pid.
  std::uint16_t pid() const noexcept
  {
    return pid_;
  }

  /// @returns Pver.
  std::uint16_t pver() const noexcept
  {
    return pver_;
  }

  /// @returns Vstr.
  const std::string& vstr() const noexcept
  {
    return vstr_;
  }

  /// @returns Pstr.
  const std::string& pstr() const noexcept
  {
    return pstr_;
  }

private:
  friend hat::Manager;

  static constexpr Type type_{Type::vendor_info};
  static constexpr int index_{}; // Per EEPROM specification.
  Uuid uuid_{};
  std::uint16_t pid_{};
  std::uint16_t pver_{};
  std::string vstr_;
  std::string pstr_;

  /**
   * @brief Resets data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    if (buf.in_avail() < 22) return false;

    // Reset uuid_.
    {
      auto* const uuid_bytes = reinterpret_cast<std::uint8_t*>(uuid_.data());
      for (unsigned i{}; i < sizeof(*uuid_.data()) * uuid_.size(); ++i) {
        Character ch;
        buf >> ch;
        uuid_bytes[i] = static_cast<std::uint8_t>(ch);
      }
    }

    // Reset pid_.
    {
      Character b0, b1;
      buf >> b0 >> b1;
      reinterpret_cast<std::uint8_t*>(&pid_)[0] = b0;
      reinterpret_cast<std::uint8_t*>(&pid_)[1] = b1;
    }

    // Reset pver_.
    {
      Character b0, b1;
      buf >> b0 >> b1;
      reinterpret_cast<std::uint8_t*>(&pver_)[0] = b0;
      reinterpret_cast<std::uint8_t*>(&pver_)[1] = b1;
    }

    // Reset vstr_, pstr_.
    {
      int vlen, plen;
      buf >> vlen >> plen;
      vstr_.resize(vlen);
      pstr_.resize(plen);
      for (int i{}; i < vlen; ++i) {
        Character ch;
        buf >> ch;
        vstr_.at(i) = ch;
      }
      for (int i{}; i < plen; ++i) {
        Character ch;
        buf >> ch;
        pstr_.at(i) = ch;
      }
    }

    return true;
  }

  /**
   * @brief Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on sucess.
   */
  bool dump(CFIFO& buf) const
  {
    // Export uuid_.
    {
      const auto* const uuid_bytes = reinterpret_cast<const std::uint8_t*>(uuid_.data());
      for (std::size_t i{}; i < sizeof(*uuid_.data()) * uuid_.size(); ++i)
        buf << uuid_bytes[i];
    }

    // Export pid_.
    {
      const auto* const pid_bytes = reinterpret_cast<const std::uint8_t*>(&pid_);
      buf << pid_bytes[0] << pid_bytes[1];
    }

    // Export pver_.
    {
      const auto* const pver_bytes = reinterpret_cast<const std::uint8_t*>(&pver_);
      buf << pver_bytes[0] << pver_bytes[1];
    }

    // Export vstr_, pstr_.
    {
      const auto vlen = vstr_.size();
      const auto plen = pstr_.size();
      buf << vlen << plen;
      for (std::size_t i{}; i < vlen; ++i)
        buf << vstr_[i];
      for (std::size_t i{}; i < plen; ++i)
        buf << pstr_[i];
    }

    return true;
  }
};

/// GPIO map atom.
class Gpio_map final {
public:
  /// The default constructor.
  Gpio_map() = default;

private:
  friend hat::Manager;

  struct {
    std::uint8_t drive      :4;
    std::uint8_t slew       :2;
    std::uint8_t hysteresis :2;
  } bank_drive_{};

  struct {
    std::uint8_t back_power :1;
    std::uint8_t reserved   :7;
  } power_{};

  struct {
    std::uint8_t func_sel :3;
    std::uint8_t reserved :2;
    std::uint8_t pulltype :2;
    std::uint8_t is_used  :1;
  } gpio_[28]{};

  static constexpr Type type_{Type::gpio_map};
  static constexpr int index_{1}; // Per EEPROM specification.

  /**
   * @brief Resets data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    constexpr auto data_size = sizeof(bank_drive_) + sizeof(power_) + sizeof(gpio_);

    if (buf.in_avail() < data_size)
      return false;

    auto* const data = reinterpret_cast<std::uint8_t*>(&bank_drive_);
    for (std::size_t i{}; i < data_size; ++i) {
      Character ch;
      buf >> ch;
      data[i] = static_cast<std::uint8_t>(ch);
    }
    return true;
  }

  /**
   * @brief Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool dump(CFIFO& buf) const
  {
    constexpr auto data_size = sizeof(bank_drive_) + sizeof(power_) + sizeof(gpio_);
    const auto* const data = reinterpret_cast<const std::uint8_t*>(&bank_drive_);
    for (std::size_t i{}; i < data_size; ++i)
      buf << data[i];
    return true;
  }
};

// -----------------------------------------------------------------------------

/**
 * @brief Calibration atom type.
 *
 * @warning Must be std::uint16_t.
 */
enum class Calibration_type : std::uint16_t {
  v_in1    = 0x0001,
  v_in2    = 0x0002,
  v_in3    = 0x0003,
  v_in4    = 0x0004,
  v_supply = 0x0005,
  c_in1    = 0x0006,
  c_in2    = 0x0007,
  c_in3    = 0x0008,
  c_in4    = 0x0009,
  ana_out  = 0x000A
};

/**
 * @returns A literal that represents the `value`, or `nullptr` if `value`
 * doesn't matches to any member of enum.
 */
constexpr const char* to_literal(const Calibration_type value)
{
  using Type = Calibration_type;
  switch (value) {
  case Type::v_in1: return "v_in1";
  case Type::v_in2: return "v_in2";
  case Type::v_in3: return "v_in3";
  case Type::v_in4: return "v_in4";
  case Type::v_supply: return "v_supply";
  case Type::c_in1: return "c_in1";
  case Type::c_in2: return "c_in2";
  case Type::c_in3: return "c_in3";
  case Type::c_in4: return "c_in4";
  case Type::ana_out: return "ana_out";
  }
  return nullptr;
}

/// Calibration atom.
class Calibration final {
public:
  /// Calibration atom entry.
  class Entry final {
  public:
    /// The default constructor.
    Entry() noexcept = default;

    /// The constructor.
    Entry(const float slope, const std::int16_t offset) noexcept
      : slope_{slope}
      , offset_{offset}
    {}

    /// @returns Value of slope.
    float slope() const noexcept
    {
      return slope_;
    }

    /// Sets value of slope.
    void set_slope(const float slope) noexcept
    {
      slope_ = slope;
    }

    /// @returns Value of offset.
    std::int16_t offset() const noexcept
    {
      return offset_;
    }

    /// Sets value of offset.
    void set_offset(const std::int16_t offset) noexcept
    {
      offset_ = offset;
    }

    /**
     * @brief Resets data fields from `buf`.
     *
     * @param buf ATOM binary image.
     *
     * @returns `true` on success.
     */
    bool reset(CFIFO& buf)
    {
      constexpr auto data_size = sizeof(slope_) + sizeof(offset_);
      if (buf.in_avail() < data_size)
        return false;

      auto* const data = reinterpret_cast<std::uint8_t*>(&slope_);
      for (std::size_t i{}; i < data_size; ++i) {
        Character ch;
        buf >> ch;
        data[i] = static_cast<std::uint8_t>(ch);
      }
      return true;
    }

    /**
     * @brief Dumps data fields to `buf`.
     *
     * @param buf ATOM binary image.
     *
     * @returns `true` on success.
     */
    bool dump(CFIFO& buf) const
    {
      const auto* const data = reinterpret_cast<const std::uint8_t*>(&slope_);
      constexpr auto data_size = sizeof(slope_) + sizeof(offset_);
      for (std::size_t i{}; i < data_size; ++i)
        buf << data[i];
      return true;
    }

  private:
    friend Calibration;

    float slope_{1};
    std::int16_t offset_{};
  };

  /// Alias of Calibration_type.
  using Type = Calibration_type;

  /// @returns A value of enum type converted from `value`.
  static Error_or<Type> to_type(const std::uint16_t value) noexcept
  {
    if (const Type result{value}; to_literal(result))
      return result;
    else
      return Errc::board_settings_calibration_data_invalid;
  }

  /// The constructor.
  Calibration(const Type type, const std::uint16_t count)
    : header_{type, count, count * sizeof(Entry)}
    , entries_(count)
  {
    const auto init_entries = [this](const float slope, const std::int16_t offset)
    {
      for (auto& entry : entries_) {
        entry.slope_ = slope;
        entry.offset_ = offset;
      }
    };
    if (type == Type::v_supply)
      init_entries(-176, 4344);
    else if (type != Type::ana_out)
      init_entries(1, 2048);
  }

  /// @returns The size in bytes.
  constexpr std::size_t size_in_bytes() const noexcept
  {
    return header_.dlen + sizeof(Header);
  }

  /// @returns The count of entries.
  std::size_t entry_count() const noexcept
  {
    return entries_.size();
  }

  /// @returns The data of the specified `index`.
  const Entry& entry(const std::size_t index) const
  {
    return entries_.at(index);
  }

  /// Sets the entry `value` at the specified `index`.
  Calibration& set_entry(const std::size_t index, const Entry& value)
  {
    entries_.at(index) = value;
    return *this;
  }

private:
  friend hat::Calibration_map;

  struct Header final {
    Type type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
    // data follows next
  } header_;
  std::vector<Entry> entries_;

  /**
   * @brief Resets data fields from `buf`.
   *
   * @param buf ATOM binary image
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    Header header;

    // Check that input can fit header.
    if (buf.in_avail() < sizeof(header))
      return false;

    // Read header into local variable.
    auto* const header_bytes = reinterpret_cast<std::uint8_t*>(&header);
    for (std::size_t i{}; i < sizeof(header); ++i) {
      Character ch;
      buf >> ch;
      header_bytes[i] = static_cast<std::uint8_t>(ch);
    }

    // Reset entries.
    for (auto& entry : entries_)
      entry.reset(buf);

    return true;
  }

  /**
   * @brief Dumps data fields to `buf`.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool dump(CFIFO& buf) const
  {
    // Dump header.
    const auto* const header_bytes = reinterpret_cast<const std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Dump data.
    for (auto& entry : entries_)
      entry.dump(buf);

    return true;
  }
};

} // namespace atom

/// Calibration map.
class Calibration_map final {
public:
  /// The default constructor.
  Calibration_map()
  {
    // Set data.
    atoms_.reserve(9);
    atoms_.emplace_back(atom::Calibration::Type::v_in1, 22);
    atoms_.emplace_back(atom::Calibration::Type::v_in2, 22);
    atoms_.emplace_back(atom::Calibration::Type::v_in3, 22);
    atoms_.emplace_back(atom::Calibration::Type::v_in4, 22);
    atoms_.emplace_back(atom::Calibration::Type::v_supply, 1);
    atoms_.emplace_back(atom::Calibration::Type::c_in1, 22);
    atoms_.emplace_back(atom::Calibration::Type::c_in2, 22);
    atoms_.emplace_back(atom::Calibration::Type::c_in3, 22);
    atoms_.emplace_back(atom::Calibration::Type::c_in4, 22);

    // Set header.
    header_.cversion = 0x01;
    header_.timestamp = 0; //???
    header_.numcatoms = static_cast<std::uint16_t>(atoms_.size());
    header_.callen = sizeof(Header);
    for (auto& atom : atoms_)
      header_.callen += atom.size_in_bytes();
  }

  /// @returns Caliration atom of the given `type`.
  const atom::Calibration& atom(const atom::Calibration::Type type) const noexcept
  {
    return atoms_.at(static_cast<std::uint16_t>(type) - 1);
  }

  /// @overload
  atom::Calibration& atom(const atom::Calibration::Type type) noexcept
  {
    return const_cast<atom::Calibration&>(static_cast<const Calibration_map*>(this)->atom(type));
  }

  /// @returns The number of atoms in this map.
  std::size_t atom_count() const noexcept
  {
    return atoms_.size();
  }

private:
  friend hat::Manager;

  struct Header final {
    std::uint8_t cversion{};
    std::uint64_t timestamp{};
    std::uint16_t numcatoms{};
    // Total size in bytes of all calibration data (including this header).
    std::uint32_t callen{};
  } __attribute__((packed)) header_;

  std::vector<atom::Calibration> atoms_;
  atom::Type type_{atom::Type::custom};
  static constexpr int index_{3}; // Per EEPROM specification.

  /**
   * @brief Resets data fields from `buf`.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    Header header;

    // Check that input can fit header.
    if (buf.in_avail() < sizeof(header))
      return false;

    // Read header into local variable.
    auto* const header_bytes = reinterpret_cast<std::uint8_t*>(&header);
    for (std::size_t i{}; i < sizeof(header); ++i) {
      Character ch;
      buf >> ch;
      header_bytes[i] = static_cast<std::uint8_t>(ch);
    }

    // Compare lengths of header and header_.
    if (header.callen != header_.callen)
      return false;

    // Reset data.
    for (auto& atom : atoms_)
      atom.reset(buf);

    return true;
  }

  /**
   * @brief Dumps data fields to `buf`.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool dump(CFIFO& buf) const
  {
    // Dump header.
    auto* const header_bytes = reinterpret_cast<const std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Dump data.
    for (auto& atom : atoms_)
      atom.dump(buf);

    return true;
  }
};

/// A manager class for working with HATs-EEPROM binary image.
class Manager final {
public:
  /**
   * @brief Constructs invalid instance.
   *
   * @details Either set_buf() or reset() must be called to make the instance
   * valid.
   *
   * @see set_buf(), reset().
   */
  Manager() noexcept = default;

  /// @return `true` if this instance is valid.
  bool is_valid() const noexcept
  {
    return static_cast<bool>(fifo_buf_);
  }

  /**
   * @brief Sets the EEPROM image buffer.
   *
   * @param fifo_buf The memory area to set. If `nullptr` the effect is the same
   * as if the method `reset()` is called.
   *
   * @par Effects
   * `is_valid()` if the `fifo_buf` is valid EEPROM data copy or `nullptr`.
   *
   * @returns Error if the given `fifo_buf` is not valid.
   *
   * @see reset().
   */
  Error set_buf(std::shared_ptr<CFIFO> fifo_buf) noexcept
  {
    // Verify.
    if (fifo_buf) {
      // Verify size.
      const auto* const mem_buf = fifo_buf->data();
      const auto mem_buf_size = fifo_buf->size();
      if (mem_buf_size < sizeof(Eeprom_header))
        return Errc::hat_eeprom_data_corrupted;

      // Verify header.
      const auto* const header = reinterpret_cast<const Eeprom_header*>(mem_buf);
      const auto* const mem_buf_end = mem_buf + mem_buf_size;
      if (header->signature != signature_ || header->ver != version_
        || header->res || header->eeplen > mem_buf_size)
        return Errc::hat_eeprom_data_corrupted;

      // Verify all the atoms.
      const char* atom_bytes = mem_buf + sizeof(Eeprom_header);
      for (std::uint16_t i{}; i < header->numatoms; ++i) {
        const auto* const atom = reinterpret_cast<const Atom_header*>(atom_bytes);
        if (const auto err = atom->error())
          return err;
        atom_bytes += sizeof(Atom_header) + atom->dlen;
        if (atom_bytes > mem_buf_end)
          return Errc::hat_eeprom_data_corrupted;
      }
    } else
      return reset();

    fifo_buf_ = std::move(fifo_buf);
    return {};
  }

  /**
   * @brief Resets all the image data to the default state (zero atom count).
   *
   * @details Must be called when start working on empty image.
   *
   * @see set_buf().
   */
  Error reset() noexcept
  {
    if (!fifo_buf_)
      fifo_buf_ = std::make_shared<CFIFO>();
    fifo_buf_->resize(sizeof(Eeprom_header));
    if (fifo_buf_->size() < sizeof(Eeprom_header))
      return Errc::out_of_memory;

    auto* const header = reinterpret_cast<Eeprom_header*>(fifo_buf_->data());
    header->signature = signature_;
    header->ver = version_;
    header->res = 0;
    header->numatoms = 0;
    header->eeplen = sizeof(Eeprom_header);
    return {};
  }

  /// @returns EEPROM image buffer.
  const std::shared_ptr<CFIFO>& buf() const noexcept
  {
    return fifo_buf_;
  }

  /// @returns Total atom count.
  std::uint16_t atom_count() const noexcept
  {
    return is_valid() ? reinterpret_cast<const Eeprom_header*>(
      fifo_buf_->data())->numatoms : 0;
  }

  /**
   * @brief Resets the atom of given type from the image.
   *
   * @par Requires
   * `is_valid()`.
   */
  template <typename A>
  Error get(A& atom) const noexcept
  {
    atom::Type type;
    CFIFO buf;
    if (const auto err = get_atom(atom.index_, type, buf))
      return err;
    else if (atom.type_ != type || !atom.reset(buf))
      return Errc::hat_eeprom_atom_corrupted;

    return {};
  }

  /**
   * @brief Sets the atom of the given type in this instance.
   *
   * @par Requires
   * `is_valid()`.
   */
  template <typename A>
  Error set(const A& atom)
  {
    CFIFO buf;
    atom.dump(buf);
    return set_atom(atom.index_, atom.type_, buf);
  }

private:
  struct Atom_header final {
    /// @returns Error if the atom is invalid.
    Error error() const noexcept
    {
      const auto dlen_no_crc = dlen - 2;
      const auto* const atom_offset = reinterpret_cast<const char*>(this);
      const auto* const data_offset = atom_offset + sizeof(*this);
      const auto* const crc_offset = data_offset + dlen_no_crc;

      // Check the CRC.
      using dmitigr::crc::crc16;
      const auto crc = *reinterpret_cast<const std::uint16_t*>(crc_offset);
      const auto calc_crc = crc16(atom_offset, dlen_no_crc + sizeof(*this));
      if (crc != calc_crc)
        return Errc::hat_eeprom_atom_corrupted;

      return {};
    };

    /// @returns The pointer to the atom data.
    const char* data() const noexcept
    {
      const auto* const atom_offset = reinterpret_cast<const char*>(this);
      return atom_offset + sizeof(*this);
    }

    /// @overload
    char* data() noexcept
    {
      return const_cast<char*>(static_cast<const Atom_header*>(this)->data());
    }

    /// Copies the atom data to the `result`.
    void get_data(CFIFO& result) const
    {
      const auto* const data_bytes = data();
      const auto dlen_no_crc = dlen - 2;
      for (int i{}; i < dlen_no_crc; ++i)
        result << data_bytes[i];
    }

    /// Sets the `input` as the atom data.
    void set_data(const CFIFO& input)
    {
      // Set the data.
      auto* const data_bytes = data();
      const auto dlen_no_crc = input.size();
      for (std::size_t i{}; i < dlen_no_crc; ++i)
        data_bytes[i] = input[i];

      // Set the data CRC.
      auto* const crc = reinterpret_cast<std::uint16_t*>(data() + dlen_no_crc);
      *crc = dmitigr::crc::crc16(reinterpret_cast<const char*>(this),
        dlen_no_crc + sizeof(*this));
    }

    atom::Type type{}; // std::uint16_t
    std::uint16_t count{};
    std::uint32_t dlen{};
    // char data_bytes;
  };

  static constexpr std::uint32_t signature_{0x69502d52};
  static constexpr std::uint8_t version_{1};
  std::shared_ptr<CFIFO> fifo_buf_;

  /**
   * @brief Searchs for the atom with the given `pos` in EEPROM buffer.
   *
   * @param pos The position of the atom to find.
   * @param[out] header The result.
   *
   * @par Requires
   * `is_valid()`.
   */
  Error get_atom_header(unsigned pos, Atom_header** const header) const noexcept
  {
    PANDA_TIMESWIPE_ASSERT(is_valid());

    Error result;
    auto* const mem_buf = fifo_buf_->data();
    auto* const mem_buf_end = mem_buf + fifo_buf_->size();
    const auto* const eeprom_header = reinterpret_cast<const Eeprom_header*>(mem_buf);

    // Check `pos` and correct if needed.
    if (pos >= eeprom_header->numatoms) {
      pos = eeprom_header->numatoms;
      result = Errc::hat_eeprom_atom_missed;
    }

    // Find atom.
    auto* atom = mem_buf + sizeof(Eeprom_header);
    for (unsigned i{}; i < pos; ++i) {
      atom += sizeof(Atom_header) + reinterpret_cast<const Atom_header*>(atom)->dlen;
      if (atom > mem_buf_end)
        return Errc::hat_eeprom_data_corrupted;
    }
    *header = reinterpret_cast<Atom_header*>(atom);

    return result;
  }

  /**
   * @brief Gets atom's raw binary data.
   *
   * @param pos Atom position (zero-based).
   * @param[out] type Atom type.
   * @param[out] Output data buffer.
   *
   * @par Requires
   * `is_valid()`.
   */
  Error get_atom(const unsigned pos, atom::Type& type, CFIFO& output) const noexcept
  {
    // Pre-conditions are checked in get_atom_header().

    // Get the atom.
    Atom_header* atom{};
    if (const auto err = get_atom_header(pos, &atom))
      return err;

    // Check the atom.
    if (const auto err = atom->error())
      return err;

    // Set the result.
    type = atom->type;
    atom->get_data(output);

    return {};
  }

  /**
   * @brief Set atom from the `input` buffer to the specified position.
   *
   * @param pos Atom position (zero-based).
   * @param type Atom type.
   * @param input Input data buffer.
   *
   * @par Requires
   * `is_valid()`.
   */
  Error set_atom(const unsigned pos, const atom::Type type, const CFIFO& input)
  {
    // Pre-conditions are checked in get_atom_header().

    const auto acount = atom_count();
    const bool is_adding{pos == acount};
    Atom_header* atom{};
    if (const auto err = get_atom_header(pos, &atom); is_adding) {
      if (err && err != Errc::hat_eeprom_atom_missed)
        return err;
    } else if (err)
      return err;

    const auto input_size = input.size();
    const auto atom_old_size = (atom->dlen + 2) * !is_adding;
    const auto atom_new_size = input_size + 2 + sizeof(Atom_header) * is_adding;
    const auto atom_offset = reinterpret_cast<const char*>(atom) +
      sizeof(Atom_header) * !is_adding; // keep header when updating

    // resize_fifo_buf() reallocates memory and invalidates `atom`!
    resize_fifo_buf(atom_offset, atom_old_size, atom_new_size);
    // Refresh the `atom` after memory reallocation!
    get_atom_header(pos, &atom);

    // Set the atom.
    atom->type = type;
    atom->count = pos;
    atom->dlen = input_size + 2;
    atom->set_data(input);

    // Update the EEPROM header.
    auto* const header = reinterpret_cast<Eeprom_header*>(fifo_buf_->data());
    header->numatoms = acount + is_adding;
    if (atom_new_size > atom_old_size)
      header->eeplen += atom_new_size - atom_old_size;
    else if (atom_new_size < atom_old_size)
      header->eeplen -= atom_old_size - atom_new_size;

    return atom->error();
  }

  /// Resizes the memory area according to the given sizes.
  void resize_fifo_buf(const char* const offset, const std::size_t old_size,
    const std::size_t new_size)
  {
    PANDA_TIMESWIPE_ASSERT(is_valid() && old_size <= fifo_buf_->size());
    const auto position = offset - fifo_buf_->data();
    if (new_size > old_size)
      fifo_buf_->insert(position, new_size - old_size, 0);
    else if (new_size < old_size)
      fifo_buf_->erase(position, old_size - new_size);
  }
};

} // namespace panda::timeswipe::detail::hat

#endif  // PANDA_TIMESWIPE_HAT_HPP
