#include <stdbool.h>
#include <stdint.h>

#include "NVM.hpp"
#include "PDI.hpp"
#include "TargetConfig.hpp"
#include "Util.hpp"

static bool activeFlag = false;

void NVM::init() {
  activeFlag = false;
  PDI::init();
}

void NVM::begin() {
  PDI::begin();
  PDI::enterResetState();
  PDI::setGuardTime(PDI::GuardTime::_32);
  PDI::Instruction::key();
  activeFlag = true;
}

static Util::Status exitResetAndWait() {
  while (1) {
    PDI::exitResetState();
    const Util::MaybeBool result = PDI::inResetState();
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
  activeFlag = false;
  // Deliberately ignore Util::Status results here - in the event of a failure
  // we should proceed with shutting down the PDI link anyway.
  NVM::Controller::waitWhileBusy();
  exitResetAndWait();
  PDI::end();
}

bool NVM::active() {
  return activeFlag;
}

uint32_t NVM::Controller::regAddr(const NVM::Controller::Reg reg) {
  return TargetConfig::NVM_REGS_START + ((uint32_t) reg);
}

void NVM::Controller::writeReg(const NVM::Controller::Reg reg, const uint8_t data) {
  const uint32_t addr = NVM::Controller::regAddr(reg);
  PDI::Instruction::sts41(addr, data);
}

void NVM::Controller::writeCmd(const NVM::Controller::Cmd cmd) {
  NVM::Controller::writeReg(NVM::Controller::Reg::CMD, (uint8_t) cmd);
}

void NVM::Controller::writeCmdex() {
  static constexpr uint8_t CMDEX_MASK = 0x01;
  NVM::Controller::writeReg(NVM::Controller::Reg::CTRLA, CMDEX_MASK);
}

void NVM::Controller::execCmd(const NVM::Controller::Cmd cmd) {
  NVM::Controller::writeCmd(cmd);
  NVM::Controller::writeCmdex();
}

static Util::Status waitWhileBusBusy() {
  static constexpr uint8_t NVMEN_MASK = 0x02;

  while (1) {
    const Util::MaybeUint8 result = PDI::Instruction::ldcs(PDI::CSReg::STATUS);
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
  const uint32_t addr = NVM::Controller::regAddr(NVM::Controller::Reg::STATUS);
  PDI::Instruction::st4(PDI::PtrMode::DIRECT, addr);

  // Poll STATUS register until BUSY flag is no longer set.
  while (1) {
    const Util::MaybeUint8 result = PDI::Instruction::ld1(PDI::PtrMode::INDIRECT);
    if (!result.ok()) {
      return result.status;
    }
    if (!(result.data & BUSY_MASK)) {
      return Util::Status::OK;
    }
  }
}

Util::Status NVM::Controller::waitWhileBusy() {
  const Util::Status status = waitWhileBusBusy();
  if (status != Util::Status::OK) {
    return status;
  }
  return waitWhileControllerBusy();
}

Util::Status NVM::read(const uint32_t addr, uint8_t * const buffer, const uint16_t len) {
  if (len == 0) { return Util::Status::OK; }

  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::READNVM);

  // Set the PDI pointer to the address of the first byte.
  PDI::Instruction::st4(PDI::PtrMode::DIRECT, addr);

  // Read `len` bytes using the auto-increment mode.
  return PDI::Instruction::bulkLd12(PDI::PtrMode::INDIRECT_INCR, buffer, len);
}

Util::Status NVM::eraseChip() {
  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::execCmd(NVM::Controller::Cmd::CHIPERASE);
  return Util::Status::OK;
}

static uint32_t realFlashAddr(const uint32_t flashAddr, const NVM::Flash::Section section) {
  using NVM::Flash::Section;

  switch (section) {
    case Section::APP:  { return TargetConfig::FLASH_APP_START + flashAddr; }
    case Section::BOOT: { return TargetConfig::FLASH_BOOT_START + flashAddr; }
    default:            { return TargetConfig::FLASH_START + flashAddr; }
  }
}

Util::Status NVM::Flash::eraseSection(const uint32_t flashAddr, const NVM::Flash::Section section) {
  using NVM::Controller::Cmd;
  using NVM::Flash::Section;

  const uint32_t addr = realFlashAddr(flashAddr, section);

  Cmd cmd;
  switch (section) {
    case Section::APP:  { cmd = Cmd::ERASEAPPSEC; break; }
    case Section::BOOT: { cmd = Cmd::ERASEBOOTSEC; break; }
    default:            { return Util::Status::INVALID_SECTION; }
  }

  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(cmd);
  PDI::Instruction::sts41(addr, 0);
  return Util::Status::OK;
}

Util::Status NVM::Flash::eraseBuffer() {
  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::execCmd(NVM::Controller::Cmd::ERASEFLASHPAGEBUFF);
  return Util::Status::OK;
}

Util::Status NVM::Flash::writeBuffer(const uint32_t flashAddr, const Util::ByteProviderCallback callback, const uint16_t len, const NVM::Flash::Section section) {
  if (len > TargetConfig::FLASH_PAGE_SIZE) {
    return Util::Status::INVALID_LENGTH;
  }

  const uint32_t addr = realFlashAddr(flashAddr, section);

  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::LOADFLASHPAGEBUFF);

  // Set the PDI pointer to the address at which to store the first byte.
  PDI::Instruction::st4(PDI::PtrMode::DIRECT, addr);

  // Write `len` bytes using the auto-increment mode.
  PDI::Instruction::bulkSt12(PDI::PtrMode::INDIRECT_INCR, callback, len);

  return Util::Status::OK;
}

Util::Status NVM::Flash::erasePage(const uint32_t flashAddr, const NVM::Flash::Section section) {
  using NVM::Controller::Cmd;
  using NVM::Flash::Section;

  const uint32_t addr = realFlashAddr(flashAddr, section);

  Cmd cmd;
  switch (section) {
    case Section::APP:  { cmd = Cmd::ERASEAPPSECPAGE; break; }
    case Section::BOOT: { cmd = Cmd::ERASEBOOTSECPAGE; break; }
    default:            { cmd = Cmd::ERASEFLASHPAGE; break; }
  }

  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(cmd);
  PDI::Instruction::sts41(addr, 0);
  return Util::Status::OK;
}

Util::Status NVM::Flash::writePageFromBuffer(const uint32_t flashAddr, const bool preErase, const NVM::Flash::Section section) {
  using NVM::Controller::Cmd;
  using NVM::Flash::Section;

  const uint32_t addr = realFlashAddr(flashAddr, section);

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

  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(cmd);
  PDI::Instruction::sts41(addr, 0);
  return Util::Status::OK;
}

Util::Status NVM::Flash::writePage(const uint32_t flashAddr, const Util::ByteProviderCallback callback, const uint16_t len, const bool preErase, const NVM::Flash::Section section) {
  const Util::Status eraseStatus = NVM::Flash::eraseBuffer();
  if (eraseStatus != Util::Status::OK) { return eraseStatus; }
  const Util::Status writeStatus = NVM::Flash::writeBuffer(flashAddr, callback, len, section);
  if (writeStatus != Util::Status::OK) { return writeStatus; }
  return NVM::Flash::writePageFromBuffer(flashAddr, preErase, section);
}

Util::Status NVM::Flash::write(const uint32_t flashAddr, const Util::ByteProviderCallback callback, const uint16_t len, const bool preErase, const NVM::Flash::Section section) {
  uint32_t currFlashAddr = flashAddr;
  uint16_t currLen = len;
  while (currLen) {
    const uint16_t chunkLen = Util::min(currLen, TargetConfig::FLASH_PAGE_SIZE);
    const Util::Status status = NVM::Flash::writePage(currFlashAddr, callback, chunkLen, preErase, section);
    if (status != Util::Status::OK) { return status; }
    currFlashAddr += (uint32_t) chunkLen;
    currLen -= chunkLen;
  }
  return Util::Status::OK;
}

Util::Status NVM::Fuse::write(const uint8_t fuseAddr, const uint8_t data) {
  const uint32_t addr = TargetConfig::FUSE_START + ((uint32_t) fuseAddr);

  const Util::Status status = NVM::Controller::waitWhileBusy();
  if (status != Util::Status::OK) { return status; }

  NVM::Controller::writeCmd(NVM::Controller::Cmd::WRITEFUSE);
  PDI::Instruction::sts41(addr, data);
  return Util::Status::OK;
}
