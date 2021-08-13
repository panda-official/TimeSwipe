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

/// Atom Type.
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
  explicit Stub(const int nIndex) noexcept
    : index_{nIndex}
  {}

private:
  friend hat::Manager;

  static constexpr Type type_{Type::Custom};
  int index_{};

  /*!
   * \brief Imports data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Import(CFIFO& buf) noexcept
  {
    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Export(CFIFO& buf) const noexcept
  {
    return true;
  }
};

/// Vendor info atom.
class VendorInfo final {
public:
  using Uuid = std::array<std::uint32_t, 4>;

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

  const Uuid& GetUuid() const noexcept
  {
    return uuid_;
  }

  std::uint16_t GetPid() const noexcept
  {
    return pid_;
  }

  std::uint16_t GetPver() const noexcept
  {
    return pver_;
  }

  const std::string& GetVstr() const noexcept
  {
    return vstr_;
  }

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

  /*!
   * \brief Imports data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
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

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
  bool Export(CFIFO& buf)
  {
    // Export uuid_.
    {
      const auto* const uuid_bytes = reinterpret_cast<std::uint8_t*>(uuid_.data());
      for (std::size_t i{}; i < sizeof(*uuid_.data()) * uuid_.size(); ++i)
        buf << uuid_bytes[i];
    }

    // Export pid_.
    {
      const Character b0{reinterpret_cast<std::uint8_t*>(&pid_)[0]},
        b1{reinterpret_cast<std::uint8_t*>(&pid_)[1]};
      buf << b0 << b1;
    }

    // Export pver_.
    {
      const Character b0{reinterpret_cast<std::uint8_t*>(&pver_)[0]},
        b1{reinterpret_cast<std::uint8_t*>(&pver_)[1]};
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

  /*!
   * \brief Imports data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
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

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Export(CFIFO &buf)
  {
    const auto* const this_bytes = reinterpret_cast<std::uint8_t*>(this);
    for (std::size_t i{}; i < sizeof(*this); ++i)
      buf << this_bytes[i];
    return true;
  }
};

/// Calibration atom.
class Calibration final {
public:
  /// Calibration atom data.
  class Data final {
  public:
    Data() = default;

    Data(const float m, const std::uint16_t b) noexcept
      : m_{m}
      , b_{b}
    {}

    void SetM(const float m) noexcept
    {
      m_ = m;
    }

    void SetB(const std::uint16_t b) noexcept
    {
      b_ = b;
    }

    float GetM() const noexcept
    {
      return m_;
    }

    std::uint16_t GetB() const noexcept
    {
      return b_;
    }

    /*!
     * \brief Imports data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
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

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool Export(CFIFO& buf)
    {
      const auto* const this_bytes = reinterpret_cast<std::uint8_t*>(this);
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

  Calibration(const Type nType, const std::uint16_t nCount)
    : header_{nType, nCount, nCount * sizeof(Data)}
    , data_{nCount}
  {}

  constexpr std::size_t GetSizeInBytes() const noexcept
  {
    return header_.dlen + sizeof(Header);
  }

  const std::vector<Data>& GetDataVector() const noexcept
  {
    return data_;
  }

  const Data& GetData(const std::size_t index, std::string& err) const
  {
    if (!(index < data_.size()))
      err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomDataIndex);
    return data_[index];
  }

  void SetData(const std::size_t index, const Data& value, std::string& err)
  {
    if (!(index < data_.size()))
      err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomDataIndex);
    data_[index] = value;
  }

private:
  friend hat::CalibrationMap;

  struct Header final {
    Type type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
    // data follows next
  } header_;
  std::vector<Data> data_;

  /*!
   * \brief Imports data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
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
    for (auto& data : data_)
      data.Import(buf);

    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Export(CFIFO& buf)
  {
    // Export header.
    const auto* const header_bytes = reinterpret_cast<std::uint8_t*>(&header_);
    for (std::size_t i{}; i < sizeof(header_); ++i)
      buf << header_bytes[i];

    // Export data.
    for (auto& data : data_)
      data.Export(buf);

    return true;
  }
};

} // namespace atom

class CalibrationMap final {
public:
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

  const atom::Calibration& GetAtom(const atom::Calibration::Type type) const noexcept
  {
    return atoms_[static_cast<std::uint16_t>(type) - 1];
  }

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

  /*!
   * \brief Imports data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
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


  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Export(CFIFO& buf)
  {
    // Export header.
    auto* const header_bytes = reinterpret_cast<std::uint8_t*>(&header_);
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
  enum class OpResult {
    OK,
    atom_not_found,
    atom_is_corrupted,
    storage_is_corrupted,
    storage_isnt_verified
  };

  /*!
   * \brief A class constructor
   * \param fifo_buf a buffer containing EEPROM binary image
   */
  explicit Manager(std::shared_ptr<CFIFO> fifo_buf = {})
    : fifo_buf_{std::move(fifo_buf)}
  {}

  /*!
   * \brief ReadAtom reads Atom's raw binary data
   * \param nAtom absolute address of the Atom
   * \param nAtomType atom type to be read out
   * \param rbuf data receive bufer
   * \return read operation result (OK or an error)
   */
  OpResult ReadAtom(unsigned int nAtom, atom::Type& nAtomType, CFIFO& rbuf)
  {
    if (storage_state_ != OpResult::OK)
      return storage_state_;

    // Get the atom.
    AtomHeader* atom{};
    if (const auto r = FindAtomHeader(nAtom,
        GetMemBuf(), GetMemBufSize(), &atom); r != OpResult::OK)
      return r;

    // Set helpers.
    const auto* const header_bytes = reinterpret_cast<const char*>(atom);
    const auto* const data_bytes = header_bytes + sizeof(AtomHeader); // &atom->data_begin;
    const auto dlen = atom->dlen - 2; // real dlen without CRC

    // Check the CRC of the atom.
    {
      const std::uint16_t calc_crc{dmitigr::crc::crc16(header_bytes, dlen + sizeof(AtomHeader))};
      const auto* const crc = reinterpret_cast<const std::uint16_t*>(data_bytes + dlen);
      if (calc_crc != *crc)
        return OpResult::atom_is_corrupted;
    }

    // Fill the output variables
    nAtomType = static_cast<atom::Type>(atom->type);
    for (int i{}; i < dlen; ++i)
      rbuf << data_bytes[i];

    return OpResult::OK;
  }

  /*!
   * \brief WriteAtom writes Atom's raw binary data
   * \param nAtom absolute address of the Atom
   * \param nAtomType atom type to be written
   * \param wbuf data bufer
   * \return read operation result (OK or an error)
   */
  OpResult WriteAtom(unsigned int nAtom, atom::Type nAtomType, CFIFO& wbuf)
  {
    if (storage_state_ != OpResult::OK) return storage_state_;

    unsigned int nAtomsCount = GetAtomsCount();
    if (nAtom > nAtomsCount)
      return OpResult::atom_not_found;

    char* pMemBuf{GetMemBuf()};
    AtomHeader* pAtom{};
    const bool bAddingNew = nAtom == nAtomsCount;
    if (const auto r = FindAtomHeader(nAtom, pMemBuf, GetMemBufSize(), &pAtom);
      bAddingNew && r != OpResult::atom_not_found || r != OpResult::OK)
      return r;

    //what can happaned here...if atom is not found this is ok, we can write a new one
    //the problem can be if whole storage is corrupted...
    //should we check it at the beginning?
    //lets assume the storage is OK: FindAtomHeader can check the header, not each atom...
    unsigned int req_size=wbuf.size();
    int nMemAdjustVal;
    if(bAddingNew) {
      nMemAdjustVal=req_size+sizeof(AtomHeader)+2;
      AdjustMemBuf((const char*)pAtom, nMemAdjustVal); //completely new
    } else {
      int dlen=pAtom->dlen-2;
      nMemAdjustVal=(int)(req_size - dlen);
      AdjustMemBuf((char*)pAtom+sizeof(AtomHeader), nMemAdjustVal); //keep header
    }


    //will it be the same address after realocation?!
    //if not have to repeat Finding procedure...
    if(GetMemBuf()!=pMemBuf) {
      pMemBuf=GetMemBuf();
      FindAtomHeader(nAtom, pMemBuf, GetMemBufSize(), &pAtom);
    }

    //after adjustion, fill the atom struct:
    pAtom->type=static_cast<uint16_t>(nAtomType);
    pAtom->count=nAtom; //also zero-based atom count
    pAtom->dlen=req_size+2;
    char *pData=(char*)pAtom+sizeof(AtomHeader);
    uint16_t *pCRC=(uint16_t*)(pData+req_size);
    for(unsigned int i=0; i<req_size; i++)
      pData[i]=wbuf[i];
    *pCRC = dmitigr::crc::crc16((char*)pAtom, req_size+sizeof(AtomHeader)); //set CRC stamp, atom is ready

    auto* const header = reinterpret_cast<EepromHeader*>(pMemBuf);
    header->eeplen += nMemAdjustVal;
    if(bAddingNew)
      //also setup the header with the new data:
      header->numatoms = nAtom + 1;

    return OpResult::OK;
  }

  void SetBuf(std::shared_ptr<CFIFO> fifo_buf)
  {
    fifo_buf_ = std::move(fifo_buf);
  }

  const std::shared_ptr<CFIFO>& GetBuf() const noexcept
  {
    return fifo_buf_;
  }

  /*!
   * \brief Returns the total atoms count
   * \return
   */
  unsigned GetAtomsCount() const noexcept
  {
    return reinterpret_cast<const EepromHeader*>(GetMemBuf())->numatoms;
  }

  /*!
   * \brief Checks the image data validity
   * \details The method must be called before performing any operations on the
   * binary image. It checks all headers and atoms validity and sets
   * `storage_state_` to `OpResult::OK` on success. If you are working on empty
   * image Reset() must be called instead.
   * \return operation result: OK on success
   */
  OpResult Verify()
  {
    return storage_state_ = VerifyStorage(GetMemBuf(), GetMemBufSize());
  }

  /*!
   * \brief Resets all image data to a default state(atoms count=0). Must be called when start working on empty image
   */
  void Reset()
  {
    SetMemBufSize(sizeof(EepromHeader));
    storage_state_=ResetStorage(GetMemBuf(), GetMemBufSize());
  }

  /*!
   * \brief Imports the atom of given type from the image
   * \return operation result: OK on success
   */
  template <typename A>
  OpResult Import(A& atom)
  {
    atom::Type type;
    CFIFO buf;
    const auto r = ReadAtom(atom.index_, type, buf);
    return (r == OpResult::OK) && (atom.type_ != type || !atom.Import(buf)) ?
      OpResult::atom_is_corrupted : r;
  }

  /*!
   * \brief Stores the atom of given type to the image
   * \return operation result: OK on success
   */
  template <typename A>
  OpResult Export(A& atom)
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

  const char* GetMemBuf() const noexcept
  {
    return fifo_buf_->data();
  }

  char* GetMemBuf() noexcept
  {
    return fifo_buf_->data();
  }

  std::size_t GetMemBufSize() const noexcept
  {
    return fifo_buf_->size();
  }

  void SetMemBufSize(const std::size_t size)
  {
    fifo_buf_->resize(size);
  }

  void AdjustMemBuf(const char* const pStart, const int nAdjustVal)
  {
    if (!nAdjustVal)
      return;

    int req_ind=pStart-fifo_buf_->data();
    int size=GetMemBufSize();
    if (nAdjustVal>0)
      fifo_buf_->insert(req_ind, nAdjustVal, 0);
    else
      fifo_buf_->erase(req_ind, -nAdjustVal);
  }

  /// @}

  /// @name Atom stuff
  /// @{

  static OpResult FindAtomHeader(unsigned nAtom, char* const pMemBuf,
    const std::size_t MemBufSize, AtomHeader** pHeaderBegin)
  {
    const auto* const header = reinterpret_cast<const EepromHeader*>(pMemBuf);
    char* mem_buf_end = pMemBuf + MemBufSize;

    OpResult rv{OpResult::OK};

    // Check if nAtom fits the boundares.
    if (nAtom >= header->numatoms) {
      nAtom = header->numatoms;
      rv = OpResult::atom_not_found;
    }

    char* pAtomPtr = pMemBuf + sizeof(EepromHeader);
    for (unsigned i{}; i < nAtom; ++i) {
      pAtomPtr += sizeof(AtomHeader) + reinterpret_cast<AtomHeader*>(pAtomPtr)->dlen;
      if (pAtomPtr > mem_buf_end)
        return OpResult::storage_is_corrupted;
    }

    // Always out the pointer to the next atom or at least where it should be.
    *pHeaderBegin = reinterpret_cast<AtomHeader*>(pAtomPtr);
    return rv;
  }

  OpResult VerifyAtom(const AtomHeader* const pAtom)
  {
    //check the atom CRC:
    const auto dlen = pAtom->dlen - 2; // real dlen without CRC
    const auto* const pAtomOffset = reinterpret_cast<const char*>(pAtom);
    const auto* const pDataOffset = pAtomOffset + sizeof(AtomHeader);
    const auto* const pCrcOffset = pDataOffset + dlen;

    const auto crc = *reinterpret_cast<const std::uint16_t*>(pCrcOffset);
    const auto calc_crc = dmitigr::crc::crc16(pAtomOffset, dlen + sizeof(AtomHeader));
    if (crc != calc_crc)
      return OpResult::atom_is_corrupted;

    return OpResult::OK;
  }

  /// @}

  /// @name Storage control
  /// @{

  OpResult VerifyStorage(const char* pMemBuf, const std::size_t MemBufSize)
  {
    if(MemBufSize < sizeof(EepromHeader))
      return OpResult::storage_is_corrupted;

    const auto* const header = reinterpret_cast<const EepromHeader*>(pMemBuf);
    const char* const pMemLimit = pMemBuf + MemBufSize;

    if (header->signature != signature || header->ver != version
      || header->res || header->eeplen > MemBufSize)
      return OpResult::storage_is_corrupted;

    // Verify all the atoms.
    const std::uint16_t nAtoms{header->numatoms};
    const char* pAtomPtr = pMemBuf + sizeof(EepromHeader);
    for(std::uint16_t i{}; i < nAtoms; ++i) {
      const auto* const atom_hdr = reinterpret_cast<const AtomHeader*>(pAtomPtr);
      const OpResult res{VerifyAtom(atom_hdr)};
      if (res != OpResult::OK)
        return res;
      pAtomPtr += sizeof(AtomHeader) + atom_hdr->dlen;
      if (pAtomPtr > pMemLimit)
        return OpResult::storage_is_corrupted;
    }
    return OpResult::OK;
  }

  OpResult ResetStorage(char* pMemBuf, const std::size_t MemBufSize)
  {
    if (MemBufSize < sizeof(EepromHeader))
      return OpResult::storage_is_corrupted;

    auto* const header = reinterpret_cast<EepromHeader*>(pMemBuf);
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
