#include "ds18b20_App.h"

// 读取指定传感器温度
int ds18b20_read_temperature(const char *dev_path, char *temp_buf, int buf_size)
{
    int fd;
    int ret;
    char raw_buf[TEMP_BUF_SIZE];
    int temp_raw;
    float temp_real;

    // 打开指定设备节点
    fd = open(dev_path, O_RDONLY);
    if (fd < 0) {
        perror("open ds18b20 device failed");
        return -1;
    }

    // 清空缓冲区
    memset(raw_buf, 0, sizeof(raw_buf));
    memset(temp_buf, 0, buf_size);

    // 读取原始温度数据
    ret = read(fd, raw_buf, sizeof(raw_buf) - 1);
    if (ret < 0) {
        perror("read ds18b20 raw data failed");
        close(fd);
        return -1;
    }
    close(fd);

    // 字符串转整数，换算实际温度
    temp_raw = atoi(raw_buf);
    temp_real = temp_raw * 0.0625f; // DS18B20官方换算公式
    snprintf(temp_buf, buf_size, "%.2f℃", temp_real);

    return 0;
}