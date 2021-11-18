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
    pa00, pa01, pa02, pa03, pa04, pa05, pa06, pa07,
    pa08, pa09, pa10, pa11, pa12, pa13, pa14, pa15,
    pa16, pa17, pa18, pa19, pa20, pa21, pa22, pa23,
    pa24, pa25, pa26, pa27, pa28, pa29, pa30, pa31,

    pb00, pb01, pb02, pb03, pb04, pb05, pb06, pb07,
    pb08, pb09, pb10, pb11, pb12, pb13, pb14, pb15,
    pb16, pb17, pb18, pb19, pb20, pb21, pb22, pb23,
    pb24, pb25, pb26, pb27, pb28, pb29, pb30, pb31,

    pc00, pc01, pc02, pc03, pc04, pc05, pc06, pc07,
    pc08, pc09, pc10, pc11, pc12, pc13, pc14, pc15,
    pc16, pc17, pc18, pc19, pc20, pc21, pc22, pc23,
    pc24, pc25, pc26, pc27, pc28, pc29, pc30, pc31,

    pd00, pd01, pd02, pd03, pd04, pd05, pd06, pd07,
    pd08, pd09, pd10, pd11, pd12, pd13, pd14, pd15,
    pd16, pd17, pd18, pd19, pd20, pd21, pd22, pd23,
    pd24, pd25, pd26, pd27, pd28, pd29, pd30, pd31
  };

  /// SAME5x pin pads
  enum Pad { pad0, pad1, pad2, pad3 };

  /// Peripheral function.
  enum Peripheral_function {
    pfa, pfb, pfc, pfd, pfe, pff, pfg,
    pfh, pfi, pfj, pfk, pfl, pfm, pfn
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
    static bool FindSercomPad(Id nPin, typeSamSercoms nSercom, Pad &nPad, Peripheral_function &nMuxF);


public:
    /*!
     * \brief Connects given pin to the corresponding Sercom
     * \param nPin - the pin to connect in the Id format
     * \param nSercom - SAME54 Sercom number
     * \param nPad - pin PAD value to be filled after connection
     * \return - true if connection is successful, false otherwise
     */
    static bool MUX(Id nPin, typeSamSercoms nSercom, Pad &nPad);

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
        m_nPinPAD=Pad::pad0;

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
    inline Pad GetPAD() const
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
    Pad   m_nPinPAD;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_PIN_HPP
