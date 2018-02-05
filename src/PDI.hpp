#ifndef __PDIPROG_PDI_HPP
#define __PDIPROG_PDI_HPP

#include <stdint.h>

#include "Util.hpp"

namespace PDI {
  class RecvResult {
  public:
    Util::Status status;
    uint8_t data;
    RecvResult(Util::Status status_, uint8_t data_)
      : status(status_), data(data_) {}
  };

  void init();
  void begin();
  void send(uint8_t byte);
  RecvResult recv();
}

#endif