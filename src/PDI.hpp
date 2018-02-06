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
}

#endif
