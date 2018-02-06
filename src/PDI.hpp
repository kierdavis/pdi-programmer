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

  namespace Link {
    void send(uint8_t byte);
    void send4(uint32_t data);
    Util::MaybeUint8 recv();
  }

  enum class CSReg : uint8_t {
    STATUS = 0,
    RESET = 1,
    CTRL = 2,
  };

  namespace Instruction {
    Util::MaybeUint8 lds41(uint32_t addr);
    void sts41(uint32_t addr, uint8_t data);

    Util::MaybeUint8 ldcs(CSReg reg);
    void stcs(CSReg reg, uint8_t data);

    void key();
  }
}

#endif
