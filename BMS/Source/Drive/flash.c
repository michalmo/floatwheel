#include "mem.h"
#include "flash.h"
#include "buffer.h"
#include "crc.h"
#include "flag.h"
#include <string.h>

// place storage in retained SRAM2 region
uint32_t bootloader_trigger __attribute__((__AT__ZERO_INIT(0x20007FFC)));

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

bool Flash_Verify_New_Firmware_Image_Hardware_Identifier(uint32_t offset, uint8_t *data, uint32_t len)
{
	return offset == 0 && strcmp((char *)(data + HW_IDENTIFIER_OFFSET + 6), (char *)HW_IDENTIFIER_ADDRESS) == 0;
}

bool Flash_Verify_New_Firmware_Hardware_Identifier(void)
{
	return strcmp((char *)(FLASH_NEW_FIRMWARE_START_ADDRESS + HW_IDENTIFIER_OFFSET + 6), (char *)HW_IDENTIFIER_ADDRESS) == 0;
}

bool Flash_Verify_New_Firmware(void)
{
	uint8_t *pdata = (uint8_t*)FLASH_NEW_FIRMWARE_START_ADDRESS;
	int32_t ind = 0;
	uint32_t firmware_size = buffer_get_uint32(pdata, &ind);
	uint16_t firmware_crc = buffer_get_uint16(pdata, &ind);
	
	return (
		firmware_size != 0 &&
		firmware_size <= FLASH_MAIN_FIRMWARE_END_ADDRESS - FLASH_MAIN_FIRMWARE_START_ADDRESS &&
		Flash_Verify_New_Firmware_Hardware_Identifier() &&
		crc16(pdata + ind, firmware_size) == firmware_crc
	);
}

bool Flash_Erase_New_Firmware(uint32_t size)
{
	if (FLASH_NEW_FIRMWARE_END_ADDRESS - FLASH_NEW_FIRMWARE_START_ADDRESS < size)
	{
		return false;
	}
	return Flash_Erase(FLASH_NEW_FIRMWARE_START_ADDRESS, FLASH_NEW_FIRMWARE_START_ADDRESS + size);
}

bool Flash_Write_New_Firmware(uint32_t offset, uint8_t *data, uint32_t len)
{
	if(offset >= VESC_TOOL_BOOTLOADER_OFFSET)
	{
		// not compatible with VESC BMS bootloader
		return false;
	}
	if(offset == 0 && !Flash_Verify_New_Firmware_Image_Hardware_Identifier(offset, data, len))
	{
		return false;
	}
	return Flash_Write(FLASH_NEW_FIRMWARE_START_ADDRESS + offset, (uint32_t*)data, len);
}

bool Flash_Verify_Main_Firmware(void)
{
	uint8_t *pdata = (uint8_t*)FLASH_NEW_FIRMWARE_START_ADDRESS;
	int32_t ind = 0;
	uint32_t firmware_size = buffer_get_uint32(pdata, &ind);
	uint16_t firmware_crc = buffer_get_uint16(pdata, &ind);
	
	return (
		firmware_size != 0 &&
		firmware_size <= FLASH_MAIN_FIRMWARE_END_ADDRESS - FLASH_MAIN_FIRMWARE_START_ADDRESS &&
		crc16((uint8_t*)FLASH_MAIN_FIRMWARE_START_ADDRESS, firmware_size) == firmware_crc
	);
}

bool Flash_Copy_New_Firmware_To_Main_Firmware(void)
{
	uint8_t *pdata = (uint8_t*)FLASH_NEW_FIRMWARE_START_ADDRESS;
	int32_t ind = 0;
	uint32_t firmware_size = buffer_get_uint32(pdata, &ind);
	uint16_t firmware_crc = buffer_get_uint16(pdata, &ind);

	// pad to multiple of 4 bytes
	while ((firmware_size % 4) != 0)
	{
		firmware_size++;
	}
	return (
		Flash_Erase(FLASH_MAIN_FIRMWARE_START_ADDRESS, FLASH_MAIN_FIRMWARE_START_ADDRESS + firmware_size) &&
		Flash_Write(FLASH_MAIN_FIRMWARE_START_ADDRESS, (uint32_t*)(pdata + ind), firmware_size)
	);
}

bool Flash_Verify_Bootloader_Image_Hardware_Identifier(uint32_t offset, uint8_t *data, uint32_t len)
{
	return offset == 0 && strcmp((char *)(data + HW_IDENTIFIER_OFFSET), (char *)HW_IDENTIFIER_ADDRESS) == 0;
}

bool Flash_Verify_Bootloader_Hardware_Identifier(void)
{
	return strcmp((char *)(FLASH_BOOTLOADER_START_ADDRESS + HW_IDENTIFIER_OFFSET), (char *)HW_IDENTIFIER_ADDRESS) == 0;
}

bool Flash_Erase_Bootloader(void)
{
	return Flash_Erase(FLASH_BOOTLOADER_START_ADDRESS, FLASH_BOOTLOADER_END_ADDRESS);
}

bool Flash_Write_Bootloader(uint32_t offset, uint8_t *data, uint32_t len)
{
	if(offset >= VESC_TOOL_BOOTLOADER_OFFSET)
	{
		// this should never happen
		return false;
	}
	if(offset == 0 && !Flash_Verify_Bootloader_Image_Hardware_Identifier(offset, data, len))
	{
		return false;
	}
	return Flash_Write(FLASH_BOOTLOADER_START_ADDRESS + offset, (uint32_t*)data, len);
}

void Flash_Enter_Bootloader(void)
{
	if(!Flash_Verify_Bootloader_Hardware_Identifier())
	{
		return;
	}
	bootloader_trigger = BOOTLOADER_INIT_MAGIC_WORD;
	Flag.Power = 3;
}

void Flash_Exit_Bootloader(void)
{
	bootloader_trigger = BOOTLOADER_DONE_MAGIC_WORD;
	NVIC_SystemReset();
}

void Flash_Maybe_Jump_To_Bootloader(void)
{
	if(bootloader_trigger != BOOTLOADER_INIT_MAGIC_WORD)
	{
		return;
	}
	// clear magic word
	bootloader_trigger = 0;

	const volatile uint32_t* bootloader_data = (volatile uint32_t*)FLASH_BOOTLOADER_START_ADDRESS;
	uint32_t stack_pointer = bootloader_data[0];
	void (*bootloader_entry_point)(void) = (void (*)(void))(bootloader_data[1]);
	// clear interrupts
	for(int i = 0;i < 8U;i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}
	// jump to bootloader
	__set_MSP(stack_pointer);
	bootloader_entry_point();
}

bool Flash_Did_Return_From_Bootloader(void)
{
	if(bootloader_trigger != BOOTLOADER_DONE_MAGIC_WORD)
	{
		return false;
	}

	bootloader_trigger = 0;
	return true;
}
