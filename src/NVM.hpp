#ifndef __PDIPROG_NVM_HPP
#define __PDIPROG_NVM_HPP

#include "Util.hpp"

namespace NVM {
  void init();
  Util::Status begin();
  void end();

  namespace Controller {
    enum class Reg : uint8_t {
      CMD = 0x0A,
      CTRLA = 0x0B,
      STATUS = 0x0F,
    };

    enum class Cmd : uint8_t {
      NOOP                   = 0x00,
      CHIPERASE              = 0x40,
      READNVM                = 0x43,
      LOADFLASHPAGEBUFF      = 0x23,
      ERASEFLASHPAGEBUFF     = 0x26,
      ERASEFLASHPAGE         = 0x2B,
      WRITEFLASHPAGE         = 0x2E,
      ERASEWRITEFLASH        = 0x2F,
      FLASHCRC               = 0x78,
      ERASEAPPSEC            = 0x20,
      ERASEAPPSECPAGE        = 0x22,
      WRITEAPPSECPAGE        = 0x24,
      ERASEWRITEAPPSECPAGE   = 0x25,
      APPCRC                 = 0x38,
      ERASEBOOTSEC           = 0x68,
      ERASEBOOTSECPAGE       = 0x2A,
      WRITEBOOTSECPAGE       = 0x2C,
      ERASEWRITEBOOTSECPAGE  = 0x2D,
      BOOTCRC                = 0x39,
      READUSERSIG            = 0x03,
      ERASEUSERSIG           = 0x18,
      WRITEUSERSIG           = 0x1A,
      READCALIBRATION        = 0x02,
      READFUSE               = 0x07,
      WRITEFUSE              = 0x4C,
      WRITELOCK              = 0x08,
      LOADEEPROMPAGEBUFF     = 0x33,
      ERASEEEPROMPAGEBUFF    = 0x36,
      ERASEEEPROM            = 0x30,
      ERASEEEPROMPAGE        = 0x32,
      WRITEEEPROMPAGE        = 0x34,
      ERASEWRITEEEPROMPAGE   = 0x35,
      READEEPROM             = 0x06,
    };

    uint32_t regAddr(Reg reg);
    void writeReg(Reg reg, uint8_t data);
    void writeCmd(Cmd cmd);
    void writeCmdex();

    Util::Status waitWhileBusy();
  }

  Util::Status read(uint32_t addr, uint8_t * buffer, uint16_t len);

  Util::Status eraseChip();
}

#endif
