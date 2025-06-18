# Компиляция модуля ядра
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += virt_i2c.o

all: module reader

module:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

reader: bmp280_reader.c
	gcc -o bmp280_reader bmp280_reader.c

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
	rm -f bmp280_reader

install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install