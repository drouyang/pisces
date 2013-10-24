
CFLAGS = 

CC = gcc
AR = ar

libs := 
execs := 	pisces_load \
		pisces_launch \
		pisces_ctrl \
		pisces_cons

build = \
	@if [ -z "$V" ]; then \
		echo '   [$1]     $@'; \
		$2; \
	else \
		echo '$2'; \
		$2; \
	fi


all: $(libs) $(execs)


% : %.c
	$(call build,CC,$(CC) $(CFLAGS) $< -o $@)

%.o : %.c
	$(call build,CC,$(CC) $(CFLAGS) -c $<  -o $@)

%.a : %.o
	$(call build,AR,$(AR) rcs $@ $^)






clean:
	rm -f $(wildcard  $(execs) $(libs))
