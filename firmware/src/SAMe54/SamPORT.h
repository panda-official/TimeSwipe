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


class CSamPin;
/*!
 * \brief The SAME54 PORT control implementation
 */
class CSamPORT
{
friend class CSamPin;
public:

    /*!
     * \brief The SAME54 pin groups
     */
    enum group{

        A=0,
        B,
        C,
        D
    };

    /*!
     * \brief The SAME54 pins in the group
     */
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

    /*!
     * \brief All possible SAME54 pins in the GroupPin format
     */
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

    /*!
     * \brief SAME54 pin pads
     */
    enum pad{

        PAD0,
        PAD1,
        PAD2,
        PAD3
    };

    /*!
     * \brief The list of possible multiplexer values for a pin
     */
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

    /*!
     * \brief Fetches pin group from the pxy(GroupPin) format
     * \param pin - the pin number in the pxy format
     * \return SAME54 pin's Group
     */
    static inline CSamPORT::group pxy2group(pxy pin){
        return static_cast<CSamPORT::group>(pin/32);
    }

    /*!
     * \brief Fetches pin number in the current Group from the pxy(GroupPin) format
     * \param pin  - the pin number in the pxy format
     * \return SAME54 pin number in the current Group
     */
    static inline CSamPORT::pin pxy2pin(pxy pin){
        return static_cast<CSamPORT::pin>(pin%32);
    }

    /*!
     * \brief Transforms pin's Group number and the pin number in the Group to the pxy(PinGroup) format
     * \param group - SAME54 pin's Group
     * \param pin - SAME54 pin number in the current Group
     * \return pin number in the pxy(PinGroup) format
     */
    static inline CSamPORT::pxy make_pxy(CSamPORT::group group, CSamPORT::pin pin){

        return static_cast<CSamPORT::pxy>(group*32+pin);
    }

    /*!
     * \brief Factory for CSamPin single pin control object
     * \param nGroup - SAME54 pin's Group
     * \param nPin - SAME54 pin number in the current Group
     * \param bOutput - true=configure pin as output, false=configure pin as an input
     * \return
     */
    static std::shared_ptr<CSamPin> FactoryPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bOutput=false);

    /*!
     * \brief Factory for CSamPin single pin control object
     * \param nPin - SAME54 pin number in the pxy(PinGroup) format
     * \param bOutput
     * \return
     */
    static inline std::shared_ptr<CSamPin> FactoryPin(CSamPORT::pxy nPin, bool bOutput=false){

        return FactoryPin(pxy2group(nPin), pxy2pin(nPin), bOutput);
    }

protected:

    /*!
     * \brief Sets logic state of the pin.
     * \param nGroup - SAME54 pin's Group
     * \param nPin - SAME54 pin number in the current Group
     * \param bHow  - the logical state to be set
     */
    static void SetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin, bool bHow);

    /*!
     * \brief Reads back set logical state of the pin
     * \param nGroup  - SAME54 pin's Group
     * \param nPin - SAME54 pin number in the current Group
     * \return set logical value of the pin
     */
    static bool RbSetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin);

    /*!
     * \brief Returns measured logic state when pin acts as an input.
     * \param nGroup   - SAME54 pin's Group
     * \param nPin  - SAME54 pin number in the current Group
     * \return measured logical value of the pin
     */
    static bool GetPin(CSamPORT::group nGroup, CSamPORT::pin  nPin);

    /*!
     * \brief Releases previously occupied pin
     * \param nGroup  - SAME54 pin's Group
     * \param nPin  - SAME54 pin number in the current Group
     */
    static void ReleasePin(CSamPORT::group nGroup, CSamPORT::pin  nPin);

    /*!
     * \brief Searches Sercom's pin PAD for the pin and determines if given Sercom-Pin combination is available
     * \param nPin  - SAME54 pin number in the pxy(PinGroup) format
     * \param nSercom - SAME54 Sercom number
     * \param nPad - pin PAD to be searched
     * \param nMuxF - required multiplexer setting for given configuration
     * \return true if the given Sercom-Pin combination is available
     */
    static bool FindSercomPad(pxy nPin, typeSamSercoms nSercom, pad &nPad, muxf &nMuxF);


public:
    /*!
     * \brief Connects given pin to the corresponding Sercom
     * \param nPin - the pin to connect in the pxy format
     * \param nSercom - SAME54 Sercom number
     * \param nPad - pin PAD value to be filled after connection
     * \return - true if connection is successful, false otherwise
     */
    static bool MUX(pxy nPin, typeSamSercoms nSercom, pad &nPad);
};

/*!
 * \brief The class implements CPin functionality for SAME54 single pin
 */
class CSamPin : public CPin
{
friend class CSamPORT;
protected:

    /*!
     * \brief Implements Set functionality of CPin
     * \param bHow - the pin value to be set: logical true or false
    */
    virtual void impl_Set(bool bHow)
    {
        CSamPORT::SetPin(m_nGroup, m_nPin, bHow);
    }

    /*!
    * \brief Implements RbSet (read back setup value) functionality of CPin
    * \return the pin value that was set: logical true or false
    */
    virtual bool impl_RbSet()
    {
        return CSamPORT::RbSetPin(m_nGroup, m_nPin);
    }

    /*!
     * \brief Implements Get functionality of CPin
     * \return actual pin state: logical true or false
     */
    virtual bool impl_Get()
    {
        return CSamPORT::GetPin(m_nGroup, m_nPin);
    }

public:
    /*!
     * \brief The virtual destructor of the class
     */
    virtual ~CSamPin()
    {
        CSamPORT::ReleasePin(m_nGroup, m_nPin);
    }

    /*!
     * \brief Connects the pin to the corresponding Sercom
     * \param nSercom - SAME54 Sercom number
     * \return - true if connection is successful, false otherwise
     */
    inline bool MUX(typeSamSercoms nSercom)
    {
        return CSamPORT::MUX( CSamPORT::make_pxy(m_nGroup, m_nPin), nSercom, m_nPinPAD);
    }

    /*!
     * \brief Returns current PADindex for connected pin
     * \return - the PADindex of the connected pin
     */
    inline CSamPORT::pad GetPAD() const
    {
        return m_nPinPAD;
    }

protected:
    /*!
     * \brief The protected constructor of the class. Called from CSamPORT factories.
     * \param nGroup - the SAME54 pin's group
     * \param nPin - the SAME54 pin number in the current Group
     */
    CSamPin(CSamPORT::group nGroup, CSamPORT::pin  nPin)
    {
        m_nGroup=nGroup;
        m_nPin=nPin;
        m_nPinPAD=CSamPORT::pad::PAD0;

        m_SetupTime_uS=50;
    }

    /*!
     * \brief SAME54 group of the pin
     */
    CSamPORT::group m_nGroup;

    /*!
     * \brief SAME54 pin number in the current group
     */
    CSamPORT::pin   m_nPin;

    /*!
     * \brief Keeps current pin's PAD (the value is filled after connection to the specified peripheral)
     */
    CSamPORT::pad   m_nPinPAD;
};
