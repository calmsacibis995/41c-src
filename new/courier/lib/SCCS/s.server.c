h50375
s 00001/00001/00151
d D 1.3 83/02/27 22:49:53 cooper 3 2
c added debugging info in ReceiveCallMessage
e
s 00004/00003/00148
d D 1.2 83/02/27 21:36:58 cooper 2 1
c fixed bug in readmsg
e
s 00151/00000/00000
d D 1.1 83/02/23 13:58:07 cooper 1 0
c date and time created 83/02/23 13:58:07 by cooper
e
u
U
t
T
I 1
/*
 * %M% %I% %G%
 */
#include <courier.h>
#include <time.h>

#if DEBUG
int CourierDebuggingFlag = 0;
#endif

/*
 * Message stream handle.
 */
static int msgs = -1;

/*
 * Information about user-specified I/O dispatch function.
 */
static void (*selectfunc)() = 0;
static int readmask, writemask, exceptmask, blockflag;
static struct timeval timeout;

Unspecified *
ReceiveCallMessage(procp)
	Cardinal *procp;
{
	Cardinal proc, n, nwords;
	Unspecified *bp;

	if (readmsg(&proc, 1) == 0)
		exit(0);				/* clean EOF */
	if (readmsg(&nwords, 1) == 0)
		goto eof;
	UnpackCardinal(procp, &proc);
	UnpackCardinal(&n, &nwords);
#if DEBUG
	if (CourierDebuggingFlag)
D 3
		fprintf(stderr, "[ReceiveCallMessage %d]\n", n);
E 3
I 3
		fprintf(stderr, "[ReceiveCallMessage %d %d]\n", *procp, n);
E 3
#endif
	bp = Allocate(n);
	if (readmsg(bp, n) == 0)
		goto eof;
	return (bp);
eof:
	exit(1);
}

SendReturnMessage(n, results)
	Cardinal n;
	Unspecified *results;
{
	Cardinal nwords;

#if DEBUG
	if (CourierDebuggingFlag)
		fprintf(stderr, "[SendReturnMessage %d]\n", n);
#endif
	PackCardinal(&n, &nwords, 1);
	writemsg(&nwords, 1);
	writemsg(results, n);
}

NoSuchProcedureValue(prog_name, proc)
	String prog_name;
	Cardinal proc;
{
	fprintf(stderr, "Courier program %s: no such procedure value %d\n",
		prog_name, proc);
}

D 2
static readmsg(addr, nwords)
E 2
I 2
static
readmsg(addr, nwords)
E 2
	char *addr;
	int nwords;
{
	register int nbytes, n;
	register char *p;
	int rbits, wbits, ebits;

D 2
	nbytes = 2 * nwords;
E 2
	for (p = addr, nbytes = 2*nwords; nbytes > 0; nbytes -= n, p += n) {
		while (selectfunc != 0) {
			rbits = (1 << msgs) | readmask;
			wbits = writemask;
			ebits = exceptmask;
			if (blockflag)
				select(32, &rbits, &wbits, &ebits, 0);
			else
				select(32, &rbits, &wbits, &ebits, &timeout);
			if (rbits & (1 << msgs))
				break;
			rbits &= ~(1 << msgs);
			(*selectfunc)(rbits, wbits, ebits);
		}
		n = read(msgs, p, nbytes);
		if (n <= 0)
			return (0);
	}
	return (1);
}

D 2
static writemsg(addr, nwords)
E 2
I 2
static
writemsg(addr, nwords)
E 2
	char *addr;
	int nwords;
{
	register int n;
	int rbits, wbits, ebits;

	for (;;) {
		rbits = readmask;
		wbits = (1 << msgs) | writemask;
		ebits = exceptmask;
		if (blockflag)
			select(32, &rbits, &wbits, &ebits, 0);
		else
			select(32, &rbits, &wbits, &ebits, &timeout);
		if (wbits & (1 << msgs))
			break;
		wbits &= ~(1 << msgs);
		(*selectfunc)(rbits, wbits, ebits);
	}
	write(msgs, addr, 2*nwords);
	return (1);
}

CourierSelect(func, in, out, except, timelimit)
	void (*func)();
	int in, out, except;
	struct timeval *timelimit;
{
	if ((selectfunc = func) != 0) {
		readmask = in;
		writemask = out;
		exceptmask = except;
		if (!(blockflag = (timelimit == 0)))
			timeout = *timelimit;
	}
}

ServerInit()
{
	write(1, "\0", 1);
	msgs = 20;
	while (dup2(0, msgs) < 0)
		--msgs;
	close(0); close(1);
}

main()
{
	Server();
}
E 1
