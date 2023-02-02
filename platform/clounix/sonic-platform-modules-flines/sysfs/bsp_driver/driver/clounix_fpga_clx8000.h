
#ifndef __CLOUNIX_FPGA_H__
#define __CLOUNIX_FPGA_H__
#include <linux/device.h>
#include "clounix/clounix_pub.h"

#define FPGA_PCI_VENDOR_ID 0x10EE
#define FPGA_PCI_DEVICE_ID 0x7021

#define MAX_PORT_NUM                (56)
#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)


struct fpga_device_attribute{
        struct device_attribute dev_attr;
        int index;
};
#define to_fpga_dev_attr(_dev_attr) \
        container_of(_dev_attr, struct fpga_device_attribute, dev_attr)

#define FPGA_ATTR(_name, _mode, _show, _store, _index)        \
        { .dev_attr = __ATTR(_name, _mode, _show, _store),      \
          .index = _index }

#define FPGA_DEVICE_ATTR(_name, _mode, _show, _store, _index) \
struct fpga_device_attribute fpga_dev_attr_##_name          \
        = FPGA_ATTR(_name, _mode, _show, _store, _index)

/*interrupt register define*/
#define FPGA_IRQ_CTRL_ADDR            0x700
#define FPGA_IRQ_STAT_ADDR            0x704
#define FPGA_IRQ_TRIG_ADDR            0x708
#define FPGA_CPLD0_INT_TRIG_ADDR      0x70C
#define FPGA_CPLD1_INT_TRIG_ADDR      0x710
#define FPGA_QDD_IRQ_TRIG_ADDR        0x714
#define FPGA_CPLD0_PRS_TRIG_ADDR      0x718
#define FPGA_CPLD1_PRS_TRIG_ADDR      0x71C
#define FPGA_DEBUG_TRIG_ADDR          0x010

#define IRQ_CTRL_RST_BIT              31
#define IRQ_CTRL_CFG_RST_BIT          0x8000007F
#define IRQ_CTRL_RST_PSU_BIT          0x00000001
#define IRQ_CTRL_RST_TEMP_BIT         0x00000002
#define IRQ_CTRL_RST_PMBUS_BIT        0x00000004
#define IRQ_CTRL_RST_USB_BIT          0x00000008
#define IRQ_CTRL_RST_FAN_BIT          0x00000010
#define IRQ_CTRL_RST_SPF_PRES_BIT     0x00000020
#define IRQ_CTRL_RST_SPF_INT_BIT      0x00000040
#define IRQ_CTRL_DISABLE_INT          0x0000007F
#define IRQ_CTRL_ENABLE_INT           0x00000000

#endif
