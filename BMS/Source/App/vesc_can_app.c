#include "vesc_can_app.h"

/**************************************************
 * @brie  :VESC_CAN_Status_Task()
 * @note  :VESC_CAN任务
 * @param :无
 * @retval:无
 **************************************************/
void VESC_CAN_Status_Task(void)
{
	static uint8_t vesc_can_send_step = 0;
		
	if((Software_Counter_1ms.VESC_CAN < 50) || (Flag.Power != 2))
	{
		return;
	}
	Software_Counter_1ms.VESC_CAN = 0;
	//VESC_CAN_DATA.pBMS_I->Input_Current.f += 0.001f;
	switch(vesc_can_send_step)
	{
		case 0:	//发送总电压
			VESC_Set_BMS_V_TOT(&VESC_CAN_DATA);
			vesc_can_send_step++;
		break;
			
		case 1:	//发送电池电压 1-2-3
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 0);
			vesc_can_send_step++;
		break;
		
		case 2:	//发送电池电压 4-5-6
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 3);
			vesc_can_send_step++;
		break;
		
		case 3: //发送电池电压 7-8-9
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 6);
			vesc_can_send_step++;
		break;
		
		case 4://发送电池电压 10-11-12
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 9);
			vesc_can_send_step++;
		break;
		
		case 5://发送电池电压 13-14-15
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 12);
			vesc_can_send_step++;
		break;
		
		case 6://发送电池电压 16-17-18
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 15);
			vesc_can_send_step++;
		break;
		
		case 7://发送电池电压 19-20
			VESC_Set_BMS_V_CELL(&VESC_CAN_DATA, 18);
			vesc_can_send_step++;
		break;
		
		case 8://发送电池电压状态
			VESC_Set_BMS_BAL(&VESC_CAN_DATA);
			vesc_can_send_step++;
		break;
		
		case 9://发送电池电流
			VESC_Set_BMS_I(&VESC_CAN_DATA);
			vesc_can_send_step++;
		break;
		
		case 10://发送IC温度
			VESC_Set_BMS_HUM(&VESC_CAN_DATA);
			vesc_can_send_step++;
		break;
		
		case 11://发送NTC温度
			VESC_Set_BMS_TEMPS(&VESC_CAN_DATA,0);
			vesc_can_send_step++;
		break;
		
		case 12://发送电流
			VESC_Set_BMS_I(&VESC_CAN_DATA);
			vesc_can_send_step++;
		break;
		
		case 13://发送错误代码
			VESC_Set_BMS_TEMPS(&VESC_CAN_DATA,3);
			vesc_can_send_step++;
		break;
		
		case 14://发送错误代码
			VESC_Set_BMS_TEMPS(&VESC_CAN_DATA,6);
			vesc_can_send_step = 0;
		break;
		
		default:
			vesc_can_send_step = 0;
		break;
		
	}
	
}

