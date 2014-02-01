INCLUDES=-Ixdm-1.1.11 -Ixdm-1.1.11/include  -I/usr/include/freetype2
CFLAGS+=-std=c99 ${INCLUDES} -DHAVE_CONFIG_H -DGREET_LIB -fPIC
CFLAGS+=-Wall -Wno-parentheses -pedantic
CFLAGS+=-D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE
LDFLAGS+=-ljpeg -lpng -lX11 -lXft -lXinerama -ldl

GOBJS=greet.o
OBJS=image.o read.o util.o cfg.o keywords.o text.o
BINS=libXdmGreet.so

.PHONY: clean tags

libXdmGreet.so: greet.o text.o ${OBJS} ${GOBJS}
	gcc ${LDFLAGS} -shared -o $@ $^

${OBJS} greet.o: %.o: %.c
	${CC} -c ${CFLAGS} $< -o $@

install: libXdmGreet.so
	mv -n /etc/X11/xdm/libXdmGreet.so /etc/X11/xdm/libXdmGreet.so~
	cp libXdmGreet.so /etc/X11/xdm/libXdmGreet.so

cfg.c:	keywords.h

keywords.c: keywords.sh
	sh ./keywords.sh

keywords.h: keywords.sh
	sh ./keywords.sh

clean:
	rm -f ${BINS} ${OBJS} ${GOBJS} gmon.out
	rm -f keywords.c keywords.h
	rm -f `find -name \*~ -o -name \#\*`
