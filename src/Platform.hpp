#ifndef __PDIPROG_PLATFORM_HPP
#define __PDIPROG_PLATFORM_HPP

#include <stdbool.h>
#include <stdint.h>

#include "PDIPin.hpp"

namespace Platform {
  namespace Pin {
    void configureAsOutput(PDIPin pin, bool initialState);
    void configureAsInput(PDIPin pin);
    void write(PDIPin pin, bool state);
    bool read(PDIPin pin);
  }

  namespace TargetSerial {
    void init();
    void enableClock();
    void disableClock();
    void enableTx();
    void disableTx();
    void enableRx();
    void disableRx();
    bool rxComplete();
    bool txComplete();
    bool txBufferEmpty();
    bool rxError();
    void resetTxComplete();
    void writeData(uint8_t data);
    uint8_t readData();
  }
}

#endif
