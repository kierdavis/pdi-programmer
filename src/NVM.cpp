#include <stdint.h>

#include "NVM.hpp"
#include "PDI.hpp"
#include "Util.hpp"

void NVM::init() {
  PDI::init();
}

Util::Status NVM::begin() {
  PDI::begin();
  PDI::enterResetState();
  PDI::setGuardTime(PDI::GuardTime::_32);
  PDI::Instruction::key();
  return NVM::Controller::waitWhileBusy();
}

static Util::Status exitResetAndWait() {
  while (1) {
    PDI::exitResetState();
    Util::MaybeBool result = PDI::inResetState();
    if (!result.ok()) {
      return result.status;
    }
    if (!result.data) {
      // inResetState is false
      return Util::Status::OK;
    }
  }
}

void NVM::end() {
  // Deliberately ignore Util::Status results here - in the event of a failure
  // we should proceed with shutting down the PDI link anyway.
  NVM::Controller::waitWhileBusy();
  exitResetAndWait();
  PDI::end();
}

uint32_t NVM::Controller::regAddr(NVM::Controller::Reg reg) {
  static constexpr uint32_t BASE_ADDR = 0x010001C0;
  return BASE_ADDR + ((uint32_t) reg);
}

void NVM::Controller::writeReg(NVM::Controller::Reg reg, uint8_t data) {
  uint32_t addr = NVM::Controller::regAddr(reg);
  PDI::Instruction::sts41(addr, data);
}

void NVM::Controller::writeCmd(NVM::Controller::Cmd cmd) {
  NVM::Controller::writeReg(NVM::Controller::Reg::CMD, (uint8_t) cmd);
}

void NVM::Controller::writeCmdex() {
  static constexpr uint8_t CMDEX_MASK = 0x01;
  NVM::Controller::writeReg(NVM::Controller::Reg::CTRLA, CMDEX_MASK);
}

static Util::Status waitWhileBusBusy() {
  static constexpr uint8_t NVMEN_MASK = 0x02;

  while (1) {
    Util::MaybeUint8 result = PDI::Instruction::ldcs(PDI::CSReg::STATUS);
    if (!result.ok()) {
      return result.status;
    }
    if (result.data & NVMEN_MASK) {
      return Util::Status::OK;
    }
  }
}

static Util::Status waitWhileControllerBusy() {
  static constexpr uint8_t BUSY_MASK = 0x80;

  // Put address of STATUS register into PDI pointer register.
  uint32_t addr = NVM::Controller::regAddr(NVM::Controller::Reg::STATUS);
  PDI::Instruction::st4(PDI::PtrMode::DIRECT, addr);

  // Poll STATUS register until BUSY flag is no longer set.
  while (1) {
    Util::MaybeUint8 result = PDI::Instruction::ld1(PDI::PtrMode::INDIRECT);
    if (!result.ok()) {
      return result.status;
    }
    if (!(result.data & BUSY_MASK)) {
      return Util::Status::OK;
    }
  }
}

Util::Status NVM::Controller::waitWhileBusy() {
  Util::Status status = waitWhileBusBusy();
  if (status != Util::Status::OK) {
    return status;
  }
  return waitWhileControllerBusy();
}

Util::Status NVM::read(uint32_t addr, uint8_t * buffer, uint16_t len) {
  if (len == 0) { return Util::Status::OK; }

  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::READNVM);

  // Set the PDI pointer to the address of the first byte.
  PDI::Instruction::st4(PDI::PtrMode::DIRECT, addr);

  // Read `len` bytes using the auto-increment mode.
  return PDI::Instruction::bulkLd12(PDI::PtrMode::INDIRECT_INCR, buffer, len);
}

Util::Status NVM::eraseChip() {
  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::CHIPERASE);
  NVM::Controller::cmdex();
  return Util::Status::OK;
}
