/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/



#pragma once
#include <typeinfo>

/*!
*   @file
*   @brief A definition file for basic storage class and object serialization interface:
*   CStorage, ISerialize
*
*/

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
     * \brief The flag defining if data is downloading(=true) from the storage or uploading to the storage(=false)
     */
    bool m_bDownloading=false;

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
     * \brief Is data downloading from the storage or uploading to the storage
     * \return true=data is downloading, false=data is uploading
     */
    bool IsDownloading(){ return m_bDownloading; }

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
     * \param st A reference to the storage from which the object content is downloading or uploading to
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

