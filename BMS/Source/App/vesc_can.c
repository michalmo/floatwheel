#include "vesc_can.h"
#include "datatypes.h"
#include "buffer.h"
#include "confparser.h"
#include "confxml.h"
#include "flash.h"
#include "mos.h"
#include "n32l40x_can.h"
#include "DVC1124_init.h"
#include "BMS_Protection.h"

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

const char* BMS_Convert_Fault_To_Name(bms_fault_code fault)
{
	switch(fault)
	{
		case BMS_FAULT_CODE_NONE:
			return "No fault";
		break;

		case BMS_FAULT_CODE_OVERVOLTAGE:
			return "High Voltage";
		break;

		case BMS_FAULT_CODE_UNDERVOLTAGE:
			return "Low Voltage";
		break;

		case BMS_FAULT_CODE_DISCHARGE_OVERCURRENT:
			return "High Charge Current";
		break;

		case BMS_FAULT_CODE_CHARGE_OVERCURRENT:
			return "Low Charge Current";
		break;

		case BMS_FAULT_CODE_SHORT_CIRCUIT:
			return "Short-Circuit";
		break;

		case BMS_FAULT_CODE_OVERTEMPERATURE:
			return "High Temperature";
		break;

		case BMS_FAULT_CODE_LOWTEMPERATURE:
			return "Low Temperature";
		break;

		default:
			return "Unknown fault";
		break;
	}
}

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
		can_tx_buffer[0] = storage.config.controller_id;
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
		can_tx_buffer[ind++] = storage.config.controller_id;
		can_tx_buffer[ind++] = send;
		buffer_append_uint16(can_tx_buffer, len, &ind);
		crc = crc16(pdata, len);
		buffer_append_uint16(can_tx_buffer, crc, &ind);

		VESC_COMM_CAN_Transmit(can_id,CAN_PACKET_PROCESS_RX_BUFFER,can_tx_buffer,6);
	}
}

void VESC_Send_Notify_Boot(void)
{
	VESC_COMM_CAN_Transmit(
		storage.config.controller_id,
		CAN_PACKET_NOTIFY_BOOT,
		(uint8_t *)HW_NAME,
		(strlen(HW_NAME) < 8) ? strlen(HW_NAME) : 8);
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
	can_tx_buffer[ind++] = storage.config.cell_num;
	if(start_cell_id < storage.config.cell_num)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_V_CELL->BMS_Single_Voltage[start_cell_id], &ind);
	}
	if(start_cell_id+1 < storage.config.cell_num)
	{
		buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_V_CELL->BMS_Single_Voltage[start_cell_id+1], &ind);
	}
	if(start_cell_id+2 < storage.config.cell_num)
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

	can_tx_buffer[ind++] = storage.config.cell_num;
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
	can_tx_buffer[0] = storage.config.controller_id;
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

	if(id == 255 || id == storage.config.controller_id)
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

bool VESC_Check_Command_HW(uint8_t *pdata,int32_t *ind)
{
	HW_TYPE hw = pdata[(*ind)++];
	char *hw_name = (char*)(pdata + *ind);
	*ind += strlen(hw_name) + 1;
	return hw == HW_TYPE_VESC_BMS && strcmp(hw_name, HW_NAME) == 0;
}

void VESC_Process_Command(uint8_t *pdata,uint16_t len,uint8_t reply_to)
{
	int32_t ind = 0;
	uint8_t buffer[RX_BUFFER_SIZE];
	int i;
	main_config_t conf;
	main_config_t *conf_ptr = &conf;
	int conf_ind;
	int16_t value16;
	int32_t value32;
	bool baud_changed = false;
	bool DVC1124_config_changed = false;
	static bool flash_bootloader = false;
	static uint32_t flash_firmware_size = 0;
	static uint16_t flash_prev_trailing_bytes;
	bool flash_res;

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
			buffer[ind++] = 1; // Custom configs
			if (reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_FW_INFO:
			buffer[ind++] = COMM_FW_INFO;
			buffer[ind++] = VESC_FW_VERSION_MAJOR;
			buffer[ind++] = VESC_FW_VERSION_MINOR;
			buffer[ind++] = VESC_FW_TEST_VERSION_NUMBER;
#ifdef GIT_COMMIT_SHA
			uint8_t sha_len = strlen(GIT_COMMIT_SHA) < 46 ? strlen(GIT_COMMIT_SHA) : 46;
			strncpy((char*)(buffer+ind),GIT_COMMIT_SHA,sha_len);
			ind += sha_len;
#endif
			buffer[ind++] = '\0';  // null terminate (possibly empty) GIT_COMMIT_SHA
			buffer[ind++] = '\0';  // empty string in place of USER_GIT_COMMIT_HASH
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
			buffer[ind++] = storage.config.cell_num;
			for(i=0;i<storage.config.cell_num;i++)
			{
				buffer_append_int16(buffer, VESC_CAN_DATA.pBMS_V_CELL->BMS_Single_Voltage[i], &ind);
			}
			for(i=0;i<storage.config.cell_num;i++)
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
			buffer[ind++] = storage.config.controller_id;
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

		case COMM_GET_CUSTOM_CONFIG:
		case COMM_GET_CUSTOM_CONFIG_DEFAULT:
			conf_ptr = &conf;
			conf_ind = pdata[0];

			if(conf_ind != 0) {
				break;
			}

			if(packet_id == COMM_GET_CUSTOM_CONFIG)
			{
				*conf_ptr = storage.config;
			}
			else
			{
				confparser_set_defaults_main_config_t(conf_ptr);
			}

			buffer[ind++] = packet_id;
			buffer[ind++] = conf_ind;
			ind += confparser_serialize_main_config_t(buffer + ind, conf_ptr);
			if(reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_SET_CUSTOM_CONFIG:
			conf_ptr = &conf;
			conf_ind = pdata[0];

			if(conf_ind == 0 &&
				 confparser_deserialize_main_config_t(pdata + 1, conf_ptr))
			{
				if(conf_ptr->can_baud_rate != storage.config.can_baud_rate)
				{
					baud_changed = true;
				}
				if(conf_ptr->short_circuit_detection_voltage != storage.config.short_circuit_detection_voltage ||
					 conf_ptr->short_circuit_detection_time != storage.config.short_circuit_detection_time)
				{
					DVC1124_config_changed = true;
				}
				storage.config = *conf_ptr;
				Flash_Write_Storage();
				if(baud_changed)
				{
					CAN_Config();
				}
				if(DVC1124_config_changed)
				{
					DVC1124_Init();
				}

				buffer[ind++] = packet_id;
				if (reply_to != 0)
				{
					VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
				}
			}
			else if(reply_to != 0)
			{
				VESC_Printf(reply_to, "Warning: Could not set configuration");
			}
		break;

		case COMM_GET_CUSTOM_CONFIG_XML:
			conf_ind = pdata[ind++];

			if(conf_ind != 0)
			{
				break;
			}

			int32_t len_conf = buffer_get_int32(pdata, &ind);
			int32_t ofs_conf = buffer_get_int32(pdata, &ind);

			if((len_conf + ofs_conf) > DATA_MAIN_CONFIG_T__SIZE ||
				 len_conf > (RX_BUFFER_SIZE - 18))
			{
				break;
			}

			ind = 0;
			buffer[ind++] = packet_id;
			buffer[ind++] = conf_ind;
			buffer_append_int32(buffer, DATA_MAIN_CONFIG_T__SIZE, &ind);
			buffer_append_int32(buffer, ofs_conf, &ind);
			memcpy(buffer + ind, data_main_config_t_ + ofs_conf, len_conf);
			ind += len_conf;
			if(reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW:
		case COMM_JUMP_TO_BOOTLOADER_HW:
		case COMM_JUMP_TO_BOOTLOADER_ALL_CAN:
		case COMM_JUMP_TO_BOOTLOADER:
			if(
				(packet_id == COMM_JUMP_TO_BOOTLOADER_ALL_CAN_HW || packet_id == COMM_JUMP_TO_BOOTLOADER_HW) &&
				!VESC_Check_Command_HW(pdata,&ind)
			)
			{
				break;
			}
			if(!flash_bootloader)
			{
				Flash_Enter_Bootloader();
			}
		break;

		case COMM_ERASE_NEW_APP_ALL_CAN_HW:
		case COMM_ERASE_NEW_APP_HW:
		case COMM_ERASE_NEW_APP_ALL_CAN:
		case COMM_ERASE_NEW_APP:
			if(
				(packet_id == COMM_ERASE_NEW_APP_ALL_CAN_HW || packet_id == COMM_ERASE_NEW_APP_HW) &&
				!VESC_Check_Command_HW(pdata,&ind)
			)
			{
				break;
			}
			flash_firmware_size = buffer_get_uint32(pdata,&ind);
			// not compatible with vesc bms bootloader, so repurpose the firmware
			// update command to flash the bootloader based on firmware size
			flash_bootloader = flash_firmware_size <= FLASH_BOOTLOADER_END_ADDRESS - FLASH_BOOTLOADER_START_ADDRESS;
			if(flash_bootloader)
			{
				// to protect the bootloader from being erased accidentally, erase
				// will actually happen right before writing the first chunk of the new
				// bootloader after we've verified that it includes the correct hardware
				// identifier
				flash_res = true;
			}
			else
			{
				flash_res = Flash_Erase_New_Firmware(flash_firmware_size);
			}

			ind = 0;
			buffer[ind++] = COMM_ERASE_NEW_APP;
			buffer[ind++] = flash_res;
			if(reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_ERASE_BOOTLOADER_ALL_CAN_HW:
		case COMM_ERASE_BOOTLOADER_HW:
		case COMM_ERASE_BOOTLOADER_ALL_CAN:
		case COMM_ERASE_BOOTLOADER:
			// not compatible with vesc bms bootloader, always reply false here
			ind = 0;
			buffer[ind++] = COMM_ERASE_BOOTLOADER;
			buffer[ind++] = 0;
			if(reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW:
		case COMM_WRITE_NEW_APP_DATA_HW:
		case COMM_WRITE_NEW_APP_DATA_ALL_CAN:
		case COMM_WRITE_NEW_APP_DATA:
			if(
				(packet_id == COMM_WRITE_NEW_APP_DATA_ALL_CAN_HW || packet_id == COMM_WRITE_NEW_APP_DATA_HW) &&
				!VESC_Check_Command_HW(pdata,&ind)
			)
			{
				break;
			}
			uint32_t new_firmware_offset = buffer_get_uint32(pdata,&ind);
			if(new_firmware_offset == 0)
			{
				flash_firmware_size = buffer_get_uint32(pdata,&ind);
				ind -= 4;
				flash_bootloader = flash_firmware_size <= FLASH_BOOTLOADER_END_ADDRESS - FLASH_BOOTLOADER_START_ADDRESS;
			}
			if(flash_bootloader)
			{
				flash_res = true;
				// VESC Tool prepends firmware with 4 bytes firmware size and 2 bytes
				// crc, this needs to be skipped when flashing the bootloader, while
				// taking care write whole words to flash.
				if(new_firmware_offset == 0)
				{
					ind += 6;
					len -= 2;
					// to protect the bootloader from being erased accidentally, check the
					// first chunk for the correct hardware identifier, and then erase
					// before performing the first write
					flash_res = (
						Flash_Verify_Bootloader_Image_Hardware_Identifier(new_firmware_offset,pdata+ind, len-ind) &&
						Flash_Erase_Bootloader()
					);
				}
				else
				{
					new_firmware_offset -= 8;
					// prepend with unaligned bytes from previous chunk
					ind -= 2;
					*(uint16_t*)(pdata+ind) = flash_prev_trailing_bytes;
					// skip last bytes from middle chunks
					if(new_firmware_offset+len-ind < flash_firmware_size)
					{
						len -= 2;
					}
					// write final chunk to end
					else
					{
						// Pad to multiple of 4 bytes
						while(((len-ind) % 4) != 0)
						{
							pdata[len++] = 0;
						}
					}
				}
				flash_res = flash_res && Flash_Write_Bootloader(new_firmware_offset,pdata+ind, len-ind);
				flash_prev_trailing_bytes = *(uint16_t*)(pdata+len);
			}
			else
			{
				// Pad to multiple of 4 bytes
				while(((len-ind) % 4) != 0)
				{
					pdata[len++] = 0;
				}
				flash_res = Flash_Write_New_Firmware(new_firmware_offset,pdata+ind, len-ind);
			}

			ind = 0;
			buffer[ind++] = COMM_WRITE_NEW_APP_DATA;
			buffer[ind++] = flash_res;
			buffer_append_uint32(buffer,new_firmware_offset,&ind);
			if(reply_to != 0)
			{
				VESC_COMM_CAN_Transmit_Buffer(reply_to,buffer,ind,1);
			}
		break;

		case COMM_FORWARD_CAN:
			VESC_COMM_CAN_Transmit_Buffer(pdata[0],pdata+1,len-1,0);
		break;

		case COMM_BMS_SET_CHARGE_ALLOWED:
			if(pdata[0])
			{
				Flag.Charge_Allowed = 1;
				VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_Allowed = 1;
				if(Flag.Overvoltage == 0 &&
					 Flag.Charging_Overcurrent == 0 &&
					 Flag.Overtemperature == 0 &&
					 Flag.Lowtemperature == 0)
				{
					Flag.Charger_ON = 0;
				}
			}
			else
			{
				CHARG_OFF;
				Flag.Charge_Allowed = 0;
				VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_Allowed = 0;
			}
		break;

		case COMM_BMS_SET_BALANCE_OVERRIDE:
		{
			if (pdata[0] >= 0 && pdata[0] < AFE_MAX_CELL_CNT)
			{
				balance_override[pdata[0]] = pdata[1];
			}
		}
		break;

		case COMM_BMS_RESET_COUNTERS:
			// counters are not supported
			if(pdata[0])
			{
				// reset AH counter
			}
			if(pdata[1])
			{
				// reset WH counter
			}
		break;

		case COMM_BMS_FORCE_BALANCE:
			if(pdata[0])
			{
				Flag.Balance_Allowed = 1;
			}
			else
			{
				Flag.Balance_Allowed = 0;
			}
		break;

		case COMM_BMS_ZERO_CURRENT_OFFSET:
			// current offset is not supported
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
	int i;

	VESC_Printf(can_id,"-> %s\n",str);

	if (strcmp(str, "ping") == 0)
	{
		VESC_Printf(can_id,"pong\n");
	}
	else if (strcmp(str, "fault") == 0)
	{
		bool fault = false;
		if(Flag.Overvoltage)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_OVERVOLTAGE));
		}
		if(Flag.Undervoltage)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_UNDERVOLTAGE));
		}
		if(Flag.Electric_Discharge_Overcurrent)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_DISCHARGE_OVERCURRENT));
		}
		if(Flag.Charging_Overcurrent)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_CHARGE_OVERCURRENT));
		}
		if(Flag.Short_Circuit)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_SHORT_CIRCUIT));
		}
		if(Flag.Overtemperature)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_OVERTEMPERATURE));
		}
		if(Flag.Lowtemperature)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_LOWTEMPERATURE));
		}
		if(!fault)
		{
			fault = true;
			VESC_Printf(can_id,"%s\n", BMS_Convert_Fault_To_Name(BMS_FAULT_CODE_NONE));
		}
	}
	else if (strcmp(str, "faults") == 0)
	{
		if(logged_faults.index == 0)
		{
			VESC_Printf(can_id,"No faults registered\n");
		}
		else
		{
			VESC_Printf(can_id,"The following faults were registered since start:\n");
			for(i = 0;i < logged_faults.index;i++) {
				VESC_Printf(
					can_id,
					"Fault            : %s\n"
					"Fault Age        : %.0f s\n"
					"Current IC       : %.2f A\n"
					"Temp Batt        : %.2f deg C\n"
					"Temp IC          : %.2f deg C\n"
					"Temp PCB         : %.2f deg C\n"
					"V Cell Min       : %.3f V\n"
					"V Cell Max       : %.3f V\n",
					BMS_Convert_Fault_To_Name(logged_faults.faults[i].fault),
					(logged_faults.time_ms - logged_faults.faults[i].fault_time_ms) / 1000.0,
					logged_faults.faults[i].current_ic,
					logged_faults.faults[i].temp_batt,
					logged_faults.faults[i].temp_ic,
					logged_faults.faults[i].temp_pcb,
					logged_faults.faults[i].v_cell_min,
					logged_faults.faults[i].v_cell_max
				);
			}
		}
	}
	else if (strcmp(str, "faults_clear") == 0)
	{
		logged_faults.index = 0;
		VESC_Printf(can_id,"Cleared.\n");
	}
	else if (strcmp(str, "volt") == 0)
	{
		VESC_Printf(can_id,"Input voltage: %.2f\n",VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f);
	}
	else if (strcmp(str, "hw_status") == 0)
	{
		uint8_t* uid =(uint8_t*)UID_BASE;
		VESC_Printf(
			can_id,
			"Hardware: %s\n"
			"Firmware: %d.%d\n"
			"UUID: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n"
			"Charging: %s\n"
			"Balancing: %s\n"
			"Charge Allowed: %s\n"
			"Charge OK: %s\n"
			"CAN TX queue now: %d highest: %d max: %d\n"
			"CAN RX queue now: %d highest: %d max: %d\n"
			"Configuration flash write counter: %d\n",
			HW_NAME,
			VESC_FW_VERSION_MAJOR,
			VESC_FW_VERSION_MINOR,
			uid[0],
			uid[1],
			uid[2],
			uid[3],
			uid[4],
			uid[5],
			uid[6],
			uid[7],
			uid[8],
			uid[9],
			uid[10],
			uid[11],
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charging ? "true" : "false",
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Balancing ? "true" : "false",
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_Allowed ? "true" : "false",
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK ? "true" : "false",
			can_tx_queue_size,
			can_tx_queue_size_max,
			CAN_TX_QUEUE_SIZE,
			can_rx_queue_size,
			can_rx_queue_size_max,
			CAN_RX_QUEUE_SIZE,
			storage.conf_flash_write_cnt
		);
	}
	else if (strcmp(str, "fw_info") == 0)
	{
		VESC_Printf(can_id, "Version: %s", VERSION);
#ifdef GIT_REF_NAME
		VESC_Printf(can_id, "Git Ref: %s", GIT_REF_NAME);
#endif
#ifdef GIT_COMMIT_SHA
		VESC_Printf(can_id, "Git Hash: %s\n", GIT_COMMIT_SHA);
#else
		VESC_Printf(can_id, " ");
#endif
	}
	else if (strcmp(str, "uptime") == 0)
	{
		VESC_Printf(can_id, "Uptime: %.2f s\n", Software_Counter_1ms.System_Time / 1000.0);
	}
	else if (strcmp(str, "bms_get_values") == 0)
	{
		VESC_Printf(
			can_id,
			"V tot: %.2f V charge: %.2f\n"
			"I in: %.2f I in_ic: %.2f\n"
			"Ah: %.2f Wh: %.2f\n"
			"Ah charge tot: %.2f discharge tot: %.2f\n"
			"Wh charge tot: %.2f discharge tot: %.2f",
			VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f,
			VESC_CAN_DATA.pBMS_V_TOT->Charge_Input_Voltage.f,
			VESC_CAN_DATA.pBMS_I->Input_Current.f,
			VESC_CAN_DATA.pBMS_I->Input_Current_BMS_IC.f,
			VESC_CAN_DATA.pBMS_AH_WH->Ah_Counter.f,
			VESC_CAN_DATA.pBMS_AH_WH->Wh_Counter.f,
			VESC_CAN_DATA.pBMS_AH_WH_CHG_TOTAL->Ah_Charge_Total.f,
			VESC_CAN_DATA.pBMS_AH_WH_DIS_TOTAL->Ah_Discharge_Total.f,
			VESC_CAN_DATA.pBMS_AH_WH_CHG_TOTAL->Wh_Charge_Total.f,
			VESC_CAN_DATA.pBMS_AH_WH_DIS_TOTAL->Wh_Discharge_Total.f
		);

		VESC_Printf(can_id,"\nCell\tV\tBalancing");
		for(i=0;i<storage.config.cell_num;i++)
		{
			VESC_Printf(can_id,"C%d\t%.3f\t%s", i+1, VESC_CAN_DATA.pBMS_V_CELL->BMS_Single_Voltage[i] / 1000.0, (VESC_CAN_DATA.pBMS_BAL->BMS_BAT.i & (1 << i)) ? "Yes" : "No");
		}

		VESC_Printf(can_id,"\nTemp\tdeg C\nIc\t%.2f", VESC_CAN_DATA.pBMS_HUM->Temp_IC / 100.0);
		for(i=0;i<MAX_TEMP_SENSORS;i++)
		{
			VESC_Printf(can_id,"T%d\t%.2f", i+2, VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[i] / 100.0);
		}

		VESC_Printf(
			can_id,
			"\nHum: %.2f temp: %.2f\n"
			"Highest cell temp: %.2f\n"
			"Soc: %.2f Soh: %.2f\n"
			"Can id: %d\n",
			VESC_CAN_DATA.pBMS_HUM->Humidity / 100.0,
			VESC_CAN_DATA.pBMS_HUM->Temp_Hum_Sensor / 100.0,
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->T_Cell_Max,
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Soc * 100,
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Soh * 100,
			storage.config.controller_id
		);
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
			"fault\n"
			"  Prints the current fault code\n"
			"faults\n"
			"  Prints all stored fault codes and conditions when they arrived\n"
			"faults_clear\n"
			"  Clears all stored fault codes\n"
			"volt\n"
			"  Prints different voltages\n"
			"hw_status\n"
			"  Print some hardware status information.\n"
			"fw_info\n"
			"  Print detailed firmware info.\n"
			"uptime\n"
			"  Prints how many seconds have passed since boot.\n"
			"bms_get_values\n"
			"  Print bms data readings.\n"
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
