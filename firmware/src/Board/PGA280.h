/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CPGA280
*/


#pragma once
#include <stdint.h>
#include <vector>
#include "SPI.h"
#include "Pin.h"

/*!
 * \brief The wrapper for PGA280 command byte sequence
 * \details Please, see detailed information about the chip programming at https://www.ti.com/lit/ds/symlink/pga280.pdf, page 26
 */
class CPGA280cmd
{
public:

    /*!
     * \brief The PGA280 command type
     */
    enum cmd{

        write=0x40,     //!<write to internal PGA280 register
        read=0x80,      //!<read from internal PGA280 register
        directCS=0xC0,  //!<direct CS command
        tbuf=0x20       //!<trigger internal buffer
    };

    /*!
     * \brief Specifies whether PGA280 internal buffer will be triggered or not
     */
    bool    m_TBUF;

    /*!
     * \brief The command type to be executed
     */
    cmd     m_Command;

    /*!
     * \brief The address of PGA280 internal register
     */
    uint8_t m_Addr;

    /*!
     * \brief The data byte to be written to the internal register
     */
    uint8_t m_OutData;

    /*!
     * \brief The data byte to be read out from the internal register
     */
    uint8_t m_InData;

    /*!
     * \brief The total length of the command byte sequence
     */
    size_t  m_CmdLen; //out=in, symmetric

    /*!
     * \brief Constructs the PGA command wrapper object
     * \param Command - the PGA command type to be executed
     * \param Addr    - the address of PGA280 internal register
     * \param OutData - the data byte to be written to the internal register
     * \param TBUF    - specifies whether PGA280 internal buffer will be triggered or not
     */
    CPGA280cmd(cmd Command, uint8_t Addr, uint8_t OutData, bool TBUF=false)
    {
        m_Command=Command;
        m_Addr=Addr;
        m_OutData=OutData;
        m_TBUF=TBUF;
    }

    /*!
     * \brief Transforms object data into the byte stream
     * \param ostr - the FIFO buffer for output stream
     * \param bCSmode - use data checksumm mode or not
     * \param bLastCmdInChain - is this the last command in the commands chain?
     *        (PGA280 communication protocol allows packing several commands in one transfer)
     */
    void PushToStream(CFIFO &ostr, bool bCSmode, bool bLastCmdInChain)
    {
        uint8_t CHKsum=0x9B;

        size_t nSend;
        typeSChar frame[4]={m_Command|m_Addr, 0, 0, 0};
        if(m_TBUF)
        {
            frame[0]|=cmd::tbuf;
        }

        if(cmd::write==m_Command)
        {
            frame[1]=m_OutData;
            if(bCSmode)
            {
                frame[2]=(uint8_t)(CHKsum+frame[0]+frame[1]);
                nSend=3;
            }
            else
            {
                nSend=bLastCmdInChain ? 2:3;
            }

        }
        else if(cmd::read==m_Command)
        {
            if(bCSmode)
            {
                nSend=4;
                frame[1]=(uint8_t)(CHKsum+frame[0]);
            }
            else
            {
                nSend=3;
            }

        }
        else
        {
            if(bCSmode)
            {
                frame[1]=(uint8_t)(CHKsum+frame[0]);
                nSend=2;
            }
            else
            {
                 nSend=1;
            }
        }

        m_CmdLen=nSend;
        for(int i=0; i<nSend; i++)
            ostr<<frame[i];

    }

    /*!
     * \brief Fills the object data from the PGA280 response byte stream (response to the command sequence)
     * \param istr - the FIFO buffer for input stream
     * \param bCSmode - is checksum mode used in this transfer?
     * \param bLastCmdInChain - is this the last command in the commands chain?
     *        (PGA280 communication protocol allows packing several commands in one transfer)
     * \return true if retrieving data was successful, false in case of any error
     */
    bool PopFromStream(CFIFO &istr, bool bCSmode, bool bLastCmdInChain)
    {
        if(istr.in_avail()<m_CmdLen)
            return false;

        //pop the buffer data:
        typeSChar frame[4];
        for(int i=0; i<m_CmdLen; i++)
            istr>>frame[i];

        if(cmd::read!=m_Command)
            return true;

        int roffs=m_CmdLen-2;

        //check cs:
        uint8_t CHKsum=0x9B+(m_Command|m_Addr)+frame[roffs];
        m_InData=frame[roffs];
        if(CHKsum!=frame[roffs+1])
            return false;

        return true;
    }

};

/*!
 * \brief The command sequence buffer
 * \details Since the PGA280 communication protocol allows packing multiple commands in one transmission,
 *  all commands are buffered in this object before being sent
 */
class CPGA280cmdBuf
{
public:
    /*!
     * \brief - use data checksumm mode or not
     */
    bool m_bCSmode=false;

    /*!
     * \brief - the input byte stream (responce from PGA280)
     */
    CFIFO m_istr;

    /*!
     * \brief - the output byte stream (the command sequence to be sent to PGA280)
     */
    CFIFO m_ostr;

    /*!
     * \brief - the sequence of command objects to be transformed into the byte stream
     */
    std::vector<CPGA280cmd> m_cmd;

    /*!
     * \brief Performs an atomic transfer operation: send sequence of commands, wait for response, process response
     * \param spi_bus - the pointer to the corresponding SPI bus
     * \param CS - the pointer to the chip select pin
     * \return true on success, false on any error
     */
    bool transfer(CSPI &spi_bus, IPin &CS);


    /*!
     * \brief resets/clears all internal object data
     */
    void reset()
    {
        m_istr.reset();
        m_ostr.reset();
        m_cmd.clear();
    }
};

/*!
 * \brief PGA280 Gain and optional MUX register(#0) bit fields
 */
typedef union {
  struct {

    uint8_t MUX        :3;  //!<multiplexer setting
    uint8_t IGAIN      :4;  //!<input gain setting
    uint8_t OGAIN      :1;  //!<1.375V/V output gain switch

  } bit;
  uint8_t reg;

} typeCPGA280GainMuxReg;


/*!
 * \brief PGA280 BUF timeout register(#3) bit fields
 */
typedef union {
  struct {

    uint8_t BUFTIM     :6;  //!<defines BUF timeout length

  } bit;
  uint8_t reg;

} typeCPGA280BufTimReg;


/*!
 * \brief PGA280 Input Switch Control Register1 (#6) bit fields
 */
typedef union {
  struct {

    uint8_t SW_D12     :1;
    uint8_t SW_C2      :1;
    uint8_t SW_C1      :1;
    uint8_t SW_B2      :1;
    uint8_t SW_B1      :1;
    uint8_t SW_A2      :1;
    uint8_t SW_A1      :1;

  } bit;
  uint8_t reg;

} typeCPGA280ISw1Reg;


/*!
 * \brief PGA280 Input Switch Control Register2 (#7) bit fields
 */
typedef union {
  struct {

    uint8_t SW_G2      :1;
    uint8_t SW_G1      :1;
    uint8_t SW_F2      :1;
    uint8_t SW_F1      :1;

  } bit;
  uint8_t reg;

} typeCPGA280ISw2Reg;


/*!
 * \brief The PGA280 amplifier control class
 */
class CPGA280
{
public:

    /*!
     * \brief The PGA280 registers
     */
    enum reg{

        gain_mux,       //!<Gain and optional MUX register
        soft_reset,     //!<Write-only register,soft reset,write 1
        CP,             //!<SPI-MODE selection to GPIO-pin
        BUFtmt,         //!<Set BUF time-out
        error,          //!<Error Register; reset error bit: write 1
        GPIO,           //!<GPIO Register Data force out or sense
        ISw1,           //!<Input switch control 1
        ISw2            //!<Input switch control 2
    };

    /*!
     * \brief The possible output gain setting
     */
    enum ogain{

        og1,            //!<1 V/V output gain
        og1_3_8         //!<1.375V/V output gain
    };

    /*!
     * \brief The possible input gain setting
     */
    enum igain{

        ig_1_8,         //!<1/8
        ig_1_4,         //!<1/4
        ig_1_2,         //!<1/2
        ig1,            //!<1
        ig2,            //!<2
        ig4,            //!<4
        ig8,            //!<8
        ig16,           //!<16
        ig32,           //!<32
        ig64,           //!<64
        ig128           //!<128
    };

    /*!
     * \brief The possible measurement mode setting
     */
    enum mode{

        Voltage=0,      //!<Voltage mode
        Current         //!<Current mode
    };

    /*!
     * \brief Sets measurement mode (Voltage or Current)
     * \param nMode - the mode to be set
     * \return true on success, false on any error
     */
    bool SetMode(mode nMode);

    /*!
     * \brief Sets input gain value
     * \param ig - the input gain to be set
     * \return true on success, false on any error
     */
    bool SetIGain(igain ig);

    /*!
     * \brief Sets output gain value
     * \param og - the output gain to be set
     * \return true on success, false on any error
     */
    bool SetOGain(ogain og);

    /*!
     * \brief Sets both input and output gain values
     * \param ig - the input gain to be set
     * \param og - the output gain to be set
     * \return true on success, false on any error
     */
    bool SetGains(igain ig, ogain og);


    /*!
     * \brief Sets measurement mode (Voltage or Current). This is a wrapper to be used with a command processor
     * \param nMode  - the mode to be set
     */
    inline void CmSetMode(unsigned int nMode){
        SetMode( static_cast<mode>(nMode) );
    }

    /*!
     * \brief Returns current measurement mode (Voltage or Current). This is a wrapper to be used with a command processor
     * \return
     */
    inline unsigned int CmGetMode(){
        return m_nMode;
    }

    /*!
     * \brief Sets input gain value. This is a wrapper to be used with a command processor
     * \param nGain - the input gain to be set
     */
    inline void CmSetIGain(unsigned int nGain){
        SetIGain( static_cast<igain>(nGain) );
    }

    /*!
     * \brief Returns current input gain value. This is a wrapper to be used with a command processor
     * \return input gain value
     */
    inline unsigned int CmGetIGain(){
        return m_GainMuxReg.bit.IGAIN;
    }

    /*!
     * \brief Sets output gain value. This is a wrapper to be used with a command processor
     * \param nGain - the input gain to be set
     */
    inline void CmSetOGain(unsigned int nGain){
        SetOGain( static_cast<ogain>(nGain) );
    }

    /*!
     * \brief Returns current output gain value. This is a wrapper to be used with a command processor
     * \return output gain value
     */
    inline unsigned int CmGetOGain(){
        return m_GainMuxReg.bit.OGAIN;
    }

    /*!
     * \brief Selects register for ReadSelectedReg() and WriteSelectedReg() operations. This is a wrapper to be used with a command processor
     * \param nReg - register number to select
     */
    void SelectReg(unsigned int nReg)
    {
        m_SelReg=static_cast<reg>(nReg);
    }

    /*!
     * \brief Returns selected register. This is a wrapper to be used with a command processor
     * \return selected register number
     */
    unsigned int GetSelectedReg()
    {
        return m_SelReg;
    }

    /*!
     * \brief Reads register value previously selected by SelectIng(). This is a wrapper to be used with a command processor
     * \return Actual register value on success, -1 on error
     */
    int ReadSelectedReg()
    {
        uint8_t rv;
        if(!ReadRegister(m_SelReg, rv))
            return -1;

        return rv;
    }

    /*!
     * \brief Writes register value previously selected by SelectIng(). This is a wrapper to be used with a command processor
     * \param nVal - the register value to be written
     */
    void WriteSelectedReg(int nVal)
    {
        WriteRegister(m_SelReg, nVal);
    }

    /*!
     * \brief The class constructor
     * \param pSPIbus - the pointer to a SPI bus
     * \param pCS     - the pointer to the chip select pin
     */
    CPGA280(std::shared_ptr<CSPI> pSPIbus, std::shared_ptr<IPin> pCS);



protected:

    /*!
     * \brief m_pSPIbus - the pointer to a SPI communication bus
     */
    std::shared_ptr<CSPI> m_pSPIbus;

    /*!
     * \brief m_pCS - the pointer to the chip select pin
     */
    std::shared_ptr<IPin> m_pCS;

    /*!
     * \brief m_CmdBuf - the buffer for storing a command sequence for a current transfer operation
     */
    CPGA280cmdBuf m_CmdBuf;

    /*!
     * \brief m_SelReg - the currently selected register
     */
    reg m_SelReg=gain_mux;

    /*!
     * \brief m_nMode - set measurement mode
     */
    mode                    m_nMode;

    /*!
     * \brief m_GainMuxReg - cashed Gain and optional MUX register value
     */
    typeCPGA280GainMuxReg   m_GainMuxReg;

    /*!
     * \brief Reads PGA280 register value
     * \param nReg - the register to read
     * \param RegValue - the register value will be received
     * \return true on success, false on any error
     */
    bool ReadRegister(reg nReg, uint8_t &RegValue);

    /*!
     * \brief Sets the value of PGA280 register
     * \param nReg - the register to set
     * \param RegValue - the register value to be written
     * \param TBUF - trigger internal buffer on write operation
     * \return true on success, false on any error
     */
    bool WriteRegister(reg nReg, uint8_t RegValue, bool TBUF=false);
};
