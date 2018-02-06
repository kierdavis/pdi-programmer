#ifndef __PDIPROG_UTIL_HPP
#define __PDIPROG_UTIL_HPP

#include <stdint.h>

namespace Util {
  enum class Status : uint8_t {
    OK,
    SERIAL_ERROR,
    SERIAL_TIMEOUT,
    UNKNOWN_ERROR,
  };

  class MaybeUint8 {
  public:
    Status status;
    uint8_t data;
    MaybeUint8(Status status_ = Status::UNKNOWN_ERROR, uint8_t data_ = 0)
      : status(status_), data(data_) {}
    bool ok() { return status == Status::OK; }
  };
}

#endif
