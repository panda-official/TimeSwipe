/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#if (0)
struct IPin
{
    virtual void Set(bool bHow)=0;
    virtual bool RbSet()=0;
    virtual bool Get()=0;

    //! default constructor
    IPin()=default;

    /*!
     * \brief ISerial remove copy constructor
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    IPin(const IPin&) = delete;

    /*!
     * \brief operator = remove copy operator
     * \return
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    IPin& operator=(const IPin&) = delete;

protected:
    //! virtual destructor
    virtual ~IPin()=default;
};
#endif

#include "os.h"
class CPin
{
protected:
    bool m_bInvertedBehaviour=false;
    unsigned long m_SetupTime_uS=0;

    virtual void impl_Set(bool bHow)=0;
    virtual bool impl_RbSet()=0;
    virtual bool impl_Get()=0;

public:
    inline void Set(bool bHow)
    {
        impl_Set(m_bInvertedBehaviour ? !bHow:bHow);

        if(m_SetupTime_uS)
            os::uwait(m_SetupTime_uS);
    }
    inline bool RbSet()
    {
        bool rv=impl_RbSet();
        return m_bInvertedBehaviour ? !rv:rv;
    }
    inline bool Get()
    {
        bool rv=impl_Get();
        return m_bInvertedBehaviour ? !rv:rv;
    }

    inline void SetInvertedBehaviour(bool how)
    {
        m_bInvertedBehaviour=how;
    }
    inline void SetPinSetupTime(unsigned long nSetupTime_uS)
    {
        m_SetupTime_uS=nSetupTime_uS;
    }


    CPin()=default;
    CPin(const CPin&) = delete;
    CPin& operator=(const CPin&) = delete;

protected:
    //! virtual destructor
    virtual ~CPin()=default;
};
typedef CPin IPin;

