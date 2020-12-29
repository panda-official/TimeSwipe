/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   @file
*   @brief A definition file for HAT data types and interfaces:
*   CHatAtomVendorInfo, CHatAtomGPIOmap, CHatsMemMan
*
*/


#pragma once

#include <array>
#include "Serial.h"
enum class typeHatsAtom:int {VendorInfo=1, GPIOmap, LinuxDTB, Custom};

struct CHatAtomStub
{
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
    void reset();

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


/*!
 * \brief The vendor info atom
 */
struct CHatAtomVendorInfo
{
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
    void reset();

    /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool load(CFIFO &buf);

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool store(CFIFO &buf);
};

/*!
 * \brief The GPIO map atom
 */
struct CHatAtomGPIOmap
{
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
    void reset();

    /*!
     * \brief Loads data fields from an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool load(CFIFO &buf);

    /*!
     * \brief Stores data fields to an ATOM binary image
     * \param buf ATOM binary image
     * \return true=successful, false=failure
     */
    bool store(CFIFO &buf);
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
        V_In,
        V_supply,
        C_In,
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

        m_atoms.resize(3);
        m_atoms[0].Setup(CCalAtom::atom_type::V_In, 22);
        m_atoms[1].Setup(CCalAtom::atom_type::V_supply, 1);
        m_atoms[2].Setup(CCalAtom::atom_type::C_In, 22);

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



/*!
 * \brief A manager class for working with HATs-EEPROM binary image
 */
class CHatsMemMan
{
public:
    /*!
     * \brief A class constructor
     * \param pFIFObuf a buffer containing EEPROM binary image
     */
    //CHatsMemMan(std::shared_ptr<CFIFO> &pFIFObuf);

    enum op_result{

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
    op_result ReadAtom(unsigned int nAtom, typeHatsAtom &nAtomType, CFIFO &rbuf);

    /*!
     * \brief WriteAtom writes Atom's raw binary data
     * \param nAtom absolute address of the Atom
     * \param nAtomType atom type to be written
     * \param wbuf data bufer
     * \return read operation result (OK or an error)
     */

    op_result WriteAtom(unsigned int nAtom, typeHatsAtom nAtomType, CFIFO &wbuf);

public:

    void SetBuf(const std::shared_ptr<CFIFO> &pBuf){

        m_pFIFObuf=pBuf;
    }
    const std::shared_ptr<CFIFO> &GetBuf(){

        return m_pFIFObuf;
    }


    /*!
     * \brief Returns the total atoms count
     * \return
     */
    unsigned int GetAtomsCount();

    /*!
     * \brief Checks the image data validity
     * \details The method must be called before performing any operations on the binary image
     * It checks all headers and atoms validity and sets m_StorageState to "OK" on success
     * If you are working on empty image Reset() must be called instead
     * \return operation result: OK on success
     */
    op_result Verify();

    /*!
     * \brief Resets all image data to a default state(atoms count=0). Must be called when start working on empty image
     */
    void Reset();


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

    const char *GetMemBuf();
    int  GetMemBufSize();
    void SetMemBufSize(int size);
    void AdjustMemBuf(const char *pStart, int nAdjustVal);

};
