#ifndef __PDIPROG_PDI_HPP
#define __PDIPROG_PDI_HPP

#include <stdbool.h>
#include <stdint.h>

#include "Util.hpp"

namespace PDI {
  static constexpr uint32_t BAUD_RATE = 2000000;

  class RecvResult {
  public:
    Util::Status status;
    uint8_t data;
    RecvResult(Util::Status status_ = Util::Status::UNKNOWN_ERROR, uint8_t data_ = 0)
      : status(status_), data(data_) {}
    bool ok() { return status == Util::Status::OK; }
  };

  void init();
  void begin();
  void end();
  void send(uint8_t byte);
  RecvResult recv();
}

#endif
