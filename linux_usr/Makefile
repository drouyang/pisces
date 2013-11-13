PETLIB := ../petlib


CFLAGS = -I$(PETLIB) -L$(PETLIB)

CC = gcc
AR = ar

libs := 	$(PETLIB)/petlib.a

execs := 	pisces_load \
		pisces_launch \
		pisces_ctrl \
		pisces_cons \
		pisces_lcall_test \
		v3_create \
		v3_launch

build = \
	@if [ -z "$V" ]; then \
		echo '   [$1]     $@'; \
		$2; \
	else \
		echo '$2'; \
		$2; \
	fi


all: $(execs) $(libs)


% : %.c $(libs)
	$(call build,CC,$(CC) $(CFLAGS)  $<  $(libs) -o $@)

%.o : %.c
	$(call build,CC,$(CC) $(CFLAGS) -c $<  -o $@)

%.a : %.o
	$(call build,AR,$(AR) rcs $@ $^)






clean:
	rm -f $(wildcard  $(execs))
