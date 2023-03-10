#	@(#)Makefile	4.1	(Berkeley)	83/02/24
#
DESTDIR=

LESSONS = files editor morefiles macros eqn C

FILES = lrnref.h \
	copy.c dounit.c learn.c list.c \
	makpipe.c maktee.c mem.c mysys.c selsub.c selunit.c \
	start.c whatnow.c wrapup.c \
	lcount.c tee.c \
	makefile

OBJECTS = copy.o dounit.o learn.o list.o mem.o \
	makpipe.o maktee.o mysys.o selsub.o selunit.o \
	start.o whatnow.o wrapup.o

CFLAGS = -O
LIBRARY =
LLIB	= $(DESTDIR)/usr/lib/learn

all:	learn tee lcount
	
install:	all
	install -s learn $(DESTDIR)/usr/bin
	install -s tee $(LLIB)
	install -s lcount $(LLIB)
	@echo "Then do 'make play; make log' to make playpen and log directories"

clean:
	rm -f *.o learn tee lcount errs

cmp:	all
	cmp learn $(DESTDIR)/bin/learn
	cmp tee $(LLIB)/tee
	cmp lcount $(LLIB)/lcount
	rm learn tee lcount *.o

learn: $(OBJECTS)
	cc -o learn $(CFLAGS) $(OBJECTS) $(LIBRARY)

$(OBJECTS): lrnref.h

lcount tee:
	cc $(CFLAGS) $@.c -o $@ $(LIBRARY)

lessons:	$(LESSONS)

$(LESSONS):
	-rm -r $(LLIB)/$@
	mkdir $(LLIB)/$@
	(cd $(LLIB)/$@; ar x ../$@.a)

play log:
	-rm -r $(LLIB)/$@; mkdir $(LLIB)/$@; chmod +w $(LLIB)/$@

check:
	-@test -r $(LLIB)/tee || echo 'tee not present; make tee'
	-@test -r $(LLIB)/lcount || echo 'lcount not present; make lcount'
	-@test -r $(LLIB)/play || echo 'play directory not present; make play'
	-@test -r $(LLIB)/log || echo 'log directory not present; make log'
	-@for i in $(LESSONS); do test -r $(LLIB)/$$i/L0 || echo $$i not unarchived, make $$i; done
