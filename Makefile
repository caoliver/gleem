INCLUDES=-Ixdm-1.1.11 -Ixdm-1.1.11/include  -I/usr/include/freetype2
CFLAGS+=-std=c99 ${INCLUDES} -DHAVE_CONFIG_H -DGREET_LIB -fPIC -Wall
LDFLAGS+=-ljpeg -lpng -lX11 -lXft -lXinerama -ldl

GOBJS=greet.o
OBJS=image.o numlock.o read.o util.o cfg.o keywords.o
BINS=testgui
BINS+=testlib libXdmGreet.so
BINS+=testlib-pam libXdmGreet-pam.so

.PHONY: clean tags

${OBJS} greet.o: %.o: %.c
	${CC} -c ${CFLAGS} -pedantic $< -o $@

testgui: ${OBJS} testgui.c text.c
	gcc ${LDFLAGS} ${CFLAGS} -pedantic -o $@ $^

testlib: testlib.c libXdmGreet.so
	gcc ${CFLAGS} -Wl,-E $< -ldl -lcrypt -o $@

libXdmGreet.so: greet.o ${OBJS} ${GOBJS}
	gcc ${LDFLAGS} -shared -o $@ $^

testlib-pam: testlib.c libXdmGreet-pam.so
	gcc ${CFLAGS} -Wl,-E $< -ldl -lcrypt -lpam -o $@

libXdmGreet-pam.so: greet.o ${OBJS} ${GOBJS}
	gcc ${LDFLAGS} -lpam -shared -o $@ $^

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
	rm -rf GTAGS GRTAGS GPATH HTML

tags:
	gtags
	htags
