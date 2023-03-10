SCCSID = "@(#)pcmakefile 1.43 2/28/83"

INSTALLDIR = $(DESTDIR)/usr/lib
INSTALLNAME = ${INSTALLDIR}/pc0
BINDIR = ../pc0
SRCDIR = ../src
WHOAMI = pc
VERSION = 2.12
DATE = 2/28/83

MKSTR = /usr/ucb/mkstr
EYACC = /usr/ucb/eyacc
RM = -rm -f
TOUCH = touch -f

CFLAGS = -O -w
LDFLAGS = -z

LIBDIR = ${DESTDIR}/usr/lib
TMPDIR = tmp

ERRORSTRINGS = ${WHOAMI}${VERSION}strings
OLDSTRINGS = ${WHOAMI}[0-9]*.[0-9]*strings
HOWFILE = how_${WHOAMI}

SRCS =	ato.c \
	call.c case.c clas.c const.c conv.c cset.c \
	error.c fdec.c fend.c fhdr.c flvalue.c forop.c func.c gen.c hash.c \
	lab.c lookup.c lval.c stklval.c \
	main.c nl.c proc.c put.c \
	rec.c rval.c stkrval.c\
	savenl.c \
	stat.c string.c subr.c \
	tmps.c tree.c type.c var.c \
	TRdata.c \
	treen.c yycopy.c \
	yycosts.c yyerror.c yyget.c yyid.c yylex.c yymain.c yyoptions.c \
	yypanic.c yyparse.c yyprint.c yyput.c yyrecover.c yyseman.c yytree.c \
	p2put.c stab.c pcproc.c pcfunc.c pccaseop.c pclval.c sconv.c

HDRS =	0.h OPnames.h align.h iorec.h objfmt.h pstab.h pc.h pcops.h \
	send.h tree.h whoami.h yy.h config.h

OTHERS = pas.y opc.c CONFIG.c gram pic.c

OBJS =	ato.o \
	call.o case.o clas.o const.o conv.o cset.o \
	error.o fdec.o fend.o fhdr.o flvalue.o forop.o func.o gen.o hash.o \
	lab.o lookup.o lval.o stklval.o \
	main.o nl.o proc.o put.o \
	rec.o rval.o stkrval.o\
	savenl.o \
	stat.o string.o subr.o \
	tmps.o tree.o type.o var.o \
	TRdata.o \
	treen.o yycopy.o \
	yycosts.o yyerror.o yyget.o yyid.o yylex.o yymain.o yyoptions.o \
	yypanic.o yyparse.o yyprint.o yyput.o yyrecover.o yyseman.o yytree.o \
	p2put.o stab.o pcproc.o pcfunc.o pccaseop.o pclval.o sconv.o

a.out: ${OBJS} y.tab.o config.c
	${CC} ${CFLAGS} ${LDFLAGS} ${OBJS} y.tab.o config.c

${OBJS}:
	${RM} $*.c
	cd ${SRCDIR}; ${MKSTR} - ${BINDIR}/${ERRORSTRINGS} ${BINDIR}/ $*.c
	${CC} ${CFLAGS} -I. -I${SRCDIR} -c $*.c
	${RM} $*.c

y.tab.h: ${SRCDIR}/pas.y ${SRCDIR}/gram
	${RM} y.tab.h y.tab.c
	${EYACC} ${SRCDIR}/pas.y > /dev/null
	ex - y.tab.c <${SRCDIR}/gram
	@echo that makes y.tab.h and y.tab.c
y.tab.o: y.tab.c y.tab.h
	${RM} x.y.tab.c
	${MKSTR} - ${ERRORSTRINGS} x. y.tab.c
	${CC} ${CFLAGS} -I${SRCDIR} -I. -c x.y.tab.c
	mv x.y.tab.o y.tab.o
	${RM} x.y.tab.c

picture: ${SRCDIR}/OPnames.h ${SRCDIR}/pic.c
	${RM} picture
	cc ${SRCDIR}/pic.c -o pic
	pic >picture
	${RM} pic

opcode.h: ${SRCDIR}/OPnames.h ${SRCDIR}/opc.c
	${RM} opcode.h
	cc ${SRCDIR}/opc.c -o opc
	opc >opcode.h
	${RM} opc

${SRCDIR}/0.h:
	${TOUCH} ${SRCDIR}/0.h

config.c: ${SRCDIR}/CONFIG.c makefile
	sed -e "s?VERSION?${VERSION}?g" \
	    -e "s?DATE?${DATE}?g" \
	    -e "s?INSTALLDIR?${INSTALLDIR}?g" \
	    -e "s?LIBDIR?${LIBDIR}?g" \
	    -e "s?ERRORSTRINGS?${ERRORSTRINGS}?g" \
	    -e "s?HOWFILE?${HOWFILE}?g" \
	    < ${SRCDIR}/CONFIG.c >config.c

clean:
	${RM} *.o *.c
	${RM} y.tab.h y.tab.c y.tab.out
	${RM} ${WHOAMI}*strings
	${RM} config.c opcode.h picture
	${RM} a.out core *.list *.bak errs
	${RM} opc pic tags

install: a.out
	${RM} ${LIBDIR}/${OLDSTRINGS}
	install -c ${ERRORSTRINGS} ${LIBDIR}/${ERRORSTRINGS}
	install -c a.out ${INSTALLNAME}

depend:
	ls ${SRCDIR}/*.c | sed \
		-e 's;^${SRCDIR}/\([a-zA-Z0-9]*\)\.c;\1.o: &;' >makedep
	/bin/grep '^#[ 	]*include' *.h ${SRCDIR}/*.h | sed \
		-e '/<.*>/d' \
		-e 's;\(.*\):[^"]*"\([^"]*\)".*;\1: \2;' >>makedep
	/bin/grep '^#[ 	]*include' ${SRCDIR}/*.c | sed \
		-e '/<.*>/d' \
		-e 's;:[^"]*"\([^"]*\)".*;: ${SRCDIR}/\1;' \
		-e 's;^${SRCDIR}/\([a-zA-Z0-9]*\)\.c;\1.o;' \
		-e 's;: ${SRCDIR}/whoami.h;: whoami.h;' \
		-e 's;: ${SRCDIR}/opcode.h;: opcode.h;' >>makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp makefile makefile.bak
	ed - makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> makefile
	echo '# see make depend above' >> makefile

# DO NOT DELETE THIS LINE -- make depend uses it

CONFIG.o: ../src/CONFIG.c
TRdata.o: ../src/TRdata.c
ato.o: ../src/ato.c
call.o: ../src/call.c
case.o: ../src/case.c
clas.o: ../src/clas.c
const.o: ../src/const.c
conv.o: ../src/conv.c
cset.o: ../src/cset.c
error.o: ../src/error.c
fdec.o: ../src/fdec.c
fend.o: ../src/fend.c
fhdr.o: ../src/fhdr.c
flvalue.o: ../src/flvalue.c
forop.o: ../src/forop.c
func.o: ../src/func.c
gen.o: ../src/gen.c
hash.o: ../src/hash.c
lab.o: ../src/lab.c
lookup.o: ../src/lookup.c
lval.o: ../src/lval.c
main.o: ../src/main.c
nl.o: ../src/nl.c
opc.o: ../src/opc.c
p2put.o: ../src/p2put.c
pccaseop.o: ../src/pccaseop.c
pcfunc.o: ../src/pcfunc.c
pclval.o: ../src/pclval.c
pcproc.o: ../src/pcproc.c
pic.o: ../src/pic.c
proc.o: ../src/proc.c
put.o: ../src/put.c
rec.o: ../src/rec.c
rval.o: ../src/rval.c
savenl.o: ../src/savenl.c
sconv.o: ../src/sconv.c
stab.o: ../src/stab.c
stat.o: ../src/stat.c
stklval.o: ../src/stklval.c
stkrval.o: ../src/stkrval.c
string.o: ../src/string.c
subr.o: ../src/subr.c
tmps.o: ../src/tmps.c
tree.o: ../src/tree.c
treen.o: ../src/treen.c
type.o: ../src/type.c
var.o: ../src/var.c
yycopy.o: ../src/yycopy.c
yycosts.o: ../src/yycosts.c
yyerror.o: ../src/yyerror.c
yyget.o: ../src/yyget.c
yyid.o: ../src/yyid.c
yylex.o: ../src/yylex.c
yymain.o: ../src/yymain.c
yyoptions.o: ../src/yyoptions.c
yypanic.o: ../src/yypanic.c
yyparse.o: ../src/yyparse.c
yyprint.o: ../src/yyprint.c
yyput.o: ../src/yyput.c
yyrecover.o: ../src/yyrecover.c
yyseman.o: ../src/yyseman.c
yytree.o: ../src/yytree.c
../src/yy.h: y.tab.h
TRdata.o: whoami.h
TRdata.o: ../src/0.h
ato.o: whoami.h
ato.o: ../src/0.h
call.o: whoami.h
call.o: ../src/0.h
call.o: ../src/tree.h
call.o: opcode.h
call.o: ../src/objfmt.h
call.o: ../src/pc.h
call.o: ../src/pcops.h
call.o: ../src/tmps.h
case.o: whoami.h
case.o: ../src/0.h
case.o: ../src/tree.h
case.o: opcode.h
clas.o: whoami.h
clas.o: ../src/0.h
clas.o: ../src/tree.h
const.o: whoami.h
const.o: ../src/0.h
const.o: ../src/tree.h
conv.o: whoami.h
conv.o: ../src/0.h
conv.o: opcode.h
conv.o: ../src/pcops.h
cset.o: whoami.h
cset.o: ../src/0.h
cset.o: ../src/tree.h
cset.o: opcode.h
cset.o: ../src/objfmt.h
cset.o: ../src/pc.h
cset.o: ../src/pcops.h
cset.o: ../src/align.h
error.o: whoami.h
error.o: ../src/0.h
error.o: ../src/yy.h
fdec.o: whoami.h
fdec.o: ../src/0.h
fdec.o: ../src/tree.h
fdec.o: opcode.h
fdec.o: ../src/objfmt.h
fdec.o: ../src/align.h
fdec.o: ../src/tmps.h
fdec.o: ../src/pc.h
fdec.o: ../src/pcops.h
fend.o: whoami.h
fend.o: ../src/0.h
fend.o: ../src/tree.h
fend.o: opcode.h
fend.o: ../src/objfmt.h
fend.o: ../src/align.h
fend.o: ../src/tmps.h
fend.o: ../src/pc.h
fend.o: ../src/pcops.h
fhdr.o: whoami.h
fhdr.o: ../src/0.h
fhdr.o: ../src/tree.h
fhdr.o: opcode.h
fhdr.o: ../src/objfmt.h
fhdr.o: ../src/align.h
fhdr.o: ../src/pc.h
fhdr.o: ../src/pcops.h
flvalue.o: whoami.h
flvalue.o: ../src/0.h
flvalue.o: ../src/tree.h
flvalue.o: opcode.h
flvalue.o: ../src/objfmt.h
flvalue.o: ../src/pc.h
flvalue.o: ../src/pcops.h
flvalue.o: ../src/tmps.h
forop.o: whoami.h
forop.o: ../src/0.h
forop.o: opcode.h
forop.o: ../src/tree.h
forop.o: ../src/objfmt.h
forop.o: ../src/pc.h
forop.o: ../src/pcops.h
forop.o: ../src/tmps.h
func.o: whoami.h
func.o: ../src/0.h
func.o: ../src/tree.h
func.o: opcode.h
gen.o: whoami.h
gen.o: ../src/0.h
gen.o: ../src/tree.h
gen.o: opcode.h
gen.o: ../src/objfmt.h
hash.o: whoami.h
hash.o: ../src/0.h
hash.o: ../src/yy.h
lab.o: whoami.h
lab.o: ../src/0.h
lab.o: ../src/tree.h
lab.o: opcode.h
lab.o: ../src/objfmt.h
lab.o: ../src/pc.h
lab.o: ../src/pcops.h
lookup.o: whoami.h
lookup.o: ../src/0.h
lval.o: whoami.h
lval.o: ../src/0.h
lval.o: ../src/tree.h
lval.o: opcode.h
lval.o: ../src/objfmt.h
lval.o: ../src/pc.h
lval.o: ../src/pcops.h
main.o: whoami.h
main.o: ../src/0.h
main.o: ../src/yy.h
main.o: ../src/objfmt.h
main.o: ../src/config.h
nl.o: whoami.h
nl.o: ../src/0.h
nl.o: opcode.h
nl.o: ../src/objfmt.h
opc.o: ../src/OPnames.h
p2put.o: whoami.h
p2put.o: ../src/0.h
p2put.o: ../src/objfmt.h
p2put.o: ../src/pcops.h
p2put.o: ../src/pc.h
p2put.o: ../src/align.h
p2put.o: ../src/tmps.h
pccaseop.o: whoami.h
pccaseop.o: ../src/0.h
pccaseop.o: ../src/tree.h
pccaseop.o: ../src/objfmt.h
pccaseop.o: ../src/pcops.h
pccaseop.o: ../src/pc.h
pccaseop.o: ../src/tmps.h
pcfunc.o: whoami.h
pcfunc.o: ../src/0.h
pcfunc.o: ../src/tree.h
pcfunc.o: ../src/objfmt.h
pcfunc.o: opcode.h
pcfunc.o: ../src/pc.h
pcfunc.o: ../src/pcops.h
pcfunc.o: ../src/tmps.h
pclval.o: whoami.h
pclval.o: ../src/0.h
pclval.o: ../src/tree.h
pclval.o: opcode.h
pclval.o: ../src/objfmt.h
pclval.o: ../src/pc.h
pclval.o: ../src/pcops.h
pcproc.o: whoami.h
pcproc.o: ../src/0.h
pcproc.o: ../src/tree.h
pcproc.o: ../src/objfmt.h
pcproc.o: opcode.h
pcproc.o: ../src/pc.h
pcproc.o: ../src/pcops.h
pcproc.o: ../src/tmps.h
pic.o: ../src/OPnames.h
proc.o: whoami.h
proc.o: ../src/0.h
proc.o: ../src/tree.h
proc.o: opcode.h
proc.o: ../src/objfmt.h
proc.o: ../src/tmps.h
put.o: whoami.h
put.o: opcode.h
put.o: ../src/0.h
put.o: ../src/objfmt.h
put.o: ../src/pc.h
put.o: ../src/align.h
put.o: ../src/OPnames.h
rec.o: whoami.h
rec.o: ../src/0.h
rec.o: ../src/tree.h
rec.o: opcode.h
rec.o: ../src/align.h
rval.o: whoami.h
rval.o: ../src/0.h
rval.o: ../src/tree.h
rval.o: opcode.h
rval.o: ../src/objfmt.h
rval.o: ../src/pc.h
rval.o: ../src/pcops.h
rval.o: ../src/tmps.h
savenl.o: whoami.h
savenl.o: ../src/0.h
savenl.o: ../src/objfmt.h
savenl.o: ../src/../pdx/defs.h
savenl.o: ../src/../pdx/object.h
savenl.o: ../src/../pdx/object/objsym.rep
savenl.o: ../src/../pdx/mappings.h
savenl.o: ../src/../pdx/mappings/filetab.h
sconv.o: whoami.h
sconv.o: ../src/0.h
sconv.o: ../src/pcops.h
stab.o: whoami.h
stab.o: ../src/0.h
stab.o: ../src/objfmt.h
stab.o: ../src/pstab.h
stab.o: ../src/pc.h
stat.o: whoami.h
stat.o: ../src/0.h
stat.o: ../src/tree.h
stat.o: ../src/objfmt.h
stat.o: ../src/pcops.h
stat.o: ../src/pc.h
stat.o: ../src/tmps.h
stat.o: opcode.h
stklval.o: whoami.h
stklval.o: ../src/0.h
stklval.o: ../src/tree.h
stklval.o: opcode.h
stklval.o: ../src/objfmt.h
stkrval.o: whoami.h
stkrval.o: ../src/0.h
stkrval.o: ../src/tree.h
stkrval.o: opcode.h
stkrval.o: ../src/objfmt.h
stkrval.o: ../src/pcops.h
string.o: whoami.h
string.o: ../src/0.h
string.o: ../src/send.h
subr.o: whoami.h
subr.o: ../src/0.h
tmps.o: whoami.h
tmps.o: ../src/0.h
tmps.o: ../src/objfmt.h
tmps.o: ../src/pc.h
tmps.o: ../src/tmps.h
tree.o: whoami.h
tree.o: ../src/0.h
type.o: whoami.h
type.o: ../src/0.h
type.o: ../src/tree.h
type.o: ../src/objfmt.h
var.o: whoami.h
var.o: ../src/0.h
var.o: ../src/objfmt.h
var.o: ../src/align.h
var.o: ../src/iorec.h
var.o: ../src/pc.h
var.o: ../src/pcops.h
var.o: ../src/tmps.h
yycopy.o: ../src/0.h
yycopy.o: ../src/yy.h
yycosts.o: whoami.h
yycosts.o: ../src/0.h
yycosts.o: ../src/yy.h
yyerror.o: whoami.h
yyerror.o: ../src/0.h
yyerror.o: ../src/yy.h
yyget.o: whoami.h
yyget.o: ../src/0.h
yyget.o: ../src/yy.h
yyid.o: whoami.h
yyid.o: ../src/0.h
yyid.o: ../src/yy.h
yylex.o: whoami.h
yylex.o: ../src/0.h
yylex.o: ../src/yy.h
yymain.o: whoami.h
yymain.o: ../src/0.h
yymain.o: ../src/yy.h
yymain.o: ../src/objfmt.h
yymain.o: ../src/config.h
yyoptions.o: whoami.h
yyoptions.o: ../src/0.h
yyoptions.o: ../src/yy.h
yypanic.o: whoami.h
yypanic.o: ../src/0.h
yypanic.o: ../src/yy.h
yyparse.o: whoami.h
yyparse.o: ../src/0.h
yyparse.o: ../src/yy.h
yyprint.o: whoami.h
yyprint.o: ../src/0.h
yyprint.o: ../src/yy.h
yyput.o: whoami.h
yyput.o: ../src/0.h
yyput.o: ../src/tree.h
yyput.o: ../src/yy.h
yyrecover.o: whoami.h
yyrecover.o: ../src/0.h
yyrecover.o: ../src/yy.h
yyseman.o: whoami.h
yyseman.o: ../src/0.h
yyseman.o: ../src/yy.h
yytree.o: whoami.h
yytree.o: ../src/0.h
yytree.o: ../src/tree.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
