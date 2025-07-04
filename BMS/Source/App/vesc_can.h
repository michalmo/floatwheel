#ifndef __VESC_CAN
#define __VESC_CAN

#include "n32l40x.h"
#include "datatypes.h"
#include "CellBalance.h"
#include "charger.h"
#include "crc.h"
#include "flag.h"

#define HW_NAME "Floatwheel BMS"
#define VESC_FW_VERSION_MAJOR 6 // must be 6 or VESC Tool will warn
#define VESC_FW_VERSION_MINOR 0
#define VESC_FW_TEST_VERSION_NUMBER 0 // must be 0 or VESC Tool will warn
#define CAN_ID 99

#define CAN_TX_QUEUE_SIZE 100
#define CAN_RX_QUEUE_SIZE 20
#define RX_BUFFER_SIZE 520

#define MAX_CELL_SERIES 20
#define MAX_TEMP_SENSORS 9

typedef struct
{
    uint8_t can_id;
    uint8_t can_packet_id;
    uint8_t data[8];
    uint8_t len;
} can_queued_message;

typedef union
{
	float f;
	uint8_t i[4];
}FLOAT_INT_TYP;

typedef union
{
	uint8_t i;
	struct {
		uint8_t Is_Charging:1;
		uint8_t Is_Balancing:1;
		uint8_t Is_Charge_Allowed:1;  // enabled/disabled by user
		uint8_t Is_Charge_OK:1;  // permitted by conditions
		uint8_t b5:1;
		uint8_t b6:1;
		uint8_t b7:1;
		uint8_t b8:1;
	} bits;
} BMS_STAT;

typedef union
{
	uint32_t i;
	uint32_t b32:1;
	uint32_t b31:1;
	uint32_t b30:1;
	uint32_t b29:1;
	uint32_t b28:1;
	uint32_t b27:1;
	uint32_t b26:1;
	uint32_t b25:1;
	uint32_t b24:1;
	uint32_t b23:1;
	uint32_t b22:1;
	uint32_t b21:1;
	uint32_t b20:1;
	uint32_t b19:1;
	uint32_t b18:1;
	uint32_t b17:1;
	uint32_t b16:1;
	uint32_t b15:1;
	uint32_t b14:1;
	uint32_t b13:1;
	uint32_t b12:1;
	uint32_t b11:1;
	uint32_t b10:1;
	uint32_t b9:1;
	uint32_t b8:1;
	uint32_t b7:1;
	uint32_t b6:1;
	uint32_t b5:1;
	uint32_t b4:1;
	uint32_t b3:1;
	uint32_t b2:1;
	uint32_t b1:1;
}INT32_BIT_TYP;

typedef struct
{
	FLOAT_INT_TYP 	Total_Voltage;			//总电压
	FLOAT_INT_TYP 	Charge_Input_Voltage;	//充电器电压
}CAN_BMS_V_TOT;

typedef struct
{
	FLOAT_INT_TYP 	Input_Current;			//输入电流
	FLOAT_INT_TYP 	Input_Current_BMS_IC;	//BMS_IC电流
}CAN_BMS_I;

typedef struct
{
	FLOAT_INT_TYP 	Ah_Counter;				//电池毫安时
	FLOAT_INT_TYP 	Wh_Counter;				//电池W时
}CAN_BMS_AH_WH;

typedef struct
{
	uint16_t		BMS_Single_Voltage[MAX_CELL_SERIES];	//单节电池电压	扩大1000倍发送
}CAN_BMS_V_CELL;

typedef struct
{
	INT32_BIT_TYP	BMS_BAT;				//单节电池状态
}CAN_BMS_BAL;

typedef struct
{
	/*
		0 - BMS temperature GP3
		1 - Battery temperature GP1
		2 - Battery temperature GP4
		3 - High voltage
		4 - Low voltage
		5 - Discharge high current
		6 - Charge high current
		7 - High temperature
		8 - Low temperature
	 */
	uint16_t		BMS_Single_Temp[MAX_TEMP_SENSORS];	//电池温度	扩大100倍发送
}CAN_BMS_TEMPS;

typedef struct
{
	uint16_t 		Humidity;				//湿度	0-10000(0%-100%);
	int16_t			Temp_Hum_Sensor;		//温度	-10000-10000(-100°-100°)
	int16_t			Temp_IC;				//IC温度
	uint16_t 		Pressure;
}CAN_BMS_HUM;

typedef struct
{
	uint16_t		V_Cell_Min;				//单节电池最低电压 扩大1000倍发送
	uint16_t		V_Cell_Max;				//单节电池最高电压 扩大1000倍发送
	uint8_t 		Soc;					//0-255(0%-100%) 充电状态
	uint8_t			Soh;					//0-255(0%-100%) 健康状态
	uint8_t			T_Cell_Max;				//单节电池最大温度
	BMS_STAT		Stat;
}CAN_BMS_SOC_SOH_TEMP_STAT;

typedef struct
{
	FLOAT_INT_TYP	Ah_Charge_Total;		//充电安时
	FLOAT_INT_TYP   Wh_Charge_Total;		//充电瓦时
}CAN_BMS_AH_WH_CHG_TOTAL;

typedef struct
{
	FLOAT_INT_TYP	Ah_Discharge_Total;		//安时
	FLOAT_INT_TYP   Wh_Discharge_Total;		//瓦时
}CAN_BMS_AH_WH_DIS_TOTAL;

typedef struct
{
	CAN_BMS_V_TOT 	*pBMS_V_TOT;
	CAN_BMS_I		*pBMS_I;
	CAN_BMS_AH_WH	*pBMS_AH_WH;
	CAN_BMS_V_CELL	*pBMS_V_CELL;
	CAN_BMS_BAL		*pBMS_BAL;
	CAN_BMS_TEMPS 	*pBMS_TEMPS;
	CAN_BMS_HUM		*pBMS_HUM;
	CAN_BMS_SOC_SOH_TEMP_STAT	*pBMS_SOC_SOH_TEMP_STAT;
	CAN_BMS_AH_WH_CHG_TOTAL		*pBMS_AH_WH_CHG_TOTAL;
	CAN_BMS_AH_WH_DIS_TOTAL		*pBMS_AH_WH_DIS_TOTAL;
}VESC_CAN_TYPE;

extern VESC_CAN_TYPE VESC_CAN_DATA;

extern int can_tx_queue_size;

void VESC_CAN_Transmit_Task(void);

void VESC_Set_BMS_V_TOT(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_I(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_AH_WH(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_V_CELL(VESC_CAN_TYPE *vesc_can_data,uint8_t start_cell_id);
void VESC_Set_BMS_BAL(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_TEMPS(VESC_CAN_TYPE *vesc_can_data,uint8_t start_sensor_id);
void VESC_Set_BMS_HUM(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_SOC_SOH_TEMP_STAT(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_AH_WH_CHG_TOTAL(VESC_CAN_TYPE *vesc_can_data);
void VESC_Set_BMS_AH_WH_DIS_TOTAL(VESC_CAN_TYPE *vesc_can_data);

typedef struct
{
	int32_t Rpm;			//转速
	float	Duty_Cycle;		//占空比
	float	Total_Current;	//总电流
}CAN_STATUS;

typedef struct
{
	float	Amp_Hours_Charged;	//充电的安时
	float	Amp_Hours;			//消耗的安时
}CAN_STATUS_2;

typedef struct
{
	float	Watt_Hours_Charged;	//充电的瓦时
	float	Watt_Hours;			//消耗的瓦时
}CAN_STATUS_3;

typedef struct
{
	float 	PID_Pos;				//不清楚数据类型
	float 	Total_Input_Current;	//输入总电流
	float 	Motor_Temp;				//电机温度
	float 	MOSFET_Temp;			//MOS管温度
}CAN_STATUS_4;

typedef struct
{
	float 	Input_Voltage;		//输入电压
	int32_t Tachometer_Value;	//转速表?不确定
}CAN_STATUS_5;

typedef struct
{
	CAN_STATUS 		*pSTATUS;
	CAN_STATUS_2	*pSTATUS_2;
	CAN_STATUS_3	*pSTATUS_3;
	CAN_STATUS_4	*pSTATUS_4;
	CAN_STATUS_5	*pSTATUS_5;
}VESC_CAN_RX_TYPE;

extern VESC_CAN_RX_TYPE VESC_CAN_RX_DATA;
extern uint8_t rx_buffer[RX_BUFFER_SIZE];

void VESC_CAN_RX_Inte(CanRxMessage *can_rx_struct);
void VESC_CAN_Receive_Task(void);
void VESC_Process_Command(uint8_t *pdata, uint16_t len, uint8_t reply_to);

#endif
