#KERN_PATH=/usr/src/kernels/`uname -r`
#KERN_PATH=../linux/
KERN_PATH=/home/jarusl/linux-3.11.8-200.fc19.x86_64.debug

obj-m += pisces.o

pisces-objs :=  src/main.o \
		src/boot.o \
		src/enclave_cons.o \
		src/pisces_ctrl.o \
		src/pisces_lcall.o \
		src/pisces_xbuf.o \
		src/enclave_fs.o \
		src/wakeup_secondary.o \
		src/ipi.o \
		src/enclave.o \
		src/pisces_lock.o \
		src/pisces_ringbuf.o \
		src/file_io.o \
		src/linux_syms.o \
		src/launch_code.o \
		src/pgtables.o \
		src/util-hashtable.o \
		src/util-queue.o \
		src/pisces_pci.o \
		src/pagemap.o \
		src/v3_console.o \


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
