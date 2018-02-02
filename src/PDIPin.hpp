#ifndef __PDIPROG_PDI_PIN_HPP
#define __PDIPROG_PDI_PIN_HPP

#include <stdint.h>

enum class PDIPin : uint8_t {
  CLK,
  TXD,
  RXD,
};

#endif
