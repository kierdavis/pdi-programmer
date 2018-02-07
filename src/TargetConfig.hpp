#ifndef __PDIPROG_TARGET_CONFIG_HPP
#define __PDIPROG_TARGET_CONFIG_HPP

namespace TargetConfig {
  static constexpr uint16_t FLASH_PAGE_SIZE = 512;
  static constexpr uint32_t FLASH_APP_PAGES = 384;
  static constexpr uint32_t FLASH_BOOT_PAGES = 16;

  static constexpr uint32_t FLASH_START = 0x00800000;
  static constexpr uint32_t FLASH_APP_START = FLASH_START;
  static constexpr uint32_t FLASH_BOOT_START = FLASH_APP_START + FLASH_APP_PAGES*FLASH_PAGE_SIZE;

  static constexpr uint16_t EEPROM_PAGE_SIZE = 32;
  static constexpr uint32_t EEPROM_PAGES = 64;

  static constexpr uint32_t EEPROM_START = 0x008C0000;

  static constexpr uint32_t FUSE_START = 0x008F0020;

  static constexpr uint32_t RAM_START = 0x01000000;

  static constexpr uint32_t NVM_REGS_OFFSET = 0x01C0;
  static constexpr uint32_t NVM_REGS_START = RAM_START + NVM_REGS_OFFSET;
}

#endif
