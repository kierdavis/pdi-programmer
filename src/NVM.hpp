#ifndef __PDIPROG_NVM_HPP
#define __PDIPROG_NVM_HPP

#include "Util.hpp"

namespace NVM {
  void init();
  Util::Status begin();
  void end();

  namespace Controller {
    enum class Reg : uint8_t {
      CMD = 0x0A,
      CTRLA = 0x0B,
      STATUS = 0x0F,
    };

    uint32_t regAddr(Reg reg);
    void writeReg(Reg reg, uint8_t data);

    Util::Status waitWhileBusy();
  }
}

#endif
