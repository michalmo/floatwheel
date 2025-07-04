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
#define CAN_ID 99

#include "conf_default.h"

#ifndef HW_DEFAULT_ID
#define HW_DEFAULT_ID CONF_CONTROLLER_ID
#endif

extern storage_data storage;

void Config_Init();

#endif
