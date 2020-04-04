/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <memory>
#include <vector>
#include <stdint.h>
#include "Storage.h"

/*!
*   @file
*   @brief A definition file for the storage where content is stored in a binary stream format:
*   CRawBinStorage
*/

/*!
 * \brief The structure represents a RAM memory layout of a data primitive (variable)
 */
struct CRawBinStorageItem
{
    /*!
     * \brief The pointer to the variable (physical address)
     */
    uint8_t *m_pRawData;

    /*!
     * \brief The amount of physical memory occupied by the variable
     */
    unsigned int m_RawDataSize;
};

/*!
 * \brief The class provides an infrastructure for storing all of the board settings in a binary format
 *
 * \details
 * 1) The objects which content should be stored must provide a serialization scheme of its internals
 * by implementing ISerialize interface <br />
 * 2) All of this objects initially have to be added to the CRawBinStorage's list by the AddItem() method <br />
 * 3) The Load() method will iterate through the objects list and grab memory layout information
 * about each serialized variable of the object. The information will be placed in the CRawBinStorage::m_Items list sequentially.
 * During iterating CRawBinStorage::m_Items list the content of each item will be downloaded from the SmartEEPROM
 * if the SmartEEPROM was filled properly <br />
 * 4) After initialization ( AddItem() + Load() ) the Update() method must be called continuously.
 * It will compare the content of each item with the same content stored in the SmartEEPROM
 * and overwrites it when changed. Thus all objects tracked from the CRawBinStorage will be stored automatically
 * without the need for additional calls
 */
class CRawBinStorage : public CStorage
{
protected:
    /*!
     * \brief The list of serializable objects
     */
    std::vector< std::shared_ptr<ISerialize> > m_Dict;

    /*!
     * \brief The list of data primitives items
     */
    std::vector< struct CRawBinStorageItem > m_Items;

    /*!
     * \brief The SamrtEEPROM was initially filled if true.
     */
    bool m_bStorageIsFilled=false;

    /*!
     * \brief An address offset inside Smart EEPROM used for R/W operations
     */
    unsigned int m_nOffset;

    /*!
     * \brief The last time the storage has been updated
     */
    unsigned long m_LastTimeUpd_mS;

    /*!
     * \brief The method used to grab information about each data primitive of the serialized object
     *  and to fill it content with the data loaded from the SmartEEPROM
     * \param pVar A pointer to the data primitive
     * \param ti The data primitive type information (in C++ RTTI form)
     */
    virtual void __ser(void *pVar, const std::type_info &ti);

public:
    /*!
     * \brief The class constructor
     */
    CRawBinStorage();

    /*!
     * \brief Adds serializable object into the tracking list
     * \param Item A reference to the object
     */
    void AddItem(const std::shared_ptr<ISerialize> &Item)
    {
        m_Dict.emplace_back(Item);
    }

    /*!
     * \brief Initializes storage internals and download objects content form the SmartEEPROM
     */
    void Load();

    void SetDefaults();

    /*!
     * \brief Tracks changes in  serializable objects and overwrites SmartEEPROM when changed
     */
    void Update();
};
