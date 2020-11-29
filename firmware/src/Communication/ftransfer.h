/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CCmdFTransferHandler
*/

#pragma once

#include "cmd.h"

template<typename typeClass>
class CCmdFTransferHandler : public CCmdCallHandler
{
protected:
    std::shared_ptr<typeClass> m_pObj;
    int (typeClass::*m_pFGetter)(CFrmStream &Stream, unsigned int nStartPos, unsigned int nRead);

public:
    CCmdFTransferHandler(const std::shared_ptr<typeClass> &pObj, int (typeClass::*pFGetter)(CFrmStream &Stream, unsigned int nStartPos, unsigned int nRead))
    {
        m_pObj=pObj;
        m_pFGetter=pFGetter;
    }

    /*!
     * \brief A command dispatcher handler override for this class
     * \param d An uniform command request descriptor.
     * \return The operation result
     */
    virtual typeCRes Call(CCmdCallDescr &d)
    {
        if(d.m_ctype & CCmdCallDescr::ctype::ctSet) //set
        {
             //error:
             return typeCRes::fset_not_supported;
        }

        unsigned int fpos;
        unsigned int flen;

        *(d.m_pIn)>>fpos>>flen;
        if( d.m_pIn->bad())
            return typeCRes::parse_err;

        d.m_pOut->push('f'); //set "file" marker
        (m_pObj.get()->*m_pFGetter)(*(d.m_pOut), fpos, flen);
    }
};
