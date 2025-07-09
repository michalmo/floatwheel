#include "mem.h"
#include "BMS_Protection.h"
#include "DVC11XX.h"
#include "flag.h"
#include "vesc_can.h"
#include "key.h"
#include "mos.h"
#include "CellBalance.h"
#include "charger.h"
#include "DVC1124_app.h"
#include "ppm.h"

logged_faults_data logged_faults __attribute__((__AT__ZERO_INIT(0x20006400)));;

void BMS_Logged_Faults_Init(void)
{
	if(logged_faults.init_flag != LOGGED_FAULTS_INIT_CODE)
	{
		logged_faults.init_flag = LOGGED_FAULTS_INIT_CODE;
		logged_faults.index = 0;
		logged_faults.time_ms = 0;
	}
}

void Log_BMS_Fault(bms_fault_code fault)
{
	bms_fault_data data;
	int i;
	
	data.fault = fault;
	data.fault_time_ms = logged_faults.time_ms;
	data.current_ic = -DVC_1124.Current_CC2;
	data.temp_batt = VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->T_Cell_Max;
	data.temp_pcb = DVC_1124.GP3_Temp;
	data.temp_ic = DVC_1124.IC_Temp;
	data.v_cell_min = DVC_1124.Single_Voltage_Min / 1000.0;
	data.v_cell_max = DVC_1124.Single_Voltage_Max / 1000.0;
	
	logged_faults.faults[logged_faults.index++] = data;
	if(logged_faults.index >= MAX_LOGGED_FAULTS)
	{
		for(i = 0;i < MAX_LOGGED_FAULTS - 1;i++)
		{
			logged_faults.faults[i] = logged_faults.faults[i + 1];
		}
		logged_faults.index = MAX_LOGGED_FAULTS - 1;
	}
}

/**************************************************
 * @brie  :BMS_Overvoltage_Protection()
 * @note  :过压保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Overvoltage_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0,val1 = 0;
	uint16_t charge_end = storage.config.vc_charge_end * 1000;  // mV
	uint16_t charge_high_threshold = storage.config.vc_charge_high_threshold * 1000;  // mV
	uint16_t overvoltage_delay = storage.config.overvoltage_delay * 1000;  // ms
	static uint8_t lock = 0;
	
	if(lock == 0)
	{
		if(DVC_1124.Single_Voltage_Max > charge_end)
		{
			val1 = 1;
		}
	}
	else
	{
		val1 = 0xFF; //已经发生过压
	}
	
	if(val1 == 0)	//没有发生欠压
	{
		Software_Counter_1ms.Overvoltage_Protection_Delay = 0;
	}
	
	if(val1 == 0xFF)	//已经发生欠压
	{
		Software_Counter_1ms.Overvoltage_Protection_Delay = 60000;
	}	
	//if(g_AfeRegs.R0.bitmap.COV)		//发生电池过压
	if((val1 != 0) && (Software_Counter_1ms.Overvoltage_Protection_Delay > overvoltage_delay))	//发生过压并保持1S
	{
		lock = 1;
		CHARG_OFF;					//关闭充电器
		VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 0;
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[3] = 9900;	//错误代码
		if(Flag.Overvoltage == 0)
		{
			Flag.Overvoltage = 1;
			Log_BMS_Fault(BMS_FAULT_CODE_OVERVOLTAGE);
		}
		
		if(DVC_1124.Single_Voltage_Max < charge_high_threshold)
		{
			lock = 0;
			Flag.Overvoltage = 0;
			if(
				CHARGER == 1 && 
				Flag.Charge_Allowed &&
				Flag.Charging_Overcurrent == 0 &&
				Flag.Overtemperature == 0 &&
				Flag.Lowtemperature == 0
			) //过压保护解除并且没有发生充电过流
			{
				Flag.Charger_ON = 0;
				VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 1;
			}
			VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[3] = 0;	//错误代码
			
			r0 = g_AfeRegs.R0.cleanflag;
			clean_flag &= ~(1<<6);
			g_AfeRegs.R0.cleanflag = clean_flag;
			
			while((!DVC11XX_WriteRegs(AFE_ADDR_R(0),1)) && (i < 10))
			{
				i++;
			}
			g_AfeRegs.R0.cleanflag = r0;
		}
	}
}

/**************************************************
 * @brie  :BMS_Undervoltage_Protection()
 * @note  :欠压保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Undervoltage_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0,val1 = 0,val2 = 0,val3 = 0;
	uint16_t charge_min = storage.config.vc_charge_min * 1000;  // mV
	uint16_t charge_start = storage.config.vc_charge_start * 1000;  // mV
	uint16_t discharge_min = storage.config.vc_discharge_min * 1000;  // mV
	uint16_t undervoltage_delay = storage.config.undervoltage_delay * 1000; // ms
	static uint8_t lock = 0;
	
	if(DVC_1124.Single_Voltage_Min < charge_min)
	{
		val3 = 1;
	}
	
	if(val3 != 0)
	{
		if(Software_Counter_1ms.Undervoltage_No_Charge_Delay >= 800) //延时800ms，避免刚上电系统不稳定误触发
		{
			Software_Counter_1ms.Undervoltage_No_Charge_Delay = 60000;
			Flag.Charger_ON = 1;	//单芯电压低于2.3V，不执行充电逻辑即禁止充电
			CHARG_OFF;				//关闭充电器
			VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 0;
		}
	}	
	else
	{
		Software_Counter_1ms.Undervoltage_No_Charge_Delay = 0;
	}
		
	if(lock == 0)
	{
		if(DVC_1124.Single_Voltage_Min < discharge_min)
		{
			val1 = 1;
		}
	}
	else
	{
		val1 = 0xFF; //已经发生欠压
	}
	
	if(val1 == 0)	//没有发生欠压
	{
		Software_Counter_1ms.Undervoltage_Protection_Delay = 0;
	}
	
	if(val1 == 0xFF)	//已经发生欠压
	{
		Software_Counter_1ms.Undervoltage_Protection_Delay = 60000;
	}
	
	//if(g_AfeRegs.R0.bitmap.CUV)		//发生电池欠压
	if((val1 != 0) && (Software_Counter_1ms.Undervoltage_Protection_Delay > 15000)) //发生欠压并保持了15S
	{
		lock = 1;
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[4] = 9900;	//错误代码
		
		if(CHARGER == 0)	//没插入充电器，欠压保护关机
		{
			DSG_OFF; // Cut power after 15s below 2.7V (any cell)
			CHG_OFF;
			CHG_OFF;
			PDSG_OFF;
			PCHG_OFF;
			Flag.Power = 3;
		}
		
		if(Flag.Undervoltage == 0)
		{
			Flag.Undervoltage = 1;
			Log_BMS_Fault(BMS_FAULT_CODE_UNDERVOLTAGE);
		}
		
		if(DVC_1124.Single_Voltage_Min <= charge_start)
		{
			val2 = 1;
		}
		
		if(val2 == 0)	//所有电池电压均大于3.0V
		{
			Flag.Undervoltage = 0;
			lock = 0;
			Software_Counter_1ms.Undervoltage_Protection_Delay = 0;
			
			VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[4] = 0;	//错误代码
			
			r0 = g_AfeRegs.R0.cleanflag;
			clean_flag &= ~(1<<5);
			g_AfeRegs.R0.cleanflag = 0;
			
			while((!DVC11XX_WriteRegs(AFE_ADDR_R(0),1)) && (i < 10))
			{
				i++;
			}
			g_AfeRegs.R0.cleanflag = r0;
		}
	}
}

/**************************************************
 * @brie  :BMS_Discharge_Overcurrent_Protection()
 * @note  :放电过流保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Discharge_Overcurrent_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0;
	float max_discharge_current = storage.config.max_discharge_current;  // A
	uint16_t discharge_overcurrent_delay = storage.config.discharge_overcurrent_delay * 1000;  // ms
	
	//if(g_AfeRegs.R0.bitmap.OCD2)	//发生2级放电过流
	
	if(DVC_1124.Current_CC2 > max_discharge_current)	//110A过流
	{
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[5] = 9900;	//错误代码
		
		if(Software_Counter_1ms.Discharge_Overcurrent_Delay >= discharge_overcurrent_delay)	//放电过流持续1S，关机
		{
			DSG_OFF;// Cut power after 1s+ of more than 110A
			CHG_OFF;
			CHG_OFF;
			PDSG_OFF;
			PCHG_OFF;
			Flag.Power = 3;
		}
			
		if(Flag.Electric_Discharge_Overcurrent == 0)
		{
			Flag.Electric_Discharge_Overcurrent = 1;
			Log_BMS_Fault(BMS_FAULT_CODE_DISCHARGE_OVERCURRENT);
		}
		
		r0 = g_AfeRegs.R0.cleanflag;
		clean_flag &= ~(1<<2);
		g_AfeRegs.R0.cleanflag = clean_flag;
		
		while((!DVC11XX_WriteRegs(AFE_ADDR_R(0),1)) && (i < 10))
		{
			i++;
		}
		g_AfeRegs.R0.cleanflag = r0;
		
	}	
	else
	{
		Flag.Electric_Discharge_Overcurrent = 0;
		Software_Counter_1ms.Discharge_Overcurrent_Delay = 0;
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[5] = 0;	//错误代码
	}
}

/**************************************************
 * @brie  :BMS_Charge_Overcurrent_Protection()
 * @note  :充电过流保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Charge_Overcurrent_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0;
	float min_charge_current = -storage.config.min_charge_current;  // A
	float max_charge_current = -storage.config.max_charge_current;  // A
	uint16_t charge_overcurrent_delay = storage.config.charge_overcurrent_reset_delay * 1000;  // ms
	static uint8_t lock = 0;
	
	//if(g_AfeRegs.R0.bitmap.OCC2)	//发生2级充电过流
	
	if(DVC_1124.Current_CC2 < max_charge_current)	//20A过流
	{
		CHARG_OFF;					//关闭充电器
		VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 0;
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[6] = 9900;	//错误代码
		lock = 1;
		if(Flag.Charging_Overcurrent == 0)
		{
			Flag.Charging_Overcurrent = 1;
			Log_BMS_Fault(BMS_FAULT_CODE_CHARGE_OVERCURRENT);
		}
		Software_Counter_1ms.Charge_Overcurrent_Delay = 0;
	}
	else if(DVC_1124.Current_CC2 > min_charge_current) //1A
	{
		if(lock == 1)
		{
			if(Software_Counter_1ms.Charge_Overcurrent_Delay > charge_overcurrent_delay)
			{
				lock = 0;
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[6] = 0;	//错误代码
				
				if(
					CHARGER == 1 && 
					Flag.Charge_Allowed &&
					Flag.Overvoltage == 0 &&
					Flag.Overtemperature == 0 &&
					Flag.Lowtemperature == 0
				)	//充电器插入并且没有发生过压
				{
					Flag.Charger_ON = 0;
					VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 1;
				}
				
				r0 = g_AfeRegs.R0.cleanflag;
				clean_flag &= ~(1<<1);
				g_AfeRegs.R0.cleanflag = clean_flag;
				
				while((!DVC11XX_WriteRegs(AFE_ADDR_R(0),1)) && (i < 10))
				{
					i++;
				}
				g_AfeRegs.R0.cleanflag = r0;
			}
		}
	}
}

/**************************************************
 * @brie  :BMS_Short_Circuit_Protection()
 * @note  :短路保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Short_Circuit_Protection(void)
{
	if(g_AfeRegs.R0.bitmap.SCD)		//发生放电短路
	{
		DSG_OFF;  // Cut Power: Short circuit detected by BMS chip
		CHG_OFF;
		CHG_OFF;
		PDSG_OFF;
		PCHG_OFF;
		Flag.Power = 3;
		if(Flag.Short_Circuit == 0)
		{
			Flag.Short_Circuit = 1;
			Log_BMS_Fault(BMS_FAULT_CODE_SHORT_CIRCUIT);
		}
	}
}

/**************************************************
 * @brie  :BMS_Overtemperature_Protection()
 * @note  :过温保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Overtemperature_Protection(void)
{
	float charge_max = storage.config.t_charge_max;  // °C
	float charge_high_threshold = storage.config.t_charge_high_threshold;  // °C
	float bms_charge_max = storage.config.t_bms_charge_max;  // °C
	float bms_charge_high_threshold = storage.config.t_bms_charge_high_threshold;  // °C
	uint16_t overtemperature_delay = storage.config.overtemperature_delay * 1000;  // ms
	static uint8_t lock = 0;
	
	if(lock == 0)	//没发生过温
	{
		if( (DVC_1124.IC_Temp > bms_charge_max) ||
			(DVC_1124.GP3_Temp > bms_charge_max) ||
			(DVC_1124.GP1_Temp > charge_max) ||
			(DVC_1124.GP4_Temp > charge_max)
			) 
		{
			if(Software_Counter_1ms.Overtemperature_Protection_Delay >= overtemperature_delay)	//过温延时1S
			{
				CHARG_OFF;					//关闭充电器
				VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 0;
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[7] = 9900;	//错误代码
				lock = 1;
				if(Flag.Overtemperature == 0)
				{
					Flag.Overtemperature = 1;
					Log_BMS_Fault(BMS_FAULT_CODE_OVERTEMPERATURE);
				}
			}
		}
		else
		{
			Software_Counter_1ms.Overtemperature_Protection_Delay = 0;
		}
	}
	else	//已经发生过温
	{
		if( (DVC_1124.IC_Temp < bms_charge_high_threshold) &&
			(DVC_1124.GP3_Temp < bms_charge_high_threshold) &&
			(DVC_1124.GP1_Temp < charge_high_threshold) &&
			(DVC_1124.GP4_Temp < charge_high_threshold )
			) 
		{
			Software_Counter_1ms.Overtemperature_Protection_Delay = 0;
			Flag.Overtemperature = 0;
			lock = 0;
			VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[7] = 0;	//错误代码 T9
			
			if(
				CHARGER == 1 && 
				Flag.Charge_Allowed &&
				Flag.Overvoltage == 0 &&
				Flag.Charging_Overcurrent == 0 &&
				Flag.Lowtemperature == 0
			)	//充电器插入并且没有发生过压 没有发生充电过流
			{
				Flag.Charger_ON = 0;
				VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 1;
			}
		}
	}
}

/**************************************************
 * @brie  :BMS_Low_Temperature_Protection()
 * @note  :低温保护
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Low_Temperature_Protection(void)
{
	float abs_min = storage.config.t_min;  // °C
	float abs_low_threshold = storage.config.t_low_threshold;  // °C
	float charge_min = storage.config.t_charge_min;  // °C
	float charge_low_threshold = storage.config.t_charge_low_threshold;  // °C
	static uint8_t lock = 0;
	
	if(lock == 0)	//没发生低温
	{
		if(CHARGER == 1)
		{
			if( (DVC_1124.GP1_Temp < charge_min) ||
				(DVC_1124.GP4_Temp < charge_min)
				)
			{
				CHARG_OFF;					//关闭充电器
				VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 0;
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 9900;	//错误代码
				lock = 1;
				if(Flag.Lowtemperature == 0)
				{
					Flag.Lowtemperature = 1;
					Log_BMS_Fault(BMS_FAULT_CODE_LOWTEMPERATURE);
				}
			}
		}
		else
		{
			if( (DVC_1124.GP1_Temp < abs_min) ||
				(DVC_1124.GP4_Temp < abs_min)
				)
			{
				//没插入充电器没有动作
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 9900;	//错误代码
				lock = 1;
				if(Flag.Lowtemperature == 0)
				{
					Flag.Lowtemperature = 1;
					Log_BMS_Fault(BMS_FAULT_CODE_LOWTEMPERATURE);
				}
			}
		}
	}
	else //已经发生低温
	{
		if(CHARGER == 1)
		{
			if( (DVC_1124.GP1_Temp > charge_low_threshold) ||
				(DVC_1124.GP4_Temp > charge_low_threshold)
				)
			{
				if(
					CHARGER == 1 && 
					Flag.Charge_Allowed &&
					Flag.Overvoltage == 0 &&
					Flag.Charging_Overcurrent == 0 &&
					Flag.Overtemperature == 0
				)	//充电器插入并且没有发生过压 没有发生充电过流 没有发生充电过温 没有发生高温
				{
					Flag.Charger_ON = 0;
					VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charge_OK = 1;
				}
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 0;	//错误代码
				Flag.Lowtemperature = 0;
				lock = 0;
			}
		}
		else
		{
			if( (DVC_1124.GP1_Temp > abs_low_threshold) ||
				(DVC_1124.GP4_Temp > abs_low_threshold)
				)
			{
				//没有动作
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 0;	//错误代码
				Flag.Lowtemperature = 0;
				lock = 0;
			}
		}
	}
}

//#define K1 -0.05f
//#define B1 1500
/**************************************************
 * @brie  :VESC_Cock_Head()
 * @note  :VESC翘头
 * @param :无
 * @retval:无
 **************************************************/
//void VESC_Cock_Head(void)
//{
//	static uint8_t lock = 0;
//	if(lock == 0)
//	{
//		if(Flag.Cock_Head == 1)	//需要翘头
//		{
//			if(VESC_CAN_RX_DATA.pSTATUS->Rpm > 0)
//			{
//				Set_PPM(1000);
//			}
//			else
//			{
//				Set_PPM(2000);
//			}
//			lock = 1;
//			Software_Counter_1ms.Cock_Head = 0;
//		}
//	}
//	else 
//	{
//		if(Flag.Cock_Head == 0)
//		{
//			lock = 0;
//			Set_PPM(1500);
//		}
//	}
//}
	
/**************************************************
 * @brie  :DVC1124_Abnormal()
 * @note  :DVC1124异常处理
 * @param :无
 * @retval:无
 **************************************************/
void BMS_Protection_Task(void)
{
	BMS_Overvoltage_Protection();			//过压保护		T5
	BMS_Undervoltage_Protection();			//欠压保护		T6
	BMS_Discharge_Overcurrent_Protection();	//放电过流保护	T7
	BMS_Charge_Overcurrent_Protection();	//充电过流保护	T8
	BMS_Short_Circuit_Protection();			//短路保护
	BMS_Overtemperature_Protection();		//过温保护		T9
	BMS_Low_Temperature_Protection();		//低温保护		T10
	//VESC_Cock_Head();
}



