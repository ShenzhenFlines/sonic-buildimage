#include <linux/io.h>

#include "sysled_driver_clx8000.h"
#include "clx_driver.h"

// external function declaration
extern void __iomem *clounix_fpga_base;

// internal function declaration
struct sysled_driver_clx8000 driver_sysled_clx8000;

/*
0: dark(灭)
1: green(绿色)
2: yellow(黄色)
3: red(红色)
4: green light flashing(绿色闪烁)
5: yellow light flashing(黄色闪烁)
6: red light flashing(红色闪烁)
7: blue(篮色)
8: blue light flashing(蓝色闪烁)
*/
static char *colors[] = {
    "off",
    "blue",
    "green",
    "yellow",
    "red",
    "blink_green",
};

typedef enum sysled_types
{
    BMC_LED_STATUS,
    PSU_LED_STATUS,
    FAN_LED_STATUS,
    LED_STATUS_RESERVED,
    SYS_LED_STATUS,
    ID_LED_STATUS,
    LED_MAX
} sysled_types_s;

#define BMC_LED_STATUS_BIT_MASK 0xF
#define PSU_LED_STATUS_BIT_MASK (0xF << 4)
#define FAN_LED_STATUS_BIT_MASK (0xF << 8)
#define SYS_LED_STATUS_BIT_MASK (0xF << 16)
#define ID_LED_STATUS_BIT_MASK (0xF << 20)
#define SYSLED_REG_BIT_DATA_RD(idx, mask, data) (((data) >> ((idx)*4)) & mask)
#define SYSLED_REG_BIT_DATA_WR(idx, mask, data) (((data)&mask) << ((idx)*4))
typedef enum user_sysled_state
{
    USER_SYSLED_DARK,
    USER_SYSLED_GREEN,
    USER_SYSLED_YELLOW,
    USER_SYSLED_RED,
    USER_SYSLED_GREEN_BLINKING,
    USER_SYSLED_YELLOW_BLINKING,
    USER_SYSLED_RED_BLINKING,
    USER_SYSLED_BLUE,
    USER_SYSLED_BLUE_BLINKING,
    USER_SYSLED_NOT_SUPPORT
} user_sysled_state_s;

/*PSU FAN*/
enum dev_led_state
{
    DEV_LED_DARK,
    DEV_LED_GREEN,
    DEV_LED_RED,
    DEV_LED_YELLOW,
    DEV_LED_NOT_SUPPORT = 13
};
/*SYSLED*/
enum dev_sysled_state
{
    DEV_SYSLED_DARK,
    DEV_SYSLED_GREEN,
    DEV_SYSLED_RED,
    DEV_SYSLED_YELLOW,
    DEV_SYSLED_GREEN_BLINKING = 9,
    DEV_SYSLED_RED_BLINKING,
    DEV_SYSLED_YELLOW_BLINKING,
    DEV_SYSLED_NOT_SUPPORT = 13
};
/*IDLED*/
enum dev_idled_state
{
    DEV_IDLED_DARK,
    DEV_IDLED_BLUE = 4,
    DEV_IDLED_BLUE_BLINKING = 12,
    DEV_IDLED_NOT_SUPPORT = 13
};

static unsigned char led_state_user_to_dev[] = {DEV_LED_DARK, DEV_LED_GREEN, DEV_LED_YELLOW, DEV_LED_RED,
                                                DEV_LED_NOT_SUPPORT, DEV_LED_NOT_SUPPORT, DEV_LED_NOT_SUPPORT, DEV_LED_NOT_SUPPORT, DEV_LED_NOT_SUPPORT};
static unsigned char sysled_state_user_to_dev[] = {DEV_SYSLED_DARK, DEV_SYSLED_GREEN, DEV_SYSLED_YELLOW, DEV_SYSLED_RED,
                                                   DEV_SYSLED_GREEN_BLINKING, DEV_SYSLED_YELLOW_BLINKING, DEV_SYSLED_RED_BLINKING, DEV_SYSLED_NOT_SUPPORT, DEV_SYSLED_NOT_SUPPORT};
static unsigned char idled_state_user_to_dev[] = {DEV_IDLED_DARK, DEV_IDLED_NOT_SUPPORT, DEV_IDLED_NOT_SUPPORT, DEV_IDLED_NOT_SUPPORT,
                                                  DEV_IDLED_NOT_SUPPORT, DEV_IDLED_NOT_SUPPORT, DEV_IDLED_NOT_SUPPORT, DEV_IDLED_BLUE, DEV_IDLED_BLUE_BLINKING};

static unsigned char led_state_dev_to_user[] = {USER_SYSLED_DARK, USER_SYSLED_GREEN, USER_SYSLED_RED, USER_SYSLED_YELLOW};
static unsigned char sysled_state_dev_to_user[] = {USER_SYSLED_DARK, USER_SYSLED_GREEN, USER_SYSLED_RED, USER_SYSLED_YELLOW, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT,
                                                   USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_GREEN_BLINKING, USER_SYSLED_RED_BLINKING, USER_SYSLED_YELLOW_BLINKING};
static unsigned char idled_state_dev_to_user[] = {USER_SYSLED_DARK, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_BLUE, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT,
                                                  USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_NOT_SUPPORT, USER_SYSLED_BLUE_BLINKING};

ssize_t front_panel_show(struct sysled_driver_clx8000 *sysled, sysled_types_s type, char *buf)
{
    unsigned int data = readl(sysled->sysled_base + FRONT_PANEL_CFG);
    unsigned char bit_info = 0;
    unsigned char user_led_state;
    char *led_type_name[LED_MAX] = {"psu", "sys", "fan", "bmc", "id"};

    if (type > ID_LED_STATUS)
    {
        return -1;
    }

    switch (type)
    {
    case PSU_LED_STATUS:
    case FAN_LED_STATUS:
    case BMC_LED_STATUS:
        bit_info = SYSLED_REG_BIT_DATA_RD(type, 0xF, data);
        user_led_state = led_state_dev_to_user[bit_info];
        break;

    case SYS_LED_STATUS:
        bit_info = SYSLED_REG_BIT_DATA_RD(type, 0xF, data);
        user_led_state = sysled_state_dev_to_user[bit_info];
        break;

    case ID_LED_STATUS:
        bit_info = SYSLED_REG_BIT_DATA_RD(type, 0xF, data);
        user_led_state = idled_state_dev_to_user[bit_info];
        break;

    default:
        user_led_state = DEV_LED_DARK;
        break;
    }

    return sprintf(buf, "%d\n", user_led_state);
}

ssize_t front_panel_store(struct sysled_driver_clx8000 *sysled, sysled_types_s type, unsigned char state)
{
    unsigned int data = readl(sysled->sysled_base + FRONT_PANEL_CFG);
    unsigned char dev_state = 0;
    unsigned char mask = 0;
    char *led_type_name[LED_MAX] = {"psu", "sys", "fan", "bmc", "id"};

    if (type >= LED_MAX)
    {
        return -1;
    }
    if (state >= USER_SYSLED_NOT_SUPPORT)
    {
        return -1;
    }

    switch (type)
    {
    case PSU_LED_STATUS:
        data &= ~(PSU_LED_STATUS_BIT_MASK);
        dev_state = led_state_user_to_dev[state];
        mask = 0xF;
        break;

    case FAN_LED_STATUS:
        data &= ~(FAN_LED_STATUS_BIT_MASK);
        dev_state = led_state_user_to_dev[state];
        mask = 0xF;
        break;

    case BMC_LED_STATUS:
        data &= ~(BMC_LED_STATUS_BIT_MASK);
        dev_state = led_state_user_to_dev[state];
        mask = 0xF;
        break;

    case SYS_LED_STATUS:
        data &= ~(SYS_LED_STATUS_BIT_MASK);
        dev_state = sysled_state_user_to_dev[state];
        mask = 0xF;
        break;

    case ID_LED_STATUS:
        data &= ~(ID_LED_STATUS_BIT_MASK);
        dev_state = idled_state_user_to_dev[state];
        mask = 0xF;
        break;

    default:
        return -1;
    }

    if ((dev_state == DEV_LED_NOT_SUPPORT) || (dev_state == DEV_SYSLED_NOT_SUPPORT) || (dev_state == DEV_IDLED_NOT_SUPPORT))
    {
        return -1;
    }

    data |= SYSLED_REG_BIT_DATA_WR(type, mask, dev_state);
    writel(data, sysled->sysled_base + FRONT_PANEL_CFG);

    return 1;
}
// to be update
ssize_t back_panel_show(struct sysled_driver_clx8000 *sysled, char *buf, int index)
{
    unsigned char data = readb(sysled->sysled_base + BACK_PANEL_CFG);
    unsigned char bit_info = (data >> index) & 0x3;
    char *ret_str;

    switch (index)
    {
    case 0:
        if (bit_info == 0)
            ret_str = OFF(colors);
        else
            ret_str = YELLOW(colors);
        break;

    case 2:
        if (bit_info == 0)
            ret_str = OFF(colors);
        else
            ret_str = BLUE(colors);
        break;

    default:
        break;
    }

    return sprintf(buf, "%s\n", ret_str);
}
// to be update
ssize_t back_panel_store(struct sysled_driver_clx8000 *sysled, const char *buf, int index)
{

    unsigned char data = readb(sysled->sysled_base + BACK_PANEL_CFG);
    unsigned char bit_info;

    if (*buf < '0')
        return 0;

    if (*buf > '3')
        return 0;

    bit_info = *buf - '0';
    bit_info = bit_info << index;

    data = data & (~(LED_MASK << index));

    data = data | bit_info;
    writeb(data, sysled->sysled_base + BACK_PANEL_CFG);

    return 1;
}

/*
 * clx_get_sys_led_status - Used to get sys led status
 * filled the value to buf, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * if not support this attributes filled "NA" to buf,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_sys_led_status(void *driver, char *buf, size_t count)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_show(sysled, SYS_LED_STATUS, buf);
}

/*
 * clx_set_sys_led_status - Used to set sys led status
 * @status: led status, led status value define as below:
 * 0: dark
 * 1: green
 * 2: yellow
 * 3: red
 * 4：blue
 * 5: green light flashing
 * 6: yellow light flashing
 * 7: red light flashing
 * 8：blue light flashing
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_sys_led_status(void *driver, int status)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_store(sysled, SYS_LED_STATUS, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_driver_clx8000_get_bmc_led_status(void *driver, char *buf, size_t count)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_show(sysled, BMC_LED_STATUS, buf);
}

/* Similar to clx_set_sys_led_status */
static int clx_driver_clx8000_set_bmc_led_status(void *driver, int status)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_store(sysled, BMC_LED_STATUS, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_driver_clx8000_get_sys_fan_led_status(void *driver, char *buf, size_t count)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_show(sysled, FAN_LED_STATUS, buf);
}

/* Similar to clx_set_sys_led_status */
static int clx_driver_clx8000_set_sys_fan_led_status(void *driver, int status)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_store(sysled, FAN_LED_STATUS, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_driver_clx8000_get_sys_psu_led_status(void *driver, char *buf, size_t count)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_show(sysled, PSU_LED_STATUS, buf);
}

/* Similar to clx_set_sys_led_status */
static int clx_driver_clx8000_set_sys_psu_led_status(void *driver, int status)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_store(sysled, PSU_LED_STATUS, status);
}

/* Similar to clx_get_sys_led_status */
static ssize_t clx_driver_clx8000_get_id_led_status(void *driver, char *buf, size_t count)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_show(sysled, ID_LED_STATUS, buf);
}

/* Similar to clx_set_sys_led_status */
static int clx_driver_clx8000_set_id_led_status(void *driver, int status)
{
    struct sysled_driver_clx8000 *sysled = (struct sysled_driver_clx8000 *)driver;

    return front_panel_store(sysled, ID_LED_STATUS, status);
}

static int clx_driver_clx8000_sysled_dev_init(struct sysled_driver_clx8000 *sysled)
{
    if (clounix_fpga_base == NULL)
    {
        SYSLED_ERR("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    sysled->sysled_base = clounix_fpga_base + LED_MANAGER_OFFSET;

    return DRIVER_OK;
}

// void clx_driver_clx8000_sysled_init(struct sysled_driver_clx8000 **sysled_driver)
void clx_driver_clx8000_sysled_init(void **sysled_driver)
{
    struct sysled_driver_clx8000 *sysled = &driver_sysled_clx8000;

    SYSLED_INFO("clx_driver_clx8000_sysled_init\n");
    clx_driver_clx8000_sysled_dev_init(sysled);
    sysled->sysled_if.get_sys_led_status = clx_driver_clx8000_get_sys_led_status;
    sysled->sysled_if.set_sys_led_status = clx_driver_clx8000_set_sys_led_status;
    sysled->sysled_if.get_bmc_led_status = clx_driver_clx8000_get_bmc_led_status;
    sysled->sysled_if.set_bmc_led_status = clx_driver_clx8000_set_bmc_led_status;
    sysled->sysled_if.get_sys_fan_led_status = clx_driver_clx8000_get_sys_fan_led_status;
    sysled->sysled_if.set_sys_fan_led_status = clx_driver_clx8000_set_sys_fan_led_status;
    sysled->sysled_if.get_sys_psu_led_status = clx_driver_clx8000_get_sys_psu_led_status;
    sysled->sysled_if.set_sys_psu_led_status = clx_driver_clx8000_set_sys_psu_led_status;
    sysled->sysled_if.get_id_led_status = clx_driver_clx8000_get_id_led_status;
    sysled->sysled_if.set_id_led_status = clx_driver_clx8000_set_id_led_status;
    *sysled_driver = sysled;
    SYSLED_INFO("SYSLED driver clx8000 initialization done.\r\n");
}
// clx_driver_define_initcall(clx_driver_clx8000_sysled_init);
