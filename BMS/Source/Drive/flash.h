#ifndef __FLASH_H
#define __FLASH_H

#include "conf_general.h"
#include "n32l40x_flash.h"

#define FLASH_PAGE_SIZE                   0x800
#define FLASH_MAIN_FIRMWARE_START_ADDRESS 0x08000000
#define FLASH_MAIN_FIRMWARE_END_ADDRESS   0x0800E000
#define FLASH_STORAGE_START_ADDRESS       0x0800E000
#define FLASH_STORAGE_END_ADDRESS         0x08010000
#define FLASH_NEW_FIRMWARE_START_ADDRESS  0x08010000
#define FLASH_NEW_FIRMWARE_END_ADDRESS    0x0801E000
#define FLASH_BOOTLOADER_START_ADDRESS    0x0801E000
#define FLASH_BOOTLOADER_END_ADDRESS      0x08020000
#define VESC_TOOL_BOOTLOADER_OFFSET       0x0001E000
#define BOOTLOADER_INIT_MAGIC_WORD        0xB00720AD
#define BOOTLOADER_DONE_MAGIC_WORD        0xB007600D

extern uint32_t bootloader_trigger;

void Flash_Init(void);
bool Flash_Erase(uint32_t start, uint32_t end);
bool Flash_Write(uint32_t start, uint32_t *data, uint32_t len);
bool Flash_Erase_Storage(void);
bool Flash_Write_Storage(void);
void Flash_Load_Storage(void);
bool Flash_Erase_New_Firmware(uint32_t size);
bool Flash_Write_New_Firmware(uint32_t offset, uint8_t *data, uint32_t len);
bool Flash_Verify_New_Firmware(void);
bool Flash_Verify_Main_Firmware(void);
bool Flash_Copy_New_Firmware_To_Main_Firmware(void);
bool Flash_Erase_Bootloader(void);
bool Flash_Write_Bootloader(uint32_t offset, uint8_t *data, uint32_t len);
void Flash_Enter_Bootloader(void);
void Flash_Exit_Bootloader(void);
void Flash_Maybe_Jump_To_Bootloader(void);
bool Flash_Did_Return_From_Bootloader(void);

#endif
