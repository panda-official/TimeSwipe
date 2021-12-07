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

/**
* @brief An I2C master class for communication with an external EEPROM chip
* (CAT24C32).
*
* @details This is a fully featured service version: read/write functional is
* implemented. Data is reading by using ISerial interface. The EEPROM memory
* address and the amount of data to read are set by method
* Sam_i2c_eeprom_master::SetDataAddrAndCountLim.
*/
class Sam_i2c_eeprom_master final : public Sam_sercom {
public:
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

  /// Resets EEPROM chip logic if it hangs and makes the bus busy.
  void reset_chip_logic();

  /// Performs initial bus setup (pinout, modes, speed with an initial reset).
  void setup_bus();

  /// Checks bus state and perfoms a chip reset/bus reinit if needed.
  void check_reset();

  /**
   * @brief Tests selected EEPROM area.
   *
   * @param pattern A pattern to test with.
   * @param offset A start address of EEPROM area.
   *
   * @returns `true` on success.
   */
  bool test_mem_area(CFIFO& pattern, int offset);

  /**
   * @brief Self test process.
   *
   * @returns `true` on success.
   */
  bool self_test_proc();

  /// Enables or disables IRQ mode.
  void enable_irq(bool enabled);

  /// @returns `true` if interrupt mode (SERCOM interrupt lines) are enabled.
  bool is_irq_enabled() const noexcept
  {
    return is_irq_enabled_;
  }

  /// Sets the EEPROM chip target address.
  void SetDeviceAddr(const int addr)
  {
    m_nDevAddr = addr;
  }

  /// @returns A EEPROM chip address.
  int device_address() const noexcept
  {
    return m_nDevAddr;
  }

  /**
   * @brief Sets the EEPROM base address for reading/writing data and the
   * maximum amount of data to read.
   *
   * @param base_addr An initial data address to read data from.
   * @param limit A maximum data amount to be read.
   */
  void SetDataAddrAndCountLim(const int base_addr, const int limit)
  {
    m_nMemAddr = base_addr;
    m_nReadDataCountLim = limit;
  }

  /// @returns A maximum amount of data to read out.
  int max_read_amount() const noexcept
  {
    return m_nReadDataCountLim;
  }

  /// Sets the EEPROM base address for reading/writing data.
  void SetDataAddr(const int base_addr)
  {
    m_nMemAddr = base_addr;
  }

  /// @returns A EEPROM memory address.
  int base_address() const noexcept
  {
    return m_nMemAddr;
  }

  /// @returns A current reading address.
  int current_reading_address() const noexcept
  {
    return m_nCurMemAddr;
  }

  /**
   * @brief Writes data to the set address with the maximum number m_nReadDataCountLim.
   *
   * @param msg A buffer contaning the data to write.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool send(CFIFO& msg) override;

  /**
   * @brief Gets data from the set address with the maximum number m_nReadDataCountLim.
   *
   * @param msg A buffer to receive the data.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool receive(CFIFO &msg) override;

  /**
   * @brief Starts chip self-test.
   *
   * @details Write chip with an arbitrary data, then read back and compare This is a wrapper to be used with a command processor.
   */
  void RunSelfTest(bool);

  /// @returns `true` if the last self-test operation was successful.
  bool GetSelfTestResult() const noexcept
  {
    return m_bSelfTestResult;
  }

  /// @returns The current I2C bus state.
  State state() const noexcept
  {
    return state_;
  }

  /// @returns `true` if IO operation direction is "writing".
  bool is_writing() const noexcept
  {
    return m_IOdir;
  }

  bool is_compare_read_mode() const noexcept
  {
    return m_bCmpReadMode;
  }

  /// @returns The number of EEPROM pages to write.
  int page_count_to_write() const noexcept
  {
    return m_nPageBytesLeft;
  }

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

private:
  State state_{State::halted};
  bool is_irq_enabled_{};
  bool m_IOdir{}; // IO direction: `true` means "writing".
  bool m_bSelfTestResult{};
  bool m_bCmpReadMode{};
  int m_nDevAddr{0xA0};
  int m_nMemAddr{};
  int m_nCurMemAddr{};
  int m_nReadDataCountLim{4096};
  int m_nPageBytesLeft{};

  std::shared_ptr<Sam_clock_generator> m_pCLK; // clock generator
  CFIFO *m_pBuf=nullptr; // IO buffer

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

  /// Activates or deactivates write protection pin of the chip.
  void SetWriteProtection(bool activate);

  /**
   * @brief Initiates a transfer process.
   *
   * @param dir The transfer direction: `true` to write, `false` to read.
   */
  void StartTransfer(bool dir);

  /// Rewinds internal FIFO buffer for read/write operations.
  void rewindMemBuf();

  /**
   * @brief Reads a byte from the IO buffer upon of write chip operation
   * (RAM->chip) and increments the counter.
   *
   * @returns A negative value on error.
   */
  int read_byte();

  /**
   * @brief Initiate data transfer to a next EEPROM page (RAM->chip).
   *
   * @warning Only 1 page can be written at once!
   *
   * @returns `true` on success.
   */
  bool write_next_page();

  /**
   * @brief Writes a byte to the IO buffer upon of read chip operation
   * (chip->RAM) and increments the counter.
   *
   * @param byte A byte to write.
   *
   * @returs The `byte` written, or negative value on error.
   */
  int writeB(int val);

  /**
   * @brief Perfoms send data to EEPROM operation without toggling write
   * protection pin (used in self-test cycle).
   *
   * @param msg A data to be written.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool __send(CFIFO &msg);

  /**
   * @brief Compares EEPROM content with given data.
   *
   * @param data A data to compare.
   *
   * @returns `true` on success.
   *
   * @remarks This function blocks the current thread.
   */
  bool __sendRB(CFIFO& data);
};

#endif  // PANDA_TIMESWIPE_FIRMWARE_SAM_I2C_EEPROM_MASTER_HPP
