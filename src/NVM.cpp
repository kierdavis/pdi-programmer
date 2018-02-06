#include <stdint.h>

#include "NVM.hpp"
#include "PDI.hpp"
#include "Util.hpp"

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
