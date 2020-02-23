/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CRGBAcol
*/

#pragma once

#include "nodeLED.h"

/*!
 * \brief A helper structure for working with RGBA colours
 */
struct CRGBAcol
{
    /*!
     * \brief Red-channel
     */
    unsigned char R;

    /*!
     * \brief Green-channel
     */
    unsigned char G;

    /*!
     * \brief Blue-channel
     */
    unsigned char B;

    /*!
     * \brief Alpha-channel
     * \details The channel determines transparency. Reserved for future use
     */
    unsigned char A;

    /*!
     * \brief The class constructor
     * \param R Red-channel
     * \param G Green-channel
     * \param B Blue-channel
     * \param A Alpha-channel
     */
    CRGBAcol(unsigned char R=0, unsigned char G=0, unsigned char B=0, unsigned char A=0)
    {
        CRGBAcol::R=R;
        CRGBAcol::G=G;
        CRGBAcol::B=B;
        CRGBAcol::A=A;

    }

    /*!
     * \brief The class constructor
     * \param col A color value in compressed 32-bit form
     */
    CRGBAcol(typeLEDcol col)
    {
        R=static_cast<unsigned char>(col>>16);
        G=static_cast<unsigned char>(col>>8);
        B=static_cast<unsigned char>(col);
    }

    /*!
     * \brief Transforms the class data to compressed 32-bit form
     */
    operator typeLEDcol () const
    {
        return LEDrgb(R,G,B);
    }

    /*!
     * \brief Multiples all colour channels by a given value
     * \param mul Multiplication factor
     * \return Transformed object
     */
    CRGBAcol operator * (float mul) const
    {
        return CRGBAcol( static_cast<unsigned char>(R*mul),
                         static_cast<unsigned char>(G*mul),
                         static_cast<unsigned char>(B*mul) );
    }
};
