#include "conf_general.h"
#include "confparser.h"

main_config_t config;

void Config_Init()
{
  confparser_set_defaults_main_config_t(&config);
}
