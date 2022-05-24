/*
This Source Code Form is subject to the terms of the GNU General Public License v3.0.
If a copy of the GPL was not distributed with this
file, You can obtain one at https://www.gnu.org/licenses/gpl-3.0.html
Copyright (c) 2019-2020 Panda Team
*/

#pragma once

#include "clock_generator.hpp"

#include <optional>

/// A possible SAME5x TC device.
enum class typeSamTC { Tc0, Tc1, Tc2, Tc3, Tc4, Tc5, Tc6, Tc7 };

/// A SAME5x basic Timer/Counter (TC).
class CSamTC {
protected:
    /*!
     * \brief The TC ID
     */
    typeSamTC m_nTC;

    /*!
     * \brief Enables internal SAM's communication bus with TC peripheral
     * \param nTC The TC's ID
     * \param how treu=enable, false=disable
     */
    void EnableAPBbus(typeSamTC nTC, bool how);

public:
    /*!
     * \brief The class constructor
     * \param nTC The TC's ID
     */
    CSamTC(typeSamTC nTC);

    /*!
     * \brief Returns the ID of current TC object instance
     * \return The TC's ID
     */
    typeSamTC GetID(){return m_nTC;}


    /*!
     * \brief Enables interrupt handling for the TC
     * \param how true=enable, false=disable
     */
    void EnableIRQ(bool how);

    /*!
     * \brief Enables internal SAM's communication bus with TC peripheral for that instance
     * \param how true=enable, false=disable
     */
    void EnableAPBbus(bool how){ EnableAPBbus(m_nTC, how); }

    /*!
     * \brief Connects a clock generator to TC device
     * \param nCLK A clock generator ID
     */
    void ConnectGCLK(std::optional<Sam_clock_generator::Id> id);
};
