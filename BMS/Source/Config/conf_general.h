#include "datatypes.h"

#ifndef CONF_GENERAL_H_
#define CONF_GENERAL_H_

// Init codes for the persistent storage. Change the config code when updating
// the config struct in a way that is not backwards compatible.
#define VAR_INIT_CODE 93462984

#define HW_NAME "Floatwheel BMS"
#define VESC_FW_VERSION_MAJOR 6 // must be 6 or VESC Tool will warn
#define VESC_FW_VERSION_MINOR 5 // must be 5 or VESC Tool will enter limited mode
#define VESC_FW_TEST_VERSION_NUMBER 0 // must be 0 or VESC Tool will warn

#ifdef S50S
// 50S
#define CONF_SOC_CURVE_100 4.2
#define CONF_SOC_CURVE_90 4.075
#define CONF_SOC_CURVE_80 4.04
#define CONF_SOC_CURVE_70 3.9
#define CONF_SOC_CURVE_60 3.82
#define CONF_SOC_CURVE_50 3.735
#define CONF_SOC_CURVE_40 3.64
#define CONF_SOC_CURVE_30 3.52
#define CONF_SOC_CURVE_20 3.375
#define CONF_SOC_CURVE_10 3.16
#define CONF_SOC_CURVE_0 3
#endif

#ifdef P42A
// P42A
#define CONF_SOC_CURVE_100 4.2
#define CONF_SOC_CURVE_90 4.065
#define CONF_SOC_CURVE_80 3.938
#define CONF_SOC_CURVE_70 3.854
#define CONF_SOC_CURVE_60 3.776
#define CONF_SOC_CURVE_50 3.695
#define CONF_SOC_CURVE_40 3.618
#define CONF_SOC_CURVE_30 3.543
#define CONF_SOC_CURVE_20 3.460
#define CONF_SOC_CURVE_10 3.342
#define CONF_SOC_CURVE_0 3
#endif

#ifdef DG40
// DG40
#define CONF_SOC_CURVE_100 4.2
#define CONF_SOC_CURVE_90 4.047
#define CONF_SOC_CURVE_80 3.944
#define CONF_SOC_CURVE_70 3.867
#define CONF_SOC_CURVE_60 3.799
#define CONF_SOC_CURVE_50 3.717
#define CONF_SOC_CURVE_40 3.6
#define CONF_SOC_CURVE_30 3.498
#define CONF_SOC_CURVE_20 3.381
#define CONF_SOC_CURVE_10 3.237
#define CONF_SOC_CURVE_0 3
#endif

#ifdef VTC6
// Sony VTC6
#define CONF_SOC_CURVE_100 4.2
#define CONF_SOC_CURVE_90 4.064
#define CONF_SOC_CURVE_80 4.015
#define CONF_SOC_CURVE_70 3.895
#define CONF_SOC_CURVE_60 3.821
#define CONF_SOC_CURVE_50 3.745
#define CONF_SOC_CURVE_40 3.655
#define CONF_SOC_CURVE_30 3.559
#define CONF_SOC_CURVE_20 3.459
#define CONF_SOC_CURVE_10 3.292
#define CONF_SOC_CURVE_0 3
#endif

#include "conf_default.h"

#ifndef HW_DEFAULT_ID
#define HW_DEFAULT_ID CONF_CONTROLLER_ID
#endif

extern storage_data storage;

void Config_Init();

#endif
