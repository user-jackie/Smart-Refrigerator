#ifndef __DS18B20_DOUBLE_APP_H
#define __DS18B20_DOUBLE_APP_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

// 两个DS18B20设备节点
#define DS18B20_DEV_PATH_0 "/dev/ds18b20_0"  // 对应GPIO3B6
#define DS18B20_DEV_PATH_1 "/dev/ds18b20_1"  // 对应GPIO3B5
// 温度缓冲区大小
#define TEMP_BUF_SIZE 32

/**
 * @brief  读取指定DS18B20传感器温度
 * @param  dev_path: 设备节点路径（DS18B20_DEV_PATH_0/1）
 * @param  temp_buf: 存储温度的缓冲区（输出）
 * @param  buf_size: 缓冲区大小
 * @retval 0:成功，-1:失败
 */
int ds18b20_read_temperature(const char *dev_path, char *temp_buf, int buf_size);

#endif /* __DS18B20_DOUBLE_APP_H */