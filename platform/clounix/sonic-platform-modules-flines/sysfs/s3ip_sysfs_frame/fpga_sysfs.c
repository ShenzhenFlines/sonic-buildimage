/*
 * fpga_sysfs.c
 *
 * This module create fpga kobjects and attributes in /sys/s3ip/fpga
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-08-31                  S3IP sysfs
 */

#include <linux/slab.h>

#include "switch.h"
#include "fpga_sysfs.h"
#include "reboot_eeprom_sysfs.h"

static int g_fpga_loglevel = 0;

#define FPGA_INFO(fmt, args...) do {                                        \
    if (g_fpga_loglevel & INFO) { \
        printk(KERN_INFO "[FPGA_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_ERR(fmt, args...) do {                                        \
    if (g_fpga_loglevel & ERR) { \
        printk(KERN_ERR "[FPGA_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FPGA_DBG(fmt, args...) do {                                        \
    if (g_fpga_loglevel & DBG) { \
        printk(KERN_DEBUG "[FPGA_SYSFS][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct fpga_obj_s {
    struct switch_obj *obj;
};

struct fpga_s {
    unsigned int fpga_number;
    struct fpga_obj_s *fpga;
};

struct reboot_eeprom_s {
    struct bin_attribute bin;
    int creat_eeprom_bin_flag;
};

static struct fpga_s g_fpga;
static struct switch_obj *g_fpga_obj = NULL;
static struct s3ip_sysfs_fpga_drivers_s *g_fpga_drv = NULL;

static struct reboot_eeprom_s g_reboot_eeprom;
static struct switch_obj *g_reboot_eeprom_obj = NULL;
static struct s3ip_sysfs_reboot_eeprom_drivers_s *g_reboot_eeprom_drv = NULL;

static ssize_t fpga_number_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%u\n", g_fpga.fpga_number);
}

static ssize_t fpga_debug_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_debug);

    ret = g_fpga_drv->get_debug(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga debug failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_debug_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->set_debug);

    ret = g_fpga_drv->set_debug(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("set fpga debug failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_loglevel_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_loglevel);

    ret = g_fpga_drv->get_loglevel(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga loglevel failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_loglevel_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->set_loglevel);

    ret = g_fpga_drv->set_loglevel(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("set fpga loglevel failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_alias_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_alias);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    ret = g_fpga_drv->get_main_board_fpga_alias(fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga%u alias failed, ret: %d\n", fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_type_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_type);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    ret = g_fpga_drv->get_main_board_fpga_type(fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga%u type failed, ret: %d\n", fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_fw_version_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_firmware_version);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    ret = g_fpga_drv->get_main_board_fpga_firmware_version(fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga%u firmware version failed, ret: %d\n", fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_board_version_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_board_version);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    ret = g_fpga_drv->get_main_board_fpga_board_version(fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga%u board version failed, ret: %d\n", fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_test_reg_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    unsigned int fpga_index;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->get_main_board_fpga_test_reg);

    fpga_index = obj->index;
    FPGA_DBG("fpga index: %u\n", fpga_index);
    ret = g_fpga_drv->get_main_board_fpga_test_reg(fpga_index, buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get fpga%u test register failed, ret: %d\n", fpga_index, ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t fpga_test_reg_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    unsigned int fpga_index, value;
    int ret;

    check_p(g_fpga_drv);
    check_p(g_fpga_drv->set_main_board_fpga_test_reg);

    fpga_index = obj->index;
    sscanf(buf, "0x%x", &value);
    ret = g_fpga_drv->set_main_board_fpga_test_reg(fpga_index, value);
    if (ret < 0) {
        FPGA_ERR("set fpga%u test reg failed, value:0x%x, ret: %d.\n", fpga_index, value, ret);
        return -EIO;
    }
    FPGA_DBG("set fpga%u test reg success, value: 0x%x.\n", fpga_index, value);
    return count;
}

/************************************fpga dir and attrs*******************************************/
static struct switch_attribute fpga_number_attr = __ATTR(num, S_IRUGO, fpga_number_show, NULL);
static struct switch_attribute fpga_debug_attr = __ATTR(debug, S_IRUGO | S_IWUSR, fpga_debug_show, fpga_debug_store);
static struct switch_attribute fpga_loglevel_attr = __ATTR(loglevel, S_IRUGO | S_IWUSR, fpga_loglevel_show, fpga_loglevel_store);

static struct attribute *fpga_dir_attrs[] = {
    &fpga_number_attr.attr,
    &fpga_debug_attr.attr,
    &fpga_loglevel_attr.attr,
    NULL,
};

static struct attribute_group fpga_root_attr_group = {
    .attrs = fpga_dir_attrs,
};

/*******************************fpga[1-n] dir and attrs*******************************************/
static struct switch_attribute fpga_alias_attr = __ATTR(alias, S_IRUGO, fpga_alias_show, NULL);
static struct switch_attribute fpga_type_attr = __ATTR(type, S_IRUGO, fpga_type_show, NULL);
static struct switch_attribute fpga_fw_version_attr = __ATTR(hw_version, S_IRUGO, fpga_fw_version_show, NULL);
static struct switch_attribute fpga_board_version_attr = __ATTR(board_version, S_IRUGO, fpga_board_version_show, NULL);
static struct switch_attribute fpga_test_reg_attr = __ATTR(reg_test, S_IRUGO | S_IWUSR, fpga_test_reg_show, fpga_test_reg_store);

static struct attribute *fpga_attrs[] = {
    &fpga_alias_attr.attr,
    &fpga_type_attr.attr,
    &fpga_fw_version_attr.attr,
    &fpga_board_version_attr.attr,
    &fpga_test_reg_attr.attr,
    NULL,
};

static struct attribute_group fpga_attr_group = {
    .attrs = fpga_attrs,
};

static int fpga_sub_single_remove_kobj_and_attrs(unsigned int index)
{
    struct fpga_obj_s *curr_fpga;

    curr_fpga = &g_fpga.fpga[index - 1];
    if (curr_fpga->obj) {
        sysfs_remove_group(&curr_fpga->obj->kobj, &fpga_attr_group);
        switch_kobject_delete(&curr_fpga->obj);
        FPGA_DBG("delete fpga%u dir and attrs success.\n", index);
    }
    return 0;
}

static int fpga_sub_single_create_kobj_and_attrs(struct kobject *parent, unsigned int index)
{
    char name[8];
    struct fpga_obj_s *curr_fpga;

    curr_fpga = &g_fpga.fpga[index - 1];
    memset(name, 0, sizeof(name));
    snprintf(name, sizeof(name), "fpga%u", index);
    curr_fpga->obj = switch_kobject_create(name, parent);
    if (!curr_fpga->obj) {
        FPGA_ERR("create %s object error!\n", name);
        return -EBADRQC;
    }
    curr_fpga->obj->index = index;
    if (sysfs_create_group(&curr_fpga->obj->kobj, &fpga_attr_group) != 0) {
        FPGA_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&curr_fpga->obj);
        return -EBADRQC;
    }
    FPGA_DBG("create %s dir and attrs success.\n", name);
    return 0;
}

static int fpga_sub_create_kobj_and_attrs(struct kobject *parent, int fpga_num)
{
    unsigned int fpga_index, i;

    g_fpga.fpga = kzalloc(sizeof(struct fpga_obj_s) * fpga_num, GFP_KERNEL);
    if (!g_fpga.fpga) {
        FPGA_ERR("kzalloc g_fpga.fpga error, fpga number = %d.\n", fpga_num);
        return -ENOMEM;
    }

    for(fpga_index = 1; fpga_index <= fpga_num; fpga_index++) {
        if(fpga_sub_single_create_kobj_and_attrs(parent, fpga_index) != 0 ) {
            goto error;
        }
    }
    return 0;
error:
    for(i = fpga_index; i > 0; i--) {
        fpga_sub_single_remove_kobj_and_attrs(i);
    }
    kfree(g_fpga.fpga);
    g_fpga.fpga = NULL;
    return -EBADRQC;
}

/* create fpga[1-n] directory and attributes*/
static int fpga_sub_create(void)
{
    int ret;

    ret = fpga_sub_create_kobj_and_attrs(&g_fpga_obj->kobj, g_fpga.fpga_number);
    return ret;
}

/* delete fpga[1-n] directory and attributes*/
static void fpga_sub_remove(void)
{
    unsigned int fpga_index;

    if (g_fpga.fpga) {
        for (fpga_index = g_fpga.fpga_number; fpga_index > 0; fpga_index--) {
            fpga_sub_single_remove_kobj_and_attrs(fpga_index);
        }
        kfree(g_fpga.fpga);
        g_fpga.fpga = NULL;
    }
    g_fpga.fpga_number = 0;
    return;
}

/* create fpga directory and number attributes */
static int fpga_root_create(void)
{
    g_fpga_obj = switch_kobject_create("fpga", NULL);
    if (!g_fpga_obj) {
        FPGA_ERR("switch_kobject_create fpga error!\n");
        return -ENOMEM;
    }

    if (sysfs_create_group(&g_fpga_obj->kobj, &fpga_root_attr_group) != 0) {
        switch_kobject_delete(&g_fpga_obj);
        FPGA_ERR("create fpga dir attrs error!\n");
        return -EBADRQC;
    }
    return 0;
}

/* delete fpga directory and number attributes */
static void fpga_root_remove(void)
{
    if (g_fpga_obj) {
        sysfs_remove_group(&g_fpga_obj->kobj, &fpga_root_attr_group);
        switch_kobject_delete(&g_fpga_obj);
    }

    return;
}

int s3ip_sysfs_fpga_drivers_register(struct s3ip_sysfs_fpga_drivers_s *drv)
{
    int ret, fpga_num;

    FPGA_INFO("s3ip_sysfs_fpga_drivers_register...\n");
    if (g_fpga_drv) {
        FPGA_ERR("g_fpga_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_main_board_fpga_number);
    g_fpga_drv = drv;

    fpga_num = g_fpga_drv->get_main_board_fpga_number();
    if (fpga_num <= 0) {
        FPGA_ERR("fpga number: %d, don't need to create fpga dirs and attrs.\n", fpga_num);
        g_fpga_drv = NULL;
        return -EINVAL;
    }

    memset(&g_fpga, 0, sizeof(struct fpga_s));
    g_fpga.fpga_number = fpga_num;
    ret = fpga_root_create();
    if (ret < 0) {
        FPGA_ERR("create fpga root dir and attrs failed, ret: %d\n", ret);
        g_fpga_drv = NULL;
        return ret;
    }
    ret = fpga_sub_create();
    if (ret < 0) {
        FPGA_ERR("create fpga sub dir and attrs failed, ret: %d\n", ret);
        fpga_root_remove();
        g_fpga_drv = NULL;
        return ret;
    }
    FPGA_INFO("s3ip_sysfs_fpga_drivers_register success\n");
    return 0;
}

void s3ip_sysfs_fpga_drivers_unregister(void)
{
    if (g_fpga_drv) {
        fpga_sub_remove();
        fpga_root_remove();
        g_fpga_drv = NULL;
        FPGA_DBG("s3ip_sysfs_fpga_drivers_unregister success.\n");
    }

    return;
}


static ssize_t reboot_eeprom_read(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    ssize_t rd_len;

    check_p(g_reboot_eeprom_drv);
    check_p(g_reboot_eeprom_drv->read_reboot_eeprom_data);

    memset(buf, 0, count);
    rd_len = g_reboot_eeprom_drv->read_reboot_eeprom_data(buf, offset, count);
    if (rd_len < 0) {
        FPGA_ERR("read reboot eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            offset, count, rd_len);
        return -EIO;
    }
    FPGA_DBG("read reboot eeprom data success, offset:0x%llx, read len:%lu, really read len:%ld.\n",
        offset, count, rd_len);
    return rd_len;
}

static ssize_t reboot_eeprom_write(struct file *filp, struct kobject *kobj, struct bin_attribute *attr,
                   char *buf, loff_t offset, size_t count)
{
    ssize_t wr_len;

    check_p(g_reboot_eeprom_drv);
    check_p(g_reboot_eeprom_drv->write_reboot_eeprom_data);

    wr_len = g_reboot_eeprom_drv->write_reboot_eeprom_data(buf, offset, count);
    if (wr_len < 0) {
        FPGA_ERR("write reboot eeprom data error, offset: 0x%llx, read len: %lu, ret: %ld.\n",
            offset, count, wr_len);
        return -EIO;
    }
    FPGA_DBG("write reboot eeprom data success, offset:0x%llx, write len:%lu, really write len:%ld.\n",
        offset, count, wr_len);
    return wr_len;
}

static int reboot_eeprom_create_eeprom_attrs(void)
{
    int ret, eeprom_size;

    eeprom_size = g_reboot_eeprom_drv->get_reboot_eeprom_size();
    if (eeprom_size <= 0) {
        FPGA_ERR("reboot eeprom size: %d, invalid.\n", eeprom_size);
        return -EINVAL;
    }

    sysfs_bin_attr_init(&g_reboot_eeprom.bin);
    g_reboot_eeprom.bin.attr.name = "reboot_cause";
    g_reboot_eeprom.bin.attr.mode = 0644;
    g_reboot_eeprom.bin.read = reboot_eeprom_read;
    g_reboot_eeprom.bin.write = reboot_eeprom_write;
    g_reboot_eeprom.bin.size = eeprom_size;

    ret = sysfs_create_bin_file(&g_reboot_eeprom_obj->kobj, &g_reboot_eeprom.bin);
    if (ret) {
        FPGA_ERR("create reboot eeprom bin error, ret: %d. \n", ret);
        return -EBADRQC;
    }
    FPGA_DBG("create reboot eeprom bin file success, eeprom size:%d.\n", eeprom_size);
    g_reboot_eeprom.creat_eeprom_bin_flag = 1;
    return 0;
}

static void reboot_eeprom_remove_eeprom_attrs(void)
{
    if (g_reboot_eeprom.creat_eeprom_bin_flag) {
        sysfs_remove_bin_file(&g_reboot_eeprom_obj->kobj, &g_reboot_eeprom.bin);
        g_reboot_eeprom.creat_eeprom_bin_flag = 0;
    }

    return;
}

static ssize_t reboot_eeprom_debug_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int ret;

    check_p(g_reboot_eeprom_drv);
    check_p(g_reboot_eeprom_drv->get_debug);

    ret = g_reboot_eeprom_drv->get_debug(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get reboot_eeprom debug failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t reboot_eeprom_debug_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;

    check_p(g_reboot_eeprom_drv);
    check_p(g_reboot_eeprom_drv->set_debug);

    ret = g_reboot_eeprom_drv->set_debug(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("set reboot_eeprom debug failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t reboot_eeprom_loglevel_show(struct switch_obj *obj, struct switch_attribute *attr, char *buf)
{
    int ret;

    check_p(g_reboot_eeprom_drv);
    check_p(g_reboot_eeprom_drv->get_loglevel);

    ret = g_reboot_eeprom_drv->get_loglevel(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("get reboot eeprom loglevel failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}

static ssize_t reboot_eeprom_loglevel_store(struct switch_obj *obj, struct switch_attribute *attr,
                   const char* buf, size_t count)
{
    int ret;

    check_p(g_reboot_eeprom_drv);
    check_p(g_reboot_eeprom_drv->set_loglevel);

    ret = g_reboot_eeprom_drv->set_loglevel(buf, PAGE_SIZE);
    if (ret < 0) {
        FPGA_ERR("set reboot eeprom loglevel failed, ret: %d\n", ret);
        return (ssize_t)snprintf(buf, PAGE_SIZE, "%s\n", SYSFS_DEV_ERROR);
    }
    return ret;
}


/************************************reboot eeprom dir and attrs*******************************************/
static struct switch_attribute reboot_eeprom_debug_att = __ATTR(debug, S_IRUGO | S_IWUSR, reboot_eeprom_debug_show, reboot_eeprom_debug_store);
static struct switch_attribute reboot_eeprom_loglevel_att = __ATTR(loglevel, S_IRUGO | S_IWUSR, reboot_eeprom_loglevel_show, reboot_eeprom_loglevel_store);

static struct attribute *reboot_eeprom_dir_attrs[] = {
    //&reboot_eeprom_debug_att.attr,
    //&reboot_eeprom_loglevel_att.attr,
    NULL,
};

static struct attribute_group reboot_eeprom_attr_group = {
    .attrs = reboot_eeprom_dir_attrs,
};

static int fpga_sub_eeprom_create_obj(struct kobject *parent, const char *name)
{
    g_reboot_eeprom_obj = switch_kobject_create(name, parent);
    
    if (!g_reboot_eeprom_obj) {
        FPGA_ERR("create %s object error!\n", name);
        return -ENOMEM;
    }
  
    if (sysfs_create_group(&g_reboot_eeprom_obj->kobj, &reboot_eeprom_attr_group) != 0) {
        FPGA_ERR("create %s attrs error.\n", name);
        switch_kobject_delete(&g_reboot_eeprom_obj);
        return -ENOMEM;
    }

    FPGA_DBG("create %s dir and attrs success.\n", name);

    return 0;

}

static void fpga_sub_eeprom_remove_obj(void)
{
    if (g_reboot_eeprom_obj) {

        sysfs_remove_group(&g_reboot_eeprom_obj->kobj, &reboot_eeprom_attr_group);

        switch_kobject_delete(&g_reboot_eeprom_obj);
    }

    return;
}

int s3ip_sysfs_reboot_eeprom_drivers_register(struct s3ip_sysfs_reboot_eeprom_drivers_s *drv)
{
    int ret;

    FPGA_INFO("s3ip_sysfs_reboot_eeprom_drivers_register...\n");
    if (g_reboot_eeprom_drv) {
        FPGA_ERR("g_reboot_eeprom_drv is not NULL, can't register\n");
        return -EPERM;
    }

    check_p(drv);
    check_p(drv->get_reboot_eeprom_size);
    g_reboot_eeprom_drv = drv;
    ret = fpga_sub_eeprom_create_obj(NULL, "reboot_eeprom");
    if (ret < 0) {
        FPGA_ERR("create reboot_eeprom root dir and attrs failed, ret: %d\n", ret);
        g_reboot_eeprom_drv = NULL;
        return ret;
    }
    ret = reboot_eeprom_create_eeprom_attrs();
    if (ret < 0) {
        FPGA_ERR("create reboot_eeprom attributes failed, ret: %d\n", ret);
        fpga_sub_eeprom_remove_obj();
        g_reboot_eeprom_drv = NULL;
        return ret;
    }

    FPGA_INFO("s3ip_sysfs_reboot_eeprom_drivers_register success.\n");
    return 0;
}

void s3ip_sysfs_reboot_eeprom_drivers_unregister(void)
{
    if (g_reboot_eeprom_drv) {
        reboot_eeprom_remove_eeprom_attrs();
        fpga_sub_eeprom_remove_obj();
        g_reboot_eeprom_drv = NULL;
        FPGA_DBG("s3ip_sysfs_reboot_eeprom_drivers_unregister success.\n");
    }

    return;
}

EXPORT_SYMBOL(s3ip_sysfs_fpga_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_fpga_drivers_unregister);
EXPORT_SYMBOL(s3ip_sysfs_reboot_eeprom_drivers_register);
EXPORT_SYMBOL(s3ip_sysfs_reboot_eeprom_drivers_unregister);
module_param(g_fpga_loglevel, int, 0644);
MODULE_PARM_DESC(g_fpga_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4).\n");

