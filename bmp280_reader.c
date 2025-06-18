#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define BMP280_ADDR 0x76
#define BMP280_ID_REG 0xD0
#define BMP280_TEMP_MSB_REG 0xFA
#define BMP280_EXPECTED_ID 0x58
#define I2C_BUS_NUM 15

static volatile int running = 1;

void signal_handler(int signum) {
    running = 0;
}

static int i2c_read_reg(int fd, uint8_t reg, uint8_t *value)
{
    if (write(fd, &reg, 1) != 1) {
        fprintf(stderr, "Ошибка записи регистра (0x%02X): %s\n", 
                reg, strerror(errno));
        return -1;
    }
    
    if (read(fd, value, 1) != 1) {
        fprintf(stderr, "Ошибка чтения регистра (0x%02X): %s\n", 
                reg, strerror(errno));
        return -1;
    }
    
    return 0;
}

static int read_temperature(int fd, float *temp)
{
    uint8_t data[3];
    int32_t raw_temp;

    // Читаем 3 байта температуры
    for (int i = 0; i < 3; i++) {
        if (i2c_read_reg(fd, BMP280_TEMP_MSB_REG + i, &data[i]) < 0)
            return -1;
    }

    // Собираем 20-битное значение
    raw_temp = ((int32_t)data[0] << 12) | ((int32_t)data[1] << 4) | 
               ((int32_t)data[2] >> 4);

    // Преобразуем в градусы Цельсия
    *temp = raw_temp / 1000.0;
    
    return 0;
}

int main(void)
{
    int fd;
    uint8_t id;
    float temperature;
    char filename[20];

    // Устанавливаем обработчик сигнала для корректного завершения
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    snprintf(filename, 19, "/dev/i2c-%d", I2C_BUS_NUM);
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "Не удалось открыть %s: %s\n", 
                filename, strerror(errno));
        return 1;
    }

    if (ioctl(fd, I2C_SLAVE, BMP280_ADDR) < 0) {
        fprintf(stderr, "Ошибка установки адреса I2C: %s\n", 
                strerror(errno));
        close(fd);
        return 1;
    }

    // Проверяем ID устройства
    if (i2c_read_reg(fd, BMP280_ID_REG, &id) < 0) {
        close(fd);
        return 1;
    }

    if (id != BMP280_EXPECTED_ID) {
        fprintf(stderr, "Неверный ID устройства: 0x%02X\n", id);
        close(fd);
        return 1;
    }

    printf("BMP280 найден, ID: 0x%02X\n", id);
    printf("Нажмите Ctrl+C для завершения\n\n");

    // Читаем температуру в цикле
    while (running) {
        if (read_temperature(fd, &temperature) == 0) {
            printf("\rТемпература: %.3f°C    ", temperature);
            fflush(stdout);
        }
        sleep(1);
    }

    printf("\nЗавершение работы...\n");
    close(fd);
    return 0;
}
