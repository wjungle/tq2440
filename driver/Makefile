S3C2440X_KERNEL_DIR=/home/jungle/tq2440/linux-2.6.30.4

ifneq ($(KERNELRELEASE),)
	obj-m := timer_leds.o
else
	KERNELDIR ?= $(S3C2440X_KERNEL_DIR)
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	rm -f *.o *.mod.* *.ko *.o.*
endif
