h24212
s 00016/00000/00000
d D 1.1 83/02/23 13:58:13 cooper 1 0
c date and time created 83/02/23 13:58:13 by cooper
e
u
U
t
T
I 1
# %M% %I% %G%

OBJS = client.o server.o misc.o
CFLAGS = -DDEBUG
DESTDIR = /usr/lib

libcr.a:	$(OBJS)
	-rm -f libcr.a
	ar r libcr.a $(OBJS)
	ranlib libcr.a

install:	libcr.a
	install -m 644 libcr.a $(DESTDIR)

clean:
	-rm -f *.o
E 1
