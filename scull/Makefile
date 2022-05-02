
ifeq ($(KERNELRELEASE),)

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: modules clean default

default: test.out modules
	sudo rmmod scull
	sudo sh scull_load.sh

test.out: test.c
	gcc test.c -o test.out

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(RM) -r *.o *.symvers .*.cmd *.mod.* *.mod *.order *.ko *.out

else

obj-m := scull.o

endif