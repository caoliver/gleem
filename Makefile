CFLAGS+=-std=c99
LDFLAGS+=-ljpeg -lpng -lX11 -lXft

OBJS=image.o numlock.o jpeg.o png.o util.o atomizer.o
BINS=test atomizer-test

.PHONY: clean

test: ${OBJS} test.c
	gcc ${LDFLAGS} -o $@ $^

atomizer.o: atomizer.c khash.h

atomizer-test: atomizer-test.c atomizer.o util.o

clean:
	rm -f ${BINS} ${OBJS} gmon.out `find -name \*~ -o -name \#\*`
