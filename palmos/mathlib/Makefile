SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

CFLAGS = -MMD -fPIC -march=armv4t -msingle-pic-base -nostartfiles

.c.o:
	arm-palmos-gcc $(CFLAGS) -c -o $@ $<

mathlib.a: $(OBJS)
	arm-palmos-ar rsc mathlib.a $(OBJS)

clean:
	rm -f *.d *.o mathlib.a
