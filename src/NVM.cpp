#include <stdint.h>

#include "NVM.hpp"
#include "PDI.hpp"
#include "Util.hpp"

uint32_t NVM::Controller::regAddr(NVM::Controller::Reg reg) {
  static constexpr uint32_t BASE_ADDR = 0x010001C0;
  return BASE_ADDR + ((uint32_t) reg);
}

void NVM::Controller::writeReg(NVM::Controller::Reg reg, uint8_t data) {
  uint32_t addr = NVM::Controller::regAddr(reg);
  PDI::Instruction::sts41(addr, data);
}

Util::Status NVM::Controller::waitWhileBusBusy() {
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

Util::Status NVM::Controller::waitWhileControllerBusy() {
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
