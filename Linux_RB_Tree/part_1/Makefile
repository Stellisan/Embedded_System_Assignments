# To run "make", you can either set up environment variables via
#		source /opt/iot-devkit/1.7.2/environment-setup-i586-poky-linux
# or set up the following make variables
#


CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = i586-poky-linux-
SDKTARGETSYSROOT=/opt/iot-devkit/1.7.2/sysroots/i586-poky-linux
export PATH:=/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin:/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux:$(PATH)

APP = rbtree_tester
obj-m:=RBHDriver.o

all:
	@echo $$PATH
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(SDKTARGETSYSROOT)/usr/src/kernel M=$(shell pwd) modules
	$(CC) -pthread -Wall -o $(APP) main.c --sysroot=$(SDKTARGETSYSROOT) -std=c99
	
clean:
	make ARCH=x86 CROSS_COMPILE=i586-poky-linux- -C $(SDKTARGETSYSROOT)/usr/src/kernel M=$(shell pwd) clean
	rm -f *.o

