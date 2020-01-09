/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/


#pragma once

/*!
 * \brief The CHatsMemMan class: the main purpose of this class is to perform RW operations on
 * ATOM's raw binary data.
 */

#include "Serial.h"
enum class typeHatsAtom:int {VendorInfo=1, GPIOmap, LinuxDTB, Custom};

//ATOMS:
class CHatAtomVendorInfo
{
public:
    std::string m_uuid;
    int         m_PID;
    std::string m_pver;
    std::string m_vstr;
    std::string m_pstr;
};

class CHatAtomGPIOmap
{
public:
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
};


class CHatsMemMan
{
public:
    CHatsMemMan(std::shared_ptr<CFIFO> &pFIFObuf);

    enum op_result{

        OK,

        atom_not_found,
        atom_is_corrupted,
        storage_is_corrupted
    };

    //interface:

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

    unsigned int GetAtomsCount();

protected:
    //helpers:

    std::shared_ptr<CFIFO>   m_pFIFObuf;

    const char *GetMemBuf();
    int  GetMemBufSize();
    void AdjustMemBuf(const char *pStart, int nAdjustVal);

};
