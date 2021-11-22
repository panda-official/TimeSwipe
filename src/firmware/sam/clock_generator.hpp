// -*- C++ -*-

// PANDA Timeswipe Project
// Copyright (C) 2021  PANDA GmbH

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_CLOCK_GENERATOR_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_CLOCK_GENERATOR_HPP

#include <list>
#include <memory>

/*!
 * \brief A clock generators manager class
 * \details A Sam_clock_generator::Factory() used to find free clock generator, reserve it and provide class methods for setup.
 */
class Sam_clock_generator final {
public:
  /// Clock generator ID.
  enum class Id {
    MCLK,  GCLK1, GCLK2, GCLK3, GCLK4, GCLK5,
    GCLK6, GCLK7, GCLK8, GCLK9, GCLK10, GCLK11 };

  /// The destructor.
  ~Sam_clock_generator();

  /// @returns The generator ID.
  Id id() const noexcept
  {
    return static_cast<Id>(m_nCLK);
  }

  /*!
   * \brief The class factory
   * \return A pointer to the created instance
   * \details The created object will be added into the m_Clocks list
   */
  static std::shared_ptr<Sam_clock_generator> Factory();


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

private:
  /*!
   * \brief A collection of clock generator objects
   */
  static std::list<Sam_clock_generator *> m_Clocks;

  /*!
   * \brief An array of "occupied" flags: if a new object is factored, the flag with index=GCLK index will be set to "true"
   */
  static bool m_bOcupied[12];

  /*!
   * \brief An integer generator index to be used with SAME54 peripheral registers
   */
  int  m_nCLK;

  /*!
   * \brief A protected class constructor: the instances should created only by Sam_clock_generator::Factory()
   */
  Sam_clock_generator(){}
};

#endif  //  PANDA_TIMESWIPE_FIRMWARE_SAM_CLOCK_GENERATOR_HPP
