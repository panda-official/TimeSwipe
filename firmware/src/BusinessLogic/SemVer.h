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

/*!
 * \brief The class implements semantic versioning support
 * \details https://semver.org
 */
class CSemVer
{
protected:

    /*!
     * \brief MAJOR version (when you make incompatible API changes)
     */
    unsigned int m_nMajor;

    /*!
     * \brief MINOR version (when you add functionality in a backwards compatible manner)
     */
    unsigned int m_nMinor;

    /*!
     * \brief PATCH version (when you make backwards compatible bug fixes)
     */
    unsigned int m_nPatch;

    /*!
     * \brief Contains version string in the semver formar: "x.y.z"
     */
    std::string m_VersionString;

public:

    /*!
     * \brief The class constructor
     * \param nMajor - MAJOR version
     * \param nMinor - MINOR version
     * \param nPatch - PATCH version
     */
    CSemVer(unsigned int nMajor, unsigned int nMinor, unsigned int nPatch);

    /*!
     * \brief Returns version string in the semver formar: "x.y.z"
     * \return version string in the semver formar: "x.y.z"
     */
    std::string GetVersionString(){ return m_VersionString; }
};
