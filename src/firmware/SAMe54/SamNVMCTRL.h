/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <stdint.h>
#include "NVMpage.h"

/*!
*   @file
*   @brief A definition file for SAM's Nonvolatile Memory Controller:
*   CSamNVMCTRL
*/

/*!
 * \brief The SAM's Nonvolatile Memory Controller implementation
 *
 * \details Provides methods for working with SmartEEPROM. Designed as singleton
 */
class CSamNVMCTRL
{
protected:
    /*!
     * \brief The size of SmartEEPROM in bytes
     */
    unsigned int m_nSmartEEPROMsize;

    /*!
     * \brief Obtains the current size of the SmartEEPROM from SEESTAT.PSZ & SEESTAT.SBLK
     * \return the current size of SmartEEPROM in bytes
     */
    unsigned int ObtainSmartEEPROMsize();

public:

    /*!
     * \brief Reads raw binary data from SmartEEPROM
     * \param nOffs An offset from the base address
     * \param pBuf A buffer to receive the data
     * \param nRead A number of data to read in bytes
     * \return true if reading was successful otherwise false
     */
    bool ReadSmartEEPROM(unsigned int nOffs, uint8_t *pBuf, unsigned int nRead);

    /*!
     * \brief Writes raw binary data to SmartEEPROM
     * \param nOffs An offset from the base address
     * \param pBuf A buffer with the data to write
     * \param nRead A number of data to write in bytes
     * \param bCompareMode Comparation mode: true=overwrite data only if it changed from the SmartEEPROM data,
     *  false=always overwrite the SmartEEPROM content
     * \return true if writing was successful otherwise false
     */
    bool WriteSmartEEPROM(unsigned int nOffs, const uint8_t *pBuf, unsigned int nWrite, bool bCompareMode=true);

    /*!
     * \brief Must be called in the end of the WriteSmartEEPROM calls sequence or single call
     *  so that all buffered data is actually stored in SamrtEEPROM flash memory
     */
    void FlushSmartEEPROM();

    /*!
     * \brief Reads the User Page
     * \param page - User Page reference
     * \return true if reading was successful otherwise false
     */
    bool ReadUserPage(struct NVM_UserPage &page);

    /*!
     * \brief Writes the User Page
     * \param page - User Page reference
     * \return true if writing was successful otherwise false
     */
    bool WriteUserPage(struct NVM_UserPage &page);

    /*!
     * \brief Sets factory User Page with default values
     * \details Use this method if User Page has been damaged. The default settings are snapshots from boards with factory settings
     * \return true on success, otherwise false
     */
    bool SetUserPageDefaults();

    /*!
     * \brief Erases a block of flash memory
     * \param nBlock - the block number to erase
     * \return
     */
    bool EraseBlock(unsigned int nBlock);

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
