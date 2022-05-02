
ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: modules clean

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(RM) -r *.o *.symvers .*.cmd *.mod.* *.mod *.order *.ko

else

obj-m := hello.o

endif