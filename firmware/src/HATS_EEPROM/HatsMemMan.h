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
    CHatsMemMan(std::shared_ptr<CFIFO> &pFIFObuf);

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
