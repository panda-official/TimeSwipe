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

#ifndef PANDA_TIMESWIPE_COMMON_HATS_HPP
#define PANDA_TIMESWIPE_COMMON_HATS_HPP

#include "../3rdparty/dmitigr/crc.hpp"
#include "Serial.h"

#include <array>
#include <cstdint>
#include <cstring>

/// EEPROM header.
struct Header final {
  std::uint32_t signature{};
  std::uint8_t ver{};
  std::uint8_t res{};
  std::uint16_t numatoms{};
  std::uint32_t eeplen{};
};

/// Atom Type.
enum class AtomType : std::uint16_t {
  Invalid = 0x0000,
  VendorInfo = 0x0001,
  GpioMap = 0x0002,
  LinuxDeviceTreeBlob = 0x0003,
  Custom = 0x0004,
  Invalid2 = 0xFFFF
};

class HatAtomStub final {
public:
  explicit HatAtomStub(const int nIndex)
    : m_index{nIndex}
  {}

private:
  friend class HatsMemMan;

  AtomType m_type{AtomType::Custom};
  int m_index{};

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool load(CFIFO &buf){return true;}

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool store(CFIFO &buf){return true;}
};

/// A vendor info atom.
class HatAtomVendorInfo final {
public:
  HatAtomVendorInfo(const std::array<std::uint32_t, 4> uuid,
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

  const std::array<std::uint32_t, 4>& uuid() const noexcept
  {
    return m_uuid;
  }

  std::uint16_t pid() const noexcept
  {
    return m_pid;
  }

  std::uint16_t pver() const noexcept
  {
    return m_pver;
  }

  const std::string& vstr() const noexcept
  {
    return m_vstr;
  }

  const std::string& pstr() const noexcept
  {
    return m_pstr;
  }

private:
  friend class HatsMemMan;

  static constexpr AtomType m_type{AtomType::VendorInfo};
  static constexpr int m_index{};
  std::array<std::uint32_t, 4> m_uuid{};
  std::uint16_t m_pid{};
  std::uint16_t m_pver{};
  std::string m_vstr;
  std::string m_pstr;

  /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
  bool load(CFIFO& buf)
  {
    //special deserialization:
    int nDSize=buf.in_avail();
    if(nDSize<22)
      return false;
    //const char *pData=buf.data();

    typeSChar ch;
    /*  uint8_t   *pBuf=(uint8_t *)(m_uuid);
        for(int i=0; i<20; i++)
        {
        buf>>ch;
        pBuf[i]=(uint8_t)ch;
        }*/

    uint8_t   *pBuf=(uint8_t *)m_uuid.data();
    for(int i=0; i<16; i++)
      {
        buf>>ch;
        pBuf[i]=(uint8_t)ch;
      }
    typeSChar b0, b1;
    buf>>b0>>b1;
    *((uint8_t*)&m_pid)=b0; *( ((uint8_t*)&m_pid) +1 )=b1;
    buf>>b0>>b1;
    *((uint8_t*)&m_pver)=b0; *( ((uint8_t*)&m_pver) +1 )=b1;

    int vslen, pslen;
    buf>>vslen>>pslen;

    m_vstr.reserve(vslen);
    m_pstr.reserve(pslen);

    for(int i=0; i<vslen; i++)
      {
        buf>>ch;
        m_vstr+=ch;
      }
    for(int i=0; i<pslen; i++)
      {
        buf>>ch;
        m_pstr+=ch;
      }
    return true;

  }

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
  bool store(CFIFO& buf)
  {
    //special serialization:
    int vslen=m_vstr.length();
    int pslen=m_pstr.length();
    buf.reserve(22+vslen+pslen); //minimum size
    /* uint8_t   *pBuf=(uint8_t *)(m_uuid);
       for(int i=0; i<20; i++)
       {
       buf<<pBuf[i];
       }*/

    uint8_t   *pBuf=(uint8_t *)m_uuid.data();
    for(int i=0; i<16; i++)
      {
        buf<<pBuf[i];
      }
    buf<<*((uint8_t*)&m_pid)<<*( ((uint8_t*)&m_pid) +1 );
    buf<<*((uint8_t*)&m_pver)<<*( ((uint8_t*)&m_pver) +1 );


    buf<<vslen<<pslen;
    for(int i=0; i<vslen; i++)
      {
        buf<<m_vstr[i];
      }
    for(int i=0; i<pslen; i++)
      {
        buf<<m_pstr[i];
      }
    return true;
  }
};

/// GPIO map atom
class HatAtomGPIOmap final {
public:
  HatAtomGPIOmap()
  {
    reset();
  }

private:
  friend class HatsMemMan;

  struct bank_drive final {
    std::uint8_t drive      :4;
    std::uint8_t slew       :2;
    std::uint8_t hysteresis :2;
  } m_bank_drive;

  struct power final {
    std::uint8_t back_power :1;
    std::uint8_t reserved   :7;
  } m_power;

  struct GPIO final {
    std::uint8_t func_sel :3;
    std::uint8_t reserved :2;
    std::uint8_t pulltype :2;
    std::uint8_t is_used  :1;
  } m_GPIO[28];

  AtomType m_type{AtomType::GpioMap};
  int m_index{1};

    /*!
     * \brief Fills data fields with zeros
     */
  void reset()
  {
    std::memset(&m_bank_drive, 0, 30);
  }

    /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
  bool load(CFIFO &buf)
  {
    //special deserialization:
    int nDSize=buf.in_avail();
    if (nDSize < 30)
      return false;

    typeSChar ch;
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(&m_bank_drive);
    for (int i{}; i < 30; ++i) {
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
  bool store(CFIFO &buf)
  {
    //special serialization:
    buf.reserve(30);

    uint8_t   *pBuf=(uint8_t *)(&m_bank_drive);
    for(int i=0; i<30; i++)
      {
        buf<<pBuf[i];
      }
    return true;
  }
};

class CalAtomPair final {
private:
  float m_{1};
  std::uint16_t b_{};

public:
  CalAtomPair() = default;

  CalAtomPair(const float m, const std::uint16_t b)
    : m_{m}
    , b_{b}
  {}

  void set(const float m, const std::uint16_t b) noexcept
  {
    m_ = m;
    b_ = b;
  }

  void set_m(const float m) noexcept
  {
    m_ = m;
  }

  void set_b(const std::uint16_t b) noexcept
  {
    b_ = b;
  }

  float m() const noexcept
  {
    return m_;
  }

  std::uint16_t b() const noexcept
  {
    return b_;
  }

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool load(CFIFO &buf){

    typeSChar ch;
    uint8_t   *pBuf=(uint8_t *)&m_;
    for(int i=0; i<6; i++)
      {
        buf>>ch;
        pBuf[i]=(uint8_t)ch;
      }
    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool store(CFIFO &buf){

    uint8_t   *pBuf=(uint8_t *)&m_;
    for(int i=0; i<6; i++)
      {
        buf<<pBuf[i];
      }
    return true;
  }
};

struct CalAtom final {
  enum class Type : std::uint16_t {
    Header   = 0x0000,
    V_In1    = 0x0001,
    V_In2    = 0x0002,
    V_In3    = 0x0003,
    V_In4    = 0x0004,
    V_supply = 0x0005,
    C_In1    = 0x0006,
    C_In2    = 0x0007,
    C_In3    = 0x0008,
    C_In4    = 0x0009,
    Ana_Out  = 0x000A,
    Invalid  = 0xFFFF
  };

  struct header final {
    Type type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
  } m_header;

  std::vector<CalAtomPair> m_data;

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool load(CFIFO &buf)
  {
    //load the header:
    header theader;

    typeSChar ch;
    uint8_t   *pBuf=(uint8_t *)&theader;
    for(size_t i=0; i<sizeof(header); i++) {
      buf >> ch;
      pBuf[i] = (uint8_t)ch;
    }

    //load the rest:
    for(auto &pair : m_data)
      pair.load(buf);
    return true;
  }

  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool store(CFIFO &buf)
  {
    //store the header:
    uint8_t   *pBuf=(uint8_t *)&m_header;
    for(std::size_t i{}; i < sizeof(header); ++i)
      buf << pBuf[i];

    //save the rest:
    for(auto &pair : m_data)
      pair.store(buf);
    return true;
  }

  void Setup(const CalAtom::Type nType, const std::uint16_t nCount)
  {
    m_header.type = nType;
    m_header.count = nCount;
    m_header.dlen = nCount * sizeof(CalAtomPair);
    m_data.resize(nCount);
  }

  std::size_t GetSizeInBytes()
  {
    return m_header.dlen + sizeof(header);
  }
};

class HatAtomCalibration final {
public:
  struct Header final {
    std::uint8_t cversion{};
    std::uint64_t timestamp{};
    std::uint16_t numcatoms{};
    // Total size in bytes of all calibration data (including this header).
    std::uint32_t callen{};
  } __attribute__((packed)) m_header;

  HatAtomCalibration()
  {
    reset();
  }

  const CalAtom& refAtom(const CalAtom::Type type) const noexcept
  {
    return m_atoms[static_cast<std::uint16_t>(type) - 1];
  }

  CalAtom& refAtom(const CalAtom::Type type) noexcept
  {
    return const_cast<CalAtom&>(static_cast<const HatAtomCalibration*>(this)->refAtom(type));
  }

  void FillHeader()
  {
    m_header.cversion = 1;
    m_header.timestamp = 0; //???
    m_header.numcatoms = static_cast<std::uint16_t>(m_atoms.size());

    std::size_t sztotal{sizeof(Header)};
    for (auto &atom : m_atoms)
      sztotal += atom.GetSizeInBytes();
    m_header.callen = sztotal;
  }

  bool CheckAtomIndex(const CalAtom::Type type, std::string &strError,
    bool bCheckExistance = true) const noexcept
  {
    if (type == CalAtom::Type::Header || type == CalAtom::Type::Invalid) {
      strError = "invalid atom type";
      return false;
    }

    if (bCheckExistance) {
      if (static_cast<std::uint16_t>(type) > m_atoms.size()) {
        strError = "atom doesn't exist";
        return false;
      }
    }
    return true;
  }

  bool CheckPairIndex(const CalAtom::Type type, const std::size_t nPairIndex, std::string &strError) const noexcept
  {
    if (!CheckAtomIndex(type, strError))
      return false;

    if (nPairIndex >= refAtom(type).m_data.size()) {
      strError="wrong pair index";
      return false;
    }
    return true;
  }

  bool GetPairsCount(const CalAtom::Type type, std::size_t& nCount, std::string& strError) const noexcept
  {
    if (!CheckAtomIndex(type, strError))
      return false;

    nCount = refAtom(type).m_data.size();
    return true;
  }

  bool SetCalPair(const CalAtom::Type type, const std::size_t nPairIndex, const CalAtomPair& Pair, std::string& strError)
  {
    if (!CheckPairIndex(type, nPairIndex, strError))
      return false;

    refAtom(type).m_data[nPairIndex] = Pair;
    return true;
  }

  bool GetCalPair(const CalAtom::Type type, const std::size_t nPairIndex, CalAtomPair& Pair, std::string& strError)
  {
    if (!CheckPairIndex(type, nPairIndex, strError))
      return false;

    Pair = refAtom(type).m_data[nPairIndex];
    return true;
  }

private:
  friend class HatsMemMan;

  std::vector<CalAtom> m_atoms;
  AtomType m_type{AtomType::Custom};
  int m_index{3};

  /// Fills data fields with default data
  void reset()
  {
    m_atoms.resize(9);
    m_atoms[0].Setup(CalAtom::Type::V_In1, 22);
    m_atoms[1].Setup(CalAtom::Type::V_In2, 22);
    m_atoms[2].Setup(CalAtom::Type::V_In3, 22);
    m_atoms[3].Setup(CalAtom::Type::V_In4, 22);
    m_atoms[4].Setup(CalAtom::Type::V_supply, 1);
    m_atoms[5].Setup(CalAtom::Type::C_In1, 22);
    m_atoms[6].Setup(CalAtom::Type::C_In2, 22);
    m_atoms[7].Setup(CalAtom::Type::C_In3, 22);
    m_atoms[8].Setup(CalAtom::Type::C_In4, 22);
    FillHeader();
  }

  /*!
   * \brief Loads data fields from an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool load(CFIFO &buf)
  {
    //load the header:
    Header theader;

    typeSChar ch;
    auto* const pBuf = reinterpret_cast<std::uint8_t*>(&theader);
    for (std::size_t i{}; i < sizeof(Header); ++i) {
      buf>>ch;
      pBuf[i]=(uint8_t)ch;
    }

    //atom's template rules at the moment:
    if (theader.callen != m_header.callen)
      return false;

    //load the rest:
    for(auto &atom : m_atoms)
      atom.load(buf);

    return true;
  }


  /*!
   * \brief Stores data fields to an ATOM binary image
   * \param buf ATOM binary image
   * \return true=successful, false=failure
   */
  bool store(CFIFO &buf)
  {
    //save the header:
    FillHeader();

    auto* const pBuf = reinterpret_cast<std::uint8_t*>(&m_header);
    for (std::size_t i{}; i < sizeof(Header); ++i)
      buf << pBuf[i];

    //save the rest:
    for(auto &atom : m_atoms)
      atom.store(buf);

    return true;
  }
};

/// A manager class for working with HATs-EEPROM binary image
class HatsMemMan {
public:
  /*!
   * \brief A class constructor
   * \param pFIFObuf a buffer containing EEPROM binary image
   */
  explicit HatsMemMan(std::shared_ptr<CFIFO> pFIFObuf = {})
    : m_pFIFObuf{std::move(pFIFObuf)}
  {}

  enum op_result {
    OK,
    atom_not_found,
    atom_is_corrupted,
    storage_is_corrupted,
    storage_isnt_verified
  };

protected:
  /*!
   * \brief ReadAtom reads Atom's raw binary data
   * \param nAtom absolute address of the Atom
   * \param nAtomType atom type to be read out
   * \param rbuf data receive bufer
   * \return read operation result (OK or an error)
   */
  op_result ReadAtom(unsigned int nAtom, AtomType &nAtomType, CFIFO &rbuf)
  {
    if(OK!=m_StorageState)
      return m_StorageState;

    atom_header* pAtom{};
    op_result res=FindAtomHeader(nAtom, GetMemBuf(), GetMemBufSize(), &pAtom);
    if(op_result::OK!=res)
      return res;

    //check the atom CRC:
    const unsigned int dlen=pAtom->dlen-2; //real dlen without CRC
    const char *pData=(const char*)pAtom + sizeof(struct atom_header); //&pAtom->data_begin;
    nAtomType=static_cast<AtomType>(pAtom->type);

    std::uint16_t calc_crc{dmitigr::crc::crc16((char*)pAtom, dlen+sizeof(atom_header))};
    std::uint16_t *pCRC=(uint16_t*)(pData+dlen);
    if(calc_crc!=*pCRC)
      return atom_is_corrupted;

    //fill the output variables:
    for(int i=0; i<dlen; i++)
      rbuf<<pData[i];
    return op_result::OK;
  }

  /*!
   * \brief WriteAtom writes Atom's raw binary data
   * \param nAtom absolute address of the Atom
   * \param nAtomType atom type to be written
   * \param wbuf data bufer
   * \return read operation result (OK or an error)
   */
  op_result WriteAtom(unsigned int nAtom, AtomType nAtomType, CFIFO &wbuf)
  {
    if(OK!=m_StorageState)
      return m_StorageState;

    unsigned int nAtomsCount=GetAtomsCount();
    if(nAtom>nAtomsCount)
      return op_result::atom_not_found;

    bool bAddingNew=(nAtom==nAtomsCount);

    char* pMemBuf{GetMemBuf()};
    atom_header* pAtom{};
    op_result res=FindAtomHeader(nAtom, pMemBuf, GetMemBufSize(), &pAtom);
    if(bAddingNew) {
      if(op_result::atom_not_found!=res)
        return res;
    } else if(op_result::OK!=res)
      return res;


    //what can happaned here...if atom is not found this is ok, we can write a new one
    //the problem can be if whole storage is corrupted...
    //should we check it at the beginning?
    //lets assume the storage is OK: FindAtomHeader can check the header, not each atom...
    unsigned int req_size=wbuf.size();
    int nMemAdjustVal;
    if(bAddingNew) {
      nMemAdjustVal=req_size+sizeof(atom_header)+2;
      AdjustMemBuf((const char*)pAtom, nMemAdjustVal); //completely new
    } else {
      int dlen=pAtom->dlen-2;
      nMemAdjustVal=(int)(req_size - dlen);
      AdjustMemBuf((char*)pAtom+sizeof(struct atom_header), nMemAdjustVal); //keep header
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
    char *pData=(char*)pAtom+sizeof(struct atom_header);
    uint16_t *pCRC=(uint16_t*)(pData+req_size);
    for(unsigned int i=0; i<req_size; i++)
      pData[i]=wbuf[i];
    *pCRC = dmitigr::crc::crc16((char*)pAtom, req_size+sizeof(atom_header)); //set CRC stamp, atom is ready

    auto* const header = reinterpret_cast<Header*>(pMemBuf);
    header->eeplen += nMemAdjustVal;
    if(bAddingNew)
      //also setup the header with the new data:
      header->numatoms = nAtom + 1;

    return op_result::OK;
  }

public:

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
  op_result Verify()
  {
    op_result res=VerifyStorage(GetMemBuf(), GetMemBufSize());
    m_StorageState=res;
    return res;
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
  template <typename typeAtom>
  op_result Load(typeAtom &atom)
  {
    CFIFO buf;
    AtomType nAtomType;
    op_result rv=ReadAtom(atom.m_index, nAtomType, buf);
    if(op_result::OK!=rv)
      return rv;
    if(atom.m_type!=nAtomType)
      return op_result::atom_is_corrupted;
    if(!atom.load(buf))
      return op_result::atom_is_corrupted;

    return rv;
  }

  /*!
   * \brief Stores the atom of given type to the image
   * \return operation result: OK on success
   */
  template <typename typeAtom>
  op_result Store(typeAtom &atom)
  {
    if(OK!=m_StorageState)
      return m_StorageState;

    CFIFO buf;
    atom.store(buf);
    return WriteAtom(atom.m_index, atom.m_type, buf);
  }

protected:
  op_result m_StorageState=storage_isnt_verified;

  std::shared_ptr<CFIFO>   m_pFIFObuf;

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

  int GetMemBufSize()
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
private:
  static constexpr std::uint32_t signature{0x69502d52};
  static constexpr unsigned char version{1};

  struct atom_header {
    std::uint16_t type{};
    std::uint16_t count{};
    std::uint32_t dlen{};
    // char data_begin;
  };

  op_result FindAtomHeader(unsigned nAtom, char* const pMemBuf,
    const std::size_t MemBufSize, atom_header** pHeaderBegin)
  {
    const auto* const header = reinterpret_cast<const Header*>(pMemBuf);
    char* mem_buf_end = pMemBuf + MemBufSize;

    op_result rv{op_result::OK};

    // Check if nAtom fits the boundares.
    if(nAtom >= header->numatoms) {
      nAtom = header->numatoms;
      rv = op_result::atom_not_found;
    }

    char* pAtomPtr = pMemBuf + sizeof(Header);
    for(unsigned int i{}; i < nAtom; ++i) {
      pAtomPtr += sizeof(atom_header) + reinterpret_cast<atom_header*>(pAtomPtr)->dlen;
      if (pAtomPtr > mem_buf_end)
        return op_result::storage_is_corrupted;
    }

    // Always out the pointer to the next atom or at least where it should be.
    *pHeaderBegin = reinterpret_cast<atom_header*>(pAtomPtr);
    return rv;
  }

  op_result VerifyAtom(const atom_header* const pAtom)
  {
    //check the atom CRC:
    const auto dlen = pAtom->dlen - 2; // real dlen without CRC
    const auto* const pAtomOffset = reinterpret_cast<const char*>(pAtom);
    const auto* const pDataOffset = pAtomOffset + sizeof(atom_header);
    const auto* const pCrcOffset = pDataOffset + dlen;

    const auto crc = *reinterpret_cast<const std::uint16_t*>(pCrcOffset);
    const auto calc_crc = dmitigr::crc::crc16(pAtomOffset, dlen + sizeof(atom_header));
    if (crc != calc_crc)
      return op_result::atom_is_corrupted;

    return op_result::OK;
  }

  op_result VerifyStorage(const char* pMemBuf, const std::size_t MemBufSize)
  {
    if(MemBufSize < sizeof(Header))
      return op_result::storage_is_corrupted;

    const auto* const header = reinterpret_cast<const Header*>(pMemBuf);
    const char* const pMemLimit = pMemBuf + MemBufSize;

    if (header->signature != signature || header->ver != version
      || header->res || header->eeplen > MemBufSize)
      return op_result::storage_is_corrupted;

    // Verify all the atoms.
    const std::uint16_t nAtoms{header->numatoms};
    const char* pAtomPtr = pMemBuf + sizeof(Header);
    for(std::uint16_t i{}; i < nAtoms; ++i) {
      const auto* const atom_hdr = reinterpret_cast<const atom_header*>(pAtomPtr);
      const op_result res{VerifyAtom(atom_hdr)};
      if (res != op_result::OK)
        return res;
      pAtomPtr += sizeof(atom_header) + atom_hdr->dlen;
      if (pAtomPtr > pMemLimit)
        return op_result::storage_is_corrupted;
    }
    return op_result::OK;
  }

  op_result ResetStorage(char* pMemBuf, const std::size_t MemBufSize)
  {
    if (MemBufSize < sizeof(Header))
      return op_result::storage_is_corrupted;

    auto* const header = reinterpret_cast<Header*>(pMemBuf);
    header->signature = signature;
    header->ver = version;
    header->res = 0;
    header->numatoms = 0;
    header->eeplen = sizeof(Header);
    return op_result::OK;
  }
};

#endif  // PANDA_TIMESWIPE_COMMON_HATS_HPP
