TARGET := pisces_ioctl mem_reserve pisces_load

all: $(TARGET)

pisces_ioctl: pisces_ioctl.c ../src/pisces_dev.h

mem_reserve: mem_reserve.c ../src/pisces_dev.h

pisces_load: pisces_load.c ../src/pisces_dev.h

clean:
	rm $(TARGET)
