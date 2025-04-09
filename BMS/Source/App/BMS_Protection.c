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

/**************************************************
 * @brie  :BMS_Overvoltage_Protection()
 * @note  :��ѹ����
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Overvoltage_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0,val1 = 0;
	static uint8_t lock = 0;
	
	if(lock == 0)
	{
		for(i=0;i<AFE_MAX_CELL_CNT;i++)
		{
			if(DVC_1124.Single_Voltage[i] > 4200)
			{
				val1++;
			}
		}
	}
	else
	{
		val1 = 0xFF; //�Ѿ�������ѹ
	}
	
	if(val1 == 0)	//û�з���Ƿѹ
	{
		Software_Counter_1ms.Overvoltage_Protection_Delay = 0;
	}
	
	if(val1 == 0xFF)	//�Ѿ�����Ƿѹ
	{
		Software_Counter_1ms.Overvoltage_Protection_Delay = 65535;
	}	
	//if(g_AfeRegs.R0.bitmap.COV)		//������ع�ѹ
	if((val1 != 0) && (Software_Counter_1ms.Overvoltage_Protection_Delay > 1000))	//������ѹ������1S
	{
		lock = 1;
		CHARG_OFF;					//�رճ����
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[3] = 9900;	//�������
		Flag.Overvoltage = 1;
		
		if(newBals == 0)	//��ѹ�������
		{
			lock = 0;
			Flag.Overvoltage = 0;
			if((CHARGER == 1) && (Flag.Charging_Overcurrent == 0)) //��ѹ�����������û�з���������
			{
				if((Flag.Overtemperature == 0) && (Flag.Lowtemperature == 0))	//û�з������º͵���
				{
					Flag.Charger_ON = 0;
				}
				
			}
			VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[3] = 0;	//�������
			
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
 * @note  :Ƿѹ����
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Undervoltage_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0,val1 = 0,val2 = 0,val3 = 0;
	static uint8_t lock = 0;
	
	for(i=0;i<AFE_MAX_CELL_CNT;i++)
	{
		if(DVC_1124.Single_Voltage[i] < 2300)
		{
			val3++;
		}
	}
	
	if(val3 != 0)	//�е�о��ѹ����2.3V ֱ�ӹػ�
	{
		DSG_OFF;
		CHG_OFF;
		CHG_OFF;
		PDSG_OFF;
		PCHG_OFF;
		Flag.Power = 3;
		return;
	}
		
	if(lock == 0)
	{
		for(i=0;i<AFE_MAX_CELL_CNT;i++)
		{
			if(DVC_1124.Single_Voltage[i] < 2700)
			{
				val1++;
			}
		}
	}
	else
	{
		val1 = 0xFF; //�Ѿ�����Ƿѹ
	}
	
	if(val1 == 0)	//û�з���Ƿѹ
	{
		Software_Counter_1ms.Undervoltage_Protection_Delay = 0;
	}
	
	if(val1 == 0xFF)	//�Ѿ�����Ƿѹ
	{
		Software_Counter_1ms.Undervoltage_Protection_Delay = 65535;
	}
	
	//if(g_AfeRegs.R0.bitmap.CUV)		//�������Ƿѹ
	if((val1 != 0) && (Software_Counter_1ms.Undervoltage_Protection_Delay > 15000)) //����Ƿѹ��������15S
	{
		lock = 1;
		Flag.Undervoltage = 1;
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[4] = 9900;	//�������
		
		if(CHARGER == 0)	//û����������Ƿѹ�����ػ�
		{
			DSG_OFF;
			CHG_OFF;
			CHG_OFF;
			PDSG_OFF;
			PCHG_OFF;
			Flag.Power = 3;
		}
			
		
		for(i=0;i<AFE_MAX_CELL_CNT;i++)
		{
			if(DVC_1124.Single_Voltage[i] > 3000)
			{
				val2++;
			}
		}
		
		if(val2 == 0)	//���е�ص�ѹ������3.0V
		{
			Flag.Undervoltage = 0;
			lock = 0;
			Software_Counter_1ms.Undervoltage_Protection_Delay = 0;
			
			VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[4] = 0;	//�������
			
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
 * @note  :�ŵ��������
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Discharge_Overcurrent_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0;
	
	//if(g_AfeRegs.R0.bitmap.OCD2)	//����2���ŵ����
	
	if(DVC_1124.Current_CC2 > 110)	//110A����
	{
		Flag.Electric_Discharge_Overcurrent = 1;
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[5] = 9900;	//�������
		
		if(Software_Counter_1ms.Discharge_Overcurrent_Delay >= 1000)	//�ŵ��������1S���ػ�
		{
			DSG_OFF;
			CHG_OFF;
			CHG_OFF;
			PDSG_OFF;
			PCHG_OFF;
			Flag.Power = 3;
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
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[5] = 0;	//�������
	}
}

/**************************************************
 * @brie  :BMS_Charge_Overcurrent_Protection()
 * @note  :����������
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Charge_Overcurrent_Protection(void)
{
	uint8_t clean_flag = 0xFF,r0;
	uint8_t i = 0;
	static uint8_t lock = 0;
	
	//if(g_AfeRegs.R0.bitmap.OCC2)	//����2��������
	
	if(DVC_1124.Current_CC2 < -20)	//20A����
	{
		Flag.Charging_Overcurrent = 1;
		CHARG_OFF;					//�رճ����
		VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[6] = 9900;	//�������
		lock = 1;
		Software_Counter_1ms.Charge_Overcurrent_Delay = 0;
	}
	else if(DVC_1124.Current_CC2 > -1) //1A
	{
		if(lock == 1)
		{
			if(Software_Counter_1ms.Charge_Overcurrent_Delay > 10000)
			{
				lock = 0;
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[6] = 0;	//�������
				
				if((CHARGER == 1) && (Flag.Overvoltage == 0))	//��������벢��û�з�����ѹ
				{
					if((Flag.Overtemperature == 0) && (Flag.Lowtemperature == 0))	//û�з������º͵���
					{
						Flag.Charger_ON = 0;
					}
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
 * @note  :��·����
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Short_Circuit_Protection(void)
{
	if(g_AfeRegs.R0.bitmap.SCD)		//�����ŵ��·
	{
		DSG_OFF;
		CHG_OFF;
		CHG_OFF;
		PDSG_OFF;
		PCHG_OFF;
		Flag.Power = 3;
		Flag.Short_Circuit = 1;
	}
}

/**************************************************
 * @brie  :BMS_Overtemperature_Protection()
 * @note  :���±���
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Overtemperature_Protection(void)
{
	static uint8_t lock = 0;
	
	if(lock == 0)	//û��������
	{
		if( (DVC_1124.IC_Temp > 65.0f) ||
			(DVC_1124.GP3_Temp > 65.0f) ||
			(DVC_1124.GP1_Temp > 55.0f) ||
			(DVC_1124.GP4_Temp > 55.0f)
			) 
		{
			if(Software_Counter_1ms.Overtemperature_Protection_Delay >= 1000)	//������ʱ1S
			{
				Flag.Overtemperature = 1;
				CHARG_OFF;					//�رճ����
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[7] = 9900;	//�������
				lock = 1;
			}
		}
		else
		{
			Software_Counter_1ms.Overtemperature_Protection_Delay = 0;
		}
	}
	else	//�Ѿ���������
	{
		if( (DVC_1124.IC_Temp < 60.0f) &&
			(DVC_1124.GP3_Temp < 60.0f) &&
			(DVC_1124.GP1_Temp < 50.0f) &&
			(DVC_1124.GP4_Temp < 50.0f)
			) 
		{
			Software_Counter_1ms.Overtemperature_Protection_Delay = 0;
			Flag.Overtemperature = 0;
			lock = 0;
			VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[7] = 0;	//������� T9
			
			if((CHARGER == 1) && (Flag.Overvoltage == 0) && (Flag.Charging_Overcurrent == 0))	//��������벢��û�з�����ѹ û�з���������
			{
				if(Flag.Lowtemperature == 0)	//û�з�������
				{
					Flag.Charger_ON = 0;
				}
			}
		}
	}
}

/**************************************************
 * @brie  :BMS_Low_Temperature_Protection()
 * @note  :���±���
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Low_Temperature_Protection(void)
{
	static uint8_t lock = 0;
	
	if(lock == 0)	//û��������
	{
		if(CHARGER == 1)
		{
			if( (DVC_1124.GP1_Temp < 0) ||
				(DVC_1124.GP4_Temp < 0)		
				)
			{
				Flag.Lowtemperature = 1;
				CHARG_OFF;					//�رճ����
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 9900;	//�������
				lock = 1;
			}
		}
		else
		{
			if( (DVC_1124.GP1_Temp < -20.0f) ||
				(DVC_1124.GP4_Temp < -20.0f)		
				)
			{
				Flag.Lowtemperature = 1;
				//û��������û�ж���
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 9900;	//�������
				lock = 1;
			}
		}
	}
	else //�Ѿ���������
	{
		if(CHARGER == 1)
		{
			if( (DVC_1124.GP1_Temp > 5.0f) ||
				(DVC_1124.GP4_Temp > 5.0f)		
				)
			{
				if((CHARGER == 1) && (Flag.Overvoltage == 0) && (Flag.Charging_Overcurrent == 0))	//��������벢��û�з�����ѹ û�з���������
				{
					if((Flag.Overtemperature == 0))	//û�з�������
					{
						Flag.Charger_ON = 0;
					}
				}
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 0;	//�������
				Flag.Lowtemperature = 0;
				lock = 0;
			}
		}
		else
		{
			if( (DVC_1124.GP1_Temp > -15.0f) ||
				(DVC_1124.GP4_Temp > -15.0f)		
				)
			{
				//û�ж���
				VESC_CAN_DATA.pBMS_TEMPS->BMS_Single_Temp[8] = 0;	//�������
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
 * @note  :VESC��ͷ
 * @param :��
 * @retval:��
 **************************************************/
//void VESC_Cock_Head(void)
//{
//	static uint8_t lock = 0;
//	if(lock == 0)
//	{
//		if(Flag.Cock_Head == 1)	//��Ҫ��ͷ
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
 * @note  :DVC1124�쳣����
 * @param :��
 * @retval:��
 **************************************************/
void BMS_Protection_Task(void)
{
	BMS_Overvoltage_Protection();			//��ѹ����		T5
	BMS_Undervoltage_Protection();			//Ƿѹ����		T6
	BMS_Discharge_Overcurrent_Protection();	//�ŵ��������	T7
	BMS_Charge_Overcurrent_Protection();	//����������	T8
	BMS_Short_Circuit_Protection();			//��·����		
	BMS_Overtemperature_Protection();		//���±���		T9
	BMS_Low_Temperature_Protection();		//���±���		T10
	//VESC_Cock_Head();
}



