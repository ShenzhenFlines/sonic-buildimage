#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/msi.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

#include "clounix_fpga_clx8000.h"
#include "device_driver_common.h"

static int g_fpga_clx8000_loglevel = 0x3;
#define FPGA_CLX8000_INFO(fmt, args...) do {                                        \
    if (g_fpga_clx8000_loglevel & INFO) { \
        printk(KERN_INFO "[FPGA_CLX8000][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_CLX8000_ERR(fmt, args...) do {                                        \
    if (g_fpga_clx8000_loglevel & ERR) { \
        printk(KERN_ERR "[FPGA_CLX8000][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_CLX8000_DBG(fmt, args...) do {                                        \
    if (g_fpga_clx8000_loglevel & DBG) { \
        printk(KERN_DEBUG "[FPGA_CLX8000][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

module_param(g_fpga_clx8000_loglevel, int, 0644);
MODULE_PARM_DESC(g_fpga_clx8000_loglevel, "The log level(info=0x1, err=0x2, dbg=0x4).\n");

void __iomem *clounix_fpga_base = NULL;
EXPORT_SYMBOL(clounix_fpga_base);

#define FPGA_GLOBAL_CFG_BASE 0x100
#define FPGA_RESET_CFG_BASE  (FPGA_GLOBAL_CFG_BASE+0) 
#define RESET_MUX_BIT  4

DEFINE_SPINLOCK(fpga_msi_lock);

static struct notifier_block reboot_nb = {0};
static struct notifier_block restart_nb = {0};


struct int_flag_reg
{
   unsigned int int_flag0_addr;
   unsigned int int_flag1_addr;
   unsigned int int_flag2_addr;
};

struct irq_vector
{
    int irq_mask_bit;
    char *irq_name;
    int mark_type;  /*mark: 1 有细分寄存器要查询；0 无细分寄存器*/
    struct int_flag_reg flag_regs;
};

const struct irq_vector fpga_irq_arry[] = {
    {IRQ_CTRL_RST_PSU_BIT, "psu interrupt",0,{0,0,0}}, /*58*/
    {IRQ_CTRL_RST_TEMP_BIT, "temperature sensor interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_PMBUS_BIT, "PMbus alert interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_USB_BIT, "usb over current  interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_FAN_BIT, "fan alarm interruput",0,{0,0,0}},
    {IRQ_CTRL_RST_SPF_PRES_BIT, "sfp present interruput",1,{FPGA_CPLD0_PRS_TRIG_ADDR,FPGA_CPLD1_PRS_TRIG_ADDR,FPGA_QDD_IRQ_TRIG_ADDR}},
    {IRQ_CTRL_RST_SPF_INT_BIT, "sfp interruput",1,{FPGA_CPLD0_INT_TRIG_ADDR,FPGA_CPLD1_INT_TRIG_ADDR,FPGA_QDD_IRQ_TRIG_ADDR}},
};


#define SYS_LED_BIT (2)
#define SYS_LED_MASK (0x3)
#define sys_led_reg_offset (0x200)
int sys_led_reboot_work(struct notifier_block *nb, unsigned long action, void *data)
{
    unsigned char reg_data = readb(clounix_fpga_base + sys_led_reg_offset);

    reg_data = reg_data & ~(SYS_LED_MASK << SYS_LED_BIT);
    writeb(reg_data, clounix_fpga_base + sys_led_reg_offset);

    return NOTIFY_DONE;
}

void reset_mux_pca9548(void)
{
    uint32_t data=0;
    if(NULL != clounix_fpga_base){
        data= readl(clounix_fpga_base + FPGA_RESET_CFG_BASE);
        SET_BIT(data, RESET_MUX_BIT);
        writel(data, clounix_fpga_base + FPGA_RESET_CFG_BASE);
        udelay(1);
        CLEAR_BIT(data, RESET_MUX_BIT);
        writel(data, clounix_fpga_base + FPGA_RESET_CFG_BASE);
    }
    return;
}
EXPORT_SYMBOL_GPL(reset_mux_pca9548);
/*used for FPGA interrupt register test*/
int fpga_irq_test(void)
{
#if 0
    unsigned int irq_flag_g,int_flag0,int_flag1,int_flag2;
    int total_irq,i;

    spin_lock(&fpga_msi_lock);
    /*pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_DATA_32, &data);*/
    /*get interrupt flag*/
    irq_flag_g = readl(clounix_fpga_base + FPGA_IRQ_TRIG_ADDR);
    /*clear|disable FPGA interrupt flag*/
    writel(IRQ_CTRL_DISABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);
    /*writel(0x00000000, clounix_fpga_base + FPGA_DEBUG_TRIG_ADDR);*/
    /*print interrupt information*/
    total_irq = sizeof(fpga_irq_arry) / sizeof(struct irq_vector);
    for(i=0; i<total_irq; i++){
        if( fpga_irq_arry[i].irq_mask_bit & irq_flag_g ){
            FPGA_CLX8000_ERR("%s: generate %s,irq_flag_g=0x%x.\n", __func__, fpga_irq_arry[i].irq_name,irq_flag_g);
            if(fpga_irq_arry[i].mark_type==1){
                int_flag0 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag0_addr);
                int_flag1 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag1_addr);
                int_flag2 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag2_addr);
                FPGA_CLX8000_ERR("cpld0_flag = 0x%x,cpld1_flag = 0x%x,qdd_flag = 0x%x.\n",int_flag0,int_flag1,int_flag2);
            }
            
        }
    }
    /*enable FPGA interrupt */
    writel(IRQ_CTRL_ENABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);    
    spin_unlock(&fpga_msi_lock);
#endif
    return 0;

}
EXPORT_SYMBOL_GPL(fpga_irq_test);


static irqreturn_t clounix_fpga_irq_hd(int irq, void *dev_id)
{
    struct pci_dev *pdev = dev_id;
    unsigned short data;
    unsigned int irq_flag_g,int_flag0,int_flag1,int_flag2;
    int total_irq,i;

    spin_lock(&fpga_msi_lock);
    pci_read_config_word(pdev, pdev->msi_cap + PCI_MSI_DATA_32, &data);
    /*get interrupt flag*/
    irq_flag_g = readl(clounix_fpga_base + FPGA_IRQ_TRIG_ADDR);
    FPGA_CLX8000_INFO("%s: generate msi interrupt,irq_flag_g=0x%x.\n", __func__, irq_flag_g);
    /*clear|disable FPGA interrupt flag*/
    writel(IRQ_CTRL_DISABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);
    /*writel(0x00000000, clounix_fpga_base + FPGA_DEBUG_TRIG_ADDR);*/
#if 1
    /*print interrupt information*/
    total_irq = sizeof(fpga_irq_arry) / sizeof(struct irq_vector);
    for(i=0; i<total_irq; i++){
        if( fpga_irq_arry[i].irq_mask_bit & irq_flag_g ){
            FPGA_CLX8000_INFO("%s: generate %s,irq_flag_g=0x%x.\n", __func__, fpga_irq_arry[i].irq_name,irq_flag_g);
            if(fpga_irq_arry[i].mark_type==1){
                int_flag0 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag0_addr);
                int_flag1 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag1_addr);
                int_flag2 = readl(clounix_fpga_base + fpga_irq_arry[i].flag_regs.int_flag2_addr);
                FPGA_CLX8000_INFO("cpld0_flag = 0x%x,cpld1_flag = 0x%x,qdd_flag = 0x%x.\n",int_flag0,int_flag1,int_flag2);
            }
            
        }
    }
    /*enable FPGA interrupt */
    writel(IRQ_CTRL_ENABLE_INT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);    
#endif
    spin_unlock(&fpga_msi_lock);

    return IRQ_HANDLED;
}



static int clounix_fpga_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
    int err;
    /*mask interrupt*/
   #if 1
    if (pci_find_capability(pdev, PCI_CAP_ID_MSI) == 0) {
        FPGA_CLX8000_ERR("%s[%d] MSI not support.\r\n", __func__, __LINE__);
        return -EPERM;
    }
       
    err = pci_enable_device(pdev);
    if (err) {
        FPGA_CLX8000_ERR("%s[%d] can't enbale device.\r\n", __func__, __LINE__);
        return -EPERM;
    }
    #endif
    if (devm_request_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0), "clounix_fpga") == 0) {
        FPGA_CLX8000_ERR("%s[%d] can't request iomem (0x%llx).\r\n", __func__, __LINE__, pci_resource_start(pdev, 0));
        err = -EBUSY;
        goto err_request;
    }
    
    pci_set_master(pdev);
    
    clounix_fpga_base = devm_ioremap(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    if (clounix_fpga_base  == NULL) {
        FPGA_CLX8000_ERR("%s[%d] ioremap resource fail.\r\n", __func__, __LINE__);
        err = -EIO;
        goto err_ioremap;
    } 
    pci_set_drvdata(pdev, clounix_fpga_base);
#if 1
    FPGA_CLX8000_ERR("support %d msi vector\n", pci_msi_vec_count(pdev));
    err = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_MSI | PCI_IRQ_AFFINITY);
    if (err < 0) {
        FPGA_CLX8000_ERR("%s[%d] MSI vector alloc fail.\r\n", __func__, __LINE__);
        goto err_alloc_msi;
    }

    err = request_irq(pci_irq_vector(pdev, 0), clounix_fpga_irq_hd, IRQF_SHARED, pdev->driver->name, pdev);
    if (err < 0) {
        FPGA_CLX8000_ERR("%s[%d] MSI vector alloc fail.\r\n", __func__, __LINE__);
        goto err_irq;
    }
    /*reset pcie interrupt core, and enable interrupt*/
    writel(IRQ_CTRL_CFG_RST_BIT, clounix_fpga_base + FPGA_IRQ_CTRL_ADDR);
#endif
    reboot_nb.notifier_call = sys_led_reboot_work;
    restart_nb.notifier_call = sys_led_reboot_work;

    register_reboot_notifier(&reboot_nb);
    register_restart_handler(&restart_nb);

    return 0;

err_irq:
    pci_free_irq_vectors(pdev);
err_alloc_msi:
    devm_iounmap(&pdev->dev, clounix_fpga_base);
err_ioremap:
    pci_clear_master(pdev);
    devm_release_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
err_request:
    pci_disable_device(pdev);

    return err;
}

static void clounix_fpga_remove(struct pci_dev *pdev)
{
    unregister_reboot_notifier(&reboot_nb);
    unregister_restart_handler(&restart_nb);

    free_irq(pci_irq_vector(pdev, 0), pdev);
    pci_free_irq_vectors(pdev);
    devm_iounmap(&pdev->dev, clounix_fpga_base);
    pci_clear_master(pdev);
    devm_release_mem_region(&pdev->dev, pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));
    pci_disable_device(pdev);

    return;
}

static const struct pci_device_id clounix_fpga_pci_tbl[] = {
    { PCI_DEVICE(FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID) },
    { 0, },
};
MODULE_DEVICE_TABLE(pci, clounix_fpga_pci_tbl);

static struct pci_driver clounix_priv_data_pci_driver = {
    .name       = "clounix_fpga",
    .id_table   = clounix_fpga_pci_tbl,
    .probe      = clounix_fpga_probe,
    .remove     = clounix_fpga_remove,
    .suspend    = NULL,
    .resume     = NULL,
};

module_pci_driver(clounix_priv_data_pci_driver);

MODULE_AUTHOR("Bao Hengxi <baohx@clounix.com>");
MODULE_DESCRIPTION("clounix fpga driver");
MODULE_LICENSE("GPL v2");
