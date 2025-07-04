#ifndef CELLBALANCE_H__
#define CELLBALANCE_H__

#include "DVC11XX.h"
#include "Voltage.h"
#include "Temperature.h"

extern u32 newBals;

float DVC11XX_Calc_ChipTemp(void);
void Balance_Contrl(u32 vlaue);
void OverTempProtect(u8 temp);
void BalanceProcess(void);
void CalcuVolMaxMin(void);

#endif

