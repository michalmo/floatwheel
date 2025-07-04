#include "conf_general.h"
#include "confparser.h"
#include "flash.h"

storage_data storage;

void Config_Init()
{
	Flash_Init();

	if (storage.controller_id_init_flag != VAR_INIT_CODE ||
			storage.can_baud_rate_init_flag != VAR_INIT_CODE ||
			storage.conf_flash_write_cnt_init_flag != VAR_INIT_CODE)
	{
		Flash_Load_Storage();
	}

	if (storage.controller_id_init_flag != VAR_INIT_CODE)
	{
		storage.controller_id = HW_DEFAULT_ID;
		storage.controller_id_init_flag = VAR_INIT_CODE;
	}

	if (storage.can_baud_rate_init_flag != VAR_INIT_CODE) {
		storage.can_baud_rate = CONF_CAN_BAUD_RATE;
		storage.can_baud_rate_init_flag = VAR_INIT_CODE;
	}

	if (storage.conf_flash_write_cnt_init_flag != VAR_INIT_CODE) {
		storage.conf_flash_write_cnt = 0;
		storage.conf_flash_write_cnt_init_flag = VAR_INIT_CODE;
	}

	if (storage.config_init_flag != MAIN_CONFIG_T_SIGNATURE) {
		confparser_set_defaults_main_config_t((main_config_t*)(&storage.config));
		storage.config_init_flag = MAIN_CONFIG_T_SIGNATURE;
		storage.config.controller_id = storage.controller_id;
		storage.config.can_baud_rate = storage.can_baud_rate;
	}
}
