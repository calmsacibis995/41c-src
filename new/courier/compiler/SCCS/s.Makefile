h40242
s 00028/00000/00000
d D 1.1 83/02/23 12:56:36 cooper 1 0
c date and time created 83/02/23 12:56:36 by cooper
e
u
U
t
T
I 1
# %M% %I% %G%

CFLAGS = -O
OBJS = y.tab.o main.o sem.o code1.o code2.o
SRCS = y.tab.c main.c sem.c code1.c code2.c
DESTDIR = /usr/local

courier:	$(OBJS)
		$(CC) $(CFLAGS) -o courier $(OBJS) -ll

y.tab.o:	lex.yy.c

lex.yy.c:	scanner.l
		lex scanner.l

y.tab.c:	courier.y
		yacc courier.y

$(OBJS):	Courier.h

lint:		y.tab.c
		lint -hnux $(SRCS)

install:	courier
		install -s courier $(DESTDIR)

clean:
		rm -f *.o y.tab.[ch] lex.yy.c
E 1
