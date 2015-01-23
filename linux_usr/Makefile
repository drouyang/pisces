
CFLAGS = -I$(PETLIB_PATH) -L$(PETLIB_PATH)


CC = gcc
AR = ar

libs := 	libpisces.a 

ext_libs := $(PETLIB_PATH)/petlib.a 

execs := 	pisces_load \
		pisces_launch \
		pisces_shutdown \
		pisces_add \
		pisces_cons \
		pisces_reset \
		pisces_run \
		v3_create \
		v3_free \
		v3_launch \
		v3_stop \
		v3_cons_sc \
		v3_cons_nosc \
		v3_dbg \
		v3_pci \


libpisces-objs := pisces.o pisces_ctrl.o


build = \
	@if [ -z "$V" ]; then \
		echo '   [$1]     $@'; \
		$2; \
	else \
		echo '$2'; \
		$2; \
	fi

ifeq ($(STATIC),y)
  CFLAGS        += -static
  CURSES_CFLAGS  = -DNCURSES_STATIC
endif



all: $(execs) $(libs)

libpisces.a: $(libpisces-objs)
	$(call build,AR,$(AR) rcs $@ $^)


% : %.c $(libs)
	$(call build,CC,$(CC) $(CFLAGS)  $<  $(libs) $(ext_libs) -o $@)

%.o : %.c
	$(call build,CC,$(CC) $(CFLAGS) -c $<  -o $@)

%.a : %.o
	$(call build,AR,$(AR) rcs $@ $^)



v3_cons_sc: v3_cons_sc.c
	$(call build,CC,$(CC) $(CFLAGS) $< $(libs) $(ext_libs) -lcurses -ltinfo -o $@)

v3_cons_nosc: v3_cons_nosc.c
	$(call build,CC,$(CC) $(CFLAGS) $< $(libs) $(ext_libs) -lcurses -ltinfo -o $@)



clean:
	rm -f $(wildcard  $(execs)) $(wildcard $(libpisces-objs)) *.a
