#	@(#)Makefile	4.1	2/17/83
#
CFLAGS=	-O
DESTDIR=

more:	more.o
	${CC} -o more more.o -lcurses -ltermcap

install: more more.help
	install -s more ${DESTDIR}/usr/ucb/more
	rm -f ${DESTDIR}/usr/ucb/page
	ln ${DESTDIR}/usr/ucb/more ${DESTDIR}/usr/ucb/page
	install -c more.help ${DESTDIR}/usr/lib/more.help

more.o:	more.c
more.o:	/usr/include/stdio.h
more.o:	/usr/include/ctype.h
more.o:	/usr/include/signal.h
more.o:	/usr/include/errno.h
more.o:	/usr/include/sgtty.h
more.o:	/usr/include/setjmp.h
more.o:	/usr/include/sys/types.h
more.o:	/usr/include/sys/stat.h
more.o:	/usr/include/local/uparm.h

clean:
	rm -f more more.o core a.out errs
