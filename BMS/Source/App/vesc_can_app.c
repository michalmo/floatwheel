#include "vesc_can_app.h"
#include "vesc_can.h"

/**************************************************
 * @brie  :VESC_CAN_Status_Task()
 * @note  :VESC_CAN任务
 * @param :无
 * @retval:无
 **************************************************/
void VESC_CAN_Status_Task(void)
{
	int i;

	if(Flag.Power != 2)
	{
		return;
	}

	if(
		// wait if there's a transmit backlog
		can_tx_queue_size > 0 ||
		// apply a rate limit
		Software_Counter_1ms.VESC_CAN < 200)
	{
		return;
	}

	Software_Counter_1ms.VESC_CAN = 0;

	VESC_Set_BMS_V_TOT(&VESC_CAN_DATA);
	VESC_Set_BMS_I(&VESC_CAN_DATA);
	VESC_Set_BMS_AH_WH(&VESC_CAN_DATA);
	for(i=0;i<MAX_CELL_SERIES;i+=3)
	{
		VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, i);
	}
	VESC_Set_BMS_BAL(&VESC_CAN_DATA);
	for(i=0;i<MAX_TEMP_SENSORS;i+=3)
	{
		VESC_Set_BMS_TEMPS(&VESC_CAN_DATA,i);
	}
	VESC_Set_BMS_HUM(&VESC_CAN_DATA);
	VESC_Set_BMS_SOC_SOH_TEMP_STAT(&VESC_CAN_DATA);
	VESC_Set_BMS_AH_WH_CHG_TOTAL(&VESC_CAN_DATA);
	VESC_Set_BMS_AH_WH_DIS_TOTAL(&VESC_CAN_DATA);
}

