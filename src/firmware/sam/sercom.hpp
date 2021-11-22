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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_SERCOM_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_SERCOM_HPP

#include "../../serial.hpp"
#include "clock_generator.hpp"

#include <optional>

extern "C" {
void SERCOM0_0_Handler();
void SERCOM0_1_Handler();
void SERCOM0_2_Handler();
void SERCOM0_3_Handler();
void SERCOM1_0_Handler();
void SERCOM1_1_Handler();
void SERCOM1_2_Handler();
void SERCOM1_3_Handler();
void SERCOM2_0_Handler();
void SERCOM2_1_Handler();
void SERCOM2_2_Handler();
void SERCOM2_3_Handler();
void SERCOM3_0_Handler();
void SERCOM3_1_Handler();
void SERCOM3_2_Handler();
void SERCOM3_3_Handler();
void SERCOM4_0_Handler();
void SERCOM4_1_Handler();
void SERCOM4_2_Handler();
void SERCOM4_3_Handler();
void SERCOM5_0_Handler();
void SERCOM5_1_Handler();
void SERCOM5_2_Handler();
void SERCOM5_3_Handler();
void SERCOM6_0_Handler();
void SERCOM6_1_Handler();
void SERCOM6_2_Handler();
void SERCOM6_3_Handler();
void SERCOM7_0_Handler();
void SERCOM7_1_Handler();
void SERCOM7_2_Handler();
void SERCOM7_3_Handler();
}

/**
 * @brief An implementation of SAME54 basic Serial Communication Interface.
 *
 * @details Depending on settings it can be turned into USART, SPI, I2C-master
 * or I2C-slave. The functionality of SERCOM is provided by handling interrupts,
 * enabling and connecting corresponding Sam_clock_generator.
 */
class Sam_sercom : virtual public CSerial {
public:
  /// SAME5x Sercom ID.
  enum class Id {
    sercom0, sercom1, sercom2, sercom3,
    sercom4, sercom5, sercom6, sercom7 };

  /// SAME5x Sercom IRQ.
  enum class Irq { irq0, irq1, irq2, irq3 };

  /// The destructor.
  ~Sam_sercom() override;

  /**
   * @brief The constructor.
   *
   * @details Connects Sercom object by the given `id` to the corresponding slot
   * to handle Cortex M/SAME5x.
   */
  Sam_sercom(Id id);

  /// @returns Sercom ID.
  Id id() const noexcept
  {
    return id_;
  }

  /// Enables or disables the given IRQ line.
  void enable_irq(Irq irq, bool enable);

  /// Enables or disables internal communication bus.
  void enable_internal_bus(bool enable);

  /// Connects the given clock generator to this Sercom device.
  void connect_clock_generator(std::optional<Sam_clock_generator::Id> type);

  /// Line 1 IRQ handler.
  virtual void handle_irq0() = 0;

  /// Line 2 IRQ handler.
  virtual void handle_irq1() = 0;

  /// Line 3 IRQ handler.
  virtual void handle_irq2() = 0;

  /// Line 4 IRQ handler.
  virtual void handle_irq3() = 0;

private:
  Id id_;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_SERCOM_HPP
