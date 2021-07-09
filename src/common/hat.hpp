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
struct Header final {
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
    : m_index{nIndex}
  {}

private:
  friend hat::Manager;

  static constexpr Type m_type{Type::Custom};
  int m_index{};

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Load(CFIFO& buf) noexcept
  {
    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Save(CFIFO& buf) const noexcept
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
    : m_uuid{uuid}
    , m_pid{pid}
    , m_pver{pver}
    , m_vstr{std::move(vstr)}
    , m_pstr{std::move(pstr)}
  {}

  const Uuid& GetUuid() const noexcept
  {
    return m_uuid;
  }

  std::uint16_t GetPid() const noexcept
  {
    return m_pid;
  }

  std::uint16_t GetPver() const noexcept
  {
    return m_pver;
  }

  const std::string& GetVstr() const noexcept
  {
    return m_vstr;
  }

  const std::string& GetPstr() const noexcept
  {
    return m_pstr;
  }

private:
  friend hat::Manager;

  static constexpr Type m_type{Type::VendorInfo};
  static constexpr int m_index{};
  Uuid m_uuid{};
  std::uint16_t m_pid{};
  std::uint16_t m_pver{};
  std::string m_vstr;
  std::string m_pstr;

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Load(CFIFO& buf)
  {
    if (buf.in_avail() < 22) return false;

    Character ch;
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(m_uuid.data());
    for (int i{}; i < sizeof(*m_uuid.data()) * m_uuid.size(); ++i) {
      buf >> ch;
      pBuf[i] = static_cast<std::uint8_t>(ch);
    }

    Character b0, b1;
    buf >> b0 >> b1;
    reinterpret_cast<std::uint8_t*>(&m_pid)[0] = b0;
    reinterpret_cast<std::uint8_t*>(&m_pid)[1] = b1;

    buf >> b0 >> b1;
    reinterpret_cast<std::uint8_t*>(&m_pver)[0] = b0;
    reinterpret_cast<std::uint8_t*>(&m_pver)[1] = b1;

    int vslen, pslen;
    buf >> vslen >> pslen;
    m_vstr.resize(vslen);
    m_pstr.resize(pslen);
    for (int i{}; i < vslen; ++i) {
      buf >> ch;
      m_vstr[i] = ch;
    }
    for (int i{}; i < pslen; ++i) {
      buf >> ch;
      m_pstr[i] = ch;
    }

    return true;
  }

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
  bool Save(CFIFO& buf)
  {
    const int vslen = m_vstr.size();
    const int pslen = m_pstr.size();

    const auto* const pBuf = reinterpret_cast<std::uint8_t*>(m_uuid.data());
    for (std::size_t i{}; i < sizeof(*m_uuid.data()) * m_uuid.size(); ++i)
      buf << pBuf[i];

    {
      const Character b0{reinterpret_cast<std::uint8_t*>(&m_pid)[0]},
        b1{reinterpret_cast<std::uint8_t*>(&m_pid)[1]};
      buf << b0 << b1;
    }

    {
      const Character b0{reinterpret_cast<std::uint8_t*>(&m_pver)[0]},
        b1{reinterpret_cast<std::uint8_t*>(&m_pver)[1]};
      buf << b0 << b1;
    }

    buf << vslen << pslen;
    for (int i{}; i < vslen; ++i)
      buf << m_vstr[i];
    for (int i{}; i < pslen; ++i)
      buf << m_pstr[i];

    return true;
  }
};

/// GPIO map atom
class GpioMap final {
public:
  GpioMap() = default;

private:
  friend hat::Manager;

  struct {
    std::uint8_t drive      :4;
    std::uint8_t slew       :2;
    std::uint8_t hysteresis :2;
  } m_bank_drive{};

  struct {
    std::uint8_t back_power :1;
    std::uint8_t reserved   :7;
  } m_power{};

  struct {
    std::uint8_t func_sel :3;
    std::uint8_t reserved :2;
    std::uint8_t pulltype :2;
    std::uint8_t is_used  :1;
  } m_gpio[28]{};

  static constexpr auto kThisSize = sizeof(m_bank_drive) + sizeof(m_power) + sizeof(m_gpio);
  static constexpr Type m_type{Type::GpioMap};
  static constexpr int m_index{1}; // FIXME ? (should be 2?)

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Load(CFIFO& buf)
  {
    if (buf.in_avail() < kThisSize) return false;

    Character ch;
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(this);
    for (std::size_t i{}; i < kThisSize; ++i) {
      buf >> ch;
      pBuf[i] = static_cast<std::uint8_t>(ch);
    }
    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Save(CFIFO &buf)
  {
    const auto* const pBuf = reinterpret_cast<std::uint8_t*>(this);
    for (std::size_t i{}; i < kThisSize; ++i) buf << pBuf[i];
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
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool Load(CFIFO& buf)
    {
      Character ch;
      auto* const pBuf = reinterpret_cast<std::uint8_t*>(this);
      for (std::size_t i{}; i < kThisSize; ++i) {
        buf >> ch;
        pBuf[i] = static_cast<std::uint8_t>(ch);
      }
      return true;
    }

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool Save(CFIFO& buf)
    {
      const auto* const pBuf = reinterpret_cast<std::uint8_t*>(this);
      for (std::size_t i{}; i < kThisSize; ++i) buf << pBuf[i];
      return true;
    }

  private:
    friend Calibration;

    float m_{1};
    std::uint16_t b_{};

    static constexpr auto kThisSize = sizeof(m_) + sizeof(b_);
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
    if (!ToLiteral(result)) err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomType);
    return result;
  }

  Calibration(const Type nType, const std::uint16_t nCount)
    : m_header{nType, nCount, nCount * sizeof(Data::kThisSize)}
    , m_data{nCount}
  {}

  constexpr std::size_t GetSizeInBytes() const noexcept
  {
    return m_header.dlen + sizeof(Header);
  }

  const std::vector<Data>& GetDataVector() const noexcept
  {
    return m_data;
  }

  const Data& GetData(const std::size_t index, std::string& err) const
  {
    if (!(index < m_data.size())) err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomDataIndex);
    return m_data[index];
  }

  void SetData(const std::size_t index, const Data& value, std::string& err)
  {
    if (!(index < m_data.size())) err = timeswipe::ToLiteral(Errc::kInvalidCalibrationAtomDataIndex);
    m_data[index] = value;
  }

private:
  friend hat::CalibrationMap;

  struct Header final {
    Type type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
  } m_header;

  std::vector<Data> m_data;

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Load(CFIFO& buf)
  {
    // Load header.
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(&m_header);
    Character ch;
    for (std::size_t i{}; i < sizeof(m_header); ++i) {
      buf >> ch;
      pBuf[i] = static_cast<std::uint8_t>(ch);
    }

    // Load data.
    for (auto& data : m_data) data.Load(buf);

    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Save(CFIFO& buf)
  {
    // Save header.
    const auto* const pBuf = reinterpret_cast<std::uint8_t*>(&m_header);
    for (std::size_t i{}; i < sizeof(m_header); ++i) buf << pBuf[i];

    // Save data.
    for (auto& data : m_data) data.Save(buf);

    return true;
  }
};

} // namespace atom

class CalibrationMap final {
public:
  CalibrationMap()
  {
    // Set data.
    m_atoms.reserve(9);
    m_atoms.emplace_back(atom::Calibration::Type::V_In1, 22);
    m_atoms.emplace_back(atom::Calibration::Type::V_In2, 22);
    m_atoms.emplace_back(atom::Calibration::Type::V_In3, 22);
    m_atoms.emplace_back(atom::Calibration::Type::V_In4, 22);
    m_atoms.emplace_back(atom::Calibration::Type::V_supply, 1);
    m_atoms.emplace_back(atom::Calibration::Type::C_In1, 22);
    m_atoms.emplace_back(atom::Calibration::Type::C_In2, 22);
    m_atoms.emplace_back(atom::Calibration::Type::C_In3, 22);
    m_atoms.emplace_back(atom::Calibration::Type::C_In4, 22);

    // Set header.
    m_header.cversion = 0x01;
    m_header.timestamp = 0; //???
    m_header.numcatoms = static_cast<std::uint16_t>(m_atoms.size());
    m_header.callen = sizeof(Header);
    for (auto& atom : m_atoms) m_header.callen += atom.GetSizeInBytes();
  }

  const atom::Calibration& GetAtom(const atom::Calibration::Type type) const noexcept
  {
    return m_atoms[static_cast<std::uint16_t>(type) - 1];
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
  } __attribute__((packed)) m_header;

  std::vector<atom::Calibration> m_atoms;
  atom::Type m_type{atom::Type::Custom};
  int m_index{3};

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Load(CFIFO& buf)
  {
    // Load header.
    Character ch;
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(&m_header);
    for (std::size_t i{}; i < sizeof(m_header); ++i) {
      buf >> ch;
      pBuf[i] = static_cast<std::uint8_t>(ch);
    }

    // Load data.
    for (auto& atom : m_atoms) atom.Load(buf);

    return true;
  }


  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool Save(CFIFO& buf)
  {
    // Save header.
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(&m_header);
    for (std::size_t i{}; i < sizeof(m_header); ++i) buf << pBuf[i];

    // Save data.
    for (auto& atom : m_atoms) atom.Save(buf);

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
   * \param pFIFObuf a buffer containing EEPROM binary image
   */
  explicit Manager(std::shared_ptr<CFIFO> pFIFObuf = {})
    : m_pFIFObuf{std::move(pFIFObuf)}
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
    if (m_StorageState != OpResult::OK) return m_StorageState;

    AtomHeader* pAtom{};
    if (const auto r = FindAtomHeader(nAtom,
        GetMemBuf(), GetMemBufSize(), &pAtom); r != OpResult::OK)
      return r;

    //check the atom CRC:
    const unsigned int dlen=pAtom->dlen-2; //real dlen without CRC
    const char *pData=(const char*)pAtom + sizeof(AtomHeader); //&pAtom->data_begin;
    nAtomType=static_cast<atom::Type>(pAtom->type);

    std::uint16_t calc_crc{dmitigr::crc::crc16((char*)pAtom, dlen + sizeof(AtomHeader))};
    std::uint16_t *pCRC=(uint16_t*)(pData+dlen);
    if (calc_crc != *pCRC)
      return OpResult::atom_is_corrupted;

    //fill the output variables:
    for(int i=0; i<dlen; i++)
      rbuf<<pData[i];

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
    if (m_StorageState != OpResult::OK) return m_StorageState;

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

    auto* const header = reinterpret_cast<Header*>(pMemBuf);
    header->eeplen += nMemAdjustVal;
    if(bAddingNew)
      //also setup the header with the new data:
      header->numatoms = nAtom + 1;

    return OpResult::OK;
  }

  void SetBuf(std::shared_ptr<CFIFO> pBuf)
  {
    m_pFIFObuf = std::move(pBuf);
  }

  const std::shared_ptr<CFIFO>& GetBuf() const noexcept
  {
    return m_pFIFObuf;
  }

  /*!
   * \brief Returns the total atoms count
   * \return
   */
  unsigned GetAtomsCount() const noexcept
  {
    return reinterpret_cast<const Header*>(GetMemBuf())->numatoms;
  }

  /*!
   * \brief Checks the image data validity
   * \details The method must be called before performing any operations on the binary image
   * It checks all headers and atoms validity and sets m_StorageState to "OK" on success
   * If you are working on empty image Reset() must be called instead
   * \return operation result: OK on success
   */
  OpResult Verify()
  {
    return m_StorageState = VerifyStorage(GetMemBuf(), GetMemBufSize());
  }

  /*!
   * \brief Resets all image data to a default state(atoms count=0). Must be called when start working on empty image
   */
  void Reset()
  {
    SetMemBufSize(sizeof(Header));
    m_StorageState=ResetStorage(GetMemBuf(), GetMemBufSize());
  }

  /*!
   * \brief Loads the atom of given type from the image
   * \return operation result: OK on success
   */
  template <typename A>
  OpResult Load(A& atom)
  {
    CFIFO buf;
    atom::Type type;
    if (const auto r = ReadAtom(atom.m_index, type, buf);
      r == OpResult::OK && (atom.m_type != type || !atom.Load(buf)))
      return OpResult::atom_is_corrupted;
    else
      return r;
  }

  /*!
   * \brief Stores the atom of given type to the image
   * \return operation result: OK on success
   */
  template <typename A>
  OpResult Save(A& atom)
  {
    if (m_StorageState != OpResult::OK) return m_StorageState;
    CFIFO buf;
    atom.Save(buf);
    return WriteAtom(atom.m_index, atom.m_type, buf);
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
  OpResult m_StorageState{OpResult::storage_isnt_verified};
  std::shared_ptr<CFIFO> m_pFIFObuf;

  /// @name Memory control
  /// @{

  const char* GetMemBuf() const noexcept
  {
    return m_pFIFObuf->data();
  }

  char* GetMemBuf() noexcept
  {
    return m_pFIFObuf->data();
  }

  int GetMemBufSize() const noexcept
  {
    return m_pFIFObuf->size();
  }

  void SetMemBufSize(int size)
  {
    m_pFIFObuf->resize(size);
  }

  void AdjustMemBuf(const char *pStart, int nAdjustVal)
  {
    if(0==nAdjustVal)
      return;

    int req_ind=pStart-m_pFIFObuf->data();
    int size=GetMemBufSize();
    if(nAdjustVal>0)
      m_pFIFObuf->insert(req_ind, nAdjustVal, 0);
    else
      m_pFIFObuf->erase(req_ind, -nAdjustVal);
  }

  /// @}

  /// @name Atom stuff
  /// @{

  static OpResult FindAtomHeader(unsigned nAtom, char* const pMemBuf,
    const std::size_t MemBufSize, AtomHeader** pHeaderBegin)
  {
    const auto* const header = reinterpret_cast<const Header*>(pMemBuf);
    char* mem_buf_end = pMemBuf + MemBufSize;

    OpResult rv{OpResult::OK};

    // Check if nAtom fits the boundares.
    if(nAtom >= header->numatoms) {
      nAtom = header->numatoms;
      rv = OpResult::atom_not_found;
    }

    char* pAtomPtr = pMemBuf + sizeof(Header);
    for(unsigned int i{}; i < nAtom; ++i) {
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
    if(MemBufSize < sizeof(Header))
      return OpResult::storage_is_corrupted;

    const auto* const header = reinterpret_cast<const Header*>(pMemBuf);
    const char* const pMemLimit = pMemBuf + MemBufSize;

    if (header->signature != signature || header->ver != version
      || header->res || header->eeplen > MemBufSize)
      return OpResult::storage_is_corrupted;

    // Verify all the atoms.
    const std::uint16_t nAtoms{header->numatoms};
    const char* pAtomPtr = pMemBuf + sizeof(Header);
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
    if (MemBufSize < sizeof(Header))
      return OpResult::storage_is_corrupted;

    auto* const header = reinterpret_cast<Header*>(pMemBuf);
    header->signature = signature;
    header->ver = version;
    header->res = 0;
    header->numatoms = 0;
    header->eeplen = sizeof(Header);
    return OpResult::OK;
  }

  /// @}
};

} // namespace panda::timeswipe::hat

/// FIXME: remove after placing the entire code base in the namespace
using namespace panda::timeswipe;

#endif  // PANDA_TIMESWIPE_COMMON_HAT_HPP
