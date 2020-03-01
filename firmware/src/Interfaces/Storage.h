/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#pragma once

#include <typeinfo>
class  CStorage
{
protected:
    bool m_bDownloading=false;

    virtual void __ser(void *pVar, const std::type_info &ti)=0;

    template<typename type>
    void vser(type &var){
        __ser(&var, typeid(var) );
    }

public:
    bool IsDownloading(){ return m_bDownloading; }

    CStorage & ser(bool &val)
    {
        vser<bool>(val);
        return *this;
    }
    CStorage & ser(int &val)
    {
        vser<int>(val);
        return *this;
    }
    CStorage & ser(unsigned int &val)
    {
        vser<unsigned int>(val);
         return *this;
    }
    CStorage & ser(float &val)
    {
        vser<float>(val);
        return *this;
    }
};

struct ISerialize
{
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

