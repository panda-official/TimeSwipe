/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include <memory>
#include <bitset>
#include <stdint.h>
#include "Pin.h"


typedef std::bitset<32> typeRegister;

class CShiftRegPin;
class CShiftReg : public std::enable_shared_from_this<CShiftReg>
{
friend class CShiftRegPin;
protected:
    typeRegister m_RegValue;
    typeRegister m_OccupiedBitsMask;
    std::size_t m_BitsInUse;


    std::shared_ptr<IPin>    m_pDataPin;
    std::shared_ptr<IPin>    m_pClockPin;
    std::shared_ptr<IPin>    m_pStrobePin;

    void SetShiftReg(typeRegister &RegValue, std::size_t BitsInUse);
    inline void SetBit(std::size_t nBit, bool bHow)
    {
        m_RegValue[nBit]=bHow;
        SetShiftReg(m_RegValue, m_BitsInUse);
    }
    inline bool GetBit(std::size_t nBit)
    {
        return m_RegValue[nBit];
    }
    inline CShiftReg(const std::shared_ptr<IPin> &pDataPin, const std::shared_ptr<IPin> &pClockPin, const std::shared_ptr<IPin> &pStrobePin,  std::size_t BitsInUse)
    {
        m_pDataPin=pDataPin;
        m_pClockPin=pClockPin;
        m_pStrobePin=pStrobePin;
        m_BitsInUse=BitsInUse;
    }

    std::shared_ptr<CShiftRegPin> FactoryPin(std::size_t nBit);

};

class CShiftRegPin : public CPin
{
friend class CShiftReg;
protected:
    std::shared_ptr<CShiftReg> m_pCont;
    std::size_t                m_nPin;

    virtual void impl_Set(bool bHow)
    {
        m_pCont->SetBit(m_nPin, bHow);
    }
    virtual bool impl_RbSet()
    {
        return impl_Get();
    }
    virtual bool impl_Get()
    {
        return m_pCont->GetBit(m_nPin);
    }

    CShiftRegPin(const std::shared_ptr<CShiftReg> &pCont, std::size_t nPin)
    {
        m_pCont=pCont;
        m_nPin=nPin;
        m_SetupTime_uS=50;
    }
public:
    virtual ~CShiftRegPin()
    {
        m_pCont->m_OccupiedBitsMask[m_nPin]=false;
    }
};

class CDMSsr : public CShiftReg
{
public:
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

    enum pga_sel{

        PGA1,
        PGA2,
        PGA3,
        PGA4
    };

    inline CDMSsr(const std::shared_ptr<IPin> &pDataPin, const std::shared_ptr<IPin> &pClockPin, const std::shared_ptr<IPin> &pStrobePin)
        :CShiftReg(pDataPin, pClockPin, pStrobePin, 16)
    {
    }
    inline std::shared_ptr<CShiftRegPin> FactoryPin(pins nPin)
    {
        return CShiftReg::FactoryPin(nPin);
    }

    inline void SelectPGA(pga_sel nPGA)
    {
        m_RegValue[pins::SPI_Ch1]=nPGA>>1;
        m_RegValue[pins::SPI_Ch0]=nPGA&1;
        CShiftReg::SetShiftReg(m_RegValue, m_BitsInUse);
    }

public:
    //debug functions for testing:
    inline void SetShiftReg(unsigned int nVal)
    {
        m_RegValue=nVal;
        CShiftReg::SetShiftReg(m_RegValue, m_BitsInUse);
    }
    inline unsigned int GetShiftReg()
    {
        return m_RegValue.to_ulong();
    }
};

class CPGA_CS : public CPin
{
protected:
    CDMSsr::pga_sel m_nPGA;
    std::shared_ptr<CDMSsr> m_pDMSsr;
    std::shared_ptr<CPin> m_pCSpin;

    virtual void impl_Set(bool bHow)
    {
        if(bHow)
           m_pDMSsr->SelectPGA(m_nPGA);

        m_pCSpin->Set(bHow);
    }
    virtual bool impl_RbSet()
    {
        return m_pCSpin->RbSet();
    }
    virtual bool impl_Get()
    {
        return m_pCSpin->Get();
    }

public:
    CPGA_CS(CDMSsr::pga_sel nPGA, const std::shared_ptr<CDMSsr> &pDMSsr, const std::shared_ptr<CPin> &pCSpin)
    {
        m_nPGA=nPGA;
        m_pDMSsr=pDMSsr;
        m_pCSpin=pCSpin;
    }
};



