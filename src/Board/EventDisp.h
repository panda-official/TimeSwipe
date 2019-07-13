/*
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.
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
