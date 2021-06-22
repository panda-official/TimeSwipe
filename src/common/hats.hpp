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

#include "../3rdparty/HATS_EEPROM/eeptypes.h"
#include "Serial.h"

#include <array>
#include <cstdint>
#include <cstring>

enum class typeHatsAtom {
  VendorInfo=1,
  GPIOmap,
  LinuxDTB,
  Custom
};

struct CHatAtomStub {
friend class CHatsMemMan;

    CHatAtomStub(int nIndex){

        m_index=nIndex;
    }

private:
    typeHatsAtom m_type=typeHatsAtom::Custom;
    int          m_index=0;

    /*!
     * \brief Fills data fields with zeros
     */
    // void reset();

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
struct CHatAtomVendorInfo {
friend class CHatsMemMan;

    //uint32_t    m_uuid[4];  //represented as 4 32-bit words
    std::array<uint32_t, 4> m_uuid;
    uint16_t    m_PID;
    uint16_t    m_pver;
    std::string m_vstr;
    std::string m_pstr;

    CHatAtomVendorInfo(){reset();}

private:
    typeHatsAtom m_type=typeHatsAtom::VendorInfo;
    int          m_index=0;

    /*!
     * \brief Fills data fields with zeros
     */
  void reset()
  {
    //memset(m_uuid, 0, 20);
    m_uuid.fill(0);
    m_PID=0;
    m_pver=0;
    m_vstr.erase();
    m_pstr.erase();
  }

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
    *((uint8_t*)&m_PID)=b0; *( ((uint8_t*)&m_PID) +1 )=b1;
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
    buf<<*((uint8_t*)&m_PID)<<*( ((uint8_t*)&m_PID) +1 );
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
struct CHatAtomGPIOmap {
friend class CHatsMemMan;

    struct bank_drive{

        uint8_t drive       :4;
        uint8_t slew        :2;
        uint8_t hysteresis  :2;

    }m_bank_drive;

    struct power{

        uint8_t back_power  :1;
        uint8_t reserved    :7;

    }m_power;

    struct GPIO{

        uint8_t func_sel    :3;
        uint8_t reserved    :2;
        uint8_t pulltype    :2;
        uint8_t is_used     :1;

    }m_GPIO[28];

    CHatAtomGPIOmap(){reset();}

private:
    typeHatsAtom m_type=typeHatsAtom::GPIOmap;
    int          m_index=1;

    /*!
     * \brief Fills data fields with zeros
     */
  void reset()
  {
    memset(&m_bank_drive, 0, 30);
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
    if(nDSize<30)
      return false;

    typeSChar ch;
    uint8_t   *pBuf=(uint8_t *)(&m_bank_drive);
    for(int i=0; i<30; i++)
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

struct CCalAtomPair{

    float m;
    uint16_t b;

    CCalAtomPair()
    {
        m=1.0f;
        b=0;
    }
    CCalAtomPair(float _m, uint16_t _b){

        m=_m;
        b=_b;
    }

    /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool load(CFIFO &buf){

        typeSChar ch;
        uint8_t   *pBuf=(uint8_t *)&m;
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

        uint8_t   *pBuf=(uint8_t *)&m;
        for(int i=0; i<6; i++)
        {
            buf<<pBuf[i];
        }
        return true;
    }
};
struct CCalAtom{

    enum atom_type{

        invalid=0,
        V_In1,
        V_In2,
        V_In3,
        V_In4,
        V_supply,
        C_In1,
        C_In2,
        C_In3,
        C_In4,
        Ana_Out
    };

    struct header{

        uint16_t  type;
        uint16_t  count;
        uint32_t  dlen;

    }m_header;

    std::vector<CCalAtomPair> m_data;


    /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool load(CFIFO &buf){

        //load the header:
        header theader;

        typeSChar ch;
        uint8_t   *pBuf=(uint8_t *)&theader;
        for(size_t i=0; i<sizeof(header); i++)
        {
            buf>>ch;
            pBuf[i]=(uint8_t)ch;
        }

        //load the rest:
        for(auto &pair : m_data)
        {
            pair.load(buf);
        }
        return true;

    }

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool store(CFIFO &buf){

        //store the header:
        uint8_t   *pBuf=(uint8_t *)&m_header;
        for(size_t i=0; i<sizeof(header); i++)
        {
            buf<<pBuf[i];
        }

        //save the rest:
        for(auto &pair : m_data)
        {
            pair.store(buf);
        }
        return true;
    }

    void Setup(atom_type nType, size_t nCount){

         m_header.type=nType;
         m_header.count=static_cast<uint16_t>(nCount);
         m_header.dlen=nCount*sizeof(CCalAtomPair);
         m_data.resize(nCount);
    }

    size_t GetSizeInBytes(){

        return m_header.dlen + sizeof(header);
    }


};

struct CHatAtomCalibration
{
friend class CHatsMemMan;

    struct header{

        uint8_t cversion;
        uint64_t timestamp;
        uint16_t numcatoms;
        uint32_t callen;        //total length in bytes of all calibration data (including this header)

    } __attribute__((packed)) m_header;

    std::vector<CCalAtom> m_atoms;

    CCalAtom &refAtom(size_t nAtomIndex){

        return m_atoms[nAtomIndex-1];
    }

    void FillHeader(){

        m_header.cversion=1;
        m_header.timestamp=0; //???
        m_header.numcatoms=static_cast<uint16_t>(m_atoms.size());

        size_t sztotal=sizeof(header);
        for(auto &atom : m_atoms)
        {
            sztotal+=atom.GetSizeInBytes();
        }
        m_header.callen=sztotal;
    }

    bool CheckAtomIndex(size_t nAtomIndex, std::string &strError, bool bCheckExistance=true){

        if(0==nAtomIndex || 0xffff==nAtomIndex)
        {
            strError="invalid index";
            return false;
        }
        if(bCheckExistance)
        {
            if(nAtomIndex>m_atoms.size())
            {
                strError="atom doesn't exist";
                return false;
            }
        }
        return true;
    }
    bool CheckPairIndex(size_t nAtomIndex, size_t nPairIndex, std::string &strError){

        if(!CheckAtomIndex(nAtomIndex, strError))
               return false;

        if(nPairIndex>=refAtom(nAtomIndex).m_data.size())
        {
            strError="wrong pair index";
            return false;
        }
        return true; //!!!!

    }

    bool GetPairsCount(size_t nAtomIndex, size_t &nCount, std::string &strError){

        if(!CheckAtomIndex(nAtomIndex, strError))
               return false;

        nCount=refAtom(nAtomIndex).m_data.size();
        return true;
    }

    bool SetCalPair(size_t nAtomIndex, size_t nPairIndex, CCalAtomPair Pair, std::string &strError){

        if(!CheckPairIndex(nAtomIndex,  nPairIndex,  strError))
            return false;

        refAtom(nAtomIndex).m_data[nPairIndex]=Pair;
        return true;
    }
    bool GetCalPair(size_t nAtomIndex, size_t nPairIndex, CCalAtomPair &Pair, std::string &strError){

        if(!CheckPairIndex(nAtomIndex,  nPairIndex,  strError))
            return false;

        Pair=refAtom(nAtomIndex).m_data[nPairIndex];
        return true;
    }

    CHatAtomCalibration(){

        reset();
    }


private:
    typeHatsAtom m_type=typeHatsAtom::Custom;
    int          m_index=3;

    /*!
     * \brief Fills data fields with default data
     */
    void reset(){

        m_atoms.resize(9);
        m_atoms[0].Setup(CCalAtom::atom_type::V_In1, 22);
        m_atoms[1].Setup(CCalAtom::atom_type::V_In2, 22);
        m_atoms[2].Setup(CCalAtom::atom_type::V_In3, 22);
        m_atoms[3].Setup(CCalAtom::atom_type::V_In4, 22);
        m_atoms[4].Setup(CCalAtom::atom_type::V_supply, 1);
        m_atoms[5].Setup(CCalAtom::atom_type::C_In1, 22);
        m_atoms[6].Setup(CCalAtom::atom_type::C_In2, 22);
        m_atoms[7].Setup(CCalAtom::atom_type::C_In3, 22);
        m_atoms[8].Setup(CCalAtom::atom_type::C_In4, 22);

        FillHeader();
    }

    /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool load(CFIFO &buf){

        //load the header:
        header theader;

        typeSChar ch;
        uint8_t   *pBuf=(uint8_t *)&theader;
        for(int i=0; i<sizeof(header); i++)
        {
            buf>>ch;
            pBuf[i]=(uint8_t)ch;
        }

        //atom's template rules at the moment:
        if(theader.callen!=m_header.callen)
            return false;

        //load the rest:
        for(auto &atom : m_atoms)
        {
            atom.load(buf);
        }
        return true;
    }


    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool store(CFIFO &buf){

        //save the header:
        FillHeader();

        uint8_t   *pBuf=(uint8_t *)&m_header;
        for(int i=0; i<sizeof(header); i++)
        {
            buf<<pBuf[i];
        }

        //save the rest:
        for(auto &atom : m_atoms)
        {
            atom.store(buf);
        }
        return true;
    }

};

/// A manager class for working with HATs-EEPROM binary image
class CHatsMemMan {
public:
  /*!
   * \brief A class constructor
   * \param pFIFObuf a buffer containing EEPROM binary image
   */
  CHatsMemMan(std::shared_ptr<CFIFO> pFIFObuf = {})
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
  op_result ReadAtom(unsigned int nAtom, typeHatsAtom &nAtomType, CFIFO &rbuf)
  {
    if(OK!=m_StorageState)
      return m_StorageState;

    struct atom_header *pAtom;
    op_result res=FindAtomHeader(nAtom, GetMemBuf(), GetMemBufSize(), &pAtom);
    if(op_result::OK!=res)
      return res;

    //check the atom CRC:
    const unsigned int dlen=pAtom->dlen-2; //real dlen without CRC
    const char *pData=(const char*)pAtom + sizeof(struct atom_header); //&pAtom->data_begin;
    nAtomType=static_cast<typeHatsAtom>(pAtom->type);

    uint16_t calc_crc=getcrc((char*)pAtom, dlen+sizeof(atom_header) );
    uint16_t *pCRC=(uint16_t*)(pData+dlen);
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
  op_result WriteAtom(unsigned int nAtom, typeHatsAtom nAtomType, CFIFO &wbuf)
  {
    if(OK!=m_StorageState)
      return m_StorageState;


    struct atom_header *pAtom;

    unsigned int nAtomsCount=GetAtomsCount();
    if(nAtom>nAtomsCount)
      return op_result::atom_not_found;

    bool bAddingNew=(nAtom==nAtomsCount);


    const char *pMemBuf=GetMemBuf();
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
    *pCRC=getcrc((char*)pAtom, req_size+sizeof(struct atom_header)); //set CRC stamp, atom is ready

    ((struct header_t *)(pMemBuf))->eeplen+=nMemAdjustVal;
    if(bAddingNew)
      //also setup the header with the new data:
      ((struct header_t *)(pMemBuf))->numatoms=nAtom+1;

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
  unsigned int GetAtomsCount()
  {
    struct header_t *pHeader=(struct header_t *)GetMemBuf();
    return pHeader->numatoms;
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
    SetMemBufSize(sizeof(struct header_t));
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
    typeHatsAtom nAtomType;
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

  const char* GetMemBuf()
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
    uint16_t type;
    uint16_t count;
    uint32_t dlen;
    // char data_begin;
  };

  op_result FindAtomHeader(unsigned int nAtom, const char *pMemBuf,
    const int MemBufSize, struct atom_header **pHeaderBegin)
  {
    struct header_t *pHeader=(struct header_t *)(pMemBuf);
    const char *pMemLimit=(pMemBuf+MemBufSize);

    op_result rv=op_result::OK;

    //check if nAtom fits the boundares:
    if(nAtom>=pHeader->numatoms)
      {
        nAtom=pHeader->numatoms;
        rv=op_result::atom_not_found;
      }

    const char *pAtomPtr=(pMemBuf+sizeof (struct header_t));
    for(int i=0; i<nAtom; i++)
      {
        pAtomPtr+=(sizeof(struct atom_header) + ((struct atom_header *)pAtomPtr)->dlen);
        if(pAtomPtr>pMemLimit)
          {
            //return memory violation
            return op_result::storage_is_corrupted;
          }
      }

    *pHeaderBegin=(struct atom_header *)pAtomPtr;  //always return the pointer to the next atom or at least where it should be...
    return rv;
  }

  op_result VerifyAtom(struct atom_header *pAtom)
  {
    //check the atom CRC:
    const unsigned int dlen=pAtom->dlen-2; //real dlen without CRC
    const char *pData=(const char*)pAtom + sizeof(struct atom_header);

    uint16_t calc_crc=getcrc((char*)pAtom, dlen+sizeof(atom_header) );
    uint16_t *pCRC=(uint16_t*)(pData+dlen);
    if(calc_crc!=*pCRC)
      return op_result::atom_is_corrupted;

    return op_result::OK;
  }

  op_result VerifyStorage(const char *pMemBuf, const int MemBufSize)
  {
    if(MemBufSize<sizeof(struct header_t))
      return op_result::storage_is_corrupted;

    struct header_t *pHeader=(struct header_t *)(pMemBuf);
    const char *pMemLimit=(pMemBuf+MemBufSize);

    if(signature!=pHeader->signature || version!=pHeader->ver || pHeader->res!=0 || pHeader->eeplen>MemBufSize)
      return  op_result::storage_is_corrupted;

    //verify all atoms:
    int nAtoms=pHeader->numatoms;
    const char *pAtomPtr=(pMemBuf+sizeof (struct header_t));
    for(int i=0; i<nAtoms; i++)
      {
        op_result res=VerifyAtom( (struct atom_header *)pAtomPtr );
        if(op_result::OK!=res)
          return res;
        pAtomPtr+=(sizeof(struct atom_header) + ((struct atom_header *)pAtomPtr)->dlen);
        if(pAtomPtr>pMemLimit)
          {
            //return memory violation
            return op_result::storage_is_corrupted;
          }
      }
    return op_result::OK;
  }

  op_result ResetStorage(const char *pMemBuf, const int MemBufSize)
  {
    if(MemBufSize<sizeof(struct header_t))
      return op_result::storage_is_corrupted;

    struct header_t *pHeader=(struct header_t *)(pMemBuf);

    pHeader->signature=signature;
    pHeader->ver=version;
    pHeader->res=0;
    pHeader->numatoms=0;
    pHeader->eeplen=sizeof(struct header_t);
    return op_result::OK;
  }
};

#endif  // PANDA_TIMESWIPE_COMMON_HATS_HPP
