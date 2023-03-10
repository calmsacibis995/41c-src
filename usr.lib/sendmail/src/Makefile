#
#  SENDMAIL Makefile.
#
#	Version:
#		@(#)Makefile.m4	3.67		12/24/82
#

LIBS=	../lib/libsys.a -ldbm -ljobs
DESTDIR=

OBJS1=	conf.o main.o collect.o parseaddr.o alias.o deliver.o \
	savemail.o err.o readcf.o stab.o headers.o recipient.o \
	stats.o daemon.o usersmtp.o srvrsmtp.o queue.o \
	macro.o util.o clock.o trace.o envelope.o
OBJS2=	sysexits.o bmove.o arpadate.o convtime.o
OBJS=	$(OBJS1) $(OBJS2)
SRCS1=	conf.h sendmail.h \
	conf.c deliver.c main.c parseaddr.c err.c alias.c savemail.c \
	sysexits.c util.c bmove.c arpadate.c version.c collect.c \
	macro.c headers.c readcf.c stab.c recipient.c stats.c daemon.c \
	usersmtp.c srvrsmtp.c queue.c clock.c trace.c envelope.c
SRCS2=	TODO convtime.c
SRCS=	Version.c $(SRCS1) $(SRCS2)
ALL=	sendmail

CHOWN=	-echo chown
CHMOD=	chmod
O=	-O
COPTS=
CCONFIG=-I../include -DVMUNIX
CFLAGS=	$O $(COPTS) $(CCONFIG)
ASMSED=	../include/asm.sed
AR=	-ar
ARFLAGS=rvu
LINT=	lint
XREF=	ctags -x
CP=	cp
MV=	mv
INSTALL=install -c -s
M4=	m4
TOUCH=	touch
ABORT=	false

GET=	sccs get
DELTA=	sccs delta
WHAT=	sccs what
PRT=	sccs prt
REL=

ROOT=	root
OBJMODE=755

.c.o:
	cc -S ${CFLAGS} $*.c
	sed -f $(ASMSED) $*.s | as -o $*.o
	rm -f $*.s

sendmail: $(OBJS1) $(OBJS2) Version.o
	$(CC) $(COPTS) -o sendmail Version.o $(OBJS1) $(OBJS2) $(LIBS)
	$(CHMOD) $(OBJMODE) sendmail
	size sendmail; ls -l sendmail; $(WHAT) < Version.o

install: all
	$(INSTALL) sendmail $(DESTDIR)/usr/lib

version: newversion $(OBJS) Version.c

newversion:
	@rm -f SCCS/p.version.c version.c
	@$(GET) $(REL) -e SCCS/s.version.c
	@$(DELTA) -s SCCS/s.version.c
	@$(GET) -t -s SCCS/s.version.c

fullversion: $(OBJS) dumpVersion Version.o

dumpVersion:
	rm -f Version.c

Version.c: version.c
	@echo generating Version.c from version.c
	@cp version.c Version.c
	@chmod 644 Version.c
	@echo "" >> Version.c
	@echo "# ifdef COMMENT" >> Version.c
	@$(PRT) SCCS/s.version.c >> Version.c
	@echo "" >> Version.c
	@echo "code versions:" >> Version.c
	@echo "" >> Version.c
	@$(WHAT) $(OBJS) >> Version.c
	@echo "" >> Version.c
	@echo "# endif COMMENT" >> Version.c

$(OBJS1): sendmail.h
$(OBJS): conf.h

sendmail.h util.o: ../include/useful.h

all: $(ALL)

#
#  Auxiliary support entries
#

clean:
	rm -f core sendmail rmail usersmtp uucp a.out XREF sendmail.cf
	rm -f *.o

sources: $(SRCS)

$(SRCS1) $(SRCS2):
	$(GET) $(REL) SCCS/s.$@

print: $(SRCS)
	@ls -l | pr -h "sendmail directory"
	@$(XREF) *.c | pr -h "cross reference listing"
	@size *.o | pr -h "object code sizes"
	@pr Makefile *.m4 *.h *.[cs]

lint:
	$(LINT) $(CCONFIG) $(SRCS1)
