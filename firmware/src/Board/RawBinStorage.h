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

struct CRawBinStorageItem
{
    uint8_t *m_pRawData;
    unsigned int m_RawDataSize;
};

class CRawBinStorage : public CStorage
{
protected:
    std::vector< std::shared_ptr<ISerialize> > m_Dict;
    std::vector< struct CRawBinStorageItem > m_Items;

    bool m_bStorageIsFilled=false;
    unsigned int m_nOffset;
    unsigned long m_LastTimeUpd_mS;


    virtual void __ser(void *pVar, const std::type_info &ti);

public:
    CRawBinStorage();

    void AddItem(const std::shared_ptr<ISerialize> &Item)
    {
        m_Dict.emplace_back(Item);
    }

    void Load();
    void Update();
};
