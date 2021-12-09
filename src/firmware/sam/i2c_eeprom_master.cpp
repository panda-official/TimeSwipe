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

#include "../error.hpp"
#include "../os.h"
#include "i2c_eeprom_master.hpp"
#include "pin.hpp"

#include <sam.h>

// #define EEPROM_8BIT_ADDR

Sercom* glob_GetSercomPtr(Sam_sercom::Id);

namespace {

#if defined(__SAME54P20A__)
constexpr int eeprom_pin_group = Sam_pin::Group::d;
constexpr int eeprom_pad0_pin_number = Sam_pin::Number::p09;
constexpr int eeprom_pad1_pin_number = Sam_pin::Number::p08;
constexpr int eeprom_pad2_pin_number = Sam_pin::Number::p10;
constexpr int eeprom_peripheral_function = Sam_pin::Peripheral_function::pfd;
#elif defined(__SAME53N19A__)
constexpr int eeprom_pin_group = Sam_pin::Group::c;
constexpr int eeprom_pad0_pin_number = Sam_pin::Number::p16;
constexpr int eeprom_pad1_pin_number = Sam_pin::Number::p17;
constexpr int eeprom_pad2_pin_number = Sam_pin::Number::p18;
constexpr int eeprom_peripheral_function = Sam_pin::Peripheral_function::pfc;
#else
#error Unsupported SAM
#endif

/// @returns Pointer to master section.
inline SercomI2cm* sam_i2cm(const Sam_sercom::Id id) noexcept
{
  return &(glob_GetSercomPtr(id)->I2CM);
}

inline void sync_bus(SercomI2cm* const bus) noexcept
{
  while (bus->SYNCBUSY.bit.SYSOP);
}

} // namespace

Sam_i2c_eeprom_master::Sam_i2c_eeprom_master()
  : Sam_sercom{Id::sercom6}
{
  PORT->Group[eeprom_pin_group].DIRSET.reg = (1L<<eeprom_pad2_pin_number);
  set_write_protection(true);

  enable_internal_bus(true);
  clock_generator_ = Sam_clock_generator::make();
  PANDA_TIMESWIPE_FIRMWARE_ASSERT(clock_generator_);

  connect_clock_generator(clock_generator_->id());
  clock_generator_->enable(true);

  setup_bus();
}

void Sam_i2c_eeprom_master::enable_irq(const bool enabled)
{
  SercomI2cm* const i2cm = sam_i2cm(id());
  is_irq_enabled_ = enabled;
  if (is_irq_enabled_)
    i2cm->INTENSET.reg =
      SERCOM_I2CM_INTENSET_MB |   // master on bus
      SERCOM_I2CM_INTENSET_SB |   // slave on bus
      SERCOM_I2CM_INTENSET_ERROR; // bus error
  else
    // Clear all.
    i2cm->INTENCLR.reg = SERCOM_I2CM_INTENSET_MASK;

  // Tune NVIC.
  for (const auto irq : {Irq::irq0, Irq::irq1, Irq::irq2, Irq::irq3})
    Sam_sercom::enable_irq(irq, is_irq_enabled_);
}

void Sam_i2c_eeprom_master::run_self_test(bool)
{
  /*
   * @brief Tests selected EEPROM area.
   *
   * @param pattern A pattern to test with.
   * @param base_addr A start address of EEPROM area.
   *
   * @returns `true` on success.
   */
  const auto is_mem_area_ok = [this](CFIFO& pattern, const int base_addr)
  {
    CFIFO buf;

    pattern.rewind();
    const auto pattern_size = static_cast<std::size_t>(pattern.in_avail());
    buf.reserve(pattern_size);

    const auto prev_addr = eeprom_base_address_;
    eeprom_base_address_ = base_addr;

    if (!submit__(pattern)) {
      eeprom_base_address_ = prev_addr;
      return false;
    }

    // Some delay is required.
    os::wait(10);

    const bool rb{receive(buf)};
    eeprom_base_address_ = prev_addr;
    if (!rb)
      return false;

    // Compare.
    for (int k{}; k < pattern_size; ++k)
      if (buf[k] != pattern[k])
        return false;

    return true;
  };

  // Perform self-test.
  set_write_protection(false);
  self_test_result_ = [&]
  {
    CFIFO pattern;
    for (int i{}; i < page_size(); ++i)
      pattern << 0xA5;

    // Test first page.
    if (!is_mem_area_ok(pattern, 0))
      return false;

    // Test last page.
    if (!is_mem_area_ok(pattern, eeprom_max_read_amount_ - page_size()))
      return false;

    return true;
  }();
  set_write_protection(true);
}

bool Sam_i2c_eeprom_master::send(CFIFO& data)
{
  constexpr int write_retries{3};
  bool result{};
  set_write_protection(false);
  for (int i{}; i < write_retries; ++i) {
    data.rewind();
    if (submit__(data)) {
      // Some delay required.
      os::wait(10);
      data.rewind();
      if ( (result = read_back_and_compare__(data)))
        break;
    }
  }
  set_write_protection(true);
  return result;
}

bool Sam_i2c_eeprom_master::receive(CFIFO& data)
{
  eeprom_current_address_ = eeprom_base_address_;
  io_buffer_ = &data;
  is_compare_read_mode_ = false;
  start_transfer(Io_direction::read);
  const auto start_time = os::get_tick_mS();
  while (state_ != State::halted && state_ != State::errTransfer) {
    if (os::get_tick_mS() - start_time > operation_timeout())
      break;
    os::wait(1);
  }
  io_buffer_ = nullptr;
  return state_ == State::halted;
}

void Sam_i2c_eeprom_master::reset_chip_logic()
{
  // Disconnect pins from I2C bus since we cannot use this interface.
  PORT->Group[eeprom_pin_group].PINCFG[eeprom_pad1_pin_number].bit.PMUXEN = 0;
  PORT->Group[eeprom_pin_group].PINCFG[eeprom_pad0_pin_number].bit.PMUXEN = 0;

  // Perform a manual 10-period clock sequence to reset the chip.
  constexpr auto bits = (1L<<eeprom_pad1_pin_number);
  PORT->Group[eeprom_pin_group].OUTCLR.reg = bits;
  for (int i{}; i < 10; ++i) {
    PORT->Group[eeprom_pin_group].DIRSET.reg = bits; // should go to 0.
    os::wait(1);
    PORT->Group[eeprom_pin_group].DIRCLR.reg = bits; // back by pull up.
    os::wait(1);
  }
}

void Sam_i2c_eeprom_master::setup_bus()
{
  // SCL.
  PORT->Group[eeprom_pin_group].PMUX[4].bit.PMUXE = eeprom_peripheral_function;
  PORT->Group[eeprom_pin_group].PINCFG[eeprom_pad1_pin_number].bit.PMUXEN = 1; // enable

  // SDA.
  PORT->Group[eeprom_pin_group].PMUX[4].bit.PMUXO = eeprom_peripheral_function;
  PORT->Group[eeprom_pin_group].PINCFG[eeprom_pad0_pin_number].bit.PMUXEN = 1; // enable

  /*
   * "Violating the protocol may cause the I2C to hang. If this happens it is
   * possible to recover from this state by a software Reset (CTRLA.SWRST='1').",
   * page 913.
   */
  SercomI2cm* const i2cm = sam_i2cm(id());
  while (i2cm->SYNCBUSY.bit.SWRST);
  i2cm->CTRLA.bit.SWRST = 1;
  while (i2cm->CTRLA.bit.SWRST);

  // Select the I2C master serial communication interface of the SERCOM.
  i2cm->CTRLA.bit.MODE = 0x05;

  //setup:
  //i2cm->CTRLB.bit.SMEN=1;    //smart mode is enabled
  //i2cm->CTRLA.bit.SCLSM=1;
  i2cm->CTRLA.bit.INACTOUT = 1;  // 55uS shoud be enough
  i2cm->CTRLB.bit.ACKACT = 0;    //sending ACK after the bit is received (auto)
  i2cm->BAUD.bit.BAUD = 0xff;
  // i2cm->BAUD.bit.BAUDLOW=0 ; //0xFF;

  //! if it was an IRQ mode don't forget to restart it:

  if (is_irq_enabled_)
    enable_irq(true);

  //enable:
  i2cm->CTRLA.bit.ENABLE = 1;

  while (!i2cm->STATUS.bit.BUSSTATE) {
    sync_bus(i2cm);
    i2cm->STATUS.bit.BUSSTATE = i2c_bus_idle;
  }
}

void Sam_i2c_eeprom_master::check_reset()
{
  SercomI2cm* const i2cm = sam_i2cm(id());
  if (i2cm->STATUS.bit.BUSSTATE == i2c_bus_busy) {
    // Chip is hanging.
    reset_chip_logic();
    setup_bus();
  }
}

void Sam_i2c_eeprom_master::set_write_protection(const bool activate)
{
  constexpr auto bits = (1L<<eeprom_pad2_pin_number);
  if (activate) {
    PORT->Group[eeprom_pin_group].OUTSET.reg = bits;
    os::uwait(100); // wait till real voltage level rise of fall
  } else {
    os::uwait(100); // wait till real voltage level rise of fall
    PORT->Group[eeprom_pin_group].OUTCLR.reg = bits;
  }
}

void Sam_i2c_eeprom_master::start_transfer(const Io_direction dir)
{
  SercomI2cm* const i2cm = sam_i2cm(id());
  check_reset();
  io_direction_ = dir;
  state_ = State::start;
  sync_bus(i2cm);
  // Set "ACK" action.
  i2cm->CTRLB.bit.ACKACT = 0;
  sync_bus(i2cm);
  // Initiate a transfer sequence.
  i2cm->ADDR.bit.ADDR = eeprom_chip_address_;
}

int Sam_i2c_eeprom_master::read_byte_from_io_buffer()
{
  if (page_bytes_left_ <= 0 || !io_buffer_ || !io_buffer_->in_avail())
    return -1;

  Character ch;
  *io_buffer_ >> ch;
  --page_bytes_left_;
  return ch;
}

int Sam_i2c_eeprom_master::write_byte_to_io_buffer(const int byte)
{
  if (!io_buffer_)
    return -1;

  if (is_compare_read_mode_) {
    if (!io_buffer_->in_avail())
      return -1;

    Character ch;
    *io_buffer_ >> ch;
    if (ch != byte) {
      state_ = State::errCmp;
      return -1;
    }
  } else {
    if (io_buffer_->size() >= eeprom_max_read_amount_) // memory protection
      return -1;

    *io_buffer_ << byte;
  }

  return byte;
}

bool Sam_i2c_eeprom_master::write_next_page()
{
  const int pbl{page_size() - eeprom_current_address_ % page_size()};
  page_bytes_left_ = pbl;
  start_transfer(Io_direction::write);
  const auto start_time = os::get_tick_mS();
  while (state_ != State::halted && state_ != State::errTransfer) {
    if (os::get_tick_mS() - start_time > operation_timeout())
      return false;
    // os::wait(1); //regulate the cpu load here
  }
  if (state_ == State::halted) {
    eeprom_current_address_ += pbl;
    return true;
  }
  return false;
}

bool Sam_i2c_eeprom_master::submit__(CFIFO& data)
{
  eeprom_current_address_ = eeprom_base_address_; // rewind mem addr
  io_buffer_ = &data;
  bool result{};
  auto start_time = os::get_tick_mS();
  do {
    if ( (result = write_next_page()))
      start_time = os::get_tick_mS();
  } while (data.in_avail() && (os::get_tick_mS() - start_time < operation_timeout()));
  io_buffer_ = nullptr;
  return result;
}

bool Sam_i2c_eeprom_master::read_back_and_compare__(CFIFO& data)
{
  eeprom_current_address_ = eeprom_base_address_;
  io_buffer_ = &data;
  is_compare_read_mode_ = true;
  start_transfer(Io_direction::read);
  const auto start_time = os::get_tick_mS();
  while (state_ != State::halted && state_ != State::errTransfer) {
    if (os::get_tick_mS() - start_time > operation_timeout())
      break;
    os::wait(1);
  }
  io_buffer_ = nullptr;
  return state_ == State::halted;
}

void Sam_i2c_eeprom_master::handle_irq()
{
  is_irq_handled_ = true;
  SercomI2cm* const i2cm = sam_i2cm(id());

  sync_bus(i2cm);

  // Proc an error: LENERR, SEXTTOUT, MEXTTOUT, LOWTOUT, ARBLOST, and BUSERR.
  if (i2cm->INTFLAG.bit.ERROR) {
    i2cm->STATUS.reg = 0xff; // clear status ?
    i2cm->INTFLAG.bit.ERROR = 1;
    state_ = State::errTransfer; //???
    return;
  }

  // Master on bus.
  if (i2cm->INTFLAG.bit.MB)  {
    if (i2cm->STATUS.bit.ARBLOST || i2cm->STATUS.bit.RXNACK) {
      // Stop the communication.
      state_ = State::errTransfer;
      i2cm->CTRLB.bit.CMD = 0x3; // stop
      //i2cm->INTFLAG.bit.MB=1;
      return;
      //how the flags will be reset? by a new start?
      //"This bit is automatically cleared when writing to the ADDR register" (Manual)
    }

    switch (state_) {
    case State::start:
      //set address HB:
#ifdef EEPROM_8BIT_ADDR
      state_ = State::addrLb;
      i2cm->DATA.bit.DATA = eeprom_current_address_ / page_size();
#else
      state_ = State::addrHb;
      i2cm->DATA.bit.DATA = (eeprom_current_address_>>8);
#endif
      return;
    case State::addrHb:
      state_ = State::addrLb;
      i2cm->DATA.bit.DATA = (eeprom_current_address_ & 0xff);
      return;
    case State::addrLb:
      // After setting the address switch the IO direction.
      if (io_direction_ == Io_direction::read) {
        // Initiate repeated start for read.
        state_ = State::read;
        i2cm->ADDR.bit.ADDR = eeprom_chip_address_ + 1;
      } else
        // Continue writing.
        state_ = State::write;
      return;
    case State::write:
      // Write data until the end.
      if (const int val = read_byte_from_io_buffer(); val < 0) {
        // End reached.
        state_ = State::halted;
        i2cm->CTRLB.bit.CMD = 0x3; // stop
      } else
        i2cm->DATA.bit.DATA = val;
      return;
    default:
      i2cm->INTFLAG.bit.MB = 1;
      return;
    }
  }

  // Slave on bus.
  if (i2cm->INTFLAG.bit.SB) {
    // If data is "ended" at the slave (setting NACK).
    if (i2cm->STATUS.bit.RXNACK) {
      state_ = State::halted;
      i2cm->CTRLB.bit.CMD = 0x3; //stop
      return;
    }

    // Read data until the end.
    if (write_byte_to_io_buffer(i2cm->DATA.bit.DATA) < 0) { //EOF
      if (state_ != State::errCmp)
        state_ = State::halted;

      i2cm->CTRLB.bit.ACKACT = 1; //setting "NACK" to the chip
      sync_bus(i2cm);
      i2cm->CTRLB.bit.CMD = 0x3; //stop
      return;
    }
    i2cm->CTRLB.bit.CMD = 0x2;
    i2cm->INTFLAG.bit.SB = 1; // clear the Slave on Bus flag
    return;
  }
}

void Sam_i2c_eeprom_master::handle_irq0()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::handle_irq1()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::handle_irq2()
{
  handle_irq();
}

void Sam_i2c_eeprom_master::handle_irq3()
{
  handle_irq();
}
