KERN_PATH=../linux


obj-m += pisces.o

pisces-objs :=  src/main.o \
		src/boot.o \
		src/enclave_cons.o \
		src/pisces_ctrl.o \
		src/pisces_lcall.o \
		src/wakeup_secondary.o \
		src/ipi.o \
		src/enclave.o \
		src/pisces_lock.o \
		src/pisces_ringbuf.o \
		src/file_io.o \
		src/linux_syms.o \
		src/launch_code.o

all:
	make -C $(KERN_PATH) M=$(PWD) modules
	make -C petlib/
	make -C linux_usr/

clean:
	make -C $(KERN_PATH) M=$(PWD) clean
	make -C petlib/ clean
	make -C linux_usr/ clean


.PHONY: tags
tags:
	ctags -R src/
