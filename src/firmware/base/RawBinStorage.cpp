/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include "os.h"
#include "RawBinStorage.h"
#include "SamNVMCTRL.h"

#define STORAGE_STAMP 10042020
CRawBinStorage::CRawBinStorage()
{
    m_Dict.reserve(10);
    m_Items.reserve(50);

    m_LastTimeUpd_mS=os::get_tick_mS();
}

void CRawBinStorage::__ser(void *pVar, const std::type_info &ti)
{

    if(m_bDefaultSettingsOrder || !m_bDownloading)
        return;

    //assume downloading mode only, grab data:
    unsigned int dsize=4;
    struct CRawBinStorageItem item={(uint8_t *)pVar, dsize};
    m_Items.emplace_back(item);

    if(m_bStorageIsFilled)
    {
        CSamNVMCTRL::Instance().ReadSmartEEPROM(m_nOffset, (uint8_t *)pVar, dsize);
        m_nOffset+=dsize;
    }
}

void CRawBinStorage::SetDefaults()
{
    m_bDownloading=true;
    m_bDefaultSettingsOrder=true;

    for(const auto &obj : m_Dict)
    {
        obj->Serialize(*this);
    }

    m_bDefaultSettingsOrder=false;
}

void CRawBinStorage::Load()
{
    m_bDownloading=true;
    m_bDefaultSettingsOrder=false;

    unsigned int stamp=0;

    os::wait(1);
    CSamNVMCTRL::Instance().ReadSmartEEPROM(0, (uint8_t *)&stamp, sizeof(stamp));
    m_bStorageIsFilled=(STORAGE_STAMP==stamp);
    m_nOffset=sizeof(stamp);

    m_Items.clear(); //the items list will be renewed
    for(const auto &obj : m_Dict)
    {
        obj->Serialize(*this);
    }

    m_bDownloading=false;
}
void CRawBinStorage::Update()
{
    //quatation:
    if( (os::get_tick_mS()-m_LastTimeUpd_mS) < 200 )
        return;
    m_LastTimeUpd_mS=os::get_tick_mS();

    //set stamp:
    unsigned int stamp=STORAGE_STAMP;
    if(!CSamNVMCTRL::Instance().WriteSmartEEPROM(0, (uint8_t *)&stamp, sizeof(stamp)))
        return;

    unsigned int offset=sizeof(stamp);

    for(const auto &item : m_Items)
    {
        if(!CSamNVMCTRL::Instance().WriteSmartEEPROM(offset, item.m_pRawData, item.m_RawDataSize))
            return;
        offset+=item.m_RawDataSize;
    }

    CSamNVMCTRL::Instance().FlushSmartEEPROM();
}
