/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/


#include <stdio.h>
#include "SemVer.h"


CSemVer::CSemVer(unsigned int nMajor, unsigned int nMinor, unsigned int nPatch)
{
    m_nMajor=nMajor;
    m_nMinor=nMinor;
    m_nPatch=nPatch;

    char tbuf[128];
    sprintf(tbuf, "%d.%d.%d", nMajor, nMinor, nPatch);
    m_VersionString=tbuf;
}
