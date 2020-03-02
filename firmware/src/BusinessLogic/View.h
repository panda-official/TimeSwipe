/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CView
*/


#pragma once

#include "board_type.h"
#include "rgbacol.h"
#include "nodeLED.h"

/*!
 * \brief The application View class
 * \details The class can be considered as somewhat usually called "view" in MCV pattern.
 * It should determine the overall behaviour of board visualization elements.
 * The actual view instance is accesible via Instance() method.
 *
 * \todo This is a prototype of a view class. All colour constants and visualization routines should be moved here.
 * Currently designed as "singleton", but the possibility of changing views can be added.
 * The CView::Instance() that returns current view must remain.
 *
 */
class CView
{
protected:

    //! board basic colours in a compressed form:
    static const constexpr typeLEDcol DMS_COLOR = LEDrgb(24, 250, 208);
    static const constexpr typeLEDcol IEPE_COLOR = LEDrgb(73, 199, 255);

    //! current board basic color
    typeLEDcol m_BasicBoardCol;

public:

    /*!
     * \brief The current view access interface
     * \return The current view
     */
    static CView& Instance()
    {
       static CView singleton;
       return singleton;
    }

    /*!
     * \brief Sets the current board type
     * \param nBrd The board type
     * \todo should be placed to nodeControl or some super-control class
     */
    void SetupBoardType(typeBoard nBrd)
    {
        m_BasicBoardCol=typeBoard::DMSBoard==nBrd ? DMS_COLOR : IEPE_COLOR;
    }

    /*!
     * \brief Returns the current board basic color
     * \return Current board basic color
     */
    CRGBAcol GetBasicColor() const {return m_BasicBoardCol;}


    /*!
     * \brief The view routine: "Hello" blinking at board startup
     */
    void BlinkAtStart() const
    {
        nodeLED::blinkMultipleLED(typeLED::LED1, typeLED::LED4, m_BasicBoardCol, 2, 300);
    }

private:
        /*!
         * \brief Private class constructor
         */
        CView(){SetupBoardType(typeBoard::IEPEBoard);}

        //! Forbid copy constructor
        CView(const CView&)=delete;

        //! Forbid copying
        CView& operator=(const CView&)=delete;
};
