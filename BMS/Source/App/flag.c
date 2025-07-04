#include "flag.h"

Software_Counter_Type Software_Counter_1ms;
Flag_Type Flag = {
	.Charge_Allowed = 1,
	.Balance_Allowed = 1,
};
uint16_t LED_Flicker_Time = 500;
