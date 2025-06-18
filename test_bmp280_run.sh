#!/bin/bash
# основная работа (загрузка модуля, запуск программы)
# Определяем целевой каталог
TARGET_DIR="/home/pi/projects/bmp280_virtual"

# Проверяем существование каталога
if [ ! -d "$TARGET_DIR" ]; then
    echo "Ошибка: Каталог $TARGET_DIR не существует!"
    exit 1
fi

# Переходим в целевой каталог
cd "$TARGET_DIR" || { echo "Не удалось перейти в $TARGET_DIR"; exit 1; }

# Проверяем наличие необходимых пакетов
if ! dpkg -l | grep -q "linux-headers-$(uname -r)"; then
    echo "Установка linux headers..."
    sudo apt-get update
    sudo apt-get install -y linux-headers-$(uname -r) || exit 1
fi

# Компиляция
echo "Компиляция в каталоге: $(pwd)"
make clean && make || exit 1

# Установка модуля
sudo cp virt_i2c.ko /lib/modules/$(uname -r)/kernel/drivers/ || exit 1
sudo depmod -a || exit 1
sudo modprobe virt_i2c || exit 1

# Запуск программы
echo "Запуск программы из каталога: $(pwd)"
sudo ./bmp280_reader