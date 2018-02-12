#include <stdbool.h>
#include <stdint.h>

#include "NVM.hpp"
#include "Platform.hpp"
#include "Util.hpp"

namespace Request {
  static constexpr uint8_t NOP = 0x00;
  static constexpr uint8_t WRITE_APP_FLASH = 0x02;
  static constexpr uint8_t WRITE_FUSE = 0x03;
  static constexpr uint8_t ERASE_CHIP = 0x04;
  static constexpr uint8_t END = 0xFF;
}

namespace Response {
  static constexpr uint8_t OK = 0x00;

  static constexpr uint8_t INVALID_REQUEST = 0x01;

  static constexpr uint8_t INTERNAL_ERROR = 0xFE;
  static constexpr uint8_t UNKNOWN_ERROR = 0xFF;
}

static void send(const uint8_t data) {
  while (!Platform::ClientSerial::txBufferEmpty()) {}
  Platform::ClientSerial::writeData(data);
}

static uint8_t recv() {
  while (!Platform::ClientSerial::rxComplete()) {}
  const uint8_t data = Platform::ClientSerial::readData();
  send(0xFF);
  return data;
}

static uint16_t recv2() {
  union {
    uint8_t bytes[2];
    uint16_t word;
  } u;
  for (uint8_t i = 0; i < 2; i++) {
    u.bytes[i] = recv();
  }
  return u.word;
}

static uint32_t recv4() {
  union {
    uint8_t bytes[4];
    uint32_t word;
  } u;
  for (uint8_t i = 0; i < 4; i++) {
    u.bytes[i] = recv();
  }
  return u.word;
}

static uint8_t statusToResponse(const Util::Status status) {
  switch (status) {
    case Util::Status::OK: {
      return Response::OK;
    }
    default: {
      return Response::INTERNAL_ERROR;
    }
  }
  return Response::UNKNOWN_ERROR;
}

static void ensureNVMActive() {
  if (!NVM::active()) {
    NVM::begin();
  }
}

static void ensureNVMInactive() {
  if (NVM::active()) {
    NVM::end();
  }
}

static uint8_t dispatch(const uint8_t request) {
  switch (request) {
    case Request::NOP: {
      return Response::OK;
    }
    case Request::ERASE_CHIP: {
      ensureNVMActive();
      return statusToResponse(NVM::eraseChip());
    }
    case Request::WRITE_APP_FLASH: {
      const uint32_t addr = recv4();
      const uint16_t len = recv2();
      ensureNVMActive();
      return statusToResponse(NVM::Flash::write(
        addr,
        recv,
        len,
        false,
        NVM::Flash::Section::APP
      ));
    }
    case Request::WRITE_FUSE: {
      const uint8_t addr = recv();
      const uint8_t data = recv();
      ensureNVMActive();
      return statusToResponse(NVM::Fuse::write(addr, data));
    }
    case Request::END: {
      ensureNVMInactive();
      return Response::OK;
    }
    default: {
      return Response::INVALID_REQUEST;
    }
  }
  return Response::UNKNOWN_ERROR;
}

int main() {
  Platform::ClientSerial::init();
  NVM::init();

  while (1) {
    const uint8_t request = recv();
    const uint8_t response = dispatch(request);
    send(response);
  }
}