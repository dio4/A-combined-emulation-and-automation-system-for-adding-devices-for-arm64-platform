#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/random.h>

#define BMP280_I2C_ADDR 0x76
#define VIRT_I2C_BUS_NUM 15

// Регистры BMP280
#define BMP280_ID_REG          0xD0
#define BMP280_RESET_REG       0xE0
#define BMP280_STATUS_REG      0xF3
#define BMP280_CTRL_MEAS_REG   0xF4
#define BMP280_CONFIG_REG      0xF5
#define BMP280_TEMP_MSB_REG    0xFA
#define BMP280_TEMP_LSB_REG    0xFB
#define BMP280_TEMP_XLSB_REG   0xFC

static struct i2c_adapter *virt_i2c_adapter;
static uint8_t bmp280_registers[256] = {0};
static uint8_t current_register = 0;
static struct task_struct *temp_update_thread;
static bool thread_should_stop = false;

// Эмуляция температуры
static int32_t current_temperature = 25000; // 25.000°C
static uint8_t ctrl_meas = 0;

static void update_temperature_registers(void)
{
    int32_t temp = current_temperature;
    // Преобразуем температуру в формат BMP280 (20 бит)
    bmp280_registers[BMP280_TEMP_MSB_REG] = (temp >> 12) & 0xFF;
    bmp280_registers[BMP280_TEMP_LSB_REG] = (temp >> 4) & 0xFF;
    bmp280_registers[BMP280_TEMP_XLSB_REG] = (temp << 4) & 0xF0;
}

static int temperature_update_thread(void *data)
{
    u32 random_val;
    
    while (!kthread_should_stop() && !thread_should_stop) {
        // Используем get_random_u32() вместо get_random_int()
        random_val = get_random_u32();
        current_temperature += (random_val % 1000) - 500;
        
        // Ограничиваем температуру в диапазоне 20-30°C
        if (current_temperature > 30000) current_temperature = 30000;
        if (current_temperature < 20000) current_temperature = 20000;

        update_temperature_registers();
        
        // Обновляем каждую секунду
        msleep(1000);
    }
    return 0;
}

static void init_bmp280_registers(void)
{
    memset(bmp280_registers, 0, sizeof(bmp280_registers));
    
    // Инициализация регистров
    bmp280_registers[BMP280_ID_REG] = 0x58;        // ID чипа
    bmp280_registers[BMP280_RESET_REG] = 0x00;     // Регистр сброса
    bmp280_registers[BMP280_STATUS_REG] = 0x00;    // Статус
    bmp280_registers[BMP280_CTRL_MEAS_REG] = 0x27; // Нормальный режим
    bmp280_registers[BMP280_CONFIG_REG] = 0x00;    // Конфигурация

    update_temperature_registers();
    
    pr_info("virt_i2c: BMP280 registers initialized\n");
}

static int virt_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs,
                        int num)
{
    int i;

    for (i = 0; i < num; i++) {
        if (msgs[i].addr != BMP280_I2C_ADDR) {
            return -ENODEV;
        }

        if (!(msgs[i].flags & I2C_M_RD)) {
            if (msgs[i].len > 0) {
                current_register = msgs[i].buf[0];
                if (msgs[i].len > 1) {
                    // Запись значения в регистр
                    bmp280_registers[current_register] = msgs[i].buf[1];
                    pr_debug("virt_i2c: Write reg 0x%02X = 0x%02X\n",
                            current_register, msgs[i].buf[1]);
                }
            }
        } else {
            if (msgs[i].len > 0) {
                memcpy(msgs[i].buf, &bmp280_registers[current_register],
                       msgs[i].len);
                pr_debug("virt_i2c: Read reg 0x%02X = 0x%02X\n",
                        current_register, msgs[i].buf[0]);
            }
        }
    }
    return num;
}

static s32 virt_i2c_smbus(struct i2c_adapter *adapter, u16 addr,
                         unsigned short flags, char read_write,
                         u8 command, int size, union i2c_smbus_data *data)
{
    if (addr != BMP280_I2C_ADDR) {
        return -ENODEV;
    }

    switch (size) {
        case I2C_SMBUS_QUICK:
            return 0;
        case I2C_SMBUS_BYTE:
        case I2C_SMBUS_BYTE_DATA:
            if (read_write == I2C_SMBUS_READ) {
                data->byte = bmp280_registers[command];
            } else {
                bmp280_registers[command] = data->byte;
            }
            return 0;
    }
    return -EOPNOTSUPP;
}

static u32 virt_i2c_functionality(struct i2c_adapter *adap)
{
    return I2C_FUNC_I2C | 
           I2C_FUNC_SMBUS_QUICK |
           I2C_FUNC_SMBUS_BYTE |
           I2C_FUNC_SMBUS_BYTE_DATA;
}

static const struct i2c_algorithm virt_i2c_algo = {
    .master_xfer = virt_i2c_xfer,
    .smbus_xfer = virt_i2c_smbus,
    .functionality = virt_i2c_functionality,
};

static int __init virt_i2c_init(void)
{
    virt_i2c_adapter = kzalloc(sizeof(struct i2c_adapter), GFP_KERNEL);
    if (!virt_i2c_adapter) {
        pr_err("virt_i2c: Failed to allocate memory\n");
        return -ENOMEM;
    }

    init_bmp280_registers();

    // Запускаем поток обновления температуры
    thread_should_stop = false;
    temp_update_thread = kthread_run(temperature_update_thread, NULL,
                                   "bmp280_temp_update");
    if (IS_ERR(temp_update_thread)) {
        pr_err("virt_i2c: Failed to create update thread\n");
        kfree(virt_i2c_adapter);
        return PTR_ERR(temp_update_thread);
    }

    virt_i2c_adapter->owner = THIS_MODULE;
    virt_i2c_adapter->class = I2C_CLASS_HWMON;
    virt_i2c_adapter->algo = &virt_i2c_algo;
    virt_i2c_adapter->nr = VIRT_I2C_BUS_NUM;
    snprintf(virt_i2c_adapter->name, sizeof(virt_i2c_adapter->name),
             "Virtual I2C-%d", VIRT_I2C_BUS_NUM);

    if (i2c_add_numbered_adapter(virt_i2c_adapter) < 0) {
        pr_err("virt_i2c: Failed to add adapter\n");
        thread_should_stop = true;
        kthread_stop(temp_update_thread);
        kfree(virt_i2c_adapter);
        return -EBUSY;
    }

    pr_info("virt_i2c: Module loaded on bus %d\n", VIRT_I2C_BUS_NUM);
    return 0;
}

static void __exit virt_i2c_exit(void)
{
    thread_should_stop = true;
    if (temp_update_thread)
        kthread_stop(temp_update_thread);
    
    if (virt_i2c_adapter) {
        i2c_del_adapter(virt_i2c_adapter);
        kfree(virt_i2c_adapter);
    }
    pr_info("virt_i2c: Module unloaded\n");
}

module_init(virt_i2c_init);
module_exit(virt_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DereviankoAV");
MODULE_DESCRIPTION("Virtual I2C Bus Driver for BMP280");
