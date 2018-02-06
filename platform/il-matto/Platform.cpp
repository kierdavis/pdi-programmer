#include <stdbool.h>
#include <stdint.h>

#include <avr/io.h>

#include "PDI.hpp"
#include "PDIPin.hpp"
#include "Platform.hpp"

static volatile uint8_t * const DDR = &DDRD;
static volatile uint8_t * const PORT = &PORTD;
static volatile uint8_t * const PIN = &PIND;

static const uint8_t CLK_INDEX = 4;
static const uint8_t TXD_INDEX = 3;
static const uint8_t RXD_INDEX = 2;

static const uint8_t CLK_MASK = _BV(CLK_INDEX);
static const uint8_t TXD_MASK = _BV(TXD_INDEX);
static const uint8_t RXD_MASK = _BV(RXD_INDEX);

static uint8_t pinMask(PDIPin pin) {
  switch (pin) {
    case PDIPin::CLK: return CLK_MASK;
    case PDIPin::TXD: return TXD_MASK;
    case PDIPin::RXD: return RXD_MASK;
    default:          return 0; // unreachable
  }
}

void Platform::Pin::configureAsOutput(PDIPin pin, bool initialState) {
  Platform::Pin::write(pin, initialState);
  *DDR |= pinMask(pin);
}

void Platform::Pin::configureAsInput(PDIPin pin) {
  *DDR &= ~pinMask(pin);
  *PORT &= ~pinMask(pin);
}

void Platform::Pin::write(PDIPin pin, bool state) {
  if (state) {
    *PORT |= pinMask(pin);
  } else {
    *PORT &= ~pinMask(pin);
  }
}

bool Platform::Pin::read(PDIPin pin) {
  return *PIN & pinMask(pin);
}

void Platform::Serial::init() {
  UBRR1 = (F_CPU / (2 * PDI::BAUD_RATE)) - 1;
  UCSR1A = 0;
  UCSR1B = 0;
  UCSR1C = _BV(UPM11) | _BV(USBS1) | _BV(UCSZ11) | _BV(UCSZ10) | _BV(UCPOL1);
}

void Platform::Serial::enableClock() {
  UCSR1C |= _BV(UMSEL10);
}

void Platform::Serial::disableClock() {
  UCSR1C &= ~_BV(UMSEL10);
}

void Platform::Serial::enableTx() {
  UCSR1B |= _BV(TXEN1);
}

void Platform::Serial::disableTx() {
  UCSR1B &= ~_BV(TXEN1);
}

void Platform::Serial::enableRx() {
  UCSR1B |= _BV(RXEN1);
}

void Platform::Serial::disableRx() {
  UCSR1B &= ~_BV(RXEN1);
}

bool Platform::Serial::rxComplete() {
  return UCSR1A & _BV(RXC1);
}

bool Platform::Serial::txComplete() {
  return UCSR1A & _BV(TXC1);
}

bool Platform::Serial::txBufferEmpty() {
  return UCSR1A & _BV(UDRE1);
}

bool Platform::Serial::rxError() {
  return UCSR1A & (_BV(FE1) | _BV(DOR1) | _BV(UPE1));
}

void Platform::Serial::resetTxComplete() {
  UCSR1A |= _BV(TXC1);
}

void Platform::Serial::writeData(uint8_t data) {
  UDR1 = data;
}

uint8_t Platform::Serial::readData() {
  return UDR1;
}
