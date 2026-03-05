#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>

#define DS18B20_NAME "ds18b20"
#define DS18B20_CNT  2          // 设备数量改为2

// 设备结构体（支持多设备，增加index标识）
struct ds18b20_dev {
    dev_t devid;              // 字符设备号
    struct cdev cdev;         // 字符设备
    struct class *class;      // 设备类（全局共享）
    struct device *device;    // 设备节点（每个设备独立）
    struct device *dev;       // 平台设备
    int major;                // 主设备号
    int minor;                // 次设备号
    int ds18b20_gpio;         // 对应GPIO编号
    struct device_node *nd;   // 设备节点
    int index;                // 设备索引（0/1）
};
static struct ds18b20_dev ds18b20_dev[DS18B20_CNT]; // 设备数组
static struct class *ds18b20_class;                 // 全局设备类

/********************* DS18B20单总线核心时序（适配多设备）*********************/
// 设置GPIO为输出模式（绑定具体设备）
static void ds18b20_gpio_output(struct ds18b20_dev *dev)
{
    gpio_direction_output(dev->ds18b20_gpio, 1);
}

// 设置GPIO为输入模式（绑定具体设备）
static void ds18b20_gpio_input(struct ds18b20_dev *dev)
{
    gpio_direction_input(dev->ds18b20_gpio);
}

// 设置数据口电平（绑定具体设备）
static void ds18b20_set_gpio(struct ds18b20_dev *dev, int val)
{
    gpio_set_value(dev->ds18b20_gpio, val);
}

// 读取数据口电平（绑定具体设备）
static int ds18b20_get_gpio(struct ds18b20_dev *dev)
{
    return gpio_get_value(dev->ds18b20_gpio);
}

// DS18B20复位（适配多设备）
static int ds18b20_reset(struct ds18b20_dev *dev)
{
    int ret;
    // 1. 主机拉低总线480~960us
    ds18b20_gpio_output(dev);
    ds18b20_set_gpio(dev, 0);
    udelay(600);
    // 2. 主机释放总线，等待15~60us
    ds18b20_set_gpio(dev, 1);
    ds18b20_gpio_input(dev);
    udelay(80);
    // 3. 检测从机应答（拉低总线60~240us）
    ret = ds18b20_get_gpio(dev);
    // 4. 等待总线释放
    udelay(500);
    return ret; // 0:复位成功，1:失败
}

// 单总线读1bit（适配多设备）
static u8 ds18b20_read_bit(struct ds18b20_dev *dev)
{
    u8 bit = 0;
    // 主机拉低总线至少1us
    ds18b20_gpio_output(dev);
    ds18b20_set_gpio(dev, 0);
    udelay(2);
    // 主机释放总线
    ds18b20_set_gpio(dev, 1);
    ds18b20_gpio_input(dev);
    udelay(10);
    // 读取总线电平
    if (ds18b20_get_gpio(dev))
        bit = 1;
    else
        bit = 0;
    // 等待时序完成（至少45us）
    udelay(50);
    return bit;
}

// 单总线读1字节（适配多设备）
static u8 ds18b20_read_byte(struct ds18b20_dev *dev)
{
    u8 i, byte = 0;
    for (i = 0; i < 8; i++) {
        byte >>= 1;
        if (ds18b20_read_bit(dev))
            byte |= 0x80;
    }
    return byte;
}

// 单总线写1字节（适配多设备）
static void ds18b20_write_byte(struct ds18b20_dev *dev, u8 byte)
{
    u8 i, bit;
    ds18b20_gpio_output(dev);
    for (i = 0; i < 8; i++) {
        bit = byte & 0x01;
        byte >>= 1;
        // 写0：拉低总线60~120us
        if (bit == 0) {
            ds18b20_set_gpio(dev, 0);
            udelay(80);
            ds18b20_set_gpio(dev, 1);
            udelay(2);
        } else { // 写1：拉低总线1~15us，然后释放
            ds18b20_set_gpio(dev, 0);
            udelay(2);
            ds18b20_set_gpio(dev, 1);
            udelay(80);
        }
    }
}

/********************* DS18B20温度读取（适配多设备）*********************/
static int ds18b20_read_temp(struct ds18b20_dev *dev, int *temp_raw)
{
    u8 temp_h, temp_l;
    u16 temp_val;
    // 1. 复位DS18B20，失败直接返回
    if (ds18b20_reset(dev) != 0) {
        printk("DS18B20_%d reset failed!\n", dev->index);
        return -EIO;
    }
    // 2. 跳过ROM（单设备挂载专用）
    ds18b20_write_byte(dev, 0xCC);
    // 3. 发送温度转换命令，等待转换完成
    ds18b20_write_byte(dev, 0x44);
    mdelay(750);
    // 4. 再次复位，准备读取原始数据
    if (ds18b20_reset(dev) != 0) {
        printk("DS18B20_%d reset failed!\n", dev->index);
        return -EIO;
    }
    // 5. 跳过ROM，发送读数据命令
    ds18b20_write_byte(dev, 0xCC);
    ds18b20_write_byte(dev, 0xBE);
    // 6. 读取原始温度数据
    temp_l = ds18b20_read_byte(dev);
    temp_h = ds18b20_read_byte(dev);
    temp_val = (temp_h << 8) | temp_l;
    // 7. 传递原始16位整数
    *temp_raw = (int)temp_val;
    return 0;
}

/********************* 字符设备操作接口（适配多设备）*********************/
static int ds18b20_open(struct inode *inode, struct file *filp)
{
    int minor = iminor(inode);          // 获取次设备号
    filp->private_data = &ds18b20_dev[minor]; // 绑定设备结构体
    return 0;
}

static ssize_t ds18b20_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
    struct ds18b20_dev *dev = filp->private_data; // 获取当前设备
    int temp_raw;
    char temp_buf[32];
    int ret;

    // 读取对应传感器的原始温度
    ret = ds18b20_read_temp(dev, &temp_raw);
    if (ret < 0)
        return ret;

    // 格式化原始整数字符串
    snprintf(temp_buf, sizeof(temp_buf), "%d\n", temp_raw);
    // 拷贝到用户空间
    ret = copy_to_user(buf, temp_buf, strlen(temp_buf));
    if (ret < 0) {
        printk("copy to user failed for ds18b20_%d!\n", dev->index);
        return -EFAULT;
    }
    // 防止重复读取
    if (*ppos > 0)
        return 0;
    *ppos += strlen(temp_buf);
    return strlen(temp_buf);
}

static int ds18b20_release(struct inode *inode, struct file *filp)
{
    return 0;
}

// 字符设备操作集
static const struct file_operations ds18b20_fops = {
    .owner = THIS_MODULE,
    .open = ds18b20_open,
    .read = ds18b20_read,
    .release = ds18b20_release,
};

/* 初始化单个DS18B20的GPIO */
static int ds18b20_gpio_init(struct ds18b20_dev *dev, struct device_node *nd)
{
    int ret;

    /* 从设备树读取GPIO属性 */
    dev->ds18b20_gpio = of_get_named_gpio(nd, "ds18b20-gpios", 0);
    if (dev->ds18b20_gpio < 0) {
        pr_err("ds18b20_%d: can't get gpio from DT\n", dev->index);
        return -EINVAL;
    }
    pr_info("ds18b20_%d: gpio num = %d\n", dev->index, dev->ds18b20_gpio);

    /* 请求并配置GPIO */
    ret = gpio_request(dev->ds18b20_gpio, "ds18b20");
    if (ret) {
        pr_err("ds18b20_%d: gpio_request failed, ret=%d\n", dev->index, ret);
        return ret;
    }
    ret = gpio_direction_output(dev->ds18b20_gpio, 1);
    if (ret) {
        pr_err("ds18b20_%d: gpio_direction_output failed\n", dev->index);
        gpio_free(dev->ds18b20_gpio);
        return -EINVAL;
    }

    return 0;
}

static int ds18b20_probe(struct platform_device *pdev)
{
    int ret, index;
    const __be32 *reg;
    struct ds18b20_dev *dev;

    // 解析设备索引（原有逻辑不变）
    reg = of_get_property(pdev->dev.of_node, "reg", NULL);
    if (!reg) {
        pr_err("ds18b20: no reg property in DT node\n");
        return -EINVAL;
    }
    index = be32_to_cpup(reg);
    if (index < 0 || index >= DS18B20_CNT) {
        pr_err("ds18b20: invalid reg value %d\n", index);
        return -EINVAL;
    }

    // ========== 新增：初始化设备结构体默认值 ==========
    dev = &ds18b20_dev[index];
    memset(dev, 0, sizeof(struct ds18b20_dev)); // 所有成员置0
    dev->index = index;
    dev->nd = pdev->dev.of_node;
    dev->ds18b20_gpio = -1; // 初始化GPIO为无效值
    dev->devid = 0;         // 初始化设备号为0

    // 2. 初始化GPIO
    ret = ds18b20_gpio_init(dev, pdev->dev.of_node);
    if (ret < 0)
        return ret;

    // 3. 动态分配字符设备号（主设备号共享，次设备号为index）
    if (index == 0) { // 第一个设备分配主设备号
        ret = alloc_chrdev_region(&dev->devid, 0, DS18B20_CNT, DS18B20_NAME);
        if (ret < 0) {
            printk("alloc chrdev region failed!\n");
            goto free_gpio;
        }
        dev->major = MAJOR(dev->devid);
        // 给第二个设备设置设备号
        ds18b20_dev[1].devid = MKDEV(dev->major, 1);
    } else { // 第二个设备复用主设备号
        dev->devid = MKDEV(ds18b20_dev[0].major, index);
    }

    // 4. 初始化并添加字符设备
    cdev_init(&dev->cdev, &ds18b20_fops);
    dev->cdev.owner = THIS_MODULE;
    ret = cdev_add(&dev->cdev, dev->devid, 1);
    if (ret < 0) {
        printk("cdev add failed for ds18b20_%d!\n", index);
        goto unregister_chrdev;
    }

    // 5. 创建设备类（仅第一个设备创建）
    if (index == 0) {
        ds18b20_class = class_create(THIS_MODULE, "ds18b20_class");
        if (IS_ERR(ds18b20_class)) {
            printk("class create failed!\n");
            goto del_cdev;
        }
    }

    // 6. 创建设备节点（ds18b20_0/ds18b20_1）
    dev->device = device_create(ds18b20_class, NULL, dev->devid, NULL, "ds18b20_%d", index);
    if (IS_ERR(dev->device)) {
        printk("device create failed for ds18b20_%d!\n", index);
        goto destroy_class;
    }

    printk("ds18b20_%d driver probe success!\n", index);
    return 0;

destroy_class:
    if (index == 0)
        class_destroy(ds18b20_class);
del_cdev:
    cdev_del(&dev->cdev);
unregister_chrdev:
    if (index == 0)
        unregister_chrdev_region(dev->devid, DS18B20_CNT);
free_gpio:
    gpio_free(dev->ds18b20_gpio);
    return -EIO;
}

static int ds18b20_remove(struct platform_device *pdev)
{
    int index = -1;
    const __be32 *reg;
    const char *node_name;
    struct ds18b20_dev *dev = NULL;
    int all_removed = 1;
    int i;

    // 步骤1：容错解析设备索引（优先reg，失败则解析节点名）
    reg = of_get_property(pdev->dev.of_node, "reg", NULL);
    if (reg) {
        index = be32_to_cpup(reg);
    } else {
        node_name = pdev->dev.of_node->name;
        if (sscanf(node_name, "ds18b20@%d", &index) != 1) {
            pr_err("ds18b20: failed to parse index in remove\n");
            return -EINVAL;
        }
    }

    // 步骤2：检查索引有效性，避免数组越界
    if (index < 0 || index >= DS18B20_CNT) {
        pr_err("ds18b20: invalid index %d in remove\n", index);
        return -EINVAL;
    }
    dev = &ds18b20_dev[index];

    // 步骤3：安全释放资源（增加空指针/有效性检查）
    // 3.1 销毁设备节点（避免访问无效device）
    if (!IS_ERR_OR_NULL(dev->device)) {
        device_destroy(ds18b20_class, dev->devid);
        dev->device = NULL; // 置空标记，防止重复释放
    }

    // 3.2 删除字符设备（仅devid有效时执行）
    if (dev->devid != 0) {
        cdev_del(&dev->cdev);
        dev->devid = 0; // 置空标记
    }

    // 3.3 释放GPIO（仅GPIO号有效时执行）
    if (dev->ds18b20_gpio >= 0) {
        gpio_free(dev->ds18b20_gpio);
        dev->ds18b20_gpio = -1; // 置为无效值
    }

    // 步骤4：全局资源（class/设备号）- 所有设备卸载后才释放
    for (i = 0; i < DS18B20_CNT; i++) {
        if (ds18b20_dev[i].devid != 0) {
            all_removed = 0;
            break;
        }
    }

    if (all_removed) {
        // 安全销毁设备类
        if (!IS_ERR_OR_NULL(ds18b20_class)) {
            class_destroy(ds18b20_class);
            ds18b20_class = NULL;
        }
        // 释放设备号（使用第一个设备的major，避免空指针）
        unregister_chrdev_region(MKDEV(ds18b20_dev[0].major, 0), DS18B20_CNT);
    }

    printk("ds18b20_%d driver removed safely!\n", index);
    return 0;
}

static const struct of_device_id rk3568_ds18b20_of_match[] = {
    { .compatible = "rockchip,rk3568-ds18b20" },
    { }
};
MODULE_DEVICE_TABLE(of, rk3568_ds18b20_of_match);

static struct platform_driver rk3568_ds18b20_driver = {
    .driver = {
        .name = "rk3568-ds18b20",
        .of_match_table = rk3568_ds18b20_of_match,
    },
    .probe = ds18b20_probe,
    .remove = ds18b20_remove,
};

static int __init ds18b20_driver_init(void)
{
    return platform_driver_register(&rk3568_ds18b20_driver);
}

static void __exit ds18b20_driver_exit(void)
{
    platform_driver_unregister(&rk3568_ds18b20_driver);
}

module_init(ds18b20_driver_init);
module_exit(ds18b20_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liaoyuan");