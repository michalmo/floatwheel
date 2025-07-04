#include "DVC1124_app.h"
#include "DVC1124.h"

DVC1124_Type	DVC_1124;

#define CurrentSenseResistance_mR      1            //电流采样电阻值（单位mR)
/**************************************************
 * @brie  :DVC1124_Voltage()
 * @note  :DVC1124计算电压
 * @param :无
 * @retval:无
 **************************************************/
void DVC1124_Voltage(void)
{
	float current = 0;
	uint8_t i = 0;
	float K = storage.config.cell_voltage_k;
	float sag_comp_table[AFE_MAX_CELL_CNT];
	static uint8_t first = 0;

	sag_comp_table[0] = storage.config.cell_voltage_sag_comp_0;
	sag_comp_table[1] = storage.config.cell_voltage_sag_comp_1;
	sag_comp_table[2] = storage.config.cell_voltage_sag_comp_2;
	sag_comp_table[3] = storage.config.cell_voltage_sag_comp_3;
	sag_comp_table[4] = storage.config.cell_voltage_sag_comp_4;
	sag_comp_table[5] = storage.config.cell_voltage_sag_comp_5;
	sag_comp_table[6] = storage.config.cell_voltage_sag_comp_6;
	sag_comp_table[7] = storage.config.cell_voltage_sag_comp_7;
	sag_comp_table[8] = storage.config.cell_voltage_sag_comp_8;
	sag_comp_table[9] = storage.config.cell_voltage_sag_comp_9;
	sag_comp_table[10] = storage.config.cell_voltage_sag_comp_10;
	sag_comp_table[11] = storage.config.cell_voltage_sag_comp_11;
	sag_comp_table[12] = storage.config.cell_voltage_sag_comp_12;
	sag_comp_table[13] = storage.config.cell_voltage_sag_comp_13;
	sag_comp_table[14] = storage.config.cell_voltage_sag_comp_14;
	sag_comp_table[15] = storage.config.cell_voltage_sag_comp_15;
	sag_comp_table[16] = storage.config.cell_voltage_sag_comp_16;
	sag_comp_table[17] = storage.config.cell_voltage_sag_comp_17;
	sag_comp_table[18] = storage.config.cell_voltage_sag_comp_18;
	sag_comp_table[19] = storage.config.cell_voltage_sag_comp_19;
	sag_comp_table[20] = storage.config.cell_voltage_sag_comp_20;
	sag_comp_table[21] = storage.config.cell_voltage_sag_comp_21;
	sag_comp_table[22] = storage.config.cell_voltage_sag_comp_22;
	sag_comp_table[23] = storage.config.cell_voltage_sag_comp_23;

	//计算总电压
	DVC_1124.Voltage = (float)(DVC11XX_Calc_VBAT()/1000.0f);
	//VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f = DVC_1124.Voltage;
	
	VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f = 0;
	//计算单节电池电压
	for(i = 0; i < storage.config.cell_num; i++)
	{
		DVC_1124.Single_Voltage[i] = (uint16_t)DVC11XX_Calc_VCell(i);
	}
	
	current = DVC_1124.Current_CC2;
	
	DVC_1124.Single_Voltage_Min = 0xffff;
	DVC_1124.Single_Voltage_Max = 0;

	for(i = 0; i < storage.config.cell_num; i++)	//电芯电压软件补偿
	{
		DVC_1124.Single_Voltage[i] = (DVC_1124.Single_Voltage[i] + ((int16_t)(current * sag_comp_table[i])));

		if(first == 0)	//刚刚开机第一次检测，上一次电芯电压等于本次电芯电压
		{
			DVC_1124.Single_Voltage_Last[i] = DVC_1124.Single_Voltage[i];
		}

		DVC_1124.Single_Voltage[i] = (uint16_t)(DVC_1124.Single_Voltage[i]*K)+(uint16_t)(DVC_1124.Single_Voltage_Last[i]*(1-K));
		DVC_1124.Single_Voltage_Last[i] = DVC_1124.Single_Voltage[i];

		if(DVC_1124.Single_Voltage[i] > DVC_1124.Single_Voltage_Max)
		{
			DVC_1124.Single_Voltage_Max = DVC_1124.Single_Voltage[i];
		}
		if(DVC_1124.Single_Voltage[i] < DVC_1124.Single_Voltage_Min)
		{
			DVC_1124.Single_Voltage_Min = DVC_1124.Single_Voltage[i];
		}

		VESC_CAN_DATA.pBMS_V_CELL->BMS_Single_Voltage[i] = DVC_1124.Single_Voltage[i];
		VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f += (float)(DVC_1124.Single_Voltage[i]/1000.0);
	}
	first = 1;

	VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->V_Cell_Min = DVC_1124.Single_Voltage_Min;
	VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->V_Cell_Max = DVC_1124.Single_Voltage_Max;

	VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Soc = GetPowerLevel(VESC_CAN_DATA.pBMS_V_TOT->Total_Voltage.f);
	//芯片温度
	DVC_1124.IC_Temp = DVC11XX_Calc_ChipTemp();
	VESC_CAN_DATA.pBMS_HUM->Temp_IC = (int16_t)(DVC_1124.IC_Temp*100);
	//GP1温度
	DVC_1124.GP1_Temp = DVC11XX_Calc_BatTemp(GP1);
	VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[1] = (int16_t)(DVC_1124.GP1_Temp*100);
	//GP3温度
	DVC_1124.GP3_Temp = DVC11XX_Calc_BatTemp(GP3);
	VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[0] = (int16_t)(DVC_1124.GP3_Temp*100);
	//GP4温度
	DVC_1124.GP4_Temp = DVC11XX_Calc_BatTemp(GP4);
	VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[2] = (int16_t)(DVC_1124.GP4_Temp*100);

	VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->T_Cell_Max = DVC_1124.GP1_Temp > DVC_1124.GP4_Temp ? DVC_1124.GP1_Temp : DVC_1124.GP4_Temp;
}

/**************************************************
 * @brie  :DVC1124_Task()
 * @note  :DVC1124任务
 * @param :无
 * @retval:无
 **************************************************/
void DVC1124_Task(void)
{	
	static float current_cc2_last = 0;
	
	if(Software_Counter_1ms.DVC_1124 < 20)
	{
		return;
	}
	
	Software_Counter_1ms.DVC_1124 = 0;
	
	if(DVC11XX_ReadRegs(AFE_ADDR_R(0), 2))
	{
		if(g_AfeRegs.R1.VADF)	//VADC已完成转换
		{			
			if(DVC11XX_ReadRegs(AFE_ADDR_R(7), 0x45))	//读电压
			{
				DVC1124_Voltage();
				if(DVC11XX_ReadRegs(AFE_ADDR_R(103), 3))
				{
					//CalcuVolMaxMin();	//电压最大最小值计算
					BalanceProcess();	//自动均衡处理
				}
				
			}
		}
		if(g_AfeRegs.R1.CC1F)	//CADC CC1已完成转换
		{
			if(DVC11XX_ReadRegs(AFE_ADDR_R(2), 2))	//读CC1
			{
				DVC_1124.Current_CC1 = DVC11XX_Calc_CurrentWithCC1(CurrentSenseResistance_mR);
			}
		}
		if(g_AfeRegs.R1.CC2F)	//CADC CC2已完成转换
		{
			if(DVC11XX_ReadRegs(AFE_ADDR_R(4), 3))	//读读CC2
			{
				DVC_1124.Current_CC2 = DVC11XX_Calc_CurrentWithCC2(CurrentSenseResistance_mR);
				VESC_CAN_DATA.pBMS_I->Input_Current_BMS_IC.f = -DVC_1124.Current_CC2;
			}
		}
		
		if(DVC_1124.Current_CC2 != current_cc2_last)	//本次电流不等于上一次电流
		{	
			Software_Counter_1ms.DVC_1124_Res = 0;
			current_cc2_last = DVC_1124.Current_CC2;
		}
		else if(Software_Counter_1ms.DVC_1124_Res > 60000)	//本次电流等于上次电流并且维持了60S
		{
			Software_Counter_1ms.DVC_1124_Res = 0;
			DVC1124_Init();
		}
		
	}
}

float GetPowerLevel(float battery_voltage)
{
	float cell_voltages[11] = {
		storage.config.vc_soc_curve_100,
		storage.config.vc_soc_curve_90,
		storage.config.vc_soc_curve_80,
		storage.config.vc_soc_curve_70,
		storage.config.vc_soc_curve_60,
		storage.config.vc_soc_curve_50,
		storage.config.vc_soc_curve_40,
		storage.config.vc_soc_curve_30,
		storage.config.vc_soc_curve_20,
		storage.config.vc_soc_curve_10,
		storage.config.vc_soc_curve_0
	};

  float battery_cell_voltage = battery_voltage / storage.config.cell_num;

	if (battery_cell_voltage >= cell_voltages[0]) {
		return 1.0;
	} else if (battery_cell_voltage <= cell_voltages[10]) {
		return 0.0;
	} else {
		for (int i = 1; i <= 10; i++) {
			if (battery_cell_voltage >= cell_voltages[i]) {
				return (1.0 - i*0.1) + (0.1 * (battery_cell_voltage - cell_voltages[i])) / (cell_voltages[i - 1] - cell_voltages[i]);
			}
		}
	}
	return 0.0;
}
