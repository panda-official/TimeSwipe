/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CShiftReg, CShiftRegPin, CDMSsr, CPGA_CS
*/


#pragma once

#include <memory>
#include <bitset>
#include <stdint.h>
#include "Pin.h"


typedef std::bitset<32> typeRegister;
class CShiftRegPin;

/*!
 * \brief The pin-controlled shift register implementation
 * \details This class implements common shift register that has Data, Clock and Strobe inputs
 */
class CShiftReg : public std::enable_shared_from_this<CShiftReg>
{
friend class CShiftRegPin;
protected:

    /*!
     * \brief m_RegValue - the register value (transfered to the chip on each write operation)
     */
    typeRegister m_RegValue;

    /*!
     * \brief m_OccupiedBitsMask - tells which of register bits are used by CShiftRegPin single pin control objects
     */
    typeRegister m_OccupiedBitsMask;

    /*!
     * \brief m_BitsInUse - tells digit capacity of the register (8/16/32)
     */
    std::size_t m_BitsInUse;

    /*!
     * \brief m_pDataPin - the pointer to the data pin
     */
    std::shared_ptr<IPin>    m_pDataPin;

    /*!
     * \brief m_pClockPin - the pointer to the clock pin
     */
    std::shared_ptr<IPin>    m_pClockPin;

    /*!
     * \brief m_pStrobePin - the pointer to the strobe pin
     */
    std::shared_ptr<IPin>    m_pStrobePin;

    /*!
     * \brief Sets shift register value
     * \param RegValue - the register value to be set
     * \param BitsInUse - digit capacity of the register (number of bits to shift into the register)
     */
    void SetShiftReg(typeRegister &RegValue, std::size_t BitsInUse);

    /*!
     * \brief Sets single bit of the shift register
     * \param nBit - the bit number to be set (from 0)
     * \param bHow - the bit value: true or false
     */
    inline void SetBit(std::size_t nBit, bool bHow)
    {
        m_RegValue[nBit]=bHow;
        SetShiftReg(m_RegValue, m_BitsInUse);
    }

    /*!
     * \brief Returns single bit value of the shift register
     * \param nBit - the bit number to read (from 0)
     * \return the actual bit value: true or false
     */
    inline bool GetBit(std::size_t nBit)
    {
        return m_RegValue[nBit];
    }

    /*!
     * \brief The class constructor
     * \param pDataPin - the pointer to the Data pin
     * \param pClockPin - the pointer to the Clock pin
     * \param pStrobePin - the pointer to the Strobe pin
     * \param BitsInUse - the digit capacity of the register (8/16/32)
     */
    inline CShiftReg(const std::shared_ptr<IPin> &pDataPin, const std::shared_ptr<IPin> &pClockPin, const std::shared_ptr<IPin> &pStrobePin,  std::size_t BitsInUse)
    {
        m_pDataPin=pDataPin;
        m_pClockPin=pClockPin;
        m_pStrobePin=pStrobePin;
        m_BitsInUse=BitsInUse;
    }

    /*!
     * \brief Factory for CShiftRegPin single pin control object
     * \param nBit - the bit number(from 0) of the shift register to be controlled
     * \return the pointer to the CShiftRegPin single pin control object on success, otherwise nullptr
     */
    std::shared_ptr<CShiftRegPin> FactoryPin(std::size_t nBit);

};

/*!
 * \brief The common shift register single pin control class
 */
class CShiftRegPin : public CPin
{
friend class CShiftReg;
protected:

    /*!
     * \brief m_pCont - the pointer to the shift register containing corresponding pin
     */
    std::shared_ptr<CShiftReg> m_pCont;

    /*!
     * \brief m_nPin - the number of controlled pin
     */
    std::size_t                m_nPin;

    /*!
     * \brief Implements Set functionality of CPin
     * \param bHow - the pin value to be set: logical true or false
     */
    virtual void impl_Set(bool bHow)
    {
        m_pCont->SetBit(m_nPin, bHow);
    }

    /*!
     * \brief Implements RbSet (read back setup value) functionality of CPin
     * \return the pin value that was set: logical true or false
     */
    virtual bool impl_RbSet()
    {
        return impl_Get();
    }

    /*!
     * \brief Implements Get functionality of CPin
     * \return actual pin state: logical true or false
     */
    virtual bool impl_Get()
    {
        return m_pCont->GetBit(m_nPin);
    }

    /*!
     * \brief The class constructor
     * \param pCont - the pointer to the shift register containing corresponding pin
     * \param nPin - the number of controlled pin
     */
    CShiftRegPin(const std::shared_ptr<CShiftReg> &pCont, std::size_t nPin)
    {
        m_pCont=pCont;
        m_nPin=nPin;
        m_SetupTime_uS=50;
    }
public:

    /*!
     * \brief The class destructor
     */
    virtual ~CShiftRegPin()
    {
        m_pCont->m_OccupiedBitsMask[m_nPin]=false;
    }
};

/*!
 * \brief The DMS board shift register implementation
 */
class CDMSsr : public CShiftReg
{
public:

    /*!
     * \brief The DMS board extension pins
     */
    enum pins{

       DAC_On=15,
       SPI_Ch2=14,     //unused
       SPI_Ch1=13,
       SPI_Ch0=12,
       QSPI_CS3=11,    //unused
       QSPI_CS2=10,    //unused
       QSPI_CS1=9,
       QSPI_CS0=8,
       UB4_On=7,      //unused
       UB3_On=6,      //unused
       UB2_On=5,      //unused
       UB1_On=4,
       IEPE4_On=3,
       IEPE3_On=2,
       IEPE2_On=1,
       IEPE1_On=0
    };

    /*!
     * \brief The DMS board channel amplifiers
     */
    enum pga_sel{

        PGA1,
        PGA2,
        PGA3,
        PGA4
    };

    /*!
     * \brief The class constructor
     * \param pDataPin - the pointer to the Data pin
     * \param pClockPin - the pointer to the Clock pin
     * \param pStrobePin - the pointer to the Strobe pin
     */
    inline CDMSsr(const std::shared_ptr<IPin> &pDataPin, const std::shared_ptr<IPin> &pClockPin, const std::shared_ptr<IPin> &pStrobePin)
        :CShiftReg(pDataPin, pClockPin, pStrobePin, 16)
    {
    }

    /*!
     * \brief Factory for CShiftRegPin single pin control object
     * \param nBit - the bit number(from 0) of the shift register to be controlled
     * \return the pointer to the CShiftRegPin single pin control object on success, otherwise nullptr
     */
    inline std::shared_ptr<CShiftRegPin> FactoryPin(pins nPin)
    {
        return CShiftReg::FactoryPin(nPin);
    }

    /*!
     * \brief Selects one of DMS PGA280 amplifier chips
     * \param nPGA - the chip to be selected
     */
    inline void SelectPGA(pga_sel nPGA)
    {
        m_RegValue[pins::SPI_Ch1]=nPGA>>1;
        m_RegValue[pins::SPI_Ch0]=nPGA&1;
        CShiftReg::SetShiftReg(m_RegValue, m_BitsInUse);
    }

public:

    /*!
     * \brief Sets shift register value. This is a wrapper to be used with a command processor
     * \param nVal - the register value to be set
     */
    inline void SetShiftReg(unsigned int nVal)
    {
        m_RegValue=nVal;
        CShiftReg::SetShiftReg(m_RegValue, m_BitsInUse);
    }

    /*!
     * \brief Returns the current set value of the shift register
     * \return the current set value of the shift register
     */
    inline unsigned int GetShiftReg()
    {
        return m_RegValue.to_ulong();
    }
};

/*!
 * \brief The DMS channel amplifier chip select pin implementation
 * \details Implements CPin interface to control selection of PGA280 chip (virtual pin that can be simply used with PGA280 control class)
 */
class CPGA_CS : public CPin
{
protected:

    /*!
     * \brief The PGA280 chip which selection will be controlled
     */
    CDMSsr::pga_sel m_nPGA;

    /*!
     * \brief The pointer to the DMS shift register (used as multiplexer for CSpin)
     */
    std::shared_ptr<CDMSsr> m_pDMSsr;

    /*!
     * \brief The pointer to the select pin which is multiplexed for each of the PGA280 chips by the DMS shift register
     */
    std::shared_ptr<CPin> m_pCSpin;

    /*!
     * \brief Implements Set functionality of CPin
     * \param bHow - the pin value to be set: logical true or false
     */
    virtual void impl_Set(bool bHow)
    {
        if(bHow)
           m_pDMSsr->SelectPGA(m_nPGA);

        m_pCSpin->Set(bHow);
    }

    /*!
     * \brief Implements RbSet (read back setup value) functionality of CPin
     * \return the pin value that was set: logical true or false
     */
    virtual bool impl_RbSet()
    {
        return m_pCSpin->RbSet();
    }

    /*!
     * \brief Implements Get functionality of CPin
     * \return actual pin state: logical true or false
     */
    virtual bool impl_Get()
    {
        return m_pCSpin->Get();
    }

public:

    /*!
     * \brief The class constructor
     * \param nPGA - the PGA280 chip which selection is controlled
     * \param pDMSsr - the pointer to the DMS shift register (used as multiplexer for CSpin)
     * \param pCSpin - the pointer to the select pin which is multiplexed for each of the PGA280 chips by the DMS shift register
     */
    CPGA_CS(CDMSsr::pga_sel nPGA, const std::shared_ptr<CDMSsr> &pDMSsr, const std::shared_ptr<CPin> &pCSpin)
    {
        m_nPGA=nPGA;
        m_pDMSsr=pDMSsr;
        m_pCSpin=pCSpin;
    }
};



