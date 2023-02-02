/*	lpr.c	4.10	83/03/09	*/
/*
 *      lpr -- off line print
 *
 * Allows multiple printers and printers on remote machines by
 * using information from a printer data base.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <pwd.h>
#include <stdio.h>
#include <ctype.h>
#include "lp.local.h"

char    *tfname;		/* tmp copy of cf before linking */
char    *cfname;		/* daemon control files, linked from tf's */
char    *dfname;		/* data files */

int	nact;			/* number of jobs to act on */
int	tfd;			/* control file descriptor */
int     mailflg;		/* send mail */
int	qflag;			/* q job, but don't exec daemon */
char	format = 'f';		/* format char for printing files */
int	rflag;			/* remove files upon completion */	
int	lflag;			/* link flag */
char	*person;		/* user name */
int	inchar;			/* location to increment char in file names */
int     ncopies = 1;		/* # of copies to make */
int	iflag;			/* indentation wanted */
int	indent;			/* amount to indent */
char	*DN;			/* path name to daemon program */
char	*LP;			/* line printer device name */
char	*RM;			/* remote machine name if no local printer */
char	*SD;			/* spool directory */
int     MX;			/* maximum size in blocks of a print file */
int	hdr = 1;		/* print header or not (default is yes) */
int     userid;			/* user id */
char	*title;			/* pr'ing title */
char	*fonts[4];		/* troff font names */
char	*width;			/* width for versatec printing */
char	host[32];		/* host name */
char	*class = host;		/* class title on header page */
char    *jobname;		/* job name on header page */
char	*name;			/* program name */

char	*pgetstr();
char	*malloc();
char	*getenv();
char	*rindex();
char	*linked();

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *getlogin();
	extern struct passwd *getpwuid(), *getpwnam();
	struct passwd *pw;
	extern char *itoa();
	register char *arg, *cp;
	int i, f, out();
	char *printer = NULL;
	struct stat stb;

	/*
	 * Strategy to maintain protected spooling area:
	 *	1. Spooling area is writable only by daemon and spooling group
	 *	2. lpr runs setuid root and setgrp spooling group; it uses
	 *	   root to access any file it wants (verifying things before
	 *	   with an access call) and group id to know how it should
	 *	   set up ownership of files in spooling area.
	 *	3. Files in spooling area are owned by daemon and spooling
	 *	   group, with mode 660.
	 *	4. lpd runs setuid root and setgrp spooling group to
	 *	   access files and printer.  Users can't get to anything
	 *	   w/o help of lpq and lprm programs.
	 */
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, out);
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, out);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, out);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, out);

	gethostname(host, sizeof (host));
	name = argv[0];

	while (argc > 1 && (arg = argv[1])[0] == '-') {
		argc--;
		argv++;
		switch (arg[1]) {

		case 'P':		/* specifiy printer name */
			printer = &arg[2];
			break;

		case 'C':		/* classification spec */
			hdr++;
			if (arg[2])
				class = &arg[2];
			else if (argc > 1) {
				argc--;
				class = *++argv;
			}
			break;

		case 'J':		/* job name */
			hdr++;
			if (arg[2])
				jobname = &arg[2];
			else if (argc > 1) {
				argc--;
				jobname = *++argv;
			}
			break;

		case 'T':		/* pr's title line */
			if (arg[2])
				title = &arg[2];
			else if (argc > 1) {
				argc--;
				title = *++argv;
			}
			break;

		case 'l':		/* literal output */
		case 'p':		/* print using ``pr'' */
		case 't':		/* print troff output */
		case 'c':		/* print cifplot output */
		case 'v':		/* print vplot output */
			format = arg[1];
			break;

		case '4':		/* troff fonts */
		case '3':
		case '2':
		case '1':
			if (argc > 1) {
				argc--;
				fonts[arg[1] - '1'] = *++argv;
				format = 't';
			}
			break;

		case 'w':		/* versatec page width */
			width = arg+2;
			break;

		case 'r':		/* remove file when done */
			rflag++;
			break;

		case 'm':		/* send mail when done */
			mailflg++;
			break;

		case 'h':		/* toggle want of header page */
			hdr = !hdr;
			break;

		case 's':		/* try to link files */
			lflag++;
			break;

		case 'q':		/* just q job */
			qflag++;
			break;

		case 'i':		/* indent output */
			iflag++;
			indent = arg[2] ? atoi(&arg[2]) : 8;
			break;

		case '#':		/* n copies */
			if (isdigit(arg[2]))
				ncopies = atoi(&arg[2]);
		}
	}
	if (printer == NULL && (printer = getenv("PRINTER")) == NULL)
		printer = DEFLP;
	if (!chkprinter(printer)) {
		printf("%s: unknown printer %s\n", name, printer);
		exit(2);
	}
	/*
	 * Get the identity of the person doing the lpr and initialize the
	 * control file.
	 */
	userid = getuid();
	if ((person = getlogin()) == NULL || strlen(person) == 0) {
		if ((pw = getpwuid(userid)) == NULL)
			person = "Unknown User";
		else
			person = pw->pw_name;
	} else if ((pw = getpwnam(person)) != NULL)
		userid = pw->pw_uid;		/* in case of su */
	mktemps();
	tfd = nfile(tfname);
	card('H', host);
	card('P', person);
	if (hdr) {
		if (jobname == NULL) {
			if (argc == 1)
				jobname = "stdin";
			else
				jobname = argv[1];
		}
		card('J', jobname);
		card('C', class);
		card('L', person);
	}
	if (iflag)
		card('I', itoa(indent));
	if (mailflg)
		card('M', person);
	if (format == 't')
		for (i = 0; i < 4; i++)
			if (fonts[i] != NULL)
				card('1'+i, fonts[i]);
	if (width != NULL)
		card('W', width);

	if (argc == 1)
		copy(0, " ");
	else while (--argc) {
		if ((f = test(arg = *++argv)) < 0)
			continue;	/* file unreasonable */

		if ((f & 1) && (cp = linked(arg)) != NULL) {
			if (format == 'p')
				card('T', title ? title : arg);
			for (i = 0; i < ncopies; i++)
				card(format, &dfname[inchar-2]);
			card('U', &dfname[inchar-2]);
			if (f & 2)
				card('U', cp);
			card('N', arg);
			dfname[inchar]++;
			nact++;
		} else {
			if ((i = open(arg, 0)) < 0) {
				printf("%s: cannot open %s\n", name, arg);
				continue;
			}
			copy(i, arg);
			(void) close(i);
			if ((f & 2) && unlink(arg))
				printf("%s: cannot remove %s\n", name, arg);
		}
	}

	if (nact) {
		tfname[inchar]--;
		if (link(tfname, cfname) < 0) {
			printf("%s: cannot rename %s\n", name, cfname);
			tfname[inchar]++;
			out();
		}
		unlink(tfname);
		if (qflag)		/* just q things up */
			exit(0);
		if (*LP && stat(LP, &stb) >= 0 && (stb.st_mode & 0777) == 0) {
			printf("jobs queued, but line printer is down.\n");
			exit(0);
		}
		execl(DN, (arg = rindex(DN, '/')) ? arg+1 : DN, printer, 0);
		printf("jobs queued, but cannot start daemon.\n");
		exit(0);
	}
	out();
	/*NOTREACHED*/
}

/*
 * Create the file n and copy from file descriptor f.
 */
copy(f, n)
	int f;
	char n[];
{
	register int fd, i, nr, nc;
	char buf[BUFSIZ];

	if (format == 'p')
		card('T', title ? title : n);
	for (i = 0; i < ncopies; i++)
		card(format, &dfname[inchar-2]);
	card('U', &dfname[inchar-2]);
	card('N', n);
	fd = nfile(dfname);
	nr = nc = 0;
	while ((i = read(f, buf, BUFSIZ)) > 0) {
		if (write(fd, buf, i) != i) {
			printf("%s: %s: temp file write error\n", name, n);
			break;
		}
		nc += i;
		if (nc >= BUFSIZ) {
			nc -= BUFSIZ;
			if (nr++ > MX) {
				printf("%s: %s: copy file is too large\n", name, n);
				break;
			}
		}
	}
	(void) close(fd);
	nact++;
}

/*
 * Try and link the file to dfname. Return a pointer to the full
 * path name if successful.
 */
char *
linked(file)
	register char *file;
{
	register char *cp;
	char buf[BUFSIZ];

	if (*file != '/') {
		if (getwd(buf) == NULL)
			return(NULL);
		while (file[0] == '.') {
			switch (file[1]) {
			case '/':
				file += 2;
				continue;
			case '.':
				if (file[2] == '/') {
					if ((cp = rindex(buf, '/')) != NULL)
						*cp = '\0';
					file += 3;
					continue;
				}
			}
			break;
		}
		strcat(buf, "/");
		strcat(buf, file);
		file = buf;
	}
	return(symlink(file, dfname) ? NULL : file);
}

/*
 * Put a line into the control file.
 */
card(c, p2)
	register char c, *p2;
{
	char buf[BUFSIZ];
	register char *p1 = buf;
	register int len = 2;

	*p1++ = c;
	while ((c = *p2++) != '\0') {
		*p1++ = c;
		len++;
	}
	*p1++ = '\n';
	write(tfd, buf, len);
}

/*
 * Create a new file in the spool directory.
 */

nfile(n)
	char *n;
{
	register f;
	int oldumask = umask(0);		/* should block signals */

	f = creat(n, FILMOD);
	(void) umask(oldumask);
	if (f < 0) {
		printf("%s: cannot create %s\n", name, n);
		out();
	}
#ifdef BSD41C
	if (chown(n, userid, -1) < 0) {
#else
	if (chown(n, userid, getegid()) < 0) {
#endif
		unlink(n);
		printf("%s: cannot chown %s\n", name, n);
		out();
	}
	n[inchar]++;
	return(f);
}

/*
 * Cleanup after interrupts and errors.
 */
out()
{
	register i;

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	i = inchar;
	if (tfname)
		while (tfname[i] != 'A') {
			tfname[i]--;
			unlink(tfname);
		}
	if (cfname)
		while (cfname[i] != 'A') {
			cfname[i]--;
			unlink(cfname);
		}
	if (dfname)
		while (dfname[i] != 'A') {
			dfname[i]--;
			unlink(dfname);
		}
	exit();
}

/*
 * Test to see if this is a printable file.
 * Return -1 if it is not, 1 if we should try to link and or in 2 if
 * we should remove it after printing.
 */
test(file)
	char *file;
{
	struct exec execb;
	struct stat statb;
	register int fd;
	register char *cp;

	if (access(file, 4) < 0) {
		printf("%s: cannot access %s\n", name, file);
		return(-1);
	}
	if (stat(file, &statb) < 0) {
		printf("%s: cannot stat %s\n", name, file);
		return(-1);
	}
	if ((statb.st_mode & S_IFMT) == S_IFDIR) {
		printf("%s: %s is a directory\n", name, file);
		return(-1);
	}
	if ((fd = open(file, 0)) < 0) {
		printf("%s: cannot open %s\n", name, file);
		return(-1);
	}
	if (read(fd, &execb, sizeof(execb)) == sizeof(execb))
		switch(execb.a_magic) {
		case A_MAGIC1:
		case A_MAGIC2:
		case A_MAGIC3:
#ifdef A_MAGIC4
		case A_MAGIC4:
#endif
			printf("%s: %s is an executable program", name, file);
			goto error1;

		case ARMAG:
			printf("%s: %s is an archive file", name, file);
			goto error1;
		}
	(void) close(fd);
	fd = 0;
	if (lflag && (statb.st_mode & 04))
		fd |= 1;
	if (rflag) {
		if ((cp = rindex(file, '/')) == NULL) {
			if (access(".", 2) == 0)
				fd |= 2;
		} else {
			*cp = '\0';
			if (access(file, 2) == 0)
				fd |= 2;
			*cp = '/';
		}
	}
	return(fd);

error1:
	printf(" and is unprintable\n");
	(void) close(fd);
	return(-1);
}

/*
 * itoa - integer to string conversion
 */
char *
itoa(i)
	register int i;
{
	static char b[10] = "########";
	register char *p;

	p = &b[8];
	do
		*p-- = i%10 + '0';
	while (i /= 10);
	return(++p);
}

/*
 * Perform lookup for printer name or abbreviation --
 */
chkprinter(s)
	register char *s;
{
	static char buf[BUFSIZ/2];
	char b[BUFSIZ];
	int stat;
	char *bp = buf;

	if ((stat = pgetent(b, s)) < 0) {
		printf("%s: can't open printer description file\n", name);
		exit(3);
	} else if (stat == 0)
		return(0);
	if ((DN = pgetstr("dn", &bp)) == NULL)
		DN = DEFDAEMON;
	if ((LP = pgetstr("lp", &bp)) == NULL)
		LP = DEFDEVLP;
	if ((SD = pgetstr("sd", &bp)) == NULL)
		SD = DEFSPOOL;
	if ((MX = pgetnum("mx")) < 0)
		MX = DEFMX;
	RM = pgetstr("rm", &bp);
	return(1);
}

/*
 * Make the temp files.
 */
mktemps()
{
	register int c, len;
	int n;
	char buf[BUFSIZ], *mktemp();
	FILE *fp;

	(void) sprintf(buf, "%s/.seq", SD);
	if ((fp = fopen(buf, "r+")) == NULL) {
		if ((fp = fopen(buf, "w")) == NULL) {
			printf("%s: cannot create %s\n", name, buf);
			exit(1);
		}
		setbuf(fp, buf);
		n = 0;
	} else {
		setbuf(fp, buf);
#ifdef BSD41C
		if (flock(fileno(fp), FEXLOCK)) {
			printf("%s: cannot lock %s\n", name, buf);
			exit(1);
		}
#endif
		n = 0;
		while ((c = getc(fp)) >= '0' && c <= '9')
			n = n * 10 + (c - '0');
	}
	len = strlen(SD) + strlen(host) + 8;
	tfname = mktemp("tf", n, len);
	cfname = mktemp("cf", n, len);
	dfname = mktemp("df", n, len);
	inchar = strlen(SD) + 3;
	n = (n + 1) % 1000;
	(void) fseek(fp, 0L, 0);
	fprintf(fp, "%d\n", n);
	(void) fclose(fp);
}

/*
 * Make a temp file name.
 */
char *
mktemp(id, num, len)
	char	*id;
	int	num, len;
{
	register char *s;

	if ((s = malloc(len)) == NULL) {
		printf("%s: out of memory\n", name);
		exit(1);
	}
	(void) sprintf(s, "%s/%sA%03d%s", SD, id, num, host);
	return(s);
}
