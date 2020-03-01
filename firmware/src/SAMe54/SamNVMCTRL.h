/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <stdint.h>
class CSamNVMCTRL
{
protected:
    uint8_t *m_pSmartEEPROM;
    unsigned int m_nSmartEEPROMsize;

public:
    bool ReadSmartEEPROM(unsigned int nOffs, uint8_t *pBuf, unsigned int nRead);
    bool WriteSmartEEPROM(unsigned int nOffs, const uint8_t *pBuf, unsigned int nWrite, bool bCompareMode=true);
    void FlushSmartEEPROM();


    /*!
     * \brief Returns the reference to the created class object instance. (The object created only once)
     * \return
     */
    static CSamNVMCTRL& Instance()
    {
       static CSamNVMCTRL instance;
       return instance;
    }

private:
    //! Forbid creating other instances of the class object
    CSamNVMCTRL();

    //! Forbid copy constructor
    CSamNVMCTRL(const CSamNVMCTRL&)=delete;

    //! Forbid copying
    CSamNVMCTRL& operator=(const CSamNVMCTRL&)=delete;
};
