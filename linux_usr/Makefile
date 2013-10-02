TARGET := \
    pisces_ioctl \
    mem_reserve \
    pisces_load \
    pisces_launch \
    pisces_cons \
    pisces_ctrl \
    pisces_enclave_add_cpu

all: $(TARGET)

pisces_launch: pisces_launch.c ../src/pisces.h

pisces_ioctl: pisces_ioctl.c ../src/pisces.h

mem_reserve: mem_reserve.c ../src/pisces.h

pisces_load: pisces_load.c ../src/pisces.h

pisces_cons: pisces_cons.c ../src/pisces.h

pisces_ctrl: pisces_ctrl.c ../src/pisces.h

pisces_enclave_add_cpu: pisces_enclave_add_cpu.c ../src/pisces.h

clean:
	rm $(TARGET)
