obj-m += pisces.o

pisces-objs :=  src/main.o \
    		src/pisces_dev.o \
		src/pisces_loader.o \
		src/wakeup_secondary.o \
		src/domain_xcall.o 

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

.PHONY: tags
tags:
	ctags -R src/
