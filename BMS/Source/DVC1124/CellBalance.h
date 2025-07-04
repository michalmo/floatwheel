#ifndef CELLBALANCE_H__
#define CELLBALANCE_H__

#include "DVC11XX.h"
#include "Voltage.h"
#include "Temperature.h"

extern u32 newBals;
extern uint8_t balance_override[AFE_MAX_CELL_CNT];


float DVC11XX_Calc_ChipTemp(void);
void Balance_Contrl(u32 vlaue);
void OverTempProtect(u8 temp);
void BalanceProcess(void);
void CalcuVolMaxMin(void);

#endif

