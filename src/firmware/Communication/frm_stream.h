/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   @file
*   @brief A basic formatted stream class
*   CFrmStream
*
*/

#pragma once

#include "Serial.h"

/*!
 * \brief A formatted stream class.
 * \details Provides a mechanism for retrieving/storing
 *  primitive data types (int, float, std::string, e.t.c) from/to the stream.
 *  similar to ios::ios, but standard library is too heavy
 */
class CFrmStream
{
protected:

    /*!
    * \brief A pointer to FIFO buffer used as stream-buffer
    */
   CFIFO *m_pBuf=nullptr;

   /*!
    * \brief Actual parsing error status (true=active)
    */
   bool  m_bErr=false;

   /*!
    * \brief Extraction operator helper
    * \param pVar void pointer to an extracted variable
    * \param ti variable type
    */
   virtual void get(void *pVar, const std::type_info &ti);

   /*!
    * \brief Insertion operator helper
    * \param pVar void pointer to an inserted variable
    * \param ti variable type
    */
   virtual void set(const void *pVar, const std::type_info &ti);

   //! start tocken used for string extraction
   typeSChar m_chStartTocken=' ';

   //! end tocken used for string extraction
   typeSChar m_chEndTocken='\0';

   /*!
    * \brief Extract a string from the stream
    * \param str Filled with extracted string (IN parameter)
    * \return Operation result: true - succesfull extraction, false - failure
    */
   bool fetch_string(std::string &str);

   template<typename type>
   void frm_vget(type &var){
       get(&var, typeid(var) );
   }
   template<typename type>
   void frm_vset(type &var){        //ref????
       set(&var, typeid(var) );
   }

public:
   /*!
     * \brief Returns the status of the last parsing operation
     * \return
     */
    bool bad(){return m_bErr;}

    CFrmStream(CFIFO *pBuf)
    {
        m_pBuf=pBuf;
    }

    CFrmStream & operator <<(typeSChar ch)
    {
         frm_vset<typeSChar>(ch);
         return *this;
    }
    CFrmStream & operator >>(typeSChar &ch)
    {
         frm_vget<typeSChar>(ch);
         return *this;
    }
    CFrmStream & operator <<(const char *ch)
    {
         frm_vset<const char *>(ch);
         return *this;
    }
    CFrmStream & operator >>(char *ch)
    {
         frm_vget<char *>(ch);
         return *this;
    }

    //03.06.2019 - string support:
    CFrmStream & operator <<(const std::string &str)
    {
         frm_vset<const std::string>(str);
         return *this;
    }
    CFrmStream & operator >>(std::string &str)
    {
         frm_vget<std::string>(str);
         return *this;
    }


    CFrmStream & operator <<(unsigned int val)
    {
        frm_vset<unsigned int>(val);
         return *this;
    }
    CFrmStream & operator >>(unsigned int &val)
    {
         frm_vget<unsigned int>(val);
         return *this;
    }
    CFrmStream & operator <<(float val)
    {
        frm_vset<float>(val);
         return *this;
    }
    CFrmStream & operator >>(float &val)
    {
         frm_vget<float>(val);
         return *this;
    }
    CFrmStream & operator <<(bool val)
    {
        frm_vset<bool>(val);
         return *this;
    }
    CFrmStream & operator >>(bool &val)
    {
        frm_vget<bool>(val);
         return *this;
    }
};
