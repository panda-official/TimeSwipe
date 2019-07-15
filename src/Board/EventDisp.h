/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

#pragma once

#include "json_evsys.h"

class CEvDisp : public CJSONEvDispatcher
{
protected:
     virtual void RaiseEventFlag(bool how);

public:
    CEvDisp(const std::shared_ptr<CCmdDispatcher> &pDisp);
};


/*#ifndef EVENTDISP_H
#define EVENTDISP_H

#endif // EVENTDISP_H*/
