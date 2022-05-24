/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "../Storage.h"

#include <cstdint>
#include <memory>
#include <vector>

/// Represents a RAM memory layout of a data primitive (variable).
struct CRawBinStorageItem final {
    /// The pointer to the variable (physical address).
    uint8_t *m_pRawData;

    /// The amount of physical memory occupied by the variable.
    unsigned int m_RawDataSize;
};

/**
 * @brief An infrastructure for storing all the board settings in a binary format.
 *
 * @details
 *   -# The objects which content should be stored must provide a serialization
 *   scheme of its internals by implementing ISerialize interface;
 *   -# all of this objects initially have to be added to the CRawBinStorage's
 *   list by the AddItem() method;
 *   -# the Import() method will iterate through the objects list and grab
 *   memory layout information about each serialized variable of the object. The
 *   information will be placed in the CRawBinStorage::m_Items list sequentially.
 *   During iterating CRawBinStorage::m_Items list the content of each item will
 *   be imported from the SmartEEPROM if the SmartEEPROM was filled properly;
 *   -# after initialization (AddItem() + Import()) the Update() method must be
 *   called continuously. It will compare the content of each item with the same
 *   content stored in the SmartEEPROM and overwrites it when changed. Thus all
 *   objects tracked from the CRawBinStorage will be stored automatically without
 *   the need for additional calls.
 */
class CRawBinStorage final : public CStorage {
private:
    /// The list of serializable objects.
    std::vector< std::shared_ptr<ISerialize> > m_Dict;

    /// The list of data primitives items.
    std::vector< struct CRawBinStorageItem > m_Items;

    /// The SmartEEPROM was initially filled if true.
    bool m_bStorageIsFilled=false;

    /// An address offset inside SmartEEPROM used for R/W operations.
    unsigned int m_nOffset;

    /// The last time the storage has been updated
    unsigned long m_LastTimeUpd_mS;

    /**
     * @brief The method used to grab information about each data primitive of
     * the serialized object and to fill it content with the data imported from
     * the SmartEEPROM.
     *
     * @param pVar A pointer to the data primitive.
     * @param ti The data primitive type information.
     */
    virtual void __ser(void *pVar, const std::type_info &ti);

public:
    /// The class constructor.
    CRawBinStorage();

    /**
     * @brief Adds serializable object into the tracking list.
     *
     * @param Item A reference to the object.
     */
    void AddItem(const std::shared_ptr<ISerialize> &Item)
    {
        m_Dict.emplace_back(Item);
    }

    /**
     * @brief Initializes the storage internals and imported objects content
     * form the SmartEEPROM.
     */
    void Import();

    void SetDefaults();

    /**
     * @brief Tracks changes in serializable objects and overwrites SmartEEPROM
     * when changed.
     */
    void Update();
};
