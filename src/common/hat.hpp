// -*- C++ -*-

// PANDA TimeSwipe Project
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

/**
 * @file
 * HAT data stuff.
 */

#ifndef PANDA_TIMESWIPE_COMMON_HAT_HPP
#define PANDA_TIMESWIPE_COMMON_HAT_HPP

#include "../3rdparty/dmitigr/crc.hpp"
#include "error.hpp"
#include "Serial.h"

#include <array>
#include <cstdint>
#include <cstring>

namespace panda::timeswipe::hat {

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

/// Atom type.
enum class Type : std::uint16_t {
  invalid = 0x0000,
  vendor_info = 0x0001,
  gpio_map = 0x0002,
  linux_device_tree_blob = 0x0003,
  custom = 0x0004,
  invalid2 = 0xFFFF
};

/// Atom stub.
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
   * Simulates successful import of data fields from an ATOM binary image.
   *
   * @returns `true`.
   */
  bool reset(CFIFO&) noexcept
  {
    return true;
  }

  /**
   * Simulates successful export of data fields to an ATOM binary image.
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
  const Uuid& get_uuid() const noexcept
  {
    return uuid_;
  }

  /// @returns Pid.
  std::uint16_t get_pid() const noexcept
  {
    return pid_;
  }

  /// @returns Pver.
  std::uint16_t get_pver() const noexcept
  {
    return pver_;
  }

  /// @returns Vstr.
  const std::string& get_vstr() const noexcept
  {
    return vstr_;
  }

  /// @returns Pstr.
  const std::string& get_pstr() const noexcept
  {
    return pstr_;
  }

private:
  friend hat::Manager;

  static constexpr Type type_{Type::vendor_info};
  static constexpr int index_{};
  Uuid uuid_{};
  std::uint16_t pid_{};
  std::uint16_t pver_{};
  std::string vstr_;
  std::string pstr_;

  /**
   * Resets data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    if (buf.in_avail() < 22) return false;

    // Import uuid_.
    {
      auto* const uuid_bytes = reinterpret_cast<std::uint8_t*>(uuid_.data());
      for (int i{}; i < sizeof(*uuid_.data()) * uuid_.size(); ++i) {
        Character ch;
        buf >> ch;
        uuid_bytes[i] = static_cast<std::uint8_t>(ch);
      }
    }

    // Import pid_.
    {
      Character b0, b1;
      buf >> b0 >> b1;
      reinterpret_cast<std::uint8_t*>(&pid_)[0] = b0;
      reinterpret_cast<std::uint8_t*>(&pid_)[1] = b1;
    }

    // Import pver_.
    {
      Character b0, b1;
      buf >> b0 >> b1;
      reinterpret_cast<std::uint8_t*>(&pver_)[0] = b0;
      reinterpret_cast<std::uint8_t*>(&pver_)[1] = b1;
    }

    // Import vstr_, pstr_.
    {
      int vlen, plen;
      buf >> vlen >> plen;
      vstr_.resize(vlen);
      pstr_.resize(plen);
      for (int i{}; i < vlen; ++i) {
        Character ch;
        buf >> ch;
        vstr_[i] = ch;
      }
      for (int i{}; i < plen; ++i) {
        Character ch;
        buf >> ch;
        pstr_[i] = ch;
      }
    }

    return true;
  }

  /**
   * Stores data fields to an ATOM binary image.
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
      const Character b0{reinterpret_cast<const std::uint8_t*>(&pid_)[0]},
        b1{reinterpret_cast<const std::uint8_t*>(&pid_)[1]};
      buf << b0 << b1;
    }

    // Export pver_.
    {
      const Character b0{reinterpret_cast<const std::uint8_t*>(&pver_)[0]},
        b1{reinterpret_cast<const std::uint8_t*>(&pver_)[1]};
      buf << b0 << b1;
    }

    // Export vstr_, pstr_.
    {
      const int vlen = vstr_.size();
      const int plen = pstr_.size();
      buf << vlen << plen;
      for (int i{}; i < vlen; ++i)
        buf << vstr_[i];
      for (int i{}; i < plen; ++i)
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
  static constexpr int index_{1};

  /**
   * Resets data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    if (buf.in_avail() < sizeof(*this))
      return false;
    auto* const this_bytes = reinterpret_cast<std::uint8_t*>(this);
    for (std::size_t i{}; i < sizeof(*this); ++i) {
      Character ch;
      buf >> ch;
      this_bytes[i] = static_cast<std::uint8_t>(ch);
    }
    return true;
  }

  /**
   * Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool dump(CFIFO& buf) const
  {
    const auto* const this_bytes = reinterpret_cast<const std::uint8_t*>(this);
    for (std::size_t i{}; i < sizeof(*this); ++i)
      buf << this_bytes[i];
    return true;
  }
};

/// Calibration atom.
class Calibration final {
public:
  /// Calibration atom entry.
  class Entry final {
  public:
    /// The default constructor.
    Entry() = default;

    /// The constructor.
    Entry(const float m, const std::uint16_t b) noexcept
      : m_{m}
      , b_{b}
    {}

    /// Sets `m`.
    void set_m(const float m) noexcept
    {
      m_ = m;
    }

    /// Sets `b`.
    void set_b(const std::uint16_t b) noexcept
    {
      b_ = b;
    }

    /// @returns `m`.
    float get_m() const noexcept
    {
      return m_;
    }

    /// @returns `b`.
    std::uint16_t get_b() const noexcept
    {
      return b_;
    }

    /**
     * Resets data fields from an ATOM binary image.
     *
     * @param buf ATOM binary image.
     *
     * @returns `true` on success.
     */
    bool reset(CFIFO& buf)
    {
      if (buf.in_avail() < sizeof(*this))
        return false;
      auto* const this_bytes = reinterpret_cast<std::uint8_t*>(this);
      for (std::size_t i{}; i < sizeof(*this); ++i) {
        Character ch;
        buf >> ch;
        this_bytes[i] = static_cast<std::uint8_t>(ch);
      }
      return true;
    }

    /**
     * Exports data fields to an ATOM binary image.
     *
     * @param buf ATOM binary image.
     *
     * @returns `true` on success.
     */
    bool dump(CFIFO& buf) const
    {
      const auto* const this_bytes = reinterpret_cast<const std::uint8_t*>(this);
      for (std::size_t i{}; i < sizeof(*this); ++i)
        buf << this_bytes[i];
      return true;
    }

  private:
    friend Calibration;

    float m_{1};
    std::uint16_t b_{};
  };

  /// Calibration atom type.
  enum class Type : std::uint16_t {
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
   * doesn't matches to any member of type Type.
   */
  static constexpr const char* to_literal(const Type value)
  {
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

  /**
   * @returns A value of type Type converted from `value`. The returned value
   * is invalid if `err` is not empty after return.
   */
  static Type make_type(const std::uint16_t value, std::string& err)
  {
    const Type result{value};
    if (!to_literal(result))
      err = timeswipe::to_literal(Errc::invalid_calibration_atom_type);
    return result;
  }

  /// The constructor.
  Calibration(const Type type, const std::uint16_t count)
    : header_{type, count, count * sizeof(Entry)}
    , entries_{count}
  {}

  /// @returns The size in bytes.
  constexpr std::size_t get_size_in_bytes() const noexcept
  {
    return header_.dlen + sizeof(Header);
  }

  /// @returns The count of entries.
  std::size_t get_entry_count() const noexcept
  {
    return entries_.size();
  }

  /// @returns The data of the specified `index`.
  const Entry& get_entry(const std::size_t index, std::string& err) const
  {
    if (!(index < entries_.size()))
      err = timeswipe::to_literal(Errc::invalid_calibration_atom_entry_index);
    return entries_[index];
  }

  /// Sets the entry `value` at the specified `index`.
  Calibration& set_entry(const std::size_t index, const Entry& value, std::string& err)
  {
    if (!(index < entries_.size()))
      err = timeswipe::to_literal(Errc::invalid_calibration_atom_entry_index);
    entries_[index] = value;
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
   * Resets data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    // Import header.
    auto* const header_bytes = reinterpret_cast<std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i) {
      Character ch;
      buf >> ch;
      header_bytes[i] = static_cast<std::uint8_t>(ch);
    }

    // Import data.
    for (auto& entry : entries_)
      entry.reset(buf);

    return true;
  }

  /**
   * Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool dump(CFIFO& buf) const
  {
    // Export header.
    const auto* const header_bytes = reinterpret_cast<const std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Export data.
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
      header_.callen += atom.get_size_in_bytes();
  }

  /// @returns Caliration atom of the given `type`.
  const atom::Calibration& get_atom(const atom::Calibration::Type type) const noexcept
  {
    return atoms_[static_cast<std::uint16_t>(type) - 1];
  }

  /// @overload
  atom::Calibration& get_atom(const atom::Calibration::Type type) noexcept
  {
    return const_cast<atom::Calibration&>(static_cast<const Calibration_map*>(this)->get_atom(type));
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
  static constexpr int index_{3};  // FIXME ? (should be 2?)

  /**
   * Resets data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool reset(CFIFO& buf)
  {
    // Import header.
    auto* const header_bytes = reinterpret_cast<std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i) {
      Character ch;
      buf >> ch;
      header_bytes[i] = static_cast<std::uint8_t>(ch);
    }

    // Import data.
    for (auto& atom : atoms_)
      atom.reset(buf);

    return true;
  }

  /**
   * Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool dump(CFIFO& buf) const
  {
    // Export header.
    auto* const header_bytes = reinterpret_cast<const std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Export data.
    for (auto& atom : atoms_)
      atom.dump(buf);

    return true;
  }
};

/// A manager class for working with HATs-EEPROM binary image
class Manager final {
public:
  /// Represents the result of the operation.
  enum class Op_result {
    ok,
    atom_not_found,
    atom_corrupted,
    storage_corrupted,
    storage_unverified
  };

  /**
   * The constructor.
   *
   * @param fifo_buf The buffer with EEPROM binary image.
   */
  explicit Manager(std::shared_ptr<CFIFO> fifo_buf = {})
    : fifo_buf_{std::move(fifo_buf)}
  {}

  /**
   * Gets atom's raw binary data.
   *
   * @param pos Atom position (zero-based).
   * @param[out] type Atom type.
   * @param[out] Output data buffer.
   *
   * @return Operation result.
   */
  Op_result get_atom(const unsigned pos, atom::Type& type, CFIFO& output) const
  {
    if (storage_state_ != Op_result::ok)
      return storage_state_;

    // Get the atom.
    Atom_header* atom{};
    if (const auto r = get_atom_header(pos, &atom); r != Op_result::ok)
      return r;

    // Set helpers.
    const auto* const header_bytes = reinterpret_cast<const char*>(atom);
    const auto* const data_bytes = header_bytes + sizeof(Atom_header); // atom->data_begin;
    const auto dlen = atom->dlen - 2; // real dlen without CRC

    // Check the CRC of the atom.
    {
      const std::uint16_t calc_crc{dmitigr::crc::crc16(header_bytes, dlen + sizeof(Atom_header))};
      const auto* const crc = reinterpret_cast<const std::uint16_t*>(data_bytes + dlen);
      if (calc_crc != *crc)
        return Op_result::atom_corrupted;
    }

    // Fill the out variables.
    type = static_cast<atom::Type>(atom->type);
    for (int i{}; i < dlen; ++i)
      output << data_bytes[i];

    return Op_result::ok;
  }

  /**
   * Resets the atom of given type from the image.
   *
   * @returns `Op_result::ok` on success.
   */
  template <typename A>
  Op_result get(A& atom) const
  {
    atom::Type type;
    CFIFO buf;
    const auto r = get_atom(atom.index_, type, buf);
    return (r == Op_result::ok) && (atom.type_ != type || !atom.reset(buf)) ?
      Op_result::atom_corrupted : r;
  }

  /**
   * Set atom from the `input` buffer to the specified position.
   *
   * @param pos Atom position (zero-based).
   * @param type Atom type.
   * @param input Input data buffer.
   *
   * @return Operation result.
   */
  Op_result set_atom(const unsigned pos, const atom::Type type, const CFIFO& input)
  {
    if (storage_state_ != Op_result::ok)
      return storage_state_;

    const auto atom_count = get_atom_count();
    if (pos > atom_count)
      return Op_result::atom_not_found;
    const bool is_adding{pos == atom_count};

    Atom_header* atom{};
    if (const auto r = get_atom_header(pos, &atom);
      is_adding && r != Op_result::atom_not_found || r != Op_result::ok)
      return r;

    // FIXME: check the whole storage for corruption before continuing here.
    // assert: storage is not corrupted.

    const auto input_size = input.size();
    const int mem_adjust_size = is_adding ?
      input_size + sizeof(Atom_header) + 2 : input_size - atom->dlen + 2;
    if (is_adding)
      adjust_mem_buf(reinterpret_cast<const char*>(atom), mem_adjust_size);
    else
      adjust_mem_buf(reinterpret_cast<const char*>(atom) + sizeof(Atom_header), mem_adjust_size); // keep header

    // adjust_mem_buf() reallocates memory and invalidates `atom`! Update it.
    get_atom_header(pos, &atom);

    // Emplace the atom to the reserved by adjust_mem_buf() space.
    atom->type = static_cast<std::uint16_t>(type);
    atom->count = pos;
    atom->dlen = input_size + 2;
    auto* const atom_bytes = reinterpret_cast<char*>(atom);
    auto* const atom_data_bytes = atom_bytes + sizeof(Atom_header);
    for (std::size_t i{}; i < input_size; ++i)
      atom_data_bytes[i] = input[i];

    // Set CRC stamp of the atom.
    *reinterpret_cast<std::uint16_t*>(atom_data_bytes + input_size) =
      dmitigr::crc::crc16(atom_bytes, input_size + sizeof(Atom_header));

    // Update the EEPROM header if needed.
    if (is_adding) {
      auto* const header = reinterpret_cast<Eeprom_header*>(get_mem_buf());
      header->eeplen += mem_adjust_size;
      header->numatoms = atom_count + 1;
    }

    return Op_result::ok;
  }

  /**
   * Stores the atom of given type to the image.
   *
   * @returns `Op_result::ok` on success.
   */
  template <typename A>
  Op_result set(const A& atom)
  {
    if (storage_state_ != Op_result::ok)
      return storage_state_;
    CFIFO buf;
    atom.dump(buf);
    return set_atom(atom.index_, atom.type_, buf);
  }

  /// Sets the EEPROM image buffer.
  void set_buf(std::shared_ptr<CFIFO> fifo_buf)
  {
    fifo_buf_ = std::move(fifo_buf);
  }

  /// @returns EEPROM image buffer.
  const std::shared_ptr<CFIFO>& get_buf() const noexcept
  {
    return fifo_buf_;
  }

  /// @returns Total atom count.
  std::uint16_t get_atom_count() const noexcept
  {
    return reinterpret_cast<const Eeprom_header*>(get_mem_buf())->numatoms;
  }

  /**
   * Checks the image data integrity.
   *
   * The method must be called before performing any operations on the binary
   * image. It checks all headers and atoms validity and sets `storage_state_`
   * to `Op_result::ok` on success. If the image is empty then reset() must be
   * called instead.
   *
   * @returns `Op_result::ok` on success.
   */
  Op_result verify() const
  {
    return storage_state_ = verify_storage();
  }

  /**
   * Resets all the image data to the default state (zero atom count). Must
   * be called when start working on empty image.
   */
  void reset()
  {
    fifo_buf_->resize(sizeof(Eeprom_header));
    storage_state_ = reset_storage();
  }

private:
  struct Atom_header final {
    std::uint16_t type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
    // char data_begin;
  };

  static constexpr std::uint32_t signature{0x69502d52};
  static constexpr std::uint8_t version{1};
  mutable Op_result storage_state_{Op_result::storage_unverified};
  std::shared_ptr<CFIFO> fifo_buf_;

  /// @name Memory control
  /// @{

  /// @returns Raw pointer to the EEPROM image buffer.
  char* get_mem_buf() const noexcept
  {
    return fifo_buf_->data();
  }

  /// @returns The size of the EEPROM image buffer.
  std::size_t get_mem_buf_size() const noexcept
  {
    return fifo_buf_->size();
  }

  /// Reallocates EEPROM image buffer according to the given `adjustment`.
  void adjust_mem_buf(const char* const offset, const int adjustment)
  {
    const auto position = offset - fifo_buf_->data();
    if (adjustment > 0)
      fifo_buf_->insert(position, adjustment, 0);
    else if (adjustment < 0)
      fifo_buf_->erase(position, -adjustment);
  }

  /// @}

  /// @name Atom stuff
  /// @{

  /**
   * Searchs for the atom with the given `pos` in EEPROM buffer.
   *
   * @param pos The position of the atom to find.
   * @param[out] header The result.
   *
   * @returns `Op_result::ok` on success.
   */
  Op_result get_atom_header(unsigned pos, Atom_header** const header) const
  {
    Op_result result{Op_result::ok};
    auto* const mem_buf = get_mem_buf();
    auto* const mem_buf_end = mem_buf + get_mem_buf_size();
    const auto* const eeprom_header = reinterpret_cast<const Eeprom_header*>(mem_buf);

    // Check `pos` and correct if needed.
    if (pos >= eeprom_header->numatoms) {
      pos = eeprom_header->numatoms;
      result = Op_result::atom_not_found;
    }

    // Find atom.
    auto* atom = mem_buf + sizeof(Eeprom_header);
    for (unsigned i{}; i < pos; ++i) {
      atom += sizeof(Atom_header) + reinterpret_cast<const Atom_header*>(atom)->dlen;
      if (atom > mem_buf_end)
        return Op_result::storage_corrupted;
    }
    *header = reinterpret_cast<Atom_header*>(atom);

    return result;
  }

  /**
   * Checks the atom.
   *
   * @returns `Op_result::ok` on success.
   */
  Op_result verify_atom(const Atom_header* const atom) const
  {
    // Check the CRC.
    const auto dlen = atom->dlen - 2; // dlen without CRC
    const auto* const atom_offset = reinterpret_cast<const char*>(atom);
    const auto* const data_offset = atom_offset + sizeof(Atom_header);
    const auto* const crc_offset = data_offset + dlen;

    const auto crc = *reinterpret_cast<const std::uint16_t*>(crc_offset);
    const auto calc_crc = dmitigr::crc::crc16(atom_offset, dlen + sizeof(Atom_header));
    if (crc != calc_crc)
      return Op_result::atom_corrupted;

    return Op_result::ok;
  }

  /// @}

  /// @name Storage control
  /// @{

  /**
   * Verifies EEPROM buffer.
   *
   * @return `Op_result::ok` on success.
   */
  Op_result verify_storage() const
  {
    const auto* const mem_buf = get_mem_buf();
    const auto mem_buf_size = get_mem_buf_size();
    if (mem_buf_size < sizeof(Eeprom_header))
      return Op_result::storage_corrupted;

    const auto* const header = reinterpret_cast<const Eeprom_header*>(mem_buf);
    const auto* const mem_buf_end = mem_buf + mem_buf_size;

    if (header->signature != signature || header->ver != version
      || header->res || header->eeplen > mem_buf_size)
      return Op_result::storage_corrupted;

    // Verify all the atoms.
    const char* atom = mem_buf + sizeof(Eeprom_header);
    for (std::uint16_t i{}; i < header->numatoms; ++i) {
      const auto* const atom_hdr = reinterpret_cast<const Atom_header*>(atom);
      if (const auto r = verify_atom(atom_hdr); r != Op_result::ok)
        return r;
      atom += sizeof(Atom_header) + atom_hdr->dlen;
      if (atom > mem_buf_end)
        return Op_result::storage_corrupted;
    }
    return Op_result::ok;
  }

  /// Invalidates the EEPROM buffer.
  Op_result reset_storage()
  {
    char* const mem_buf = get_mem_buf();
    const std::size_t mem_buf_size = get_mem_buf_size();

    if (mem_buf_size < sizeof(Eeprom_header))
      return Op_result::storage_corrupted;

    auto* const header = reinterpret_cast<Eeprom_header*>(mem_buf);
    header->signature = signature;
    header->ver = version;
    header->res = 0;
    header->numatoms = 0;
    header->eeplen = sizeof(Eeprom_header);
    return Op_result::ok;
  }

  /// @}
};

} // namespace panda::timeswipe::hat

/// FIXME: remove after placing the entire code base in the namespace
using namespace panda::timeswipe;

#endif  // PANDA_TIMESWIPE_COMMON_HAT_HPP
