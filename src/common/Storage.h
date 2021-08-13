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

/**
 * @file
 * Basic storage class and object serialization interface
 */

#ifndef PANDA_TIMESWIPE_COMMON_STORAGE_HPP
#define PANDA_TIMESWIPE_COMMON_STORAGE_HPP

#include <typeinfo>

/*!
 * \brief   The base class defining an interface for a persistent data storage
 *
 * \details The class defines methods for serialization of primitive data types: bool, int, unsigned int, float
 *
 * \todo    Extend/add stored data types
 *
 */
class  CStorage
{
protected:
    /*!
     * \brief The flag defining if data is importing(=true) from the storage or exporting to the storage(=false)
     */
    bool m_bImporting=false;

    bool m_bDefaultSettingsOrder=false;

    /*!
     * \brief The virtual method must be overridden to provide an implementation of storing primitive data type
     * \param pVar a pointer to the data primitive
     * \param ti the data primitive type information (in C++ RTTI form)
     */
    virtual void __ser(void *pVar, const std::type_info &ti)=0;

    /*!
     * \brief The helper template function used to transform serialization method for current data type to uniform __ser(void *pVar, const std::type_info &ti) call
     */
    template<typename type>
    void vser(type &var){
        __ser(&var, typeid(var) );
    }

public:
    /*!
     * \brief Is data importing from the storage or exporting to the storage
     * \return true=data is importing, false=data is exporting
     */
    bool IsImporting(){ return m_bImporting; }

    bool IsDefaultSettingsOrder(){ return m_bDefaultSettingsOrder; }

    /*!
     * \brief The serialization method for a bool
     * \param val A data primitive to serialize (variable)
     * \return A reference to *this
     */
    CStorage & ser(bool &val)
    {
        vser<bool>(val);
        return *this;
    }

    /*!
     * \brief The serialization method for an int
     * \param val A data primitive to serialize (variable)
     * \return A reference to *this
     */
    CStorage & ser(int &val)
    {
        vser<int>(val);
        return *this;
    }

    /*!
     * \brief The serialization method for an unsigned int
     * \param val A data primitive to serialize (variable)
     * \return A reference to *this
     */
    CStorage & ser(unsigned int &val)
    {
        vser<unsigned int>(val);
         return *this;
    }

    /*!
     * \brief The serialization method for a float
     * \param val A data primitive to serialize (variable)
     * \return A reference to *this
     */
    CStorage & ser(float &val)
    {
        vser<float>(val);
        return *this;
    }
};

/*!
 * \brief A callback interface used to seialize the content of the derived class
 */
struct ISerialize
{
    /*!
     * \brief Should provide the serialization of the object content
     * \param st A reference to the storage from which the object content is importing or exporting to
     */
    virtual void Serialize(CStorage &st)=0;

    //! default constructor
    ISerialize()=default;

    /*!
     * \brief remove copy constructor
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
    ISerialize(const ISerialize&) = delete;

    /*!
     * \brief remove copy operator
     * \return
     * \details forbid copying by referencing only to this interface (by default it will be copied only this class part
     *  that is unacceptable)
     */
     ISerialize& operator=(const ISerialize&) = delete;

protected:
    //! virtual destructor
    virtual ~ISerialize()=default;

};

#endif  // PANDA_TIMESWIPE_COMMON_STORAGE_HPP
