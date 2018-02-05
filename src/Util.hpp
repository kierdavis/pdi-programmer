#ifndef __PDIPROG_UTIL_HPP
#define __PDIPROG_UTIL_HPP

#include <stdint.h>

namespace Util {
  enum class Status : uint8_t {
    OK,
    SERIAL_ERROR,
    UNKNOWN_ERROR,
  };
}

#endif
