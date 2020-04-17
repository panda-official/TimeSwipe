/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   CSemVer
*/

#pragma once

#include <string>

class CSemVer
{
protected:
    unsigned int m_nMajor;
    unsigned int m_nMinor;
    unsigned int m_nPatch;

    std::string m_VersionString;

public:
    CSemVer(unsigned int nMajor, unsigned int nMinor, unsigned int nPatch);
    std::string GetVersionString(){ return m_VersionString; }
};
