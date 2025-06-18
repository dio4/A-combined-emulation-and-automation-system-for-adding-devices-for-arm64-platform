#!/bin/bash
# подготовка (компиляция DT, добавление в config.txt)
cd /home/pi/projects/bmp280_virtual
echo "Компиляция Device Tree..."
dtc -@ -I dts -O dtb -o virt_bmp280.dtbo virt_bmp280.dts || exit 1

echo "Копирование DTBO в /boot/firmware/overlays..."
sudo cp virt_bmp280.dtbo /boot/firmware/overlays/ || exit 1

echo "Добавление оверлея в config.txt..."
grep -q "dtoverlay=virt_bmp280" /boot/firmware/config.txt || \
echo "dtoverlay=virt_bmp280" | sudo tee -a /boot/firmware/config.txt

echo "Настройка завершена. Требуется перезагрузка."
echo "После перезагрузки запустится сервис test_bmp280_run.service"
