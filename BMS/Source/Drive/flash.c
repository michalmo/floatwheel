#include "flash.h"
#include <string.h>

void Flash_Init(void)
{
	FLASH_ClockInit();
}

bool Flash_Erase(uint32_t start, uint32_t end)
{
	uint32_t addr;
	FLASH_Unlock();

	for(addr = start; addr < end; addr += FLASH_PAGE_SIZE)
	{
		if(FLASH_EraseOnePage(addr) != FLASH_COMPL)
		{
			return false;
		}
	}

	FLASH_Lock();
	return true;
}

bool Flash_Write(uint32_t start, uint32_t *data, uint32_t len)
{
	uint32_t ofs;
	FLASH_Unlock();

	for(ofs = 0; ofs < len; ofs += 4)
	{
		if(FLASH_ProgramWord(start + ofs, data[ofs>>2]) != FLASH_COMPL)
		{
			return false;
		}
	}

	FLASH_Lock();
	return true;
}

bool Flash_Erase_Storage(void)
{
	return Flash_Erase(FLASH_STORAGE_START_ADDRESS, FLASH_STORAGE_END_ADDRESS);
}

bool Flash_Write_Storage(void)
{
	storage.controller_id = storage.config.controller_id;
	storage.can_baud_rate = storage.config.can_baud_rate;
	storage.conf_flash_write_cnt++;
	// writing back from uint8 to float applies the rounding so the user gets visual feedback in the UI
	storage.config.short_circuit_detection_voltage = (uint8_t)storage.config.short_circuit_detection_voltage;
	storage.config.short_circuit_detection_time = (uint8_t)storage.config.short_circuit_detection_time;
	return (
		Flash_Erase_Storage() &&
		Flash_Write(FLASH_STORAGE_START_ADDRESS, (uint32_t*)&storage, sizeof(storage_data))
	);
}

void Flash_Load_Storage(void)
{
	memcpy((void*)&storage, (uint8_t*)FLASH_STORAGE_START_ADDRESS, sizeof(storage_data));
}
