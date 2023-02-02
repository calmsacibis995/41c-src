h63635
s 00001/00001/00118
d D 1.2 83/02/27 22:48:43 cooper 2 1
c read pad byte after program name if odd
e
s 00119/00000/00000
d D 1.1 83/02/23 12:50:41 cooper 1 0
c date and time created 83/02/23 12:50:41 by cooper
e
u
U
t
T
I 1
/*
 * %M% %I% %G%
 *
 * Courier program instantiation server
 *
 * Reads program name as a Courier string.
 * Writes back a null terminated error message if instantiation was
 * unsuccessful; otherwise the server process will write a single zero byte
 * when it starts up.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <wait.h>
#include <netdb.h>
#include <courier.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int f, s, pid, len;
	struct sockaddr_in sin, from;
	struct servent *srvp;
	extern int errno;

	if (argc != 1)
		fprintf(stderr, "Courier daemon: arguments ignored\n");
	srvp = getservbyname("courier", "tcp");
	if (srvp == 0) {
		fprintf(stderr, "tcp/courier: unknown service\n");
		exit(1);
	}

#ifndef DEBUG
	if (chdir("/usr/lib/courier")) {
		perror("/usr/lib/courier");
		exit(1);
	}
#endif

	sin.sin_port = srvp->s_port;
	f = socket(AF_INET, SOCK_STREAM, 0, 0);
	if (f < 0) {
		perror("Courier daemon: socket");
		exit(1);
	}
	sin.sin_family = AF_INET;
	if (bind(f, &sin, sizeof(struct sockaddr_in), 0) < 0) {
		perror("Courier daemon: bind");
		exit(1);
	}
	listen(f, 10);
	for (;;) {
		len = sizeof(from);
		s = accept(f, &from, &len, 0);
		if (s < 0) {
			if (errno != EINTR) {
				perror("Courier daemon: accept");
				sleep(1);
			}
			continue;
		}
		if ((pid = fork()) == 0) {
			close(f);
			instantiate(s);
		}
		close(s);
		if (pid == -1)
			error("Try again.\n");
		while(wait3(0, WNOHANG, 0) > 0)
			continue;
	}
}

instantiate(f)
	int f;
{
	Cardinal n, nbytes;
	struct stat statbuf;
	char name[200];

	setpgrp(0, getpid());
	sigset(SIGHUP, SIG_DFL);
	sigset(SIGINT, SIG_DFL);
	sigset(SIGQUIT, SIG_DFL);

	dup2(f, 0);
	dup2(f, 1);
	close(f);

	alarm(60);
	read(0, &n, sizeof(Cardinal));
	alarm(0);

	UnpackCardinal(&nbytes, &n);
D 2
	read(0, name, nbytes);
E 2
I 2
	read(0, name, nbytes+(nbytes%2));	/* read pad byte if odd */
E 2
	name[nbytes] = '\0';

	if (name[0] == '/' || name[0] == '.' || stat(name, &statbuf) != 0)
		goto bad;
	setgroups(0, 0);
	setregid(statbuf.st_gid, statbuf.st_gid);
	setreuid(statbuf.st_uid, statbuf.st_uid);
	execl(name, name, 0);
bad:
	error("Unknown Courier program.\n");
	exit(1);
}

error(s)
	char *s;
{
	write(1, s, strlen(s)+1);
}
E 1
