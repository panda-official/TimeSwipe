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
