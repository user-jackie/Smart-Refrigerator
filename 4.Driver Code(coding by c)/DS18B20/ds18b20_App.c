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

// 主函数：同时读取两个传感器温度
int main(int argc, char *argv[])
{
    char temp_buf0[TEMP_BUF_SIZE];
    char temp_buf1[TEMP_BUF_SIZE];
    int ret0, ret1;

    printf("DS18B20 Double Sensor Test (RK3568)\n");
    printf("Sensor0(GPIO4C4)  Sensor1(GPIO3B5)\n");
    printf("----------------------------------\n");

    while (1) {
        // 读取传感器0温度
        ret0 = ds18b20_read_temperature(DS18B20_DEV_PATH_0, temp_buf0, TEMP_BUF_SIZE);
        // 读取传感器1温度
        ret1 = ds18b20_read_temperature(DS18B20_DEV_PATH_1, temp_buf1, TEMP_BUF_SIZE);

        // 输出温度
        if (ret0 == 0 || ret1 == 0) {
            printf("\rSensor0(GPIO4C4): %-10s  Sensor1(GPIO3B5): %-10s", temp_buf0, temp_buf1);
        } else {
            printf("\rSensor0(GPIO4C4): 读取失败  Sensor1(GPIO3B5): 未连接/读取失败");
        }
        fflush(stdout);

        sleep(1);
    }

    return 0;
}