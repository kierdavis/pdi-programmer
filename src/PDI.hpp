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
    void send2(uint16_t word);
    void send4(uint32_t word);
    Util::MaybeUint8 recv();
  }

  enum class PtrMode : uint8_t {
    INDIRECT = 0 << 2,
    INDIRECT_INCR = 1 << 2,
    DIRECT = 2 << 2,
  };

  enum class CSReg : uint8_t {
    STATUS = 0,
    RESET = 1,
    CTRL = 2,
  };

  namespace Instruction {
    Util::MaybeUint8 lds41(uint32_t addr);
    void sts41(uint32_t addr, uint8_t data);

    void st4(PtrMode pm, uint32_t data);

    Util::MaybeUint8 ldcs(CSReg reg);
    void stcs(CSReg reg, uint8_t data);

    void repeat1(uint8_t count);
    void repeat2(uint16_t count);
    void key();
  }
}

#endif
