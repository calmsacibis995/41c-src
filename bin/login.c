static	char *sccsid = "@(#)login.c	4.20 82/12/21";
/*
 * login [ name ]
 * login -r
 */

#include <sys/types.h>
#include <sgtty.h>
#include <utmp.h>
#include <signal.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <lastlog.h>

#define	SCPYN(a, b)	strncpy(a, b, sizeof(a))

#define NMAX	sizeof(utmp.ut_name)
#define LMAX	sizeof(utmp.ut_line)

#define	FALSE	0
#define	TRUE	-1

char	nolog[] =	"/etc/nologin";
char	qlog[]  =	".hushlogin";
char	securetty[] =	"/etc/securetty";
char	maildir[30] =	"/usr/spool/mail/";
char	lastlog[] =	"/usr/adm/lastlog";
struct	passwd nouser = {"", "nope", -1, -1, -1, "", "", "", "" };
struct	sgttyb ttyb;
struct	utmp utmp;
char	minusnam[16] = "-";

char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	term[64] = "TERM=";
char	user[20] = "USER=";
char	*speeds[] =
    { "0", "50", "75", "110", "134", "150", "200", "300",
      "600", "1200", "1800", "2400", "4800", "9600", "19200", "38400" };
#define	NSPEEDS	(sizeof (speeds) / sizeof (speeds[0]))

char	*envinit[] =
    {homedir, shell, "PATH=:/usr/ucb:/bin:/usr/bin", term, user, 0};

struct	passwd *pwd;
struct	passwd *getpwnam();
char	*strcat(), *rindex(), *index();
int	setpwent();
char	*ttyname();
char	*crypt();
char	*getpass();
char	*rindex();
char	*stypeof();
extern	char **environ;

struct	ttychars tc = {
	CERASE,	CKILL,	CINTR,	CQUIT,	CSTART,
	CSTOP,	CEOF,	CBRK,	CSUSP,	CDSUSP,
	CRPRNT,	CFLUSH,	CWERASE,CLNEXT
};

int	rflag;
char	rusername[NMAX+1], lusername[NMAX+1];
char	rpassword[NMAX+1];
char	name[NMAX+1];
char	*rhost;

main(argc, argv)
char **argv;
{
	register char *namep;
	int t, f, c;
	int invalid;
	int quietlog;
	int i;
	FILE *nlfd;
	char *ttyn;
	int ldisc = 0, zero = 0;
	FILE *hostf; int first = 1;

	alarm(60);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	nice(-100);
	nice(20);
	nice(0);
	if (argc > 1 && !strcmp(argv[1], "-r")) {
		rflag++;
		rhost = argv[2];
		argc = 1;
		getstr(rusername, sizeof (rusername), "remuser");
		getstr(lusername, sizeof (lusername), "locuser");
		getstr(term+5, sizeof(term)-5, "Terminal type");
		if (getuid())
			goto abnormal;
		setpwent();
		pwd = getpwnam(lusername);
		endpwent();
		if (pwd == NULL) {
			if (strcmp(rusername, lusername))
				printf("%s: No such user\r\n", lusername);
			goto abnormal;
		}
		hostf = pwd->pw_uid ? fopen("/etc/hosts.equiv", "r") : 0;
	again:
		if (hostf) {
		  char ahost[32];
		  while (fgets(ahost, sizeof (ahost), hostf)) {
			char *user;
			if (index(ahost, '\n'))
				*index(ahost, '\n') = 0;
			user = index(ahost, ' ');
			if (user)
				*user++ = 0;
			if (!strcmp(rhost, ahost) &&
			    !strcmp(rusername, user ? user : lusername)) {
				fclose(hostf);
				goto normal;
			}
		  }
		  fclose(hostf);
		}
		if (first == 1) {
			first = 0;
			if (chdir(pwd->pw_dir) < 0)
				goto again;
			hostf = fopen(".rhosts", "r");
			goto again;
		}
abnormal:
		rhost = 0;
		rflag = -1;
	}
normal:
	ioctl(0, TIOCLSET, &zero);	/* XXX */
	ioctl(0, TIOCNXCL, 0);
	ioctl(0, FIONBIO, &zero);
	ioctl(0, FIOASYNC, &zero);
	ioctl(0, TIOCGETP, &ttyb);	/* XXX */
	if (rflag) {
		char *cp = index(term, '/');
		if (cp) {
			int i;
			*cp++ = 0;
			for (i = 0; i < NSPEEDS; i++)
				if (!strcmp(speeds[i], cp)) {
					ttyb.sg_ispeed = ttyb.sg_ospeed = i;
					break;
				}
		}
		ttyb.sg_flags = ECHO|CRMOD|ANYP|XTABS;
	}
	ioctl(0, TIOCSETP, &ttyb);	/* XXX */
	ioctl(0, TIOCCSET, &tc);
	for (t=3; t<20; t++)
		close(t);
	ttyn = ttyname(0);
	if (ttyn==(char *)0)
		ttyn = "/dev/tty??";
	do {
		ldisc = 0;
		ioctl(0, TIOCSETD, &ldisc);
		invalid = FALSE;
		SCPYN(utmp.ut_name, "");
		if (argc>1) {
			SCPYN(utmp.ut_name, argv[1]);
			argc = 0;
		}
		if (rflag) {
			SCPYN(utmp.ut_name, lusername);
			if (rflag == -1)
				rflag = 0;
		} else
			while (utmp.ut_name[0] == '\0') {
				namep = utmp.ut_name;
				{ char hostname[32];
				  gethostname(hostname, sizeof (hostname));
				  printf("%s login: ", hostname); }
				while ((c = getchar()) != '\n') {
					if (c == ' ')
						c = '_';
					if (c == EOF)
						exit(0);
					if (namep < utmp.ut_name+NMAX)
						*namep++ = c;
				}
			}
		if (rhost == 0) {
			setpwent();
			if ((pwd = getpwnam(utmp.ut_name)) == NULL)
				pwd = &nouser;
			endpwent();
		}
		if (!strcmp(pwd->pw_shell, "/bin/csh")) {
			ldisc = NTTYDISC;
			ioctl(0, TIOCSETD, &ldisc);
		}
		if (rhost == 0) {
			if (*pwd->pw_passwd != '\0') {
				char *pp;
				nice(-4);
				if (rflag == 0)
					pp = getpass("Password:");
				else
					pp = rpassword;
				namep = crypt(pp,pwd->pw_passwd);
				nice(4);
				if (strcmp(namep, pwd->pw_passwd))
					invalid = TRUE;
			}
		}
		if (pwd->pw_uid != 0 && (nlfd = fopen(nolog, "r")) > 0) {
			/* logins are disabled except for root */
			while ((c = getc(nlfd)) != EOF)
				putchar(c);
			fflush(stdout);
			sleep(5);
			exit(0);
		}
		if (!invalid && pwd->pw_uid == 0 &&
		    !rootterm(ttyn+sizeof("/dev/")-1)) {
			logerr("ROOT LOGIN REFUSED %s",
			    ttyn+sizeof("/dev/")-1);
			invalid = TRUE;
		}
		if (invalid) {
			printf("Login incorrect\n");
			if (ttyn[sizeof("/dev/tty")-1] == 'd')
				logerr("BADDIALUP %s %s\n",
				    ttyn+sizeof("/dev/")-1, utmp.ut_name);
		}
		if (*pwd->pw_shell == '\0')
			pwd->pw_shell = "/bin/sh";
		i = strlen(pwd->pw_shell);
		if (chdir(pwd->pw_dir) < 0 && !invalid ) {
			if (chdir("/") < 0) {
				printf("No directory!\n");
				invalid = TRUE;
			} else {
				printf("No directory!  Logging in with home=/\n");
				pwd->pw_dir = "/";
			}
		}
		if (rflag && invalid)
			exit(1);
	} while (invalid);


	time(&utmp.ut_time);
	t = ttyslot();
	if (t>0 && (f = open("/etc/utmp", 1)) >= 0) {
		lseek(f, (long)(t*sizeof(utmp)), 0);
		SCPYN(utmp.ut_line, rindex(ttyn, '/')+1);
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	}
	if (t>0 && (f = open("/usr/adm/wtmp", 1)) >= 0) {
		lseek(f, 0L, 2);
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	}
	quietlog = 0;
	if (access(qlog, 0) == 0)
		quietlog = 1;
	if ((f = open(lastlog, 2)) >= 0) {
		struct lastlog ll;

		lseek(f, (long)pwd->pw_uid * sizeof (struct lastlog), 0);
		if (read(f, (char *) &ll, sizeof ll) == sizeof ll &&
		    ll.ll_time != 0) {
			if (quietlog == 0)
			printf("Last login: %.*s on %.*s\n"
			    , 24-5
			    , (char *) ctime(&ll.ll_time)
			    , sizeof(ll.ll_line)
			    , ll.ll_line
			);
		}
		lseek(f, (long)pwd->pw_uid * sizeof (struct lastlog), 0);
		time(&ll.ll_time);
		SCPYN(ll.ll_line, rindex(ttyn, '/')+1);
		write(f, (char *) &ll, sizeof ll);
		close(f);
	}
	chown(ttyn, pwd->pw_uid, pwd->pw_gid);
	chmod(ttyn, 0622);
	setgid(pwd->pw_gid);
	strncpy(name, utmp.ut_name, NMAX);
	name[NMAX] = '\0';
	initgroups(name, pwd->pw_gid);
	setuid(pwd->pw_uid);
	environ = envinit;
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	if (term[strlen("TERM=")] == 0)
		strncat(term, stypeof(ttyn), sizeof(term)-6);
	strncat(user, pwd->pw_name, sizeof(user)-6);
	if ((namep = rindex(pwd->pw_shell, '/')) == NULL)
		namep = pwd->pw_shell;
	else
		namep++;
	strcat(minusnam, namep);
	alarm(0);
	umask(022);
	if (ttyn[sizeof("/dev/tty")-1] == 'd')
		logerr("DIALUP %s %s\n", ttyn+sizeof("/dev/")-1, pwd->pw_name);
	if (!quietlog) {
		showmotd();
		strcat(maildir, pwd->pw_name);
		if (access(maildir,4)==0) {
			struct stat statb;
			stat(maildir, &statb);
			if (statb.st_size)
				printf("You have mail.\n");
		}
	}
	
	signal(SIGQUIT, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGTSTP, SIG_IGN);
	execlp(pwd->pw_shell, minusnam, 0);
	perror(pwd->pw_shell);
	printf("No shell\n");
	exit(0);
}

int	stopmotd;
catch()
{

	signal(SIGINT, SIG_IGN);
	stopmotd++;
}

rootterm(tty)
	char *tty;
{
	register FILE *fd;
	char buf[100];

	if (rflag)
		return(1);
	if ((fd = fopen(securetty, "r")) == NULL)
		return(1);
	while (fgets(buf, sizeof buf, fd) != NULL) {
		buf[strlen(buf)-1] = '\0';
		if (strcmp(tty, buf) == 0) {
			fclose(fd);
			return(1);
		}
	}
	fclose(fd);
	return(0);
}

showmotd()
{
	FILE *mf;
	register c;

	signal(SIGINT, catch);
	if ((mf = fopen("/etc/motd","r")) != NULL) {
		while ((c = getc(mf)) != EOF && stopmotd == 0)
			putchar(c);
		fclose(mf);
	}
	signal(SIGINT, SIG_IGN);
}

#undef	UNKNOWN
#define UNKNOWN "su"

char *
stypeof(ttyid)
char	*ttyid;
{
	static char	typebuf[16];
	char		buf[50];
	register FILE	*f;
	register char	*p, *t, *q;

	if (ttyid == NULL)
		return (UNKNOWN);
	f = fopen("/etc/ttytype", "r");
	if (f == NULL)
		return (UNKNOWN);
	/* split off end of name */
	for (p = q = ttyid; *p != 0; p++)
		if (*p == '/')
			q = p + 1;

	/* scan the file */
	while (fgets(buf, sizeof buf, f) != NULL)
	{
		for (t=buf; *t!=' ' && *t != '\t'; t++)
			;
		*t++ = 0;
		while (*t == ' ' || *t == '\t')
			t++;
		for (p=t; *p>' '; p++)
			;
		*p = 0;
		if (strcmp(q,t)==0) {
			strcpy(typebuf, buf);
			fclose(f);
			return (typebuf);
		}
	}
	fclose (f);
	return (UNKNOWN);
}

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		if (--cnt < 0) {
			printf("%s too long\r\n", err);
			exit(1);
		}
		*buf++ = c;
	} while (c != 0);
}

logerr(fmt, a1, a2, a3)
	char *fmt, *a1, *a2, *a3;
{

}
