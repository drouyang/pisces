KERN_PATH=../linux


obj-m += pisces.o

pisces-objs :=  src/main.o \
		src/boot.o \
		src/pisces_cons.o \
		src/pisces_ctrl.o \
		src/wakeup_secondary.o \
		src/enclave_xcall.o \
		src/buddy.o \
		src/enclave.o \
		src/pisces_lock.o \
		src/pisces_ringbuf.o \
		src/numa.o \
		src/mm.o \
		src/file_io.o \
		src/launch_code.o

all:
	make -C $(KERN_PATH) M=$(PWD) modules
	make -C linux_usr/

clean:
	make -C $(KERN_PATH) M=$(PWD) clean
	make -C linux_usr/ clean

.PHONY: tags
tags:
	ctags -R src/
