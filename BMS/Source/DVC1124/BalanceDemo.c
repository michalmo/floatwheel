/******************************************************************************
;  *   	@MCU				 STM32F103C8T6
;  *   	@Create Date         2023.03.16
;  *    @Official website		 http://www.devechip.com/
;  *----------------------Abstract Description---------------------------------
;  *			    						 		自动均衡处理
**************************************************************************************/
#include "CellBalance.h"
#include "DVC1124_app.h"
#include "datatypes.h"
#include <math.h>

u8 bOTC;//过温标志位
int uiCellVmin,uiCellVmax;
int	uiCellvotage[AFE_MAX_CELL_CNT];
u32	uiBalMaskFlags=0,uiBalMaskFlags_Prepared=0;
u32 newBals = 0;
u32 newBals_max = 0;
/*
	Override balancing decision per cell
	0: Do not override balancing
	1: Override and disable balancing on cell
	2: Override and enable balancing on cell
 */
uint8_t balance_override[AFE_MAX_CELL_CNT];

/**
	* @说明	芯片过温保护
	* @参数	过温阈值 u8 temp
	* @返回值	
	* @注	
*/
void OverTempProtect(u8 temp){
float t;
	t=DVC11XX_Calc_ChipTemp();//获取芯片温度
	
	if(t>=temp)
	bOTC=1;		//过温置位
	else
	bOTC=0;
}

/**
	* @说明	电芯最大最小电压计算
	* @参数	
	* @返回值	
	* @注	
*/
void CalcuVolMaxMin(void){
	s8 cellIndex;
	u8 i;
	int cellVoltage;

	for(i=0;i<storage.config.cell_num;i++)
	{
		uiCellvotage[i]=DVC11XX_Calc_VCell(i);//各串电压实际值还原
	}

	uiCellVmin = uiCellVmax = uiCellvotage[0];//初始化

	for(cellIndex=storage.config.cell_num-1;cellIndex>=0;cellIndex--)
	{
		cellVoltage=uiCellvotage[cellIndex];
		if( cellVoltage>uiCellVmax)
		{
			uiCellVmax = cellVoltage;
			if(uiCellVmin==0)
			{
				uiCellVmin=uiCellVmax;
			}
		}
		else if(cellVoltage<uiCellVmin && cellVoltage>0)
		{//防0
			uiCellVmin = cellVoltage;
		}
	}
}

/**
	* @说明	自动均衡处理
	* @参数	
	* @返回值	
	* @注	
*/
void BalanceProcess(void)
{
	static u32 shouldBals = 0;
	u8 i;
	uint16_t charge_start = storage.config.vc_charge_start * 1000;  // mV
	uint16_t balance_min = storage.config.vc_balance_min * 1000;  // mV
	uint16_t balance_start = storage.config.vc_balance_start * 1000;  // mV
	uint16_t balance_end = storage.config.vc_balance_end * 1000;  // mV
	uint16_t balance_high_threshold = storage.config.vc_balance_high_threshold_delta * 1000;  // mV
	uint16_t limit;
	
	newBals = 0;
	
	uiBalMaskFlags=(g_AfeRegs.R103_R105.CB[0]<<16)+(g_AfeRegs.R103_R105.CB[1]<<8)+g_AfeRegs.R103_R105.CB[2];//刷新当前均衡位
	
	for(i=0;i<storage.config.cell_num;i++)
	{
		limit = shouldBals & (1<<i) ? balance_end : balance_start;
		if(
			// only balance above the minimum threshold
			DVC_1124.Single_Voltage[i] > balance_min &&
			// ... only down to within vc_balance_start to vc_balance_end of the lowest cell
			DVC_1124.Single_Voltage[i] > DVC_1124.Single_Voltage_Min + limit
		)
		{
			shouldBals |= (1<<i);
			if (
				// ... and only drain the highest cells
				DVC_1124.Single_Voltage_Max - DVC_1124.Single_Voltage[i] <= balance_high_threshold
			)
			{
				newBals |= (1<<i);	//计算出电芯最大电压需要开启均衡的各个位
			}
		}
	}
	if(!newBals)
	{
		// balancing ended - reset flags
		shouldBals = 0;
	}
	if(
		// ... respect the balance mode setting
		storage.config.balance_mode == BALANCE_MODE_DISABLED ||
		(storage.config.balance_mode == BALANCE_MODE_CHARGING_ONLY && !VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Charging) ||
		// ... respect the temporary balance override
		Flag.Balance_Allowed == 0 ||
		// ... respect the current limit setting
		fabsf(DVC_1124.Current_CC2) > storage.config.balance_max_current)
	{
		newBals = 0;
	}
	for(i=0;i<storage.config.cell_num;i++)
	{
		// ... respect per cell overrides
		switch(balance_override[i])
		{
			case 1:
				newBals &= ~(1<<i);
			break;

			case 2:
				// safety net to prevent discharging down to zero
				if(DVC_1124.Single_Voltage[i] > charge_start)
				{
					newBals |= (1<<i);
				}
			break;

			default:
			break;
		}
	}
	if(newBals != uiBalMaskFlags)
	{
		Balance_Contrl(newBals);
		VESC_CAN_DATA.pBMS_BAL->BMS_BAT.i = newBals;
		VESC_CAN_DATA.pBMS_SOC_SOH_TEMP_STAT->Stat.bits.Is_Balancing = newBals ? 1 : 0;
	}
	
//	for(i=0;i<storage.config.cell_num;i++)
//	{
//		if(uiCellvotage[i] > uiBalanceVol_max)
//		{
//			newBals_max |= (1<<i);	//计算出电芯最大电压需要开启均衡的各个位
//		}
//	}
//	
//	if(((uiCellVmin>uiBalanceVol)&&(uiCellVmax-uiCellVmin)>=uiBalanceVolDiff) || (newBals_max != 0))
//	{//未过温、最低电压高于均衡开启最小电压、压差高于均衡开启阈值
//		
//		int cellVoltage=0;
//		for(i=0;i<storage.config.cell_num;i++)
//		{
//			cellVoltage=uiCellvotage[i];
//			if( (cellVoltage-uiCellVmin)>=uiBalanceVolDiff)
//			{
//				newBals |= (1<<i);	//计算出需要开启均衡的各个位
//			}
//		}
//		newBals |= newBals_max;
//	}
//	if(newBals!=uiBalMaskFlags)
//	{ //均衡位变化
//		if(newBals==uiBalMaskFlags_Prepared)
//		{
//			
//			uiBalMaskFlags=uiBalMaskFlags_Prepared;

//			Balance_Contrl(uiBalMaskFlags);
//			
//		}	
//		else
//		uiBalMaskFlags_Prepared=newBals; 
//	}
//	else 
//	{
//		uiBalMaskFlags_Prepared=uiBalMaskFlags;
//	}
		  
}
