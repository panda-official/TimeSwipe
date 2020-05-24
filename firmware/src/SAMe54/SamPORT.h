/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSamPORT
*/

#pragma once

#include <memory>
#include "Pin.h"
#include "SamSercom.h"



/*!
 * \brief The SAME54 PORT control implementation
 */
class CSamPin;
class CSamPORT
{
friend class CSamPin;
public:
    enum group{

        A=0,
        B,
        C,
        D
    };
    enum pin{

        P00=0,
        P01,
        P02,
        P03,
        P04,
        P05,
        P06,
        P07,

        P08,
        P09,
        P10,
        P11,
        P12,
        P13,
        P14,
        P15,

        P16,
        P17,
        P18,
        P19,
        P20,
        P21,
        P22,
        P23,

        P24,
        P25,
        P26,
        P27,
        P28,
        P29,
        P30,
        P31
    };

    enum pxy{

        PA00=0,
        PA01,
        PA02,
        PA03,
        PA04,
        PA05,
        PA06,
        PA07,

        PA08,
        PA09,
        PA10,
        PA11,
        PA12,
        PA13,
        PA14,
        PA15,

        PA16,
        PA17,
        PA18,
        PA19,
        PA20,
        PA21,
        PA22,
        PA23,

        PA24,
        PA25,
        PA26,
        PA27,
        PA28,
        PA29,
        PA30,
        PA31,


        PB00,
        PB01,
        PB02,
        PB03,
        PB04,
        PB05,
        PB06,
        PB07,

        PB08,
        PB09,
        PB10,
        PB11,
        PB12,
        PB13,
        PB14,
        PB15,

        PB16,
        PB17,
        PB18,
        PB19,
        PB20,
        PB21,
        PB22,
        PB23,

        PB24,
        PB25,
        PB26,
        PB27,
        PB28,
        PB29,
        PB30,
        PB31,


        PC00,
        PC01,
        PC02,
        PC03,
        PC04,
        PC05,
        PC06,
        PC07,

        PC08,
        PC09,
        PC10,
        PC11,
        PC12,
        PC13,
        PC14,
        PC15,

        PC16,
        PC17,
        PC19,
        PC20,
        PC21,
        PC22,
        PC23,

        PC24,
        PC25,
        PC26,
        PC27,
        PC28,
        PC29,
        PC30,
        PC31,



        PD00,
        PD01,
        PD02,
        PD03,
        PD04,
        PD05,
        PD06,
        PD07,

        PD08,
        PD09,
        PD10,
        PD11,
        PD12,
        PD13,
        PD14,
        PD15,

        PD16,
        PD17,
        PD18,
        PD19,
        PD20,
        PD21,
        PD22,
        PD23,

        PD24,
        PD25,
        PD26,
        PD27,
        PD28,
        PD29,
        PD30,
        PD31,

        none=-1
    };

    enum pad{

        PAD0,
        PAD1,
        PAD2,
        PAD3
    };

    enum muxf{

        fA=0,
        fB,
        fC,
        fD,
        fE,
        fF,
        fG,
        fH,
        fI,
        fJ,
        fK,
        fL,
        fM,
        fN
    };

    static std::shared_ptr<CSamPin> FactoryPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bOutput=false);

protected:
    static void SetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bHow);
    static bool RbSetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin);
    static bool GetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin);
    static void ReleasePin(CSamPORT::group nGroup, CSamPORT::pin  nPin);


    static bool FindSercomPad(pxy nPin, typeSamSercoms nSercom, pad &nPad, muxf &nMuxF);


public:
    //multiplexing:
    static bool MUX(pxy nPin, typeSamSercoms nSercom, pad &nPad);


};

class CSamPin : public IPin
{
friend class CSamPORT;
public:
    virtual void Set(bool bHow)
    {
        CSamPORT::SetPin(m_nGroup, m_nPin, bHow);
    }
    virtual bool RbSet()
    {
        return CSamPORT::RbSetPin(m_nGroup, m_nPin);
    }
    virtual bool Get()
    {
        return CSamPORT::GetPin(m_nGroup, m_nPin);
    }

    virtual ~CSamPin()
    {
        CSamPORT::ReleasePin(m_nGroup, m_nPin);
    }
protected:
    CSamPin(CSamPORT::group nGroup, CSamPORT::pin  nPin)
    {
        m_nGroup=nGroup;
        m_nPin=nPin;
    }
    CSamPORT::group m_nGroup;
    CSamPORT::pin   m_nPin;
};
