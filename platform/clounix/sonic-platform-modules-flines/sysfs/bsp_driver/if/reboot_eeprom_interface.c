#include "reboot_eeprom_interface.h"
#include "clx_driver.h"
// #include <linux/compiler.h>

extern void clx_driver_clx8000_reboot_eeprom_init(void **reboot_eeprom_driver);

struct reboot_eeprom_fn_if *reboot_eeprom_driver;

static struct driver_map reboot_eeprom_drv_map[] = {
	{"reboot_eeprom_clx8000", clx_driver_clx8000_reboot_eeprom_init},
};

struct reboot_eeprom_fn_if *get_reboot_eeprom(void)
{
	return reboot_eeprom_driver;
}

void reboot_eeprom_if_create_driver(void)
{
	char *driver_type = NULL;
	struct driver_map *it;
	int i;

	REBOOTE2_DBG("reboot_eeprom_if_create_driver\n");
	driver_type = clx_driver_identify(CLX_DRIVER_TYPES_REBOOT_EEPROM);
	REBOOTE2_DBG("reboot_eeprom_if_create_driver type:%s\n", driver_type);
	for (i = 0; i < sizeof(reboot_eeprom_drv_map) / sizeof(reboot_eeprom_drv_map[0]); i++)
	{
		it = &reboot_eeprom_drv_map[i];
		if (strcmp((const char *)driver_type, (const char *)it->name) == 0)
		{
			it->driver_init((void *)&reboot_eeprom_driver);
		}
	}
}
void reboot_eeprom_if_delete_driver(void)
{
}
