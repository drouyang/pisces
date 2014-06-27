#KERN_PATH=/usr/src/kernels/`uname -r`
#KERN_PATH=../linux/
KERN_PATH=/home/jarusl/linux-3.11.8-200.fc19.x86_64.debug

XPMEM_KERN_PATH=/home/briankoco/xpmem/kernel

obj-m += pisces.o

pisces-objs :=  src/main.o             \
		src/pisces_boot_params.o \
		src/boot.o             \
		src/enclave_cons.o     \
		src/enclave_ctrl.o     \
		src/pisces_lcall.o     \
		src/pisces_xbuf.o      \
		src/enclave_fs.o       \
		src/enclave_pci.o      \
		src/ipi.o              \
		src/enclave.o          \
		src/pisces_lock.o      \
		src/pisces_ringbuf.o   \
		src/file_io.o          \
		src/linux_syms.o       \
		src/launch_code.o      \
		src/pgtables.o         \
		src/util-hashtable.o   \
		src/util-queue.o       \
		src/v3_console.o 

ifeq ($(XPMEM),y)
EXTRA_CFLAGS         += -I$(XPMEM_KERN_PATH)/include -DUSING_XPMEM
KBUILD_EXTRA_SYMBOLS += $(XPMEM_KERN_PATH)/Module.symvers

pisces-objs +=  src/pisces_xpmem.o 

endif


pisces-objs += src/linux_trampoline/trampoline.o


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
