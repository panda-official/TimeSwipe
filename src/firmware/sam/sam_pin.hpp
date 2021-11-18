// -*- C++ -*-

// PANDA TimeSwipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_PIN_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_PIN_HPP

#include "../pin.h"
#include "SamSercom.h"

#include <memory>

/// Single pin functionality for SAME5x.
class Sam_pin : public Pin {
public:
  /// The SAME5x pin group.
  enum Group { a, b, c, d };

  /// The SAME5x pin number.
  enum Number {
    p00, p01, p02, p03, p04, p05, p06, p07,
    p08, p09, p10, p11, p12, p13, p14, p15,
    p16, p17, p18, p19, p20, p21, p22, p23,
    p24, p25, p26, p27, p28, p29, p30, p31
  };

  /// SAME5x pin unique identifier.
  enum Id {
    PA00, PA01, PA02, PA03, PA04, PA05, PA06, PA07,
    PA08, PA09, PA10, PA11, PA12, PA13, PA14, PA15,
    PA16, PA17, PA18, PA19, PA20, PA21, PA22, PA23,
    PA24, PA25, PA26, PA27, PA28, PA29, PA30, PA31,

    PB00, PB01, PB02, PB03, PB04, PB05, PB06, PB07,
    PB08, PB09, PB10, PB11, PB12, PB13, PB14, PB15,
    PB16, PB17, PB18, PB19, PB20, PB21, PB22, PB23,
    PB24, PB25, PB26, PB27, PB28, PB29, PB30, PB31,

    PC00, PC01, PC02, PC03, PC04, PC05, PC06, PC07,
    PC08, PC09, PC10, PC11, PC12, PC13, PC14, PC15,
    PC16, PC17, PC18, PC19, PC20, PC21, PC22, PC23,
    PC24, PC25, PC26, PC27, PC28, PC29, PC30, PC31,

    PD00, PD01, PD02, PD03, PD04, PD05, PD06, PD07,
    PD08, PD09, PD10, PD11, PD12, PD13, PD14, PD15,
    PD16, PD17, PD18, PD19, PD20, PD21, PD22, PD23,
    PD24, PD25, PD26, PD27, PD28, PD29, PD30, PD31,

    none=-1 // FIXME: remove me
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
     * \brief Fetches pin group from pin identifier.
     * \param pin - the pin number in the Id format
     * \return SAME54 pin's Group
     */
    static inline Group id2group(Id pin){
        return static_cast<Group>(pin/32);
    }

    /*!
     * \brief Fetches pin number in the current Group from the Id(GroupPin) format
     * \param pin  - the pin number in the Id format
     * \return SAME54 pin number in the current Group
     */
    static inline Number id2pin(Id pin){
        return static_cast<Number>(pin%32);
    }

    /*!
     * \brief Transforms pin's Group number and the pin number in the Group to the Id(PinGroup) format
     * \param group - SAME54 pin's Group
     * \param pin - SAME54 pin number in the current Group
     * \return pin number in the Id(PinGroup) format
     */
    static inline Id make_id(Group group, Number pin){

        return static_cast<Id>(group*32+pin);
    }

    /*!
     * \brief Factory for Sam_pin single pin control object
     * \param nGroup - SAME54 pin's Group
     * \param nPin - SAME54 pin number in the current Group
     * \param bOutput - true=configure pin as output, false=configure pin as an input
     * \return
     */
    static std::shared_ptr<Sam_pin> FactoryPin(Group nGroup, Number  nPin, bool bOutput=false);

    /*!
     * \brief Factory for Sam_pin single pin control object
     * \param nPin - SAME54 pin number in the Id(PinGroup) format
     * \param bOutput
     * \return
     */
    static inline std::shared_ptr<Sam_pin> FactoryPin(Id nPin, bool bOutput=false){

        return FactoryPin(id2group(nPin), id2pin(nPin), bOutput);
    }

protected:

    /*!
     * \brief Sets logic state of the pin.
     * \param nGroup - SAME54 pin's Group
     * \param nPin - SAME54 pin number in the current Group
     * \param bHow  - the logical state to be set
     */
    static void SetPin(Group nGroup, Number  nPin, bool bHow);

    /*!
     * \brief Reads back set logical state of the pin
     * \param nGroup  - SAME54 pin's Group
     * \param nPin - SAME54 pin number in the current Group
     * \return set logical value of the pin
     */
    static bool RbSetPin(Group nGroup, Number  nPin);

    /*!
     * \brief Returns measured logic state when pin acts as an input.
     * \param nGroup   - SAME54 pin's Group
     * \param nPin  - SAME54 pin number in the current Group
     * \return measured logical value of the pin
     */
    static bool GetPin(Group nGroup, Number  nPin);

    /*!
     * \brief Releases previously occupied pin
     * \param nGroup  - SAME54 pin's Group
     * \param nPin  - SAME54 pin number in the current Group
     */
    static void ReleasePin(Group nGroup, Number  nPin);

    /*!
     * \brief Searches Sercom's pin PAD for the pin and determines if given Sercom-Pin combination is available
     * \param nPin  - SAME54 pin number in the Id(PinGroup) format
     * \param nSercom - SAME54 Sercom number
     * \param nPad - pin PAD to be searched
     * \param nMuxF - required multiplexer setting for given configuration
     * \return true if the given Sercom-Pin combination is available
     */
    static bool FindSercomPad(Id nPin, typeSamSercoms nSercom, pad &nPad, muxf &nMuxF);


public:
    /*!
     * \brief Connects given pin to the corresponding Sercom
     * \param nPin - the pin to connect in the Id format
     * \param nSercom - SAME54 Sercom number
     * \param nPad - pin PAD value to be filled after connection
     * \return - true if connection is successful, false otherwise
     */
    static bool MUX(Id nPin, typeSamSercoms nSercom, pad &nPad);

public:
    /*!
     * \brief The virtual destructor of the class
     */
    virtual ~Sam_pin()
    {
        ReleasePin(m_nGroup, m_nPin);
    }

    /*!
     * \brief The protected constructor of the class. Called from Sam_pin factories.
     * \param nGroup - the SAME54 pin's group
     * \param nPin - the SAME54 pin number in the current Group
     */
    Sam_pin(Group nGroup, Number  nPin)
    {
        m_nGroup=nGroup;
        m_nPin=nPin;
        m_nPinPAD=Sam_pin::pad::PAD0;

        m_SetupTime_uS=50;
    }

    /*!
     * \brief Connects the pin to the corresponding Sercom
     * \param nSercom - SAME54 Sercom number
     * \return - true if connection is successful, false otherwise
     */
    inline bool MUX(typeSamSercoms nSercom)
    {
        return Sam_pin::MUX( Sam_pin::make_id(m_nGroup, m_nPin), nSercom, m_nPinPAD);
    }

    /*!
     * \brief Returns current PADindex for connected pin
     * \return - the PADindex of the connected pin
     */
    inline Sam_pin::pad GetPAD() const
    {
        return m_nPinPAD;
    }

protected:

    /*!
     * \brief Implements Set functionality of Pin
     * \param bHow - the pin value to be set: logical true or false
    */
    void impl_Set(bool bHow) override
    {
        SetPin(m_nGroup, m_nPin, bHow);
    }

    /*!
    * \brief Implements RbSet (read back setup value) functionality of Pin
    * \return the pin value that was set: logical true or false
    */
    bool impl_RbSet() override
    {
        return RbSetPin(m_nGroup, m_nPin);
    }

    /*!
     * \brief Implements Get functionality of Pin
     * \return actual pin state: logical true or false
     */
    bool impl_Get() override
    {
        return GetPin(m_nGroup, m_nPin);
    }

private:
    /*!
     * \brief SAME54 group of the pin
     */
    Group m_nGroup;

    /*!
     * \brief SAME54 pin number in the current group
     */
    Number   m_nPin;

    /*!
     * \brief Keeps current pin's PAD (the value is filled after connection to the specified peripheral)
     */
    Sam_pin::pad   m_nPinPAD;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_PIN_HPP
