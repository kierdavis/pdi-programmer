#include <stdbool.h>
#include <stdint.h>

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

    Platform::Serial::enableTx();
    Platform::Serial::disableRx();

    mode = Mode::TRANSMITTING;
  }
}

static void ensureReceiveMode() {
  if (mode != Mode::RECEIVING) {
    // Wait for transmissions to complete.
    while (!Platform::Serial::txComplete()) {}
    Platform::Serial::resetTxComplete();

    Platform::Serial::enableRx();
    Platform::Serial::disableTx();

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
  Platform::Serial::init();
}

void PDI::begin() {
  Platform::Pin::configureAsOutput(PDIPin::CLK, true);
  Platform::Pin::configureAsOutput(PDIPin::TXD, false);
  _delay_us(100);

  Platform::Pin::write(PDIPin::TXD, true);
  _delay_us(20);

  mode = Mode::TRANSMITTING;
  Platform::Serial::enableClock();
  Platform::Serial::enableTx();

  // Minimum 16 clock cycles.
  for (uint8_t i = 0; i < 18; i++) {
    waitForClockCycle();
  }
}

void PDI::end() {
  // Switch to receiving mode to ensure all pending transmissions are complete.
  ensureReceiveMode();

  // Turn off UART.
  Platform::Serial::disableRx();
  Platform::Serial::disableTx();
  Platform::Serial::disableClock();
  Platform::Serial::resetTxComplete();

  // Tri-state all pins.
  Platform::Pin::configureAsInput(PDIPin::CLK);
  Platform::Pin::configureAsInput(PDIPin::TXD);
  Platform::Pin::configureAsInput(PDIPin::RXD);
}

void PDI::send(uint8_t byte) {
  ensureTransmitMode();

  while (!Platform::Serial::txBufferEmpty()) {}
  Platform::Serial::resetTxComplete();
  Platform::Serial::writeData(byte);
}

static Util::MaybeUint8 getReceivedFrame() {
  if (Platform::Serial::rxError()) {
    return Util::MaybeUint8(Util::Status::SERIAL_ERROR);
  } else {
    uint8_t data = Platform::Serial::readData();
    return Util::MaybeUint8(Util::Status::OK, data);
  }
}

Util::MaybeUint8 PDI::recv() {
  ensureReceiveMode();

  for (uint16_t i = 0; i < PDI::TIMEOUT_CYCLES; i++) {
    if (Platform::Serial::rxComplete()) {
      return getReceivedFrame();
    }
    waitForClockCycle();
  }

  // TIMEOUT_CYCLES clock cycles passed without a frame being received.
  return Util::MaybeUint8(Util::Status::SERIAL_TIMEOUT);
}

Util::MaybeUint8 PDI::Instruction::ldcs(PDI::CSReg reg) {
  uint8_t regNum = ((uint8_t) reg) & 0xF;
  PDI::send(0x80 | regNum);
  return PDI::recv();
}
