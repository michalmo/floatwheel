#include "vesc_can.h"

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
};

CAN_BMS_SOC_SOH_TEMP_STAT	BMS_SOC_SOH_TEMP_STAT = 
{
	.V_Cell_Min = 0,				//单节电池最低电压 扩大1000倍发送
	.V_Cell_Max = 0,				//单节电池最高电压 扩大1000倍发送
	.Soc = 0,						//0-255(0%-100%)
	.Soh = 0,						//0-255(0%-100%)
	.T_Cell_Max = 0,				//单节电池最大温度
	.Stat = 0,						//
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

uint8_t can_tx_buffer[8];

uint8_t VESC_COMM_CAN_Transmit(CanTxMessage *can_tx_struct,uint8_t can_id,CAN_PACKET_ID can_packet_id,uint8_t *buffer,unsigned int len)
{
	int i;
	uint32_t eid = (can_id|(can_packet_id<<8));

	if(len>8) return CAN_TxSTS_NoMailBox;

	can_tx_struct->StdId = 0;
	can_tx_struct->ExtId = eid;
	can_tx_struct->IDE = CAN_ID_EXT;
	can_tx_struct->RTR = CAN_RTRQ_DATA;
	can_tx_struct->DLC = len;
	for(i=0;i<len;i++){
		can_tx_struct->Data[i] = buffer[i];
	}

	return CAN_TransmitMessage(CAN,can_tx_struct);
}

/**************************************************
 * @brie  :VESC_Set_BMS_V_TOT()
 * @note  :设置总电压 	充电器电压
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_V_TOT(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_V_TOT->Total_Voltage.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_V_TOT->Charge_Input_Voltage.f, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_V_TOT,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_I()
 * @note  :设置输入电流 	BMS_IC电流
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_I(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_I->Input_Current.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_I->Input_Current_BMS_IC.f, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_I,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_AH_WH()
 * @note  :设置电池毫安时 	电池W时 
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_AH_WH(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH->Ah_Counter.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH->Wh_Counter.f, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_AH_WH,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_V_CELL()
 * @note  :设置单节电池电压 
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_V_CELL(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data,uint8_t start_cell_id)
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
	
	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_V_CELL,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_BAL()
 * @note  :设置单节电池状态 
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_BAL(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;

	can_tx_buffer[ind++] = MAX_CELL_SERIES;
	can_tx_buffer[ind++] = 0;
	can_tx_buffer[ind++] = 0;
	can_tx_buffer[ind++] = 0;
	buffer_append_uint32(can_tx_buffer, vesc_can_data->pBMS_BAL->BMS_BAT.i, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_BAL,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_TEMPS()
 * @note  :设置温度 
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_TEMPS(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data,uint8_t start_sensor_id)
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
	
	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_TEMPS,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_HUM()
 * @note  :设置湿度 温度 IC温度  
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_HUM(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Temp_Hum_Sensor, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Humidity, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_HUM->Temp_IC, &ind);
	buffer_append_int16(can_tx_buffer, 0, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_HUM,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_SOC_SOH_TEMP_STAT()
 * @note  :设置单节电池最低电压
 *			   单节电池最高电压
 *			    Soc
 *				Soh
 *			   单节电池最大温度
 *			   状态
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_SOC_SOH_TEMP_STAT(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->V_Cell_Min, &ind);
	buffer_append_int16(can_tx_buffer, vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->V_Cell_Max, &ind);
	can_tx_buffer[ind++] =  (uint8_t)(vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->Soc * 255);
	can_tx_buffer[ind++] =  (uint8_t)(vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->Soh * 255);
	can_tx_buffer[ind++] =  (uint8_t)(roundf(vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->T_Cell_Max));
	can_tx_buffer[ind++] =  vesc_can_data->pBMS_SOC_SOH_TEMP_STAT->Stat.i;

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_SOC_SOH_TEMP_STAT,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_AH_WH_CHG_TOTAL()
 * @note  :设置充电安时 	充电瓦时
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_AH_WH_CHG_TOTAL(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_CHG_TOTAL->Ah_Charge_Total.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_CHG_TOTAL->Wh_Charge_Total.f, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_AH_WH_CHG_TOTAL,can_tx_buffer,ind);
}

/**************************************************
 * @brie  :VESC_Set_BMS_AH_WH_DIS_TOTAL()
 * @note  :设置安时 瓦时
 * @param :can_tx_struct	CAN发送结构体
 *		   vesc_can_data	VESC_CAN_TYPE
 * @retval:无
 **************************************************/
void VESC_Set_BMS_AH_WH_DIS_TOTAL(CanTxMessage *can_tx_struct,VESC_CAN_TYPE *vesc_can_data)
{
	int ind = 0;
	
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_DIS_TOTAL->Ah_Discharge_Total.f, &ind);
	buffer_append_float32_auto(can_tx_buffer, vesc_can_data->pBMS_AH_WH_DIS_TOTAL->Wh_Discharge_Total.f, &ind);

	VESC_COMM_CAN_Transmit(can_tx_struct,0xFF,CAN_PACKET_BMS_AH_WH_DIS_TOTAL,can_tx_buffer,ind);
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

void VESC_CAN_RX_Inte(CanRxMessage *can_rx_struct,VESC_CAN_RX_TYPE *vesc_can_rx_data)
{
	uint16_t vesc_can_cmd;
	uint8_t  *pdata =  can_rx_struct->Data;
	
	int ind = 0;

	vesc_can_cmd = can_rx_struct->ExtId>>8;

	switch(vesc_can_cmd)
	{
		case CAN_PACKET_STATUS:
			vesc_can_rx_data->pSTATUS->Rpm = buffer_get_int32(pdata, &ind);
			vesc_can_rx_data->pSTATUS->Total_Current = (float)buffer_get_int16(pdata, &ind) / 10;
			vesc_can_rx_data->pSTATUS->Duty_Cycle = (float)buffer_get_int16(pdata, &ind) / 1000;
		break;
		
		case CAN_PACKET_STATUS_2:
			vesc_can_rx_data->pSTATUS_2->Amp_Hours = (float)buffer_get_int32(pdata, &ind) / 10000;
			vesc_can_rx_data->pSTATUS_2->Amp_Hours_Charged	= (float)buffer_get_int32(pdata, &ind) / 10000;
		break;
		
		case CAN_PACKET_STATUS_3:
			vesc_can_rx_data->pSTATUS_3->Watt_Hours = (float)buffer_get_int32(pdata, &ind) / 10000;
			vesc_can_rx_data->pSTATUS_3->Watt_Hours_Charged = (float)buffer_get_int32(pdata, &ind) / 10000;
		break;
		
		case CAN_PACKET_STATUS_4:
			vesc_can_rx_data->pSTATUS_4->MOSFET_Temp = (float)buffer_get_int16(pdata, &ind) / 10;
			vesc_can_rx_data->pSTATUS_4->Motor_Temp = (float)buffer_get_int16(pdata, &ind) / 10;
			vesc_can_rx_data->pSTATUS_4->Total_Input_Current = (float)buffer_get_int16(pdata, &ind) / 10;
			vesc_can_rx_data->pSTATUS_4->PID_Pos = (float)buffer_get_int16(pdata, &ind) / 50;
		break;
		
		case CAN_PACKET_STATUS_5:
			vesc_can_rx_data->pSTATUS_5->Tachometer_Value = buffer_get_int32(pdata, &ind);
			vesc_can_rx_data->pSTATUS_5->Input_Voltage = (float)buffer_get_int16(pdata, &ind) / 10;
		break;

		default:

		break;

	}
}
