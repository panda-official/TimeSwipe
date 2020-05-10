/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

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

