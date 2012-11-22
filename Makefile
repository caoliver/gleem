CFLAGS+=-std=c99
LDFLAGS+=-ljpeg -lpng -lX11 -lXft

OBJS=test.o image.o numlock.o jpeg.o png.o util.o
BINS=test

.PHONY: clean

test: ${OBJS}
	gcc ${LDFLAGS} -o $@ $^

clean:
	rm -f ${BINS} ${OBJS} gmon.out `find -name \*~ -o -name \#\*`
