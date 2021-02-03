/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019 Panda Team
*/

/*!
*   \file
*   \brief A definition file for
*   typeSamCLK and CSamCLK
*/

#pragma once

#include <list>
#include <memory>

/*!
 * \brief An enumeration of possible clock generators
 */
enum class typeSamCLK : int {none=-1,  MCLK=0, GCLK1, GCLK2, GCLK3, GCLK4, GCLK5, GCLK6, GCLK7, GCLK8, GCLK9, GCLK10, GCLK11};


/*!
 * \brief A clock generators manager class
 * \details A CSamCLK::Factory() used to find free clock generator, reserve it and provide class methods for setup.
 */
class CSamCLK
{
protected:

    /*!
     * \brief A collection of clock generator objects
     */
    static std::list<CSamCLK *> m_Clocks;

    /*!
     * \brief An array of "occupied" flags: if a new object is factored, the flag with index=GCLK index will be set to "true"
     */
    static bool m_bOcupied[12];

    /*!
     * \brief An integer generator index to be used with SAME54 peripheral registers
     */
    int  m_nCLK;

    /*!
     * \brief A protected class constructor: the instances should created only by CSamCLK::Factory()
     */
    CSamCLK(){}

public:

    /*!
     * \brief Returns clock index as the typeSamCLK enumeration type
     * \return
     */
    typeSamCLK CLKind(){return static_cast<typeSamCLK>(m_nCLK);}

    /*!
     * \brief The class factory
     * \return A pointer to the created instance
     * \details The created object will be added into the m_Clocks list
     */
    static std::shared_ptr<CSamCLK> Factory();

    /*!
     * \brief The class destructor
     * \details The object will be removed from the m_Clocks list
     */
    ~CSamCLK();

    /*!
     * \brief Waiting until bus synchronization is completed
     * \details Required for some GCLK operations, please see SAME54 manual for details:
     * "Due to asynchronicity between the main clock domain and the peripheral clock domains, some registers
        need to be synchronized when written or read." page 159
     */
    void WaitSync();

    /*!
     * \brief Sets clock generator output frequency divider
     * \param div A divider value
     * \details "The Generator clock frequency equals the clock source frequency divided by 2^(N+1), where
       N is the Division Factor Bits for the selected generator" page 165
     */
    void SetDiv(unsigned short div);

    /*!
     * \brief Enables the generator
     * \param how true=enabled, false=disabled
     */
    void Enable(bool how);

};
