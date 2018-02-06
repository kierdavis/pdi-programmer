#ifndef __PDIPROG_PDI_HPP
#define __PDIPROG_PDI_HPP

#include <stdbool.h>
#include <stdint.h>

#include "Util.hpp"

namespace PDI {
  static constexpr uint32_t BAUD_RATE = 2000000;
  static constexpr uint16_t TIMEOUT_CYCLES = 1024;

  void init();
  void begin();
  void end();
  void send(uint8_t byte);
  Util::MaybeUint8 recv();

  void send4(uint32_t data);

  enum class CSReg : uint8_t {
    STATUS = 0,
    RESET = 1,
    CTRL = 2,
  };

  namespace Instruction {
    Util::MaybeUint8 lds41(uint32_t addr);

    Util::MaybeUint8 ldcs(CSReg reg);
    void stcs(CSReg reg, uint8_t data);
  }
}

#endif
