#include "n32l40x_rcc.h"
#include "led.h"
#include "flash.h"
#include <stdint.h>

#define WRITE_ATTEMPTS 5

RCC_ClocksType System_Clock;

int main(void)
{
	RCC_GetClocksFreqValue(&System_Clock);
	Flash_Init();
	LED_Init();
	RCC_ClrFlag();
	LED_ON;

	if(Flash_Verify_New_Firmware())
	{
		for(int i = 0; i < WRITE_ATTEMPTS; i++)
		{
			if(Flash_Copy_New_Firmware_To_Main_Firmware())
			{
				if(Flash_Verify_Main_Firmware())
				{
					break;
				}
			}
		}
	}

	LED_OFF;
	Flash_Exit_Bootloader();
}
