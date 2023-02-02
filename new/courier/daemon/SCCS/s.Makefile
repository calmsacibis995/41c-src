h21160
s 00012/00000/00000
d D 1.1 83/02/23 14:34:22 cooper 1 0
c date and time created 83/02/23 14:34:22 by cooper
e
u
U
t
T
I 1
# %M% %I% %G%

OBJS = courierd.o
CFLAGS =  -O
LIBS = -ljobs
DESTDIR = /etc

courierd:	$(OBJS)
	$(CC) $(CFLAGS) -o courierd $(OBJS) $(LIBS)

install:	courierd
	install -s courierd $(DESTDIR)
E 1
