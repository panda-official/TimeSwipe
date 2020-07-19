/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

/*!
*   \file
*   \brief A definition file for
*   CPin
*/


#include "os.h"

/*!
 * \brief The implementation of an abstract interface of the pin
 */
class CPin
{
protected:

    /*!
     * \brief Inverted pin behaviour flag.
     *  Inverted pin behaviour means: logical pin state=true gives real output level=0 and vice versa: logical state=false gives output level=1.
     *  When normal behaviour: true=1, false=0.
     */
    bool m_bInvertedBehaviour=false;

    /*!
     * \brief Setup time for the output level.
     *  Usually pin output level does not change immediately but some short time is required to wait for the level rise or fall
     */
    unsigned long m_SetupTime_uS=0;

    /*!
     * \brief Implements Set functionality of CPin. Must be re-implemented in the derived class
     * \param bHow - the pin value to be set: logical true or false
     */
    virtual void impl_Set(bool bHow)=0;

    /*!
     * \brief Implements RbSet (read back setup value) functionality of CPin. Must be re-implemented in the derived class
     * \return the pin value that was set: logical true or false
     */
    virtual bool impl_RbSet()=0;


    /*!
     * \brief Implements Get functionality of CPin. Must be re-implemented in the derived class
     * \return actual pin state: logical true or false
     */
    virtual bool impl_Get()=0;

public:

    /*!
     * \brief Sets logic state of the pin. May differ from actual output level (see SetInvertedBehaviour() )
     * \param bHow - the logical state to be set
     */
    inline void Set(bool bHow)
    {
        impl_Set(m_bInvertedBehaviour ? !bHow:bHow);

        if(m_SetupTime_uS)
            os::uwait(m_SetupTime_uS);
    }

    /*!
     * \brief Reads back set logical state of the pin
     * \return set logical value of the pin
     */
    inline bool RbSet()
    {
        bool rv=impl_RbSet();
        return m_bInvertedBehaviour ? !rv:rv;
    }

    /*!
     * \brief Returns measured logic state when pin acts as an input. May differ from actual output level (see SetInvertedBehaviour() )
     * \return measured logical value of the pin
     */
    inline bool Get()
    {
        bool rv=impl_Get();
        return m_bInvertedBehaviour ? !rv:rv;
    }

    /*!
     * \brief Inverts logic behaviour of the pin
     * \details Normal behaviour: logical true=high output level(1), logical false=low output level(0)
     * Inverted behaviour: logical true=low output level(0), logical false=high output level(1)
     * \param how - true=inverted behaviour, false=normal behaviour (default)
     */
    inline void SetInvertedBehaviour(bool how)
    {
        m_bInvertedBehaviour=how;
    }

    /*!
     * \brief Sets output level setup time
     * \details Usually pin output level does not change immediately but some short time is required to wait for the level rise or fall
     * \param nSetupTime_uS - the setup time in uS
     */
    inline void SetPinSetupTime(unsigned long nSetupTime_uS)
    {
        m_SetupTime_uS=nSetupTime_uS;
    }


    /*!
     * The class default constructor
     */
    CPin()=default;

    /*!
     * \brief remove copy constructor
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    CPin(const CPin&) = delete;

    /*!
     * \brief remove copy operator
     * \return
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    CPin& operator=(const CPin&) = delete;

protected:
    /*!
     * The virtual destructor of the class
     */
    virtual ~CPin()=default;
};
typedef CPin IPin;

