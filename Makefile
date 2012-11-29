INCLUDES=-Ixdm-1.1.11 -Ixdm-1.1.11/include  -I/usr/include/freetype2
CFLAGS+=-std=c99 ${INCLUDES} -DHAVE_CONFIG_H -DGREET_LIB -fPIC -Wall -pedantic
LDFLAGS+=-ljpeg -lpng -lX11 -lXft -ldl

GOBJS=greet.o
OBJS=image.o numlock.o jpeg.o png.o util.o
BINS=test atomizer-test

.PHONY: clean

test: ${OBJS} test.c
	 gcc ${LDFLAGS} ${CFLAGS} -o $@ $^

libXdmGreet.so: greet.o ${OBJS} ${GOBJS}
	gcc ${LDFLAGS} -shared -o $@ $^

install: libXdmGreet.so
	mv -n /etc/X11/xdm/libXdmGreet.so /etc/X11/xdm/libXdmGreet.so~
	cp libXdmGreet.so /etc/X11/xdm/libXdmGreet.so

clean:
	rm -f ${BINS} ${OBJS} ${GOBJS} gmon.out libXdmGreet.so
	rm -f `find -name \*~ -o -name \#\*`
