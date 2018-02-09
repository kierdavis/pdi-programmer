#include <stdbool.h>
#include <stdint.h>

#include <avr/pgmspace.h>
#include <util/delay.h>

#include "PDI.hpp"
#include "PDIPin.hpp"
#include "Platform.hpp"
#include "Util.hpp"

enum class Mode : uint8_t {
  NEITHER,
  TRANSMITTING,
  RECEIVING,
};

static Mode mode = Mode::NEITHER;

static void waitForClockCycle() {
  while (Platform::Pin::read(PDIPin::CLK)) {}
  while (!Platform::Pin::read(PDIPin::CLK)) {}
  while (Platform::Pin::read(PDIPin::CLK)) {}
}

static void ensureTransmitMode() {
  if (mode != Mode::TRANSMITTING) {
    // Wait for a clock cycle.
    waitForClockCycle();

    Platform::Pin::configureAsOutput(PDIPin::TXD, true);

    Platform::TargetSerial::enableTx();
    Platform::TargetSerial::disableRx();

    mode = Mode::TRANSMITTING;
  }
}

static void ensureReceiveMode() {
  if (mode != Mode::RECEIVING) {
    // Wait for transmissions to complete.
    while (!Platform::TargetSerial::txComplete()) {}
    Platform::TargetSerial::resetTxComplete();

    Platform::TargetSerial::enableRx();
    Platform::TargetSerial::disableTx();

    Platform::Pin::configureAsInput(PDIPin::TXD);

    mode = Mode::RECEIVING;
  }
}

void PDI::init() {
  mode = Mode::NEITHER;

  // Configure initial pin modes and states.
  Platform::Pin::configureAsInput(PDIPin::CLK);
  Platform::Pin::configureAsInput(PDIPin::TXD);
  Platform::Pin::configureAsInput(PDIPin::RXD);
  Platform::TargetSerial::init();
}

void PDI::begin() {
  Platform::Pin::configureAsOutput(PDIPin::CLK, true);
  Platform::Pin::configureAsOutput(PDIPin::TXD, false);
  _delay_us(100);

  Platform::Pin::write(PDIPin::TXD, true);
  _delay_us(20);

  mode = Mode::TRANSMITTING;
  Platform::TargetSerial::enableClock();
  Platform::TargetSerial::enableTx();

  // Minimum 16 clock cycles.
  for (uint8_t i = 0; i < 18; i++) {
    waitForClockCycle();
  }
}

void PDI::end() {
  // Switch to receiving mode to ensure all pending transmissions are complete.
  ensureReceiveMode();

  // Turn off UART.
  Platform::TargetSerial::disableRx();
  Platform::TargetSerial::disableTx();
  Platform::TargetSerial::disableClock();
  Platform::TargetSerial::resetTxComplete();

  // Tri-state all pins.
  Platform::Pin::configureAsInput(PDIPin::CLK);
  Platform::Pin::configureAsInput(PDIPin::TXD);
  Platform::Pin::configureAsInput(PDIPin::RXD);
}

void PDI::Link::send(uint8_t byte) {
  ensureTransmitMode();

  while (!Platform::TargetSerial::txBufferEmpty()) {}
  Platform::TargetSerial::resetTxComplete();
  Platform::TargetSerial::writeData(byte);
}

void PDI::Link::send2(uint16_t word) {
  uint8_t * const bytes = (uint8_t *) &word;
  PDI::Link::send(bytes[0]);
  PDI::Link::send(bytes[1]);
}

void PDI::Link::send4(uint32_t word) {
  uint8_t * const bytes = (uint8_t *) &word;
  PDI::Link::send(bytes[0]);
  PDI::Link::send(bytes[1]);
  PDI::Link::send(bytes[2]);
  PDI::Link::send(bytes[3]);
}

static Util::MaybeUint8 getReceivedFrame() {
  if (Platform::TargetSerial::rxError()) {
    return Util::MaybeUint8(Util::Status::SERIAL_ERROR);
  } else {
    uint8_t data = Platform::TargetSerial::readData();
    return Util::MaybeUint8(Util::Status::OK, data);
  }
}

Util::MaybeUint8 PDI::Link::recv() {
  ensureReceiveMode();

  for (uint16_t i = 0; i < PDI::TIMEOUT_CYCLES; i++) {
    if (Platform::TargetSerial::rxComplete()) {
      return getReceivedFrame();
    }
    waitForClockCycle();
  }

  // TIMEOUT_CYCLES clock cycles passed without a frame being received.
  return Util::MaybeUint8(Util::Status::SERIAL_TIMEOUT);
}

Util::MaybeUint8 PDI::Instruction::lds41(uint32_t addr) {
  PDI::Link::send(0x0C);
  PDI::Link::send4(addr);
  return PDI::Link::recv();
}

void PDI::Instruction::sts41(uint32_t addr, uint8_t data) {
  PDI::Link::send(0x4C);
  PDI::Link::send4(addr);
  PDI::Link::send(data);
}

Util::MaybeUint8 PDI::Instruction::ld1(PDI::PtrMode pm) {
  uint8_t pmMask = ((uint8_t) pm) & 0xC;
  PDI::Link::send(0x20 | pmMask);
  return PDI::Link::recv();
}

Util::Status PDI::Instruction::bulkLd12(PDI::PtrMode pm, uint8_t * buffer, uint16_t len) {
  // Check for shortcuts.
  if (len == 0) { return Util::Status::OK; }
  if (len == 1) {
    Util::MaybeUint8 result = PDI::Instruction::ld1(pm);
    if (!result.ok()) { return result.status; }
    buffer[0] = result.data;
    return Util::Status::OK;
  }
  // Send the REPEAT instruction.
  // `len` cannot be 0, so `len - 1` cannot underflow.
  PDI::Instruction::repeat2(len - 1);
  // Send the LD instruction byte.
  uint8_t pmMask = ((uint8_t) pm) & 0xC;
  PDI::Link::send(0x20 | pmMask);
  // The target device executes LD `len` times, responding with the same number
  // of bytes.
  for (uint16_t i = 0; i < len; i++) {
    Util::MaybeUint8 result = PDI::Link::recv();
    if (!result.ok()) { return result.status; }
    buffer[i] = result.data;
  }
  return Util::Status::OK;
}

void PDI::Instruction::st1(PDI::PtrMode pm, uint8_t data) {
  uint8_t pmMask = ((uint8_t) pm) & 0xC;
  PDI::Link::send(0x60 | pmMask);
  PDI::Link::send(data);
}

void PDI::Instruction::st4(PDI::PtrMode pm, uint32_t data) {
  uint8_t pmMask = ((uint8_t) pm) & 0xC;
  PDI::Link::send(0x63 | pmMask);
  PDI::Link::send4(data);
}

void PDI::Instruction::bulkSt12(PDI::PtrMode pm, Util::ByteProviderCallback callback, uint16_t len) {
  // Check for shortcuts.
  if (len == 0) { return; }
  if (len != 1) {
    // Send the REPEAT instruction.
    // `len` cannot be 0, so `len - 1` cannot underflow.
    PDI::Instruction::repeat2(len - 1);
  }
  // Send the ST instruction byte.
  uint8_t pmMask = ((uint8_t) pm) & 0xC;
  PDI::Link::send(0x60 | pmMask);
  // The target device executes ST `len` times, accepting the same number of
  // bytes.
  for (uint16_t i = 0; i < len; i++) {
    uint8_t byte = callback();
    PDI::Link::send(byte);
  }
}

Util::MaybeUint8 PDI::Instruction::ldcs(PDI::CSReg reg) {
  uint8_t regNum = ((uint8_t) reg) & 0xF;
  PDI::Link::send(0x80 | regNum);
  return PDI::Link::recv();
}

void PDI::Instruction::stcs(PDI::CSReg reg, uint8_t data) {
  uint8_t regNum = ((uint8_t) reg) & 0xF;
  PDI::Link::send(0xC0 | regNum);
  PDI::Link::send(data);
}

void PDI::Instruction::repeat1(uint8_t count) {
  PDI::Link::send(0xA0);
  PDI::Link::send(count);
}

void PDI::Instruction::repeat2(uint16_t count) {
  PDI::Link::send(0xA1);
  PDI::Link::send2(count);
}

void PDI::Instruction::key() {
  static constexpr uint8_t LEN = 9;
  static const uint8_t BYTES[LEN] PROGMEM = {
    0xE0, // Instruction byte
    0xFF, 0x88, 0xD8, 0xCD,
    0x45, 0xAB, 0x89, 0x12,
  };
  for (uint8_t i = 0; i < LEN; i++) {
    PDI::Link::send(pgm_read_byte(&BYTES[i]));
  }
}

void PDI::enterResetState() {
  static constexpr uint8_t RESET_SIGNATURE = 0x59;
  PDI::Instruction::stcs(PDI::CSReg::RESET, RESET_SIGNATURE);
}

void PDI::exitResetState() {
  PDI::Instruction::stcs(PDI::CSReg::RESET, 0);
}

Util::MaybeBool PDI::inResetState() {
  static constexpr uint8_t MASK = 0x01;

  Util::MaybeUint8 result = PDI::Instruction::ldcs(PDI::CSReg::RESET);
  bool inResetState = result.data & MASK;
  return Util::MaybeBool(result.status, inResetState);
}

void PDI::setGuardTime(PDI::GuardTime gt) {
  uint8_t data = ((uint8_t) gt) & 0x7;
  PDI::Instruction::stcs(PDI::CSReg::CTRL, data);
}
