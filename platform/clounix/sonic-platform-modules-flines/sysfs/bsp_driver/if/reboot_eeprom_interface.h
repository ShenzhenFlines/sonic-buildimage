#ifndef _REBOOT_EEPROM_INTERFACE_H_
#define _REBOOT_EEPROM_INTERFACE_H_

#include "device_driver_common.h"
#include "clx_driver.h"

// interface for REBOOT EEPROM
struct reboot_eeprom_fn_if
{
    int (*get_reboot_eeprom_size)(void *driver);
    ssize_t (*read_reboot_eeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
    ssize_t (*write_reboot_eeprom_data)(void *driver, char *buf, loff_t offset, size_t count);
};

#define REBOOT_EEPROM_DEV_VALID(dev) \
    if (dev == NULL)                 \
        return (-1);

// REBOOT EEPROM ERROR CODE
// #define ERR_REBOOT_EEPROM_INIT_FAIL ((ERR_MODULLE_REBOOT_EEPROM << 16) | 0x1)
// #define ERR_REBOOT_EEPROM_REG_FAIL ((ERR_MODULLE_REBOOT_EEPROM << 16) | 0x2)

extern int g_dev_loglevel[];
#define REBOOT_EEPROM_LOG_INFO(_prefix, fmt, args...)                                        \
    do                                                                                       \
    {                                                                                        \
        dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_REBOOT_EEPROM], INFO, fmt, ##args); \
    } while (0)

#define REBOOT_EEPROM_LOG_ERR(_prefix, fmt, args...)                                        \
    do                                                                                      \
    {                                                                                       \
        dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_REBOOT_EEPROM], ERR, fmt, ##args); \
    } while (0)

#define REBOOT_EEPROM_LOG_DBG(_prefix, fmt, args...)                                        \
    do                                                                                      \
    {                                                                                       \
        dev_log(_prefix, g_dev_loglevel[CLX_DRIVER_TYPES_REBOOT_EEPROM], DBG, fmt, ##args); \
    } while (0)

#define REBOOTE2_INFO(fmt, args...) REBOOT_EEPROM_LOG_INFO("rebooteeprom@INFO ", fmt, ##args)
#define REBOOTE2_ERR(fmt, args...) REBOOT_EEPROM_LOG_ERR("rebooteeprom@ERR ", fmt, ##args)
#define REBOOTE2_DBG(fmt, args...) REBOOT_EEPROM_LOG_DBG("rebooteeprom@DBG ", fmt, ##args)

struct reboot_eeprom_fn_if *get_reboot_eeprom(void);
void reboot_eeprom_if_create_driver(void);
void reboot_eeprom_if_delete_driver(void);
#endif //_REBOOT_EEPROM_INTERFACE_H_
