#ASU ID: 1217643921
#Name: Sanjay Arivazhagan
CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SDKTARGETSYSROOT=/opt/iot-devkit/1.7.2/sysroots/i586-poky-linux
export PATH:=/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin:/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux:$(PATH)

PWD:= $(shell pwd)

LDLIBS = -L$(SDKTARGETSYSROOT)/usr/lib
CCFLAGS = -I$(SDKTARGETSYSROOT)/usr/include/libnl3

APP = genl_ex

obj-m:= Kernel_Server.o

.PHONY:all
all: User_prog Kernel_Server.ko

Kernel_Server.ko:
	make -Wall ARCH=x86 CROSS_COMPILE=$(CROSS_COMPILE) -C $(SDKTARGETSYSROOT)/usr/src/kernel M=$(shell pwd) modules

User_prog:
	$(CC) -Wall -o $(APP) User_prog.c $(CCFLAGS) -lnl-genl-3 -lnl-3 -std=c99


.PHONY:clean
clean:
	make -C $(SDKTARGETSYSROOT)/usr/src/kernel M=$(shell pwd) clean
	rm -f *.o $(APP)

deploy:
	tar czf programs.tar.gz $(APP) genl_drv.ko
	scp programs.tar.gz root@10.0.1.100:/home/root
	ssh root@10.0.1.100 'tar xzf programs.tar.gz'