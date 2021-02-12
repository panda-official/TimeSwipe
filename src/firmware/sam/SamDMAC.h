/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamDMABlock, CSamDMAChannel, CSamDMAC
*/

#pragma once

#include <memory>
#include <vector>
#include <list>
class CSamDMAChannel;
class CSamDMAC;

/*!
 * \brief The helper class for working with single data block of DMA transfer
 * \details The DMA transfer organized as a sequence of data blocks assigned to a DMA channel
 */
class CSamDMABlock
{
friend class CSamDMAChannel;
protected:
    /*!
     * \brief The pointer to the block descriptor (struct DmacDescriptor, 128bit-aligned)
     */
    unsigned char        *m_pDescriptor;

    /*!
     * \brief The pointer to a memory buffer allocated for a new block (non-aligned)
     */
    unsigned char        *m_pDescrMemBlock;

public:
    /*!
     * \brief The class constructor. Called from DMA channel CSamDMAChannel::AddBlock
     * \param pCont - a pointer to a DMA chennel containing this block
     * \param nInd - an index of the block in the DMA transfer sequence
     */
    CSamDMABlock(CSamDMAChannel *pCont, int nInd);

public:
    /*!
     * \brief THe class destructor
     */
    ~CSamDMABlock();

public:

    /*!
     * \brief The beatsize enum
     * \details "Beat" is an elementary DMA bus transfer operation: can be 8,16, or 32 bits size.
     * Block transfer is splitted into a burst of beats
     */
    enum beatsize{

        BYTE=0,
        HWORD16,
        WORD32
    };

    /*!
     * \brief Sets essential transfer block parameters
     * \param pSourceAddres - data source address
     * \param pDestAddress  - dada destination address
     * \param nBeats        - number of beats to transfer from source to destionation
     * \param nBsize        - a beat size for this block (8, 16 or 32 bits)
     */
    void Setup(const void *pSourceAddres, const void *pDestAddress, unsigned int nBeats=1,
               CSamDMABlock::beatsize nBsize= CSamDMABlock::beatsize::BYTE);

};

/*!
 * \brief The helper class providing a DMA channel interface
 * \details The class instance must be factored from DMA controller instance: CSamDMAC::Instance().Factory()
 */
class CSamDMAChannel
{
friend class CSamDMABlock;
friend class CSamDMAC;
protected:

    /*!
     * \brief The channel index
     */
    int m_nInd;

    /*!
     * \brief A pointer to the DMA controller instance containing this channel
     */
    CSamDMAC    *m_pCont;

    /*!
     * \brief Represents a DMA transfer sequence containing blocks
     */
    std::vector<CSamDMABlock> m_Transfer;

    /*!
     * \brief Returns a pointer to the first block descriptor
     * \return A pointer to the first block descriptor
     */
    inline unsigned char *get_descr_base_addr();

    /*!
     * \brief The class constructor. Called from CSamDMAC::Factory()
     * \param pCont - a pointer to the DMA controller instance containing this channel
     * \param nInd  - the channel index
     */
    CSamDMAChannel(CSamDMAC *pCont, int nInd);

public:

    /*!
     * \brief The action to perform on transfer request from a peripheral
     */
    enum trigact{

        BLOCK=0,        //!<single block transfer
        BURST=2,        //!<burst transfer of the current block
        TRANSACTION=3   //!<run full transfer sequence (all blocks will be transfered)
    };

    /*!
     * \brief The peripheral source of the transfer request
     * \todo Complete the table with rest possible values
     */
    enum trigsrc{

        TC0OVF=0x2C,    //!<TC0 counter overflow
        TC0MC0=0x2D,    //!<TC0 counter match0
        TC0MC1=0x2E,    //!<TC0 counter match1

        TC1OVF=0x2F,    //!<TC1 counter overflow
        TC1MC0=0x30,    //!<TC1 counter match0
        TC1MC1=0x31,    //!<TC1 counter match1

        TC2OVF=0x32,    //!<TC2 counter overflow
        TC2MC0=0x33,    //!<TC2 counter match0
        TC2MC1=0x34,     //!<TC2 counter match1

        TC6OVF=0x3E,    //!<TC6 counter overflow
        TC6MC0=0x3F,    //!<TC6 counter match0
        TC6MC1=0x40     //!<TC6 counter match1
    };

    /*!
     * \brief THe class destructor
     */
    ~CSamDMAChannel();

    /*!
     * \brief Adds a new block into the transfer sequence
     * \return A reference to the block
     */
    CSamDMABlock &AddBlock();

    /*!
     * \brief Returns the block reference by its index
     * \param nInd - the block index
     * \return A reference to the block
     */
    CSamDMABlock &GetBlock(int nInd){ return m_Transfer[nInd]; }

    /*!
     * \brief Triggers a transfer sequence (software trigger)
     * \param how - true=start, false=break
     * \todo implement the method
     */
    void StartTransfer(bool how);

    /*!
     * \brief Sets the periferial trigger action and source for the channel
     * \param act - a trigger action
     * \param src - a peripheral source
     */
    void SetupTrigger(CSamDMAChannel::trigact act, CSamDMAChannel::trigsrc src);

    /*!
     * \brief Sets transfer loop mode: makes transfer running in the infinite loop
     * \param how true=infinite loop mode, false=single transfer
     */
    void SetLoopMode(bool how=true);

    /*!
     * \brief Enables the channel
     * \param how true=enable, false=disable
     */
    void Enable(bool how);

};

/*!
 * \brief The DMA controller class
 */
class CSamDMAC
{
friend class CSamDMAChannel;
protected:

    /*!
     *  \brief maximum channels number
     */
    enum{
       nMaxChannels=8
    };

    /*!
    *   \brief is channel occupied or free
    *
    */
    bool m_bChannelOcupied[nMaxChannels]={false};

    /*!
     * \brief The pointer to the list of first block descriptor of each channel
     */
    unsigned char *m_pBaseAddr;

    /*!
     * \brief The pointer to the "read-back" descriptor list
     */
    unsigned char *m_pWrbAddr;

    /*!
     * \brief Returns a pointer to the first block descriptor of a channel
     * \param chan - a reference to the channel
     * \return A pointer to the first block descriptor
     */
    unsigned char *get_chan_descr_base_addr(CSamDMAChannel &chan);

public:

    /*!
     * \brief Creates a new DMA channel if the free channel is available
     * \return A pointer to the channel
     */
    std::shared_ptr<CSamDMAChannel> Factory();

    /*!
     * \brief Returns the reference to the created class object instance. (The object created only once)
     * \return the reference to the created class object instance
     */
    static CSamDMAC& Instance()
    {
       static CSamDMAC instance;
       return instance;
    }
private:
    //! Forbid creating other instances of the class object
    CSamDMAC();

    //! Forbid copy constructor
    CSamDMAC(const CSamDMAC&)=delete;

    //! Forbid copying
    CSamDMAC& operator=(const CSamDMAC&)=delete;
};
