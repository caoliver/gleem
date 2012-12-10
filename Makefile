INCLUDES=-Ixdm-1.1.11 -Ixdm-1.1.11/include  -I/usr/include/freetype2
CFLAGS+=-std=c99 ${INCLUDES} -DHAVE_CONFIG_H -DGREET_LIB -fPIC -Wall -pedantic
LDFLAGS+=-ljpeg -lpng -lX11 -lXft -lXinerama -ldl

GOBJS=greet.o
OBJS=image.o numlock.o read.o util.o cfg.o keywords.o
BINS=test testcfg

.PHONY: clean tags

test: ${OBJS} test.c
	gcc ${LDFLAGS} ${CFLAGS} -o $@ $^

testcfg: cfg.o ${OBJS}
	gcc ${LDFLAGS} ${CFLAGS} -o $@ $^

libXdmGreet.so: greet.o ${OBJS} ${GOBJS}
	gcc ${LDFLAGS} -shared -o $@ $^

install: libXdmGreet.so
	mv -n /etc/X11/xdm/libXdmGreet.so /etc/X11/xdm/libXdmGreet.so~
	cp libXdmGreet.so /etc/X11/xdm/libXdmGreet.so

cfg.c:	keywords.h

keywords.c: keywords.sh
	sh ./keywords.sh

keywords.h: keywords.sh
	sh ./keywords.sh

clean:
	rm -f ${BINS} ${OBJS} ${GOBJS} gmon.out libXdmGreet.so
	rm -f keywords.c keywords.h
	rm -f `find -name \*~ -o -name \#\*`
	rm -rf GTAGS GRTAGS GPATH HTML

tags:
	gtags
	htags
