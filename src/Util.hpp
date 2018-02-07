#ifndef __PDIPROG_UTIL_HPP
#define __PDIPROG_UTIL_HPP

#include <stdbool.h>
#include <stdint.h>

namespace Util {
  static uint16_t min(uint16_t x, uint16_t y) {
    return (x < y) ? x : y;
  }

  enum class Status : uint8_t {
    OK,
    SERIAL_ERROR,
    SERIAL_TIMEOUT,
    INVALID_LENGTH,
    INVALID_SECTION,
    UNKNOWN_ERROR,
  };

  class MaybeBool {
  public:
    Status status;
    bool data;
    MaybeBool(Status status_ = Status::UNKNOWN_ERROR, uint8_t data_ = false)
      : status(status_), data(data_) {}
    bool ok() { return status == Status::OK; }
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
