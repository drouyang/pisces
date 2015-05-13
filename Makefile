#KERN_PATH=/usr/src/kernels/`uname -r`
KERN_PATH=/lib/modules/`uname -r`/build/
#KERN_PATH=../linux/
#KERN_PATH=/home/jarusl/linux-3.11.8-200.fc19.x86_64.debug

BUILD_DIR=$(PWD)

XPMEM_PATH=$(BUILD_DIR)/../xpmem/
PETLIB_PATH=$(BUILD_DIR)/../petlib


VERSION_CMD=$(PWD)/version

obj-m += pisces.o

pisces-objs :=  src/main.o             \
		src/pisces_boot_params.o \
		src/boot.o             \
		src/enclave_cons.o     \
		src/enclave_ctrl.o     \
		src/enclave_fs.o       \
		src/enclave.o          \
		src/pisces_lcall.o     \
		src/pisces_xbuf.o      \
		src/pisces_lock.o      \
		src/pisces_ringbuf.o   \
		src/pisces_irq.o       \
		src/file_io.o          \
		src/launch_code.o      \
		src/pgtables.o         \
		src/util-hashtable.o   \
		src/util-queue.o       \
		src/v3_console.o       \
		src/linux_syms.o

ifeq ($(XPMEM),y)
  EXTRA_CFLAGS         += -I$(XPMEM_PATH)/include -DUSING_XPMEM
  KBUILD_EXTRA_SYMBOLS += $(XPMEM_PATH)/mod/Module.symvers

  pisces-objs +=  src/pisces_xpmem.o 

endif

#
# If the version command doesn't exist it will be built as a top level dependency
#   OLD_VERSION will then be set on the reinvocation
#
ifneq ($(wildcard $(VERSION_CMD)),) 
  OLD_VERSION=$(shell $(VERSION_CMD))
endif


ifeq ($(OLD_VERSION),3)
# Known Latest Version
  pisces-objs    +=  src/trampoline_3/trampoline.o \
		     src/enclave_pci.o  
  EXTRA_CFLAGS   += -DPCI_ENABLED
  TRAMPOLINE_TGT := trampoline_3
else ifeq ($(OLD_VERSION), 2)
# VERSION < 3.5.0
  pisces-objs  += src/trampoline_2/trampoline_64.o \
		  src/trampoline_2/trampoline.o
  USR_FLAGS    += STATIC=y
  TRAMPOLINE_TGT := trampoline_2

else ifeq ($(OLD_VERSION), 1)
# VERSION < 2.6.39
  pisces-objs  += src/trampoline_1/trampoline_64.o \
		  src/trampoline_1/trampoline.o
  USR_FLAGS    += STATIC=y
  TRAMPOLINE_TGT := trampoline_1

else ifeq ($(OLD_VERSION), 0)
# VERSION < 2.6.33
  pisces-objs  += src/trampoline_0/trampoline_64.o \
		  src/trampoline_0/trampoline.o
  USR_FLAGS    += STATIC=y
  TRAMPOLINE_TGT := trampoline_0
endif




# All default targets must be invoked via a call to make 
all: version_exec
	make build_setup
	make -C $(KERN_PATH) M=$(PWD) modules
	make -C linux_usr/ PETLIB_PATH=$(PETLIB_PATH) $(USR_FLAGS)

version_exec: version.c $(VERSION_CMD)
	gcc -I$(KERN_PATH)/include version.c -o $(VERSION_CMD)

build_setup:
	ln -s $(TRAMPOLINE_TGT) src/trampoline

clean: 
	make -C $(KERN_PATH) M=$(PWD) clean
	make -C linux_usr/ clean
	rm -f $(shell find src/ -name "*.o")
	rm -f  $(VERSION_CMD) src/trampoline

.PHONY: tags
tags:
	ctags -R src/
