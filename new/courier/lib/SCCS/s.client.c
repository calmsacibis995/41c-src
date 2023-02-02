h45711
s 00002/00002/00144
d D 1.2 83/02/27 21:36:53 cooper 2 1
c fixed bug in readmsg
e
s 00146/00000/00000
d D 1.1 83/02/23 13:57:54 cooper 1 0
c date and time created 83/02/23 13:57:54 by cooper
e
u
U
t
T
I 1
/*
 * %M% %I% %G%
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <courier.h>

#if DEBUG
int CourierDebuggingFlag = 0;
#endif

/*
 * Message stream handle.
 */
static int msgs = -1;

SendCallMessage(procedure, nwords, arguments)
	Cardinal procedure, nwords;
	Unspecified *arguments;
{
	Cardinal p, n;

#if DEBUG
	if (CourierDebuggingFlag)
		fprintf(stderr, "[SendCallMessage %d %d]\n", procedure, nwords);
#endif
	PackCardinal(&procedure, &p, 1);
	PackCardinal(&nwords, &n, 1);
	write(msgs, &p, sizeof(Cardinal));
	write(msgs, &n, sizeof(Cardinal));
	write(msgs, arguments, nwords*sizeof(Unspecified));
}

Unspecified *
ReceiveReturnMessage()
{
	Cardinal nwords, n;
	Unspecified *bp;

	if (readmsg(&nwords, 1) == 0)
		goto eof;
	UnpackCardinal(&n, &nwords);
#if DEBUG
	if (CourierDebuggingFlag)
		fprintf(stderr, "[ReceiveReturnMessage %d]\n", n);
#endif
	bp = Allocate(n);
	if (readmsg(bp, n) == 0)
		goto eof;
	return (bp);
eof:
	fprintf(stderr, "\n\r\7Lost connection to server.\n");
	exit(1);
}

CourierActivate(host, program_name)
	String host, program_name;
{
	struct hostent *hp;
	struct servent *srvp;
	struct sockaddr_in sin;
	Unspecified buf[100];
	Cardinal n;
	char c;

	if (msgs >= 0)
		CourierClose();
	hp = gethostbyname(host);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", host);
		return (-1);
	}
	srvp = getservbyname("courier", "tcp");
	if (srvp == 0) {
		fprintf(stderr, "tcp/courier: unknown service\n");
		return (-1);
	}
	msgs = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (msgs < 0) {
		perror("socket");
		return (-1);
	}
	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = 0;
	if (bind(msgs, (caddr_t)&sin, sizeof (sin), 0) < 0) {
		perror("bind");
		goto bad;
	}
	sin.sin_family = hp->h_addrtype;
	sin.sin_addr = *(struct in_addr *) hp->h_addr;
	sin.sin_port = srvp->s_port;
	if (connect(msgs, (caddr_t)&sin, sizeof(sin), 0) < 0) {
		perror(hp->h_name);
		goto bad;
	}
#if DEBUG
	if (CourierDebuggingFlag)
		fprintf(stderr, "[CourierActivate: connected to %s]\n",
			hp->h_name);
#endif
	n = PackString(&program_name, buf, 1);
	write(msgs, buf, n*sizeof(Unspecified));
	if (read(msgs, &c, 1) != 1) {
		perror(host);
		goto bad;
	}
	if (c != 0) {
		do write(2, &c, 1); while (read(msgs, &c, 1) == 1 && c != 0);
		goto bad;
	}
#if DEBUG
	if (CourierDebuggingFlag)
		fprintf(stderr, "[CourierActivate: running %s]\n",
			program_name);
#endif
	return (0);
bad:
	CourierClose();
	return (-1);
}

CourierClose()
{
	close(msgs);
	msgs = -1;
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

D 2
	nbytes = 2 * nwords;
E 2
	for (p = addr, nbytes = 2*nwords; nbytes > 0; nbytes -= n, p += n) {
		n = read(msgs, p, nbytes);
		if (n <= 0)
			return (0);
	}
	return (1);
}
E 1
