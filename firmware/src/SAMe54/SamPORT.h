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
    static std::shared_ptr<CSamPin> FactoryPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bOutput=false);

protected:
    static void SetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bHow);
    static bool RbSetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin);
    static bool GetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin);
    static void ReleasePin(CSamPORT::group nGroup, CSamPORT::pin  nPin);

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
