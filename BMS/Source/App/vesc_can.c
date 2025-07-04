#include "vesc_can.h"
#include "datatypes.h"
#include "n32l40x_can.h"

CAN_BMS_V_TOT 	BMS_V_TOT = 
{
	.Total_Voltage = 0,				//总电压
	.Charge_Input_Voltage = 0,		//充电器电压
};

CAN_BMS_I		BMS_I = 
{
	.Input_Current = 0,				//输入电流
	.Input_Current_BMS_IC = 0,		//BMS_IC电流
};


CAN_BMS_AH_WH	BMS_AH_WH = 
{
	.Ah_Counter = 0,				//电池毫安时
	.Wh_Counter = 0,				//电池W时 
};

CAN_BMS_V_CELL	BMS_V_CELL;

CAN_BMS_BAL		BMS_BAL = 
{
	.BMS_BAT.i = 0,					//单节电池状态
};

CAN_BMS_TEMPS	BMS_TEMPS;

CAN_BMS_HUM		BMS_HUM = 
{
	.Humidity = 0,					//湿度	0-10000(0%-100%);
	.Temp_Hum_Sensor = 0,			//温度	-10000-10000(-100°-100°)
	.Temp_IC = 0,					//IC温度
	.Pressure = 0,
};

CAN_BMS_SOC_SOH_TEMP_STAT	BMS_SOC_SOH_TEMP_STAT = 
{
	.V_Cell_Min = 0,				//单节电池最低电压 扩大1000倍发送
	.V_Cell_Max = 0,				//单节电池最高电压 扩大1000倍发送
	.Soc = 0,						//0-255(0%-100%)
	.Soh = 0,						//0-255(0%-100%)
	.T_Cell_Max = 0,				//单节电池最大温度
	.Stat.bits = {
		.Is_Charge_Allowed = 1,
		.Is_Charge_OK = 1,
	},
};

CAN_BMS_AH_WH_CHG_TOTAL		BMS_AH_WH_CHG_TOTAL =
{
	.Ah_Charge_Total = 0,			//安时
	.Wh_Charge_Total = 0,			//瓦时
};

CAN_BMS_AH_WH_DIS_TOTAL		BMS_AH_WH_DIS_TOTAL =
{
	.Ah_Discharge_Total = 0,		//安时
	.Wh_Discharge_Total = 0,		//瓦时
};	

VESC_CAN_TYPE VESC_CAN_DATA = 
{
	.pBMS_V_TOT 	= &BMS_V_TOT,
	.pBMS_I 		= &BMS_I,
	.pBMS_AH_WH		= &BMS_AH_WH,
	.pBMS_V_CELL	= &BMS_V_CELL,
	.pBMS_BAL		= &BMS_BAL,
	.pBMS_TEMPS     = &BMS_TEMPS,
	.pBMS_HUM		= &BMS_HUM,
	.pBMS_SOC_SOH_TEMP_STAT	= &BMS_SOC_SOH_TEMP_STAT,
	.pBMS_AH_WH_CHG_TOTAL 	= &BMS_AH_WH_CHG_TOTAL,
	.pBMS_AH_WH_DIS_TOTAL	= &BMS_AH_WH_DIS_TOTAL,
};

CanTxMessage CAN_TX_Config;
can_queued_message can_tx_queue[CAN_TX_QUEUE_SIZE];
int can_tx_queue_head = 0;
int can_tx_queue_size = 0;
int can_tx_queue_size_max = 0;

void VESC_CAN_Transmit_Task(void)
{
	int i, j;
	can_queued_message* message;

	// fill all mailboxes
	for(i = 0; i < 3; i++)
	{
		// check if there is a new message to transmit
		if(can_tx_queue_size == 0)
		{
			return;
		}

		message = can_tx_queue + can_tx_queue_head;
		CAN_TX_Config.StdId = 0;
		CAN_TX_Config.ExtId = (uint32_t)(message->can_id|(message->can_packet_id<<8));
		CAN_TX_Config.IDE = CAN_ID_EXT;
		CAN_TX_Config.RTR = CAN_RTRQ_DATA;
		CAN_TX_Config.DLC = message->len;
		memcpy(CAN_TX_Config.Data,message->data,message->len);

		if(CAN_TransmitMessage(CAN,&CAN_TX_Config) == CAN_TxSTS_NoMailBox)
		{
			return;
		}

		can_tx_queue_head = (can_tx_queue_head + 1) % CAN_TX_QUEUE_SIZE;
		can_tx_queue_size--;
	}
}

uint8_t can_tx_buffer[8];

void VESC_COMM_CAN_Transmit(uint8_t can_id,CAN_PACKET_ID can_packet_id,uint8_t *buffer,unsigned int len)
{
	int i;
	uint32_t eid = (can_id|(can_packet_id<<8));
	can_queued_message *message = can_tx_queue + (can_tx_queue_head + can_tx_queue_size) % CAN_TX_QUEUE_SIZE;

	if(can_tx_queue_size == CAN_TX_QUEUE_SIZE || len > 8)
	{
		return;
	}

	message->can_id = can_id;
	message->can_packet_id = can_packet_id;
	message->len = len;
	memcpy(message->data,buffer,len);

	can_tx_queue_size++;
	if(can_tx_queue_size > can_tx_queue_size_max)
	{
		can_tx_queue_size_max = can_tx_queue_size;
	}
}

void VESC_COMM_CAN_Transmit_Buffer(uint8_t can_id,uint8_t *pdata,unsigned int len,uint8_t send)
{
	int ind;
	unsigned int i;
	unsigned int end_i = 0;
	uint16_t crc;

	if(len<=6)
	{
		can_tx_buffer[0] = CAN_ID;
		can_tx_buffer[1] = send;
		ind = 2;
		memcpy(can_tx_buffer+ind,pdata,len);
		ind += len;

		VESC_COMM_CAN_Transmit(can_id,CAN_PACKET_PROCESS_SHORT_BUFFER,can_tx_buffer,ind);
	}
	else
	{
		for (i=0;i<len;i+=7)
		{
			if (i > 255)
			{
				break;
			}
			end_i = i+7;

			can_tx_buffer[0] = i;
			if ((i+7) <= len)
			{
				memcpy(can_tx_buffer+1,pdata+i,7);
				ind = 8;
			}
			else
			{
				memcpy(can_tx_buffer+1,pdata+i,len-i);
				ind = len-i+1;
			}

			VESC_COMM_CAN_Transmit(can_id,CAN_PACKET_FILL_RX_BUFFER,can_tx_buffer,ind);
		}

		for (i = end_i;i<len;i += 6)
		{
			ind = 0;
			buffer_append_uint16(can_tx_buffer, i, &ind);
			if ((i+6) <= len)
			{
				memcpy(can_tx_buffer+ind,pdata+i,6);
				ind = 8;
			}
			else
			{
				memcpy(can_tx_buffer+ind,pdata+i,len-i);
				ind += len-i;
			}

			VESC_COMM_CAN_Transmit(can_id,CAN_PACKET_FILL_RX_BUFFER_LONG,can_tx_buffer,ind);
		}

		ind = 0;
		can_tx_buffer[ind++] = CAN_ID;
		can_tx_buffer[ind++] = send;
		buffer_append_uint16(can_tx_buffer, len, &ind);
		crc = crc16(pdata, len);
		buffer_append_uint16(can_tx_buffer, crc, &ind);

		VESC_COMM_CAN_Transmit(can_id,CAN_PACKET_PROCESS_RX_BUFFER,can_tx_buffer,6);
	}
}

/**************************************************
 * @brie  :VESC_Set_BMS_V_TOT()
 * @note  :设置总电压 	充电器电压
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_V_TOT(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_V_TOT->Total_Voltage.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_V_TOT->Charge_Input_Voltage.f, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_V_TOT,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_I()
 * @note  :设置输入电流 	BMS_IC电流
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_I(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_I->Input_Current.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_I->Input_Current_BMS_IC.f, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_I,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_AH_WH()
 * @note  :设置电池毫安时 	电池W时 
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_AH_WH(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH->Ah_Counter.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH->Wh_Counter.f, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_AH_WH,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_V_CELL()
 * @note  :设置单节电池电压 
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_V_CELL(VESC_CAN_TYPE *vesc_can_data,uint8_t start_cell_id)
{
	int ind = 0;

	can_tx_buffer[ind++] = start_cell_id;
	can_tx_buffer[ind++] = MAX_CELL_SERIES;
	if(start_cell_id < MAX_CELL_SERIES)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_V_CELL->BMS_Single_Voltage[start_cell_id], &ind);
	}
	if(start_cell_id+1 < MAX_CELL_SERIES)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_V_CELL->BMS_Single_Voltage[start_cell_id+1], &ind);
	}
	if(start_cell_id+2 < MAX_CELL_SERIES)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_V_CELL->BMS_Single_Voltage[start_cell_id+2], &ind);
	}
	
	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_V_CELL,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_BAL()
 * @note  :设置单节电池状态 
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_BAL(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;

	can_tx_buffer[ind++] = MAX_CELL_SERIES;
	can_tx_buffer[ind++] = 0;
	can_tx_buffer[ind++] = 0;
	can_tx_buffer[ind++] = 0;
	buffer_append_uint32(can_tx_buffer, vesc_can_data->pBMS_BAL->BMS_BAT.i, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_BAL,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_TEMPS()
 * @note  :设置温度 
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_TEMPS(VESC_CAN_TYPE *vesc_can_data,uint8_t start_sensor_id)
{
	int ind = 0;
	
	can_tx_buffer[ind++] = start_sensor_id;
	can_tx_buffer[ind++] = MAX_TEMP_SENSORS;
	if(start_sensor_id < MAX_TEMP_SENSORS)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_TEMPS->BMS_Single_Temp[start_sensor_id], &ind);
	}
	if(start_sensor_id+1 < MAX_TEMP_SENSORS)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_TEMPS->BMS_Single_Temp[start_sensor_id+1], &ind);
	}
	if(start_sensor_id+2 < MAX_TEMP_SENSORS)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_TEMPS->BMS_Single_Temp[start_sensor_id+2], &ind);
	}
	
	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_TEMPS,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_HUM()
 * @note  :设置湿度 温度 IC温度  
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_HUM(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Temp_Hum_Sensor, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Humidity, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Temp_IC, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Pressure, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_HUM,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_SOC_SOH_TEMP_STAT()
 * @note  :设置单节电池最低电压
 *			   单节电池最高电压
 *			    Soc
 *				Soh
 *			   单节电池最大温度
 *			   状态
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_SOC_SOH_TEMP_STAT(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->V_Cell_Min, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->V_Cell_Max, &ind);
	can_tx_buffer[ind++] =  (uint8_t)(vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->Soc * 255);
	can_tx_buffer[ind++] =  (uint8_t)(vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->Soh * 255);
	can_tx_buffer[ind++] =  (uint8_t)(roundf(vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->T_Cell_Max));
	can_tx_buffer[ind++] =  vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->Stat.i;

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_SOC_SOH_TEMP_STAT,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_AH_WH_CHG_TOTAL()
 * @note  :设置充电安时 	充电瓦时
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_AH_WH_CHG_TOTAL(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_CHG_TOTAL->Ah_Charge_Total.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_CHG_TOTAL->Wh_Charge_Total.f, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_AH_WH_CHG_TOTAL,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_AH_WH_DIS_TOTAL()
 * @note  :设置安时 瓦时
 * @param :vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_AH_WH_DIS_TOTAL(VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_DIS_TOTAL->Ah_Discharge_Total.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_DIS_TOTAL->Wh_Discharge_Total.f, &ind);

	VESC_COMM_CAN_Transmit(0xFF,CAN_PACKET_BMS_AH_WH_DIS_TOTAL,can_tx_buffer,ind);
}

void VESC_Send_Pong(uint8_t can_id)
{
	can_tx_buffer[0] = CAN_ID;
	can_tx_buffer[1] = HW_TYPE_VESC_BMS;

	VESC_COMM_CAN_Transmit(can_id,CAN_PACKET_PONG,can_tx_buffer,2);
}

#define PRINTF_BUFFER_SIZE 600

void VESC_Printf(uint8_t can_id,const char *fmt,...)
{
	va_list arg;
	va_start(arg,fmt);
	int len;
	uint8_t buffer[PRINTF_BUFFER_SIZE];

	buffer[0] = COMM_PRINT;
	len = vsnprintf((char*)(buffer+1),PRINTF_BUFFER_SIZE-1,fmt,arg)+1;
	va_end(arg);

	if(len>=PRINTF_BUFFER_SIZE)
	{
		len = PRINTF_BUFFER_SIZE;
	}

	if (can_id != 0)
	{
		VESC_COMM_CAN_Transmit_Buffer(can_id,buffer,len,1);
	}
}

CAN_STATUS STATUS =
{
	.Rpm = 0,			//转速
	.Duty_Cycle = 0,	//占空比
	.Total_Current = 0,	//总电流
};

CAN_STATUS_2 STATUS_2 = 
{
	.Amp_Hours_Charged = 0,	//充电的安时
	.Amp_Hours = 0,			//消耗的安时
};

CAN_STATUS_3 STATUS_3 = 
{
	.Watt_Hours_Charged = 0,	//充电的瓦时
	.Watt_Hours = 0,			//消耗的瓦时
};

CAN_STATUS_4 STATUS_4 = 
{
	.PID_Pos = 0,				//不清楚数据类型
	.Total_Input_Current = 0,	//输入总电流
	.Motor_Temp = 0,			//电机温度
	.MOSFET_Temp = 0,			//MOS管温度
};

CAN_STATUS_5 STATUS_5 = 
{
	.Input_Voltage = 0,		//输入电压
	.Tachometer_Value = 0,	//转速表?不确定
};

VESC_CAN_RX_TYPE VESC_CAN_RX_DATA = 
{
	.pSTATUS = &STATUS,
	.pSTATUS_2 = &STATUS_2,
	.pSTATUS_3 = &STATUS_3,
	.pSTATUS_4 = &STATUS_4,
	.pSTATUS_5 = &STATUS_5,
};

can_queued_message can_rx_queue[CAN_RX_QUEUE_SIZE];
int can_rx_queue_head = 0;
int can_rx_queue_size = 0;
int can_rx_queue_size_max = 0;

void VESC_CAN_RX_Inte(CanRxMessage *can_rx_struct)
{
	int i;
	uint8_t id = can_rx_struct->ExtId & 0xFF;
	uint32_t vesc_can_cmd = can_rx_struct->ExtId>>8;
	can_queued_message *message = can_rx_queue + (can_rx_queue_head + can_rx_queue_size) % CAN_RX_QUEUE_SIZE;

	if(id != 255 &&
		 id != storage.config.controller_id &&
		 vesc_can_cmd != CAN_PACKET_STATUS &&
		 vesc_can_cmd != CAN_PACKET_STATUS_2 &&
		 vesc_can_cmd != CAN_PACKET_STATUS_3 &&
		 vesc_can_cmd != CAN_PACKET_STATUS_4 &&
		 vesc_can_cmd != CAN_PACKET_STATUS_5)
	{
		return;
	}
	if(can_rx_queue_size == CAN_RX_QUEUE_SIZE)
	{
		return;
	}

	message->can_id = can_rx_struct->ExtId & 0xFF;
	message->can_packet_id = can_rx_struct->ExtId>>8;
	message->len = can_rx_struct->DLC;
	memcpy(message->data, can_rx_struct->Data, can_rx_struct->DLC);

	can_rx_queue_size++;
	if(can_rx_queue_size > can_rx_queue_size_max)
	{
		can_rx_queue_size_max = can_rx_queue_size;
	}
}

uint8_t rx_buffer[RX_BUFFER_SIZE];

void VESC_CAN_Receive_Task(void)
{
	if(
		 // check if there is a new message to process
		 can_rx_queue_size == 0 ||
		 // wait if there's a transmit backlog
		 can_tx_queue_size > 0)
	{
		return;
	}

	can_queued_message *message = can_rx_queue + can_rx_queue_head;
	uint8_t id = message->can_id;
	uint8_t vesc_can_cmd = message->can_packet_id;
	uint8_t len = message->len;
	uint8_t *pdata =  message->data;
	
	int ind = 0;
	uint16_t rx_buffer_len;
	uint16_t rx_buffer_ind;
	uint16_t crc;
	static uint8_t received_id = 0;
	uint8_t commands_send;

	if(id == 255 || id == CAN_ID)
	{
		switch(vesc_can_cmd)
		{
			case CAN_PACKET_FILL_RX_BUFFER:
				memcpy(rx_buffer+pdata[0],pdata+1,len-1);
			break;

			case CAN_PACKET_FILL_RX_BUFFER_LONG:
				rx_buffer_ind = buffer_get_uint16(pdata, &ind);
				if(rx_buffer_ind < RX_BUFFER_SIZE)
				{
					memcpy(rx_buffer+rx_buffer_ind,pdata+ind,len-2);
				}
			break;

			case CAN_PACKET_PROCESS_RX_BUFFER:
				received_id = pdata[ind++];
				commands_send = pdata[ind++];

				rx_buffer_len = buffer_get_uint16(pdata, &ind);
				if(rx_buffer_len > RX_BUFFER_SIZE)
				{
					break;
				}

				crc = buffer_get_uint16(pdata, &ind);
				if(crc16(rx_buffer,rx_buffer_len) == crc)
				{
					switch(commands_send)
					{
						case 0:
							VESC_Process_Command(rx_buffer,rx_buffer_len,received_id);
						break;

						case 1:
							VESC_COMM_CAN_Transmit_Buffer(received_id,rx_buffer,rx_buffer_len,1);
						break;

						case 2:
							received_id = 0;
							VESC_Process_Command(rx_buffer,rx_buffer_len,0);
						break;

						default:
						break;
					}
				}
			break;

			case CAN_PACKET_PROCESS_SHORT_BUFFER:
				received_id = pdata[ind++];
				commands_send = pdata[ind++];

				switch(commands_send)
				{
					case 0:
						VESC_Process_Command(pdata+ind,len-ind,received_id);
					break;

					case 1:
						VESC_COMM_CAN_Transmit_Buffer(received_id,pdata+ind,len-ind,1);
					break;

					case 2:
						received_id = 0;
						VESC_Process_Command(pdata+ind,len-ind,0);
					break;

					default:
					break;
				}
			break;

			case CAN_PACKET_PING:
				VESC_Send_Pong(pdata[0]);
			break;

			case CAN_PACKET_SHUTDOWN:
				if(Flag.Power != 0 &&
					 Flag.Charger_ON == 0 &&
					 VESC_CAN_RX_DATA.pSTATUS->Rpm < 100 &&
					 VESC_CAN_RX_DATA.pSTATUS->Rpm > -100)
				{
					Flag.Power = 3;
				}
			break;

			default:
			break;
		}
	}

	switch(vesc_can_cmd)
	{
		case CAN_PACKET_STATUS:
			VESC_CAN_RX_DATA.pSTATUS->Rpm = buffer_get_int32(pdata, &ind);
			VESC_CAN_RX_DATA.pSTATUS->Total_Current = (float)buffer_get_int16(pdata, &ind) / 10;
			VESC_CAN_RX_DATA.pSTATUS->Duty_Cycle = (float)buffer_get_int16(pdata, &ind) / 1000;
		break;
		
		case CAN_PACKET_STATUS_2:
			VESC_CAN_RX_DATA.pSTATUS_2->Amp_Hours = (float)buffer_get_int32(pdata, &ind) / 10000;
			VESC_CAN_RX_DATA.pSTATUS_2->Amp_Hours_Charged	= (float)buffer_get_int32(pdata, &ind) / 10000;
		break;
		
		case CAN_PACKET_STATUS_3:
			VESC_CAN_RX_DATA.pSTATUS_3->Watt_Hours = (float)buffer_get_int32(pdata, &ind) / 10000;
			VESC_CAN_RX_DATA.pSTATUS_3->Watt_Hours_Charged = (float)buffer_get_int32(pdata, &ind) / 10000;
		break;
		
		case CAN_PACKET_STATUS_4:
			VESC_CAN_RX_DATA.pSTATUS_4->MOSFET_Temp = (float)buffer_get_int16(pdata, &ind) / 10;
			VESC_CAN_RX_DATA.pSTATUS_4->Motor_Temp = (float)buffer_get_int16(pdata, &ind) / 10;
			VESC_CAN_RX_DATA.pSTATUS_4->Total_Input_Current = (float)buffer_get_int16(pdata, &ind) / 10;
			VESC_CAN_RX_DATA.pSTATUS_4->PID_Pos = (float)buffer_get_int16(pdata, &ind) / 50;
		break;
		
		case CAN_PACKET_STATUS_5:
			VESC_CAN_RX_DATA.pSTATUS_5->Tachometer_Value = buffer_get_int32(pdata, &ind);
			VESC_CAN_RX_DATA.pSTATUS_5->Input_Voltage = (float)buffer_get_int16(pdata, &ind) / 10;
		break;

		default:

		break;

	}

	can_rx_queue_head = (can_rx_queue_head + 1) % CAN_RX_QUEUE_SIZE;
	can_rx_queue_size--;
}

void VESC_Process_Command(uint8_t *pdata,uint16_t len,uint8_t reply_to)
{
	int32_t ind = 0;
	uint8_t buffer[RX_BUFFER_SIZE];
	int i;
	int16_t value16;
	int32_t value32;

	if(!len)
	{
		return;
	}

	COMM_PACKET_ID packet_id = pdata[0];
	pdata++;
	len--;

	switch(packet_id)
	{
		case COMM_FW_VERSION:
			buffer[ind++] = COMM_FW_VERSION;
			buffer[ind++] = VESC_FW_VERSION_MAJOR;
			buffer[ind++] = VESC_FW_VERSION_MINOR;
			strncpy((char*)(buffer+ind),HW_NAME,strlen(HW_NAME));
			ind += strlen(HW_NAME);
			buffer[ind++] = '\0';
			memcpy(buffer+ind,(uint8_t*)(UID_BASE),UID_LENGTH);
			ind += UID_LENGTH;
			buffer[ind++] = 0; // Is paired?
			buffer[ind++] = VESC_FW_TEST_VERSION_NUMBER;
			buffer[ind++] = HW_TYPE_VESC_BMS;
			buffer[ind++] = 0; // Custom configs
			if (reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_BMS_GET_VALUES:
			buffer[ind++] = COMM_BMS_GET_VALUES;
			buffer_append_float32(buffer, VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f, 1000000, &ind);
			buffer_append_float32(buffer, VESC_CAN_DATA.pBMS_V_TOT->Charge_Input_Voltage.f, 1000000, &ind);
			buffer_append_float32(buffer, VESC_CAN_DATA.pBMS_I->Input_Current.f, 1000000, &ind);
			buffer_append_float32(buffer, VESC_CAN_DATA.pBMS_I->Input_Current_BMS_IC.f, 1000000, &ind);
			buffer_append_float32(buffer, VESC_CAN_DATA.pBMS_AH_WH->Ah_Counter.f, 1000, &ind);
			buffer_append_float32(buffer, VESC_CAN_DATA.pBMS_AH_WH->Wh_Counter.f, 1000, &ind);
			buffer[ind++] = MAX_CELL_SERIES;
			for(i=0;i<MAX_CELL_SERIES;i++)
			{
				buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_V_CELL->BMS_Single_Voltage[i], &ind);
			}
			for(i=0;i<MAX_CELL_SERIES;i++)
			{
				buffer[ind++] = (VESC_CAN_DATA.pBMS_BAL->BMS_BAT.i & (1 << i)) ? 1 : 0;
			}
			buffer[ind++] = MAX_TEMP_SENSORS;
			for(i=0;i<MAX_TEMP_SENSORS;i++)
			{
				buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[i], &ind);
			}
			buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_HUM->Temp_IC, &ind);
			buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_HUM->Temp_Hum_Sensor, &ind);
			buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_HUM->Humidity, &ind);
			buffer_append_float16(buffer, VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->T_Cell_Max, 100, &ind);
			buffer_append_float16(buffer, VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Soc, 1000, &ind);
			buffer_append_float16(buffer, VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Soh, 1000, &ind);
			buffer[ind++] = CAN_ID;
			buffer_append_float32_auto(buffer, VESC_CAN_DATA.pBMS_AH_WH_CHG_TOTAL->Ah_Charge_Total.f, &ind);
			buffer_append_float32_auto(buffer, VESC_CAN_DATA.pBMS_AH_WH_CHG_TOTAL->Wh_Charge_Total.f, &ind);
			buffer_append_float32_auto(buffer, VESC_CAN_DATA.pBMS_AH_WH_DIS_TOTAL->Ah_Discharge_Total.f, &ind);
			buffer_append_float32_auto(buffer, VESC_CAN_DATA.pBMS_AH_WH_DIS_TOTAL->Wh_Discharge_Total.f, &ind);
			buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_HUM->Pressure, &ind);
			if (reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_REBOOT:
			if(Flag.Power != 0 &&
				 Flag.Charger_ON == 0 &&
				 (VESC_CAN_RX_DATA.pSTATUS->Rpm < 100 &&
				   VESC_CAN_RX_DATA.pSTATUS->Rpm > -100))
			{
				Flag.Software_Reset = 1;
				Flag.Power = 3;
			}
		break;

		case COMM_SHUTDOWN:
			if(Flag.Power != 0 &&
				 Flag.Charger_ON == 0 &&
				 (pdata[0] == 1 || // force
				  (VESC_CAN_RX_DATA.pSTATUS->Rpm < 100 &&
				   VESC_CAN_RX_DATA.pSTATUS->Rpm > -100)))
			{
				if(pdata[1] == 1) // restart
				{
					Flag.Software_Reset = 1;
				}
				Flag.Power = 3;
			}
		break;

		case COMM_TERMINAL_CMD:
			if(len >= sizeof(buffer))
			{
				return;
			}
		  pdata[len] = '\0';
		  VESC_Process_Terminal_Command((char*)pdata,reply_to);
		break;

		default:
		break;
	}
}

void VESC_Process_Terminal_Command(char *str,uint8_t can_id)
{
	VESC_Printf(can_id,"-> %s\n",str);

	if (strcmp(str, "ping") == 0)
	{
		VESC_Printf(can_id,"pong\n");
	}
	else if (strcmp(str, "help") == 0)
	{
		VESC_Printf(
			can_id,
			"Valid commands are:\n"
			"help\n"
			"  Show this help\n"
			"ping\n"
			"  Print pong here to see if the reply works\n"
		);
	}
	else
	{
		VESC_Printf(
			can_id,
			"Invalid command: %s\n"
			"type help to list all available commands\n",
			str
		);
	}
}
