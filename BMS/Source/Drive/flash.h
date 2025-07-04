#ifndef __FLASH_H
#define __FLASH_H

#include "conf_general.h"
#include "n32l40x_flash.h"

#define FLASH_PAGE_SIZE             0x800
#define FLASH_STORAGE_START_ADDRESS 0x0800E000
#define FLASH_STORAGE_END_ADDRESS   0x08010000

void Flash_Init(void);
bool Flash_Erase(uint32_t start, uint32_t end);
bool Flash_Write(uint32_t start, uint32_t *data, uint32_t len);
bool Flash_Erase_Storage(void);
bool Flash_Write_Storage(void);
void Flash_Load_Storage(void);

#endif
