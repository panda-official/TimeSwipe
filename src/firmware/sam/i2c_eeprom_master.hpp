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

#ifndef PANDA_TIMESWIPE_FIRMWARE_SAM_I2C_EEPROM_MASTER_HPP
#define PANDA_TIMESWIPE_FIRMWARE_SAM_I2C_EEPROM_MASTER_HPP

#include "sercom.hpp"

#include <cstdint>

/// I2C bus state.
enum I2c_bus_state : std::uint16_t {
  /**
   * State is unknown to the I2C master and will wait for a Stop condition
   * to be detected or wait to be forced into an Idle state by software.
   */
  i2c_bus_unknown = 0x0,
  /// Waiting for a transaction to be initialized.
  i2c_bus_idle = 0x1,
  /// The I2C master is the current owner of the bus.
  i2c_bus_owner = 0x2,
  /// Some other I2C master owns the bus.
  i2c_bus_busy = 0x3
};

/**
* @brief An I2C master for communication with an external (real) EEPROM chip
* CAT24C32 (CAT2430).
*/
class Sam_i2c_eeprom_master final : public Sam_sercom {
public:
  /// IO direction.
  enum class Io_direction { read, write };

  /**
   * @brief I2C bus state according to the communication algorithm.
   *
   * @see CAT24C32 manual
   */
  enum class State {
    /// Stopped, idle state.
    halted,
    /// A start/repeated start condition met.
    start,
    /// A high address byte written.
    addrHb,
    /// A low address byte written.
    addrLb,
    /// Continuous data read mode until the EOF.
    read,
    /// Continuous data write mode.
    write,
    /// An error occurred during transmission.
    errTransfer,
    /// An error occured during data comparison.
    errCmp
  };

  /**
   * @brief The class constructor.
   *
   * @details The constructor does the following:
   *   - calls Sam_sercom constructor;
   *   - enables communication bus with corresponding SERCOM;
   *   - setups corresponding PINs and its multiplexing;
   *   - turns SERCOM to I2C master;
   *   - performs final tuning and enables SERCOM I2 master.
   */
  Sam_i2c_eeprom_master();

  /// Enables or disables IRQ mode.
  void enable_irq(bool enabled);

  /// @returns `true` if interrupt mode (SERCOM interrupt lines) are enabled.
  bool is_irq_enabled() const noexcept
  {
    return is_irq_enabled_;
  }

  /// Sets the EEPROM chip target address.
  void set_eeprom_chip_address(const int addr)
  {
    eeprom_chip_address_ = addr;
  }

  /// @returns A EEPROM chip address.
  int eeprom_chip_address() const noexcept
  {
    return eeprom_chip_address_;
  }

  /// Sets the EEPROM base address for reading/writing data.
  void set_eeprom_base_address(const int base_addr)
  {
    eeprom_base_address_ = base_addr;
  }

  /// @returns A EEPROM memory address.
  int eeprom_base_address() const noexcept
  {
    return eeprom_base_address_;
  }

  /**
   * @brief The maximum amount of data to read out.
   *
   * @param amount A maximum data amount to be read.
   */
  void set_eeprom_max_read_amount(const int amount)
  {
    eeprom_max_read_amount_ = amount;
  }

  /// @returns A maximum amount of data to read out.
  int eeprom_max_read_amount() const noexcept
  {
    return eeprom_max_read_amount_;
  }

  /**
   * @brief Starts chip self-test.
   *
   * @details Write chip with an arbitrary data, then read back and compare.
   * This is a wrapper to be used with a command processor.
   */
  void run_self_test(bool);

  /// @returns `true` if the last self-test operation was successful.
  bool self_test_result() const noexcept
  {
    return self_test_result_;
  }

  /**
   * @brief Writes data to the set address.
   *
   * @details `data` size cannot be greater than eeprom_max_read_amount().
   *
   * @param data A buffer contaning the data to write.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool send(CFIFO& data) override;

  /**
   * @brief Gets data from the set address.
   *
   * @details `data` size cannot be greater than eeprom_max_read_amount().
   *
   * @param data A buffer to receive the data.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool receive(CFIFO& data) override;

private:
  State state_{State::halted};
  bool is_irq_enabled_{};
  Io_direction io_direction_{Io_direction::read};
  bool self_test_result_{};
  bool is_compare_read_mode_{};
  int eeprom_chip_address_{0xA0};
  int eeprom_base_address_{};
  int eeprom_current_address_{};
  int eeprom_max_read_amount_{4096};
  int page_bytes_left_{};
  std::shared_ptr<Sam_clock_generator> clock_generator_;
  CFIFO* io_buffer_{};
  bool is_irq_handled_{}; // FIXME: remove. This is for debugging.

  /// @returns The size of the EEPROM page.
  static constexpr int page_size() noexcept
  {
    return 16;
  }

  /// @return An operation timeout, ms.
  static constexpr unsigned long operation_timeout() noexcept
  {
    return 500;
  }

  /// Resets EEPROM chip logic if it hangs and makes the bus busy.
  void reset_chip_logic();

  /**
   * @brief Performs initial bus setup (pinout, modes, speed with an initial reset).
   *
   * @see Registers are described in section 36.10 of the manual.
   */
  void setup_bus();

  /// Checks chip & bus states and perfoms a chip reset and/or bus reinit if needed.
  void check_reset();

  /// Activates or deactivates write protection pin of the chip.
  void set_write_protection(bool activate);

  /// Initiates the data transfer.
  void start_transfer(Io_direction dir);

  /**
   * @brief Reads a byte from the IO buffer upon of write chip operation
   * (RAM->chip) and increments the counter.
   *
   * @returns A negative value on error.
   */
  int read_byte_from_io_buffer();

  /**
   * @brief Writes a byte to the IO buffer upon of read chip operation
   * (chip->RAM) and increments the counter.
   *
   * @param byte A byte to write.
   *
   * @returs The `byte` written, or negative value on error.
   */
  int write_byte_to_io_buffer(int byte);

  /**
   * @brief Initiate data transfer to a next EEPROM page (RAM->chip).
   *
   * @warning Only 1 page can be written at once!
   *
   * @returns `true` on success.
   */
  bool write_next_page();

  /**
   * @brief Submits `data` to EEPROM without toggling write protection pin.
   *
   * @param data A data to be written.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool submit__(CFIFO& data);

  /**
   * @brief Reads back and compares EEPROM content with given `data`.
   *
   * @param data A data to compare.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool read_back_and_compare__(CFIFO& data);

  /// I2C bus IRQ handler.
  void handle_irq();

  /// @see Sam_sercom::handle_irq0();
  void handle_irq0() override;

  /// @see Sam_sercom::handle_irq1();
  void handle_irq1() override;

  /// @see Sam_sercom::handle_irq2();
  void handle_irq2() override;

  /// @see Sam_sercom::handle_irq3();
  void handle_irq3() override;
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_I2C_EEPROM_MASTER_HPP
