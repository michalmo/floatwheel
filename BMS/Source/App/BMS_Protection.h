#ifndef __BMS_PROTECTION_H
#define __BMS_PROTECTION_H

#include "datatypes.h"

#define MAX_LOGGED_FAULTS 20

#define LOGGED_FAULTS_INIT_CODE 560187065

typedef struct {
	uint32_t init_flag;
	uint8_t index;
	bms_fault_data faults[MAX_LOGGED_FAULTS];
	uint64_t time_ms;
} logged_faults_data;

extern logged_faults_data logged_faults;

void BMS_Logged_Faults_Init(void);
void BMS_Protection_Task(void);

#endif
