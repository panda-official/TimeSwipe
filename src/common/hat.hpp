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
class CalibrationMap;

/// EEPROM header.
struct EepromHeader final {
  std::uint32_t signature{};
  std::uint8_t ver{};
  std::uint8_t res{};
  std::uint16_t numatoms{};
  std::uint32_t eeplen{};
};

namespace atom {

/// Atom type.
enum class Type : std::uint16_t {
  Invalid = 0x0000,
  VendorInfo = 0x0001,
  GpioMap = 0x0002,
  LinuxDeviceTreeBlob = 0x0003,
  Custom = 0x0004,
  Invalid2 = 0xFFFF
};

/// Atom stub.
class Stub final {
public:
  /// The constructor.
  explicit Stub(const int nIndex) noexcept
    : index_{nIndex}
  {}

private:
  friend hat::Manager;

  static constexpr Type type_{Type::Custom};
  int index_{};

  /**
   * Simulates successful import of data fields from an ATOM binary image.
   *
   * @returns `true`.
   */
  bool Import(CFIFO&) noexcept
  {
    return true;
  }

  /**
   * Simulates successful export of data fields to an ATOM binary image.
   *
   * @returns `true`.
   */
  bool Export(CFIFO&) const noexcept
  {
    return true;
  }
};

/// Vendor info atom.
class VendorInfo final {
public:
  /// Alias for Uuid.
  using Uuid = std::array<std::uint32_t, 4>;

  /// The constructor.
  VendorInfo(const Uuid uuid,
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
  const Uuid& GetUuid() const noexcept
  {
    return uuid_;
  }

  /// @returns Pid.
  std::uint16_t GetPid() const noexcept
  {
    return pid_;
  }

  /// @returns Pver.
  std::uint16_t GetPver() const noexcept
  {
    return pver_;
  }

  /// @returns Vstr.
  const std::string& GetVstr() const noexcept
  {
    return vstr_;
  }

  /// @returns Pstr.
  const std::string& GetPstr() const noexcept
  {
    return pstr_;
  }

private:
  friend hat::Manager;

  static constexpr Type type_{Type::VendorInfo};
  static constexpr int index_{};
  Uuid uuid_{};
  std::uint16_t pid_{};
  std::uint16_t pver_{};
  std::string vstr_;
  std::string pstr_;

  /**
   * Imports data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool Import(CFIFO& buf)
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
  bool Export(CFIFO& buf) const
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
class GpioMap final {
public:
  /// The default constructor.
  GpioMap() = default;

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

  static constexpr Type type_{Type::GpioMap};
  static constexpr int index_{1};

  /**
   * Imports data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image
   *
   * @returns `true` on success.
   */
  bool Import(CFIFO& buf)
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
  bool Export(CFIFO& buf) const
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
    void SetM(const float m) noexcept
    {
      m_ = m;
    }

    /// Sets `b`.
    void SetB(const std::uint16_t b) noexcept
    {
      b_ = b;
    }

    /// @returns `m`.
    float GetM() const noexcept
    {
      return m_;
    }

    /// @returns `b`.
    std::uint16_t GetB() const noexcept
    {
      return b_;
    }

    /**
     * Imports data fields from an ATOM binary image.
     *
     * @param buf ATOM binary image.
     *
     * @returns `true` on success.
     */
    bool Import(CFIFO& buf)
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
    bool Export(CFIFO& buf) const
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
    V_In1    = 0x0001,
    V_In2    = 0x0002,
    V_In3    = 0x0003,
    V_In4    = 0x0004,
    V_supply = 0x0005,
    C_In1    = 0x0006,
    C_In2    = 0x0007,
    C_In3    = 0x0008,
    C_In4    = 0x0009,
    Ana_Out  = 0x000A
  };

  /**
   * @returns A literal that represents the `value`, or `nullptr` if `value`
   * doesn't matches to any member of type Type.
   */
  static constexpr const char* ToLiteral(const Type value)
  {
    switch (value) {
    case Type::V_In1: return "V_In1";
    case Type::V_In2: return "V_In2";
    case Type::V_In3: return "V_In3";
    case Type::V_In4: return "V_In4";
    case Type::V_supply: return "V_supply";
    case Type::C_In1: return "C_In1";
    case Type::C_In2: return "C_In2";
    case Type::C_In3: return "C_In3";
    case Type::C_In4: return "C_In4";
    case Type::Ana_Out: return "Ana_Out";
    }
    return nullptr;
  }

  /**
   * @returns A value of type Type converted from `value`. The returned value
   * is invalid if `err` is not empty after return.
   */
  static Type MakeType(const std::uint16_t value, std::string& err)
  {
    const Type result{value};
    if (!ToLiteral(result))
      err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomType);
    return result;
  }

  /// The constructor.
  Calibration(const Type type, const std::uint16_t count)
    : header_{type, count, count * sizeof(Entry)}
    , entries_{count}
  {}

  /// @returns The size in bytes.
  constexpr std::size_t GetSizeInBytes() const noexcept
  {
    return header_.dlen + sizeof(Header);
  }

  /// @returns The count of elements.
  std::size_t GetEntryCount() const noexcept
  {
    return entries_.size();
  }

  /// @returns The data of the specified `index`.
  const Entry& GetEntry(const std::size_t index, std::string& err) const
  {
    if (!(index < entries_.size()))
      err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomEntryIndex);
    return entries_[index];
  }

  /// Sets the data `value` at the specified `indexe.
  void SetEntry(const std::size_t index, const Entry& value, std::string& err)
  {
    if (!(index < entries_.size()))
      err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomEntryIndex);
    entries_[index] = value;
  }

private:
  friend hat::CalibrationMap;

  struct Header final {
    Type type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
    // data follows next
  } header_;
  std::vector<Entry> entries_;

  /**
   * Imports data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image
   *
   * @returns `true` on success.
   */
  bool Import(CFIFO& buf)
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
      entry.Import(buf);

    return true;
  }

  /**
   * Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool Export(CFIFO& buf) const
  {
    // Export header.
    const auto* const header_bytes = reinterpret_cast<const std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Export data.
    for (auto& entry : entries_)
      entry.Export(buf);

    return true;
  }
};

} // namespace atom

/// Calibration map.
class CalibrationMap final {
public:
  /// The default constructor.
  CalibrationMap()
  {
    // Set data.
    atoms_.reserve(9);
    atoms_.emplace_back(atom::Calibration::Type::V_In1, 22);
    atoms_.emplace_back(atom::Calibration::Type::V_In2, 22);
    atoms_.emplace_back(atom::Calibration::Type::V_In3, 22);
    atoms_.emplace_back(atom::Calibration::Type::V_In4, 22);
    atoms_.emplace_back(atom::Calibration::Type::V_supply, 1);
    atoms_.emplace_back(atom::Calibration::Type::C_In1, 22);
    atoms_.emplace_back(atom::Calibration::Type::C_In2, 22);
    atoms_.emplace_back(atom::Calibration::Type::C_In3, 22);
    atoms_.emplace_back(atom::Calibration::Type::C_In4, 22);

    // Set header.
    header_.cversion = 0x01;
    header_.timestamp = 0; //???
    header_.numcatoms = static_cast<std::uint16_t>(atoms_.size());
    header_.callen = sizeof(Header);
    for (auto& atom : atoms_)
      header_.callen += atom.GetSizeInBytes();
  }

  /// @returns Caliration atom of the given `type`.
  const atom::Calibration& GetAtom(const atom::Calibration::Type type) const noexcept
  {
    return atoms_[static_cast<std::uint16_t>(type) - 1];
  }

  /// @overload
  atom::Calibration& GetAtom(const atom::Calibration::Type type) noexcept
  {
    return const_cast<atom::Calibration&>(static_cast<const CalibrationMap*>(this)->GetAtom(type));
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
  atom::Type type_{atom::Type::Custom};
  static constexpr int index_{3};  // FIXME ? (should be 2?)

  /**
   * @brief Imports data fields from an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool Import(CFIFO& buf)
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
      atom.Import(buf);

    return true;
  }

  /**
   * Stores data fields to an ATOM binary image.
   *
   * @param buf ATOM binary image.
   *
   * @returns `true` on success.
   */
  bool Export(CFIFO& buf) const
  {
    // Export header.
    auto* const header_bytes = reinterpret_cast<const std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Export data.
    for (auto& atom : atoms_)
      atom.Export(buf);

    return true;
  }
};

/// A manager class for working with HATs-EEPROM binary image
class Manager final {
public:
  /// Represents the result of the operation.
  enum class OpResult {
    OK,
    atom_not_found,
    atom_is_corrupted,
    storage_is_corrupted,
    storage_isnt_verified
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
   * Reads Atom's raw binary data.
   *
   * @param pos Atom position (zero-based).
   * @param[out] type Atom type.
   * @param[out] Output data buffer.
   *
   * @return Operation result.
   */
  OpResult ReadAtom(const unsigned pos, atom::Type& type, CFIFO& output) const
  {
    if (storage_state_ != OpResult::OK)
      return storage_state_;

    // Get the atom.
    AtomHeader* atom{};
    if (const auto r = FindAtomHeader(pos, &atom); r != OpResult::OK)
      return r;

    // Set helpers.
    const auto* const header_bytes = reinterpret_cast<const char*>(atom);
    const auto* const data_bytes = header_bytes + sizeof(AtomHeader); // atom->data_begin;
    const auto dlen = atom->dlen - 2; // real dlen without CRC

    // Check the CRC of the atom.
    {
      const std::uint16_t calc_crc{dmitigr::crc::crc16(header_bytes, dlen + sizeof(AtomHeader))};
      const auto* const crc = reinterpret_cast<const std::uint16_t*>(data_bytes + dlen);
      if (calc_crc != *crc)
        return OpResult::atom_is_corrupted;
    }

    // Fill the out variables.
    type = static_cast<atom::Type>(atom->type);
    for (int i{}; i < dlen; ++i)
      output << data_bytes[i];

    return OpResult::OK;
  }

  /**
   * Writes atom from the `input` buffer to the specified position.
   *
   * @param pos Atom position (zero-based).
   * @param type Atom type.
   * @param input Input data buffer.
   *
   * @return Operation result.
   */
  OpResult WriteAtom(const unsigned pos, const atom::Type type, const CFIFO& input)
  {
    if (storage_state_ != OpResult::OK)
      return storage_state_;

    const auto atom_count = GetAtomCount();
    if (pos > atom_count)
      return OpResult::atom_not_found;
    const bool is_adding{pos == atom_count};

    AtomHeader* atom{};
    if (const auto r = FindAtomHeader(pos, &atom);
      is_adding && r != OpResult::atom_not_found || r != OpResult::OK)
      return r;

    // FIXME: check the whole storage for corruption before continuing here.
    // assert: storage is not corrupted.

    const auto input_size = input.size();
    const int mem_adjust_size = is_adding ?
      input_size + sizeof(AtomHeader) + 2 : input_size - atom->dlen + 2;
    if (is_adding)
      AdjustMemBuf(reinterpret_cast<const char*>(atom), mem_adjust_size);
    else
      AdjustMemBuf(reinterpret_cast<const char*>(atom) + sizeof(AtomHeader), mem_adjust_size); // keep header

    // AdjustMemBuf() reallocates memory and invalidates `atom`! Update it.
    FindAtomHeader(pos, &atom);

    // Emplace the atom to the reserved by AdjustMemBuf() space.
    atom->type = static_cast<std::uint16_t>(type);
    atom->count = pos;
    atom->dlen = input_size + 2;
    auto* const atom_bytes = reinterpret_cast<char*>(atom);
    auto* const atom_data_bytes = atom_bytes + sizeof(AtomHeader);
    for (std::size_t i{}; i < input_size; ++i)
      atom_data_bytes[i] = input[i];

    // Set CRC stamp of the atom.
    *reinterpret_cast<std::uint16_t*>(atom_data_bytes + input_size) =
      dmitigr::crc::crc16(atom_bytes, input_size + sizeof(AtomHeader));

    // Update the EEPROM header if needed.
    if (is_adding) {
      auto* const header = reinterpret_cast<EepromHeader*>(GetMemBuf());
      header->eeplen += mem_adjust_size;
      header->numatoms = atom_count + 1;
    }

    return OpResult::OK;
  }

  /// Sets the EEPROM image buffer.
  void SetBuf(std::shared_ptr<CFIFO> fifo_buf)
  {
    fifo_buf_ = std::move(fifo_buf);
  }

  /// @returns EEPROM image buffer.
  const std::shared_ptr<CFIFO>& GetBuf() const noexcept
  {
    return fifo_buf_;
  }

  /// @returns Total atom count.
  std::uint16_t GetAtomCount() const noexcept
  {
    return reinterpret_cast<const EepromHeader*>(GetMemBuf())->numatoms;
  }

  /**
   * Checks the image data integrity.
   *
   * The method must be called before performing any operations on the binary
   * image. It checks all headers and atoms validity and sets `storage_state_`
   * to `OpResult::OK` on success. If the image is empty then Reset() must be
   * called instead.
   *
   * @returns `OpResult::OK` on success.
   */
  OpResult Verify()
  {
    return storage_state_ = VerifyStorage();
  }

  /**
   * Resets all the image data to the default state (zero atom count). Must
   * be called when start working on empty image.
   */
  void Reset()
  {
    fifo_buf_->resize(sizeof(EepromHeader));
    storage_state_ = ResetStorage();
  }

  /**
   * Imports the atom of given type from the image.
   *
   * @returns `OpResult::OK` on success.
   */
  template <typename A>
  OpResult Get(A& atom) const
  {
    atom::Type type;
    CFIFO buf;
    const auto r = ReadAtom(atom.index_, type, buf);
    return (r == OpResult::OK) && (atom.type_ != type || !atom.Import(buf)) ?
      OpResult::atom_is_corrupted : r;
  }

  /**
   * Stores the atom of given type to the image.
   *
   * @returns `OpResult::OK` on success.
   */
  template <typename A>
  OpResult Put(const A& atom)
  {
    if (storage_state_ != OpResult::OK)
      return storage_state_;
    CFIFO buf;
    atom.Export(buf);
    return WriteAtom(atom.index_, atom.type_, buf);
  }

private:
  struct AtomHeader final {
    std::uint16_t type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
    // char data_begin;
  };

  static constexpr std::uint32_t signature{0x69502d52};
  static constexpr std::uint8_t version{1};
  OpResult storage_state_{OpResult::storage_isnt_verified};
  std::shared_ptr<CFIFO> fifo_buf_;

  /// @name Memory control
  /// @{

  /// @returns Raw pointer to the EEPROM image buffer.
  char* GetMemBuf() const noexcept
  {
    return fifo_buf_->data();
  }

  /// @returns The size of the EEPROM image buffer.
  std::size_t GetMemBufSize() const noexcept
  {
    return fifo_buf_->size();
  }

  /// Reallocates EEPROM image buffer according to the given `adjustment`.
  void AdjustMemBuf(const char* const offset, const int adjustment)
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
   * @returns `OpResult::OK` on success.
   */
  OpResult FindAtomHeader(unsigned pos, AtomHeader** const header) const
  {
    OpResult result{OpResult::OK};
    auto* const mem_buf = GetMemBuf();
    auto* const mem_buf_end = mem_buf + GetMemBufSize();
    const auto* const eeprom_header = reinterpret_cast<const EepromHeader*>(mem_buf);

    // Check `pos` and correct if needed.
    if (pos >= eeprom_header->numatoms) {
      pos = eeprom_header->numatoms;
      result = OpResult::atom_not_found;
    }

    // Find atom.
    auto* atom = mem_buf + sizeof(EepromHeader);
    for (unsigned i{}; i < pos; ++i) {
      atom += sizeof(AtomHeader) + reinterpret_cast<const AtomHeader*>(atom)->dlen;
      if (atom > mem_buf_end)
        return OpResult::storage_is_corrupted;
    }
    *header = reinterpret_cast<AtomHeader*>(atom);

    return result;
  }

  /**
   * Checks the atom.
   *
   * @returns `OpResult::OK` on success.
   */
  OpResult VerifyAtom(const AtomHeader* const atom) const
  {
    // Check the CRC.
    const auto dlen = atom->dlen - 2; // dlen without CRC
    const auto* const atom_offset = reinterpret_cast<const char*>(atom);
    const auto* const data_offset = atom_offset + sizeof(AtomHeader);
    const auto* const crc_offset = data_offset + dlen;

    const auto crc = *reinterpret_cast<const std::uint16_t*>(crc_offset);
    const auto calc_crc = dmitigr::crc::crc16(atom_offset, dlen + sizeof(AtomHeader));
    if (crc != calc_crc)
      return OpResult::atom_is_corrupted;

    return OpResult::OK;
  }

  /// @}

  /// @name Storage control
  /// @{

  /**
   * Verifies EEPROM buffer.
   *
   * @return `OpResult::OK` on success.
   */
  OpResult VerifyStorage() const
  {
    const auto* const mem_buf = GetMemBuf();
    const auto mem_buf_size = GetMemBufSize();
    if (mem_buf_size < sizeof(EepromHeader))
      return OpResult::storage_is_corrupted;

    const auto* const header = reinterpret_cast<const EepromHeader*>(mem_buf);
    const auto* const mem_buf_end = mem_buf + mem_buf_size;

    if (header->signature != signature || header->ver != version
      || header->res || header->eeplen > mem_buf_size)
      return OpResult::storage_is_corrupted;

    // Verify all the atoms.
    const char* atom = mem_buf + sizeof(EepromHeader);
    for (std::uint16_t i{}; i < header->numatoms; ++i) {
      const auto* const atom_hdr = reinterpret_cast<const AtomHeader*>(atom);
      if (const auto r = VerifyAtom(atom_hdr); r != OpResult::OK)
        return r;
      atom += sizeof(AtomHeader) + atom_hdr->dlen;
      if (atom > mem_buf_end)
        return OpResult::storage_is_corrupted;
    }
    return OpResult::OK;
  }

  /// Invalidates the EEPROM buffer.
  OpResult ResetStorage()
  {
    char* const mem_buf = GetMemBuf();
    const std::size_t mem_buf_size = GetMemBufSize();

    if (mem_buf_size < sizeof(EepromHeader))
      return OpResult::storage_is_corrupted;

    auto* const header = reinterpret_cast<EepromHeader*>(mem_buf);
    header->signature = signature;
    header->ver = version;
    header->res = 0;
    header->numatoms = 0;
    header->eeplen = sizeof(EepromHeader);
    return OpResult::OK;
  }

  /// @}
};

} // namespace panda::timeswipe::hat

/// FIXME: remove after placing the entire code base in the namespace
using namespace panda::timeswipe;

#endif  // PANDA_TIMESWIPE_COMMON_HAT_HPP
