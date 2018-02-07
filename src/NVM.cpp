#include <stdint.h>

#include "NVM.hpp"
#include "PDI.hpp"
#include "TargetConfig.hpp"
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
  return TargetConfig::NVM_REGS_START + ((uint32_t) reg);
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

void NVM::Controller::execCmd(NVM::Controller::Cmd cmd) {
  NVM::Controller::writeCmd(cmd);
  NVM::Controller::writeCmdex();
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

  NVM::Controller::execCmd(NVM::Controller::Cmd::CHIPERASE);
  return Util::Status::OK;
}

Util::Status NVM::Flash::eraseSection(uint32_t addr, NVM::Flash::Section section) {
  using NVM::Controller::Cmd;
  using NVM::Flash::Section;

  Cmd cmd;
  switch (section) {
    case Section::APP:  { cmd = Cmd::ERASEAPPSEC; break; }
    case Section::BOOT: { cmd = Cmd::ERASEBOOTSEC; break; }
    default:            { return Util::Status::INVALID_SECTION; }
  }

  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(cmd);
  PDI::Instruction::sts41(addr, 0);
  return Util::Status::OK;
}

Util::Status NVM::Flash::eraseBuffer() {
  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::execCmd(NVM::Controller::Cmd::ERASEFLASHPAGEBUFF);
  return Util::Status::OK;
}

Util::Status NVM::Flash::writeBuffer(uint32_t addr, Util::ByteProviderCallback callback, uint16_t len) {
  if (len > TargetConfig::FLASH_PAGE_SIZE) {
    return Util::Status::INVALID_LENGTH;
  }

  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::LOADFLASHPAGEBUFF);

  // Set the PDI pointer to the address at which to store the first byte.
  PDI::Instruction::st4(PDI::PtrMode::DIRECT, addr);

  // Write `len` bytes using the auto-increment mode.
  PDI::Instruction::bulkSt12(PDI::PtrMode::INDIRECT_INCR, callback, len);

  return Util::Status::OK;
}

Util::Status NVM::Flash::erasePage(uint32_t addr, NVM::Flash::Section section) {
  using NVM::Controller::Cmd;
  using NVM::Flash::Section;

  Cmd cmd;
  switch (section) {
    case Section::APP:  { cmd = Cmd::ERASEAPPSECPAGE; break; }
    case Section::BOOT: { cmd = Cmd::ERASEBOOTSECPAGE; break; }
    default:            { cmd = Cmd::ERASEFLASHPAGE; break; }
  }

  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(cmd);
  PDI::Instruction::sts41(addr, 0);
  return Util::Status::OK;
}

Util::Status NVM::Flash::writePageFromBuffer(uint32_t addr, bool preErase, NVM::Flash::Section section) {
  using NVM::Controller::Cmd;
  using NVM::Flash::Section;

  Cmd cmd;
  if (preErase) {
    switch (section) {
      case Section::APP:  { cmd = Cmd::ERASEWRITEAPPSECPAGE; break; }
      case Section::BOOT: { cmd = Cmd::ERASEWRITEBOOTSECPAGE; break; }
      default:            { cmd = Cmd::ERASEWRITEFLASH; break; }
    }
  }
  else {
    switch (section) {
      case Section::APP:  { cmd = Cmd::WRITEAPPSECPAGE; break; }
      case Section::BOOT: { cmd = Cmd::WRITEBOOTSECPAGE; break; }
      default:            { cmd = Cmd::WRITEFLASHPAGE; break; }
    }
  }

  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(cmd);
  PDI::Instruction::sts41(addr, 0);
  return Util::Status::OK;
}

Util::Status NVM::Flash::writePage(uint32_t addr, Util::ByteProviderCallback callback, uint16_t len, bool preErase, NVM::Flash::Section section) {
  Util::Status status;
  status = NVM::Flash::eraseBuffer();
  if (status != Util::Status::OK) { return status; }
  status = NVM::Flash::writeBuffer(addr, callback, len);
  if (status != Util::Status::OK) { return status; }
  return NVM::Flash::writePageFromBuffer(addr, preErase, section);
}

Util::Status NVM::Flash::write(uint32_t addr, Util::ByteProviderCallback callback, uint16_t len, bool preErase, NVM::Flash::Section section) {
  while (len) {
    uint16_t chunkLen = Util::min(len, TargetConfig::FLASH_PAGE_SIZE);
    Util::Status status = NVM::Flash::writePage(addr, callback, chunkLen, preErase, section);
    if (status != Util::Status::OK) { return status; }
    addr += (uint32_t) chunkLen;
    len -= chunkLen;
  }
  return Util::Status::OK;
}

Util::Status NVM::Fuse::write(uint32_t addr, uint8_t data) {
  Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::WRITEFUSE);
  PDI::Instruction::sts41(addr, data);
  return Util::Status::OK;
}
