#	@(#)Makefile	4.6	(Berkeley)	2/8/83
#
DESTDIR=
CFLAGS=	-O

# The following sources are incomplete and cannot be compiled without
# some work
WORK=	aard ddl mpu

# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	adventure backgammon boggle compat cribbage doctor fortune \
	mille monop quiz snake zork hangman

# Shell scripts that need only be installed and are never removed.
#
#SCRIPT=	

# C programs that live in the current directory and do not need
# explicit make lines.
#
STD=	arithmetic btlgammon banner bcd chess fish number wump

# C programs that live in the current directory and need explicit make lines.
#
NSTD=	canfield rain worm worms

all:	${SUBDIR} ${STD} ${NSTD}

${SUBDIR}: /tmp
	cd $@; make ${MFLAGS}

${STD}:
	cc ${CFLAGS} -o $@ $@.c

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} DESTDIR=${DESTDIR} install); done
#	for i in ${SCRIPT}; do (install -c $$i ${DESTDIR}/usr/games/$$i); done
	for i in ${STD} ${NSTD}; do (install $$i ${DESTDIR}/usr/games/$$i); done
	install -c chess.bin ${DESTDIR}/usr/games/lib/chess

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} clean); done
	rm -f ${STD} ${NSTD}

depend:
	cat </dev/null >x.c
	for i in ${STD} ${NSTD}; do \
		(echo $$i: $$i.c >>makedep; \
		/bin/grep '^#[ 	]*include' x.c $$i.c | sed \
			-e 's,<\(.*\)>,"/usr/include/\1",' \
			-e 's/:[^"]*"\([^"]*\)".*/: \1/' \
			-e 's/\.c//' >>makedep); done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep x.c
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# Files listed in ${NSTD} have explicit make lines given below.

canfield:
	cc -o canfield canfield.c -lcurses -ltermcap


rain:
	cc -o rain rain.c -lcurses -ltermcap

worm:
	cc -o worm worm.c -lcurses -ltermcap

worms:
	cc -o worms worms.c -lcurses -ltermcap

# DO NOT DELETE THIS LINE -- make depend uses it

arithmetic: arithmetic.c
arithmetic: /usr/include/stdio.h
arithmetic: /usr/include/signal.h
btlgammon: btlgammon.c
btlgammon: /usr/include/stdio.h
banner: banner.c
banner: /usr/include/stdio.h
bcd: bcd.c
chess: chess.c
chess: /usr/include/stdio.h
fish: fish.c
arithmetic: arithmetic.c
arithmetic: /usr/include/stdio.h
arithmetic: /usr/include/signal.h
btlgammon: btlgammon.c
btlgammon: /usr/include/stdio.h
banner: banner.c
banner: /usr/include/stdio.h
bcd: bcd.c
chess: chess.c
chess: /usr/include/stdio.h
fish: fish.c
fish: /usr/include/stdio.h
number: number.c
wump: wump.c
wump: /usr/include/stdio.h
wump: /usr/include/sgtty.h
canfield: canfield.c
canfield: /usr/include/curses.h
canfield: /usr/include/ctype.h
canfield: /usr/include/signal.h
rain: rain.c
rain: /usr/include/stdio.h
rain: /usr/include/sgtty.h
rain: /usr/include/signal.h
worm: worm.c
worm: /usr/include/ctype.h
worm: /usr/include/curses.h
worm: /usr/include/signal.h
worms: worms.c
worms: /usr/include/stdio.h
worms: /usr/include/sgtty.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
