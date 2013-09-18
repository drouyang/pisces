TARGET := pisces_ioctl mem_reserve pisces_load

all: $(TARGET)

pisces_ioctl: pisces_ioctl.c ../src/pisces.h

mem_reserve: mem_reserve.c ../src/pisces.h

pisces_load: pisces_load.c ../src/pisces.h

clean:
	rm $(TARGET)
