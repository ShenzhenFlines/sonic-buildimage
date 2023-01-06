#include <linux/io.h>

#include "watchdog_driver_clx8000.h"
#include "clx_driver.h"

//external function declaration
extern void __iomem *clounix_fpga_base;

//internal function declaration
struct watchdog_driver_clx8000 driver_watchdog_clx8000;

/*
 * clx_get_watchdog_identify - Used to get watchdog identify, such as iTCO_wdt
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_identify(void *driver, char *buf, size_t count)
{
   int ret = -1;
    /* add vendor codes here */
    ret = snprintf(buf, count,"hardware_wdt\n");
    return ret;
}

/*
 * clx_get_watchdog_state - Used to get watchdog state,
 * filled the value to buf, 0: inactive, 1: active
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_state(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int  data = 0 ;
    unsigned char value = 0;
    int ret = -1;

    if(NULL != clounix_fpga_base){
        data= readl(clounix_fpga_base + WATCHDOG_ENABLE_CONFIG);
        value = data & WATCHDOG_ENABLE_BIT ;
        
        ret = sprintf(buf,"%d\n",value);
      }    
    else{
        WDT_INFO("fpga resource is not available.\r\n");
        ret = -1;
      }
    return ret;
}

/*
 * clx_get_watchdog_timeleft - Used to get watchdog timeleft,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_timeleft(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    int ret = -1;
    unsigned int  data = 0 ;
    unsigned int  timeleft ;
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }  
    data = readl(clounix_fpga_base + WATCHDOG_SET_CNT);
    data &= WATCHDOG_CNT_MASK ;
    timeleft = readl(clounix_fpga_base + WATCHDOG_CNT_VAL) & WATCHDOG_CNT_MASK ;
    timeleft = data - timeleft ;

    ret = snprintf(buf, count,"%d\n",(int)timeleft);
    return ret;
}

/*
 * clx_get_watchdog_timeout - Used to get watchdog timeout,
 * filled the value to buf
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_timeout(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    int ret = -1;
    unsigned int  timeout ;
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }  
    timeout = readl(clounix_fpga_base + WATCHDOG_SET_CNT) & WATCHDOG_CNT_MASK ;
    ret = snprintf(buf, count,"%d\n",(int)timeout);

    return ret;
}

/*
 * clx_set_watchdog_timeout - Used to set watchdog timeout,
 * @value: timeout value
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_watchdog_timeout(void *driver, int value)
{
    /* add vendor codes here */
    unsigned int  timeout ;
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    timeout = (unsigned int)value & WATCHDOG_CNT_MASK ;
    writel(timeout, clounix_fpga_base + WATCHDOG_SET_CNT);
  
    return 0;
}

/*
 * clx_get_watchdog_enable_status - Used to get watchdog enable status,
 * filled the value to buf, 0: disable, 1: enable
 * @buf: Data receiving buffer
 * @count: length of buf
 *
 * This function returns the length of the filled buffer,
 * otherwise it returns a negative value on failed.
 */
static ssize_t clx_driver_clx8000_get_watchdog_enable_status(void *driver, char *buf, size_t count)
{
    /* add vendor codes here */
    unsigned int  data = 0 ;
    unsigned char value = 0;
    int ret = -1;

    if(NULL != clounix_fpga_base){
        data= readl(clounix_fpga_base + WATCHDOG_ENABLE_CONFIG);
        value = data & WATCHDOG_ENABLE_BIT ;
        
        ret = sprintf(buf,"%d\n",value);
      }    
    else{
        WDT_INFO("fpga resource is not available.\r\n");
        ret = -1;
      }
    return ret;
}

/*
 * clx_set_watchdog_enable_status - Used to set watchdog enable status,
 * @value: enable status value, 0: disable, 1: enable
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_watchdog_enable_status(void *driver, int value)
{
    /* add vendor codes here */
    
    unsigned int  data = 0 ;
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }   
    /*data= readl(clounix_fpga_base + WATCHDOG_ENABLE_CONFIG);*/
    data = (unsigned int)value & WATCHDOG_ENABLE_BIT;
    writel(data, clounix_fpga_base + WATCHDOG_ENABLE_CONFIG);
    return 0;
}

/*
 * clx_driver_clx8000_set_watchdog_wdi - Used to set watchdog input(wdi),
 * @value: any value is ok
 *
 * This function returns 0 on success,
 * otherwise it returns a negative value on failed.
 */
static int clx_driver_clx8000_set_watchdog_wdi(void *driver, int value)
{
    /* add vendor codes here */
    unsigned int  data = 0 ;
    
   /* printk(KERN_INFO "watchdog,clx_driver_clx8000_set_watchdog_wdi,value = %d\r\n",value);*/
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }   
 /*   data= readl(clounix_fpga_base + WATCHDOG_WDI_ADDR);*/
    data = 0x0001;
    writel(data, clounix_fpga_base + WATCHDOG_WDI_ADDR);
    return 0;
}


static int clx_driver_clx8000_watchdog_dev_init(struct watchdog_driver_clx8000 *watchdog)
{
    if (clounix_fpga_base == NULL) {
        WDT_INFO("fpga resource is not available.\r\n");
        return -ENXIO;
    }
    watchdog->watchdog_base = clounix_fpga_base + WATCHDOG_BASE_ADDRESS;

    return DRIVER_OK;
}

//void clx_driver_clx8000_watchdog_init(struct watchdog_driver_clx8000 **watchdog_driver)
void clx_driver_clx8000_watchdog_init(void **watchdog_driver)
{
    struct watchdog_driver_clx8000 *watchdog = &driver_watchdog_clx8000;

    WDT_INFO("clx_driver_clx8000_watchdog_init\n");
    clx_driver_clx8000_watchdog_dev_init(watchdog);
    watchdog->watchdog_if.get_watchdog_identify = clx_driver_clx8000_get_watchdog_identify;
    watchdog->watchdog_if.get_watchdog_state = clx_driver_clx8000_get_watchdog_state;
    watchdog->watchdog_if.get_watchdog_timeleft = clx_driver_clx8000_get_watchdog_timeleft;
    watchdog->watchdog_if.get_watchdog_timeout = clx_driver_clx8000_get_watchdog_timeout;
    watchdog->watchdog_if.set_watchdog_timeout = clx_driver_clx8000_set_watchdog_timeout;
    watchdog->watchdog_if.get_watchdog_enable_status = clx_driver_clx8000_get_watchdog_enable_status;
    watchdog->watchdog_if.set_watchdog_enable_status = clx_driver_clx8000_set_watchdog_enable_status;
    watchdog->watchdog_if.set_watchdog_reset = clx_driver_clx8000_set_watchdog_wdi ;
    *watchdog_driver = watchdog;
    WDT_INFO("WATCHDOG driver clx8000 initialization done.\r\n");
}
//clx_driver_define_initcall(clx_driver_clx8000_watchdog_init);

