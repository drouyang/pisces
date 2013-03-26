obj-m += pisces.o

pisces-objs := main.o wakeup_secondary.o loader.o #trampoline.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

