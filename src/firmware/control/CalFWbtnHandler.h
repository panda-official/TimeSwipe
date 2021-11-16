/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CCalFWbtnHandler
*/

#pragma once

#include "../button.h"
#include "View.h"

class CCalFWbtnHandler  : public CButtonEvent
{
public:

    /*!
    * \brief A handler for button events
    * \param nState current button state
    */
    virtual void OnButtonState(typeButtonState nState)
    {
        if(typeButtonState::released==nState)
            return;

        CView::Instance().BreakCalUItest();
    }

    /*!
     * \brief Starts Calibration UI test
     */
    void StartUItest(bool bHow)
    {
        CView &v=CView::Instance();
        if(bHow)
            v.CalUItest();
        else
            v.BreakCalUItest();
    }

    /*!
     * \brief Shows whether Calibration UI test has been done(true) or not (false)
     */
    bool HasUItestBeenDone()
    {
        return CView::Instance().HasCalUItestBeenDone();
    }
};
