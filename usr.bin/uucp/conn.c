static char sccsid[] = "@(#)conn.c	4.1	(Berkeley)	9/11/82";

	/*  conn 2.1  5/23/79  19:07:44  */

/**********************************************************************
* Modified 9/81 by jrl for autodiallers built into a modem.  The table
* "autotbl" below can be expanded to deal with other autodiallers.
* 
* Modifed 2/82 by ctc to add the Port Selector Code.  This allows
* the new \b and \c type constructs.  Also modified to make use of the
* new UCB tty TIOSBRK and TIOCBRK iotcl's.  And last of all increased
* MR from 1000 to 2000 as per Bill Shannon's instructions.
*
* Modified 6/82 by Dave Martindale, U of Waterloo to support the Vadic
* AD3451 autodialing modem and the extended getargs (control chars in
* strings, embedded blanks in strings).  Expect() now takes an argument
* specifying how long the timeout should be - dialing takes a long time.
* sendthem() no longer scribbles on the string it is passed, and appends
* CR rather than NL to end of string by default.
* notin() now calls wprefix, so the expect string can contain '?' to
* match any character. ifdate() handles times which cross midnight.
* 2400 baud is supported.  Finally, a few bugs fixed:  login() closes
* the line's file descriptor on login failure; expect() checks for
* overflow of rdvec before stuffin the null in; a few strings fixed up.
**********************************************************************/

#define CONN
#include "uucp.h"
#include <signal.h>
#include <sgtty.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>

static char SiD[] = "@(#)conn	2.1";

#ifdef DATAKIT
#include <dk.h>
#endif


#define F_NAME 0
#define F_TIME 1
#define F_LINE 2
#define F_SPEED 3
#define F_PHONE 4
#define F_LOGIN 5

jmp_buf Sjbuf;
int alarmtr();
#define INVOKE(a, r) ret = a; if (ret<0) return(r);
/*******
 *	conn(system)
 *	char *system;
 *
 *	conn - place a telephone call to system and
 *	login, etc.
 *
 *	return codes:
 *		CF_SYSTEM: don't know system
 *		CF_TIME: wrong time to call
 *		CF_DIAL: call failed
 *		CF_LOGIN: login/password dialog failed
 *
 *		>0  - file no.  -  connect ok
 *
 */

conn(system)
char *system;
{
	int ret, nf;
	int fn;
	char *flds[50];
	DEBUG(4, "gdial %s\n", "called");
	INVOKE(gdial(), CF_DIAL)
	DEBUG(4, "finds %s\n", "called");
	INVOKE(nf = finds(system, flds), nf)
	DEBUG(4, "getto %s\n", "called");
	INVOKE(fn = getto(flds), CF_DIAL)
	DEBUG(4, "login %s\n", "called");
	INVOKE(login(nf, flds, fn), CF_LOGIN)
	return(fn);
}

/***
 *	char *
 *	lastc(s)	return pointer to last character
 *	char *s;
 *
 */

char *
lastc(s)
char *s;
{
	while (*s != '\0') s++;
	return(s);
}

#define MAXDEV 10
#define MAXDCH MAXDEV*20
#define MAXCODE 30
#define MAXCCH MAXCODE*20
	/* This array tells us about possible acu's, etc. */
struct Devices {
	char *D_type;
	char *D_line;
	char *D_calldev;
	int   D_speed;
	} Devs [MAXDEV];

char Devbuff[MAXDCH];

struct Codes {
	char *C_locs;
	char *C_prefix;
	} Dialcodes [MAXCODE];

char Codebuff[MAXCCH];


/***
 *	gdial()		get device and dial info
 *
 *	return codes:  0  |  FAIL
 */

gdial()
{
	char *flds[10], *lt;
	char *lb = Devbuff;
	char *lc = Codebuff;
	FILE *fn;
	int nr;
	struct Devices *pd;
	struct Codes *pc;

	fn = fopen(Devfile, "r");
	ASSERT(fn != NULL, "CAN'T OPEN %s", Devfile);
	for (pd = Devs; fgets(lb, 200, fn); pd++) {
		lt = lastc(lb);
		nr = getargs(lb, flds);
		ASSERT(nr == 4, "BAD LINE %s", lb);
		pd->D_type = flds[0];
		pd->D_line = flds[1];
		pd->D_calldev = flds[2];
		pd->D_speed = atoi(flds[3]);
		lb = lt;
		ASSERT(lb < Devbuff + MAXDCH, "TOO LONG %s", Devbuff);
		ASSERT(pd < Devs + MAXDEV, "TOO MANY DEVICES %d", MAXCODE);
	}
	pd->D_line = NULL;
	fclose(fn);
	ASSERT(pd > Devs, "BAD FILE %s", Devfile);
	/* Now dialcodes, same way */
	fn = fopen(Dialfile, "r");
	ASSERT(fn != NULL, "CAN'T OPEN %s", Dialfile);
	for (pc = Dialcodes; fgets(lc, 200, fn); pc++) {
		lt = lastc(lc);
		nr = getargs(lc, flds);
		if (nr == 1) flds[nr++] = "";
		ASSERT(nr == 2, "BAD LINE %s", lc);
		pc->C_locs = flds[0];
		pc->C_prefix = flds[1];
		lc = lt;
		ASSERT(lc < Codebuff + MAXCCH, "TOO LONG %s", Codebuff);
		ASSERT(pc < Dialcodes + MAXCODE, "MANY DEVICES %d", MAXCODE);
	}
	pc->C_locs = 0;
	fclose(fn);
	return(0);
}


/***
 *	ckdev(type, speed, ndev)
 *	char *type, *speed;
 *	int ndev;
 *
 *	ckdev  -  return the device number in table Devs for
 *	a device with proper attributes.
 *
 *	return codes: >= 0 (ok)  |  FAIL
 */

ckdev(type, speed, ndev)
char *type, *speed;
int ndev;
{
	int sp;
	struct Devices *pd;

	sp = atoi(speed);
	for (pd = &Devs[ndev]; pd->D_line != NULL; pd++) {
		if (sp != pd->D_speed)
			continue;
		if ((prefix(type, pd->D_type))  /* allow prefix here */
		    && !mlock(pd->D_line))
			return(ndev = pd - Devs);
		if ((strcmp(pd->D_line, type) == SAME)
		    && !mlock(type))
			return(ndev = pd - Devs);
	}
	return(FAIL);
}


/***
 *	getto(flds)		connect to remote machine
 *	char *flds[];
 *
 *	return codes:
 *		>0  -  file number - ok
 *		FAIL  -  failed
 */

getto(flds)
char *flds[];
{
	DEBUG(4, "call: no. %s ", flds[F_PHONE]);
	DEBUG(4, "for sys %s ", flds[F_NAME]);

	if (prefix("ACU", flds[F_LINE]))
		return(call(flds));

#ifdef DATAKIT
	else if (prefix("DK", flds[F_LINE]))
		return(dkcall(flds));
#endif

	else
		return(direct(flds));
}

/***
 *	call(flds)		call remote machine
 *	char *flds[];
 *
 *	"flds" contains the call information (name, date, type, speed,
 *	phone no. ...
 *	Ndev has the device no.
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		FAIL  -  failed
 */

call(flds)
char *flds[];
{
	char *pno, pref[20], phone[20];
	char *s1, *s2;
	int dcr, i;
	struct Codes *pc;

	pno = flds[F_PHONE];
	s1 = pref; s2 = pno;
	while (isalpha(*s2))
		*s1++ = *s2++;
	*s1 = '\0';
	for (pc = Dialcodes; pc->C_locs; pc++)
		if (strcmp(pc->C_locs, pref) == SAME) {
			s1 = pc->C_prefix;
			break;
		}
	sprintf(phone, "%s%s", s1, s2);
	for (i = 0; i < TRYCALLS; i++) {
		DEBUG(4, "Dial %s\n", phone);
		dcr = dialup(phone, flds);
		DEBUG(4, "dcr returned as %d\n", dcr);
		if (dcr != FAIL)
			break;
	}
	return(dcr);

}

	/*  file descriptor for call unit  */
int Dnf = 0;

/***
 *	dialup(ph, flds)	dial remote machine
 *	char *ph;
 *	char *flds[];
 *
 *	return codes:
 *		file descriptor  -  succeeded
 *		FAIL  -  failed
 */

int vad3451P(), ven212(), decdial();

struct autotbl {
	char *atb_name;
	int (*atb_fn)();
} autotbl[] = {
	"ACUVADIC3451P",	vad3451P,
	"ACUVENTEL212+",	ven212,
	"ACUDEC",		decdial,
	0,
};

dialup(ph, flds)
char *ph;
char *flds[];
{
	char dcname[20], dnname[20], phone[20];
	register struct Devices *pd;
	int nw, lt, pid, dcf, ndev, timelim;
	extern int Error;

	for (ndev = 0;;ndev++) {
		register struct autotbl *ap;

		ndev = ckdev(flds[F_LINE], flds[F_SPEED], ndev);
		if (ndev < 0) {
			logent("AVAILABLE DEVICE", "NO");
			DEBUG(4, "NO AVAILABLE DEVICE %s\n", "");
			return(FAIL);
		}
		pd = &Devs[ndev];
		sprintf(dcname, "/dev/%s", pd->D_line);

		/* disable logins on the line */
		disable(pd->D_line);

		/* check for autodialler */
		for(ap = autotbl; ap->atb_name; ap++)
			if(!strcmp(ap->atb_name, pd->D_type)) {
				DEBUG(4, "AUTODIAL %s\n", ap->atb_name);
				dcf = (*ap->atb_fn)(dcname, ph, pd->D_speed,
				  pd->D_line);
				if(dcf < 0) { /* dismal failure */
					logent("AUTODIAL FAILED", pd->D_line);
					DEBUG(4, "DIAL %s FAILED\n", pd->D_line);
					break;
				}
				fixline(dcf, pd->D_speed);
				return(dcf);
			}
		/* make sure it's a regular ACU */
		if(strcmp("ACU", pd->D_type) != SAME) {
			delock(pd->D_line);
			reenable();
			continue;
		}
		/* use regular dialler */
		sprintf(dnname, "/dev/%s", pd->D_calldev);
		/*  open call unit  */
		Dnf = open(dnname, 1);
		if (Dnf >= 0)
			break;
		delock(pd->D_line);
		reenable();
	}
	sprintf(phone, "%s%s", ph, ACULAST);
	DEBUG(4, "dc - %s, ", dcname);
	DEBUG(4, "acu - %s\n", dnname);
	if (setjmp(Sjbuf)) {
		DEBUG(1, "DN write %s\n", "timeout");
		logent("DIALUP DN write", "TIMEOUT");
		kill(pid, 9);
		delock(pd->D_line);
		reenable();
		close(Dnf);
		return(FAIL);
	}
	signal(SIGALRM, alarmtr);
	timelim = 5 * strlen(phone);
	alarm(timelim < 30 ? 30 : timelim);
	if ((pid = fork()) == 0) {
		sleep(2);
		fclose(stdin);
		fclose(stdout);
		nw = write(Dnf, phone, lt = strlen(phone));
		if (nw != lt) {
			DEBUG(1, "ACU write %s\n", "error");
			logent("DIALUP ACU write", "FAILED");
			exit(1);
		}
		DEBUG(4, "ACU write ok%s\n", "");
		exit(0);
	}
	/*  open line - will return on carrier */
	dcf = open(dcname, 2);
	DEBUG(4, "dcf is %d\n", dcf);
	if (dcf < 0) {
		DEBUG(1, "Line open %s\n", "failed");
		logent("DIALUP LINE open", "FAILED");
		alarm(0);
		kill(pid, 9);
		close(Dnf);
		return(FAIL);
	}
	ioctl(dcf, TIOCHPCL, 0);
	while ((nw = wait(&lt)) != pid && nw != -1)
		;
	alarm(0);
	fflush(stdout);
	fixline(dcf, pd->D_speed);
	DEBUG(4, "Forked %d ", pid);
	DEBUG(4, "Wait got %d ", nw);
	DEBUG(4, "Status %o\n", lt);
	if (lt != 0) {
		close(dcf);
		close(Dnf);
		reenable();
		return(FAIL);
	}
	return(dcf);
}


/***
 *	clsacu()	close call unit
 *
 *	return codes:  none
 */

clsacu()
{
	if (Dnf > 0) {
		close(Dnf);
		sleep(5);
		Dnf = 0;
	}
	return;
}


/***
 *	direct(flds)	connect to hardware line
 *	char *flds[];
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		FAIL  -  failed
 */

direct(flds)
char *flds[];
{
	int dcr, ndev;
	char dcname[20];

	ndev = 0;
	if ((ndev = ckdev(flds[F_LINE], flds[F_SPEED], ndev)) < 0) {
		logent("DEVICE", "NOT AVAILABLE");
		return(FAIL);
	}
	sprintf(dcname, "/dev/%s", Devs[ndev].D_line);
	disable(Devs[ndev].D_line);
	signal(SIGALRM, alarmtr);
	alarm(10);
	if (setjmp(Sjbuf)) {
		reenable();
		return(FAIL);
	}
	dcr = open(dcname, 2); /* read/write */
	alarm(0);
	if (dcr < 0) {
		reenable();
		return(FAIL);
	}
	fflush(stdout);
	fixline(dcr, Devs[ndev].D_speed);
	return(dcr);
}

#ifdef DATAKIT

#define DKTRIES 2

/***
 *	dkcall(flds)	make datakit connection
 *
 *	return codes:
 *		>0 - file number - ok
 *		FAIL - failed
 */

dkcall(flds)
char *flds[];
{
	int dkphone;
	register char *cp;
	register ret, i;

	if (setjmp(Sjbuf))
		return(FAIL);
	signal(SIGALRM, alarmtr);
	dkphone = 0;
	cp = flds[F_PHONE];
	while(*cp)
		dkphone = 10 * dkphone + (*cp++ - '0');
	DEBUG(4, "dkphone (%d) ", dkphone);
	for (i = 0; i < DKTRIES; i++) {
		ret = dkdial(D_UU, dkphone, 0);
		DEBUG(4, "dkdial (%d)\n", ret);
		if (ret > -1)
			break;
	}
	return(ret);
}
#endif

#define MAXC 300

/***
 *	finds(sysnam, flds)	set system attribute vector
 *	char *sysnam, *flds[];
 *
 *	return codes:
 *		>0  -  number of arguments in vector - succeeded
 *		CF_SYSTEM  -  system name not found
 *		CF_TIME  -  wrong time to call
 */

finds(sysnam, flds)
char *sysnam, *flds[];
{
	FILE *fsys;
	static char info[MAXC];
	char **fnp;
	int na;
	int fnd = 0;

	for (fnp = Sysfiles; *fnp != NULL && !fnd; fnp++) {
		fsys = fopen(*fnp, "r");
		if (fsys == NULL)
			continue;
		while (!fnd && (fgets(info, MAXC, fsys) != NULL)) {
			na = getargs(info, flds);
			if (prefix(sysnam, flds[F_NAME]))
				fnd = 1;
		}
		fclose(fsys);
	}
	if (fnd == 0)
		return(CF_SYSTEM);
	/* format of fields
	 *	0 name;
	 *	1 time
	 *	2 acu/hardwired/slave
	 *	3 speed
	 *	etc
	 */
	if (ifdate(flds[F_TIME]) == 0) {
		DEBUG(1, "Wrong time to call %s\n", sysnam);
		logent(sysnam, "WRONG TIME TO CALL");
		return(CF_TIME);
	}
	if (strcmp(flds[F_LINE], "Slave") == 0) {
		DEBUG(1, "Slave only, no call to %s tried\n", sysnam);
		logent(sysnam, "NO CALL FROM SLAVE");
		return(CF_TIME);
	}
	return(na);
}


/***
 *	login(nf, flds, dcr)		do log conversation
 *	char *flds[];
 *	int nf;
 *
 *	return codes:  0  |  FAIL
 */

login(nf, flds, fn)
char *flds[];
int nf, fn;
{
	char *want, *altern;
	extern char *index();
	int k, ok;

	ASSERT(nf > 4, "TOO FEW LOG FIELDS %d", nf);
	for (k = F_LOGIN; k < nf; k += 2) {
		want = flds[k];
		ok = FAIL;
		while (ok != 0) {
			altern = index(want, '-');
			if (altern != NULL)
				*altern++ = '\0';
			DEBUG(4, "wanted %s ", want);
			ok = expect(want, fn, MAXCHARTIME);
			DEBUG(4, "got %s\n", ok ? "?" : "that");
			if (ok == 0)
				break;
			if (altern == NULL) {
				logent("LOGIN", "FAILED");
				close(fn);
				return(FAIL);
			}
			want = index(altern, '-');
			if (want != NULL)
				*want++ = '\0';
			sendthem(altern, fn);
		}
		sleep(2);
		sendthem(flds[k+1], fn);
	}
	return(0);
}


struct sg_spds {int sp_val, sp_name;} spds[] = {
	{ 300,  B300},
	{1200, B1200},
	{2400, B2400},
	{4800, B4800},
	{9600, B9600},
	{0, 0} };

/***
 *	fixline(tty, spwant)	set speed/echo/mode...
 *	int tty, spwant;
 *
 *	return codes:  none
 */

fixline(tty, spwant)
int tty, spwant;
{
	struct sgttyb ttbuf;
	struct sg_spds *ps;
	int speed = -1;
	int ret;

	for (ps = spds; ps->sp_val; ps++)
		if (ps->sp_val == spwant)
			speed = ps->sp_name;
	ASSERT(speed >= 0, "BAD SPEED %d", speed);
	ioctl(tty, TIOCGETP, &ttbuf);
	ttbuf.sg_flags =(ANYP|RAW);
	ttbuf.sg_ispeed = ttbuf.sg_ospeed = speed;
	DEBUG(4, "Speed: want %d ", spwant);
	DEBUG(4, "use %o ", speed);
	DEBUG(4, "ps %d\n", ps-spds);
	ret = ioctl(tty, TIOCSETP, &ttbuf);
	ASSERT(ret >= 0, "RETURN FROM STTY %d", ret);
	ioctl(tty, TIOCHPCL, 0);
	ioctl(tty, TIOCEXCL, 0);
	return;
}


/*#define MR 1000 1000 chars should be enough */
/* 2000 is recommend by Bill Shannon */
#define MR 2000

int Error = 0;

/***
 *	expect(str, fn, timeout) look for expected string
 *	char *str;
 *
 *	return codes:
 *		0  -  found
 *		FAIL  -  lost line or too many characters read
 *		some character  -  timed out
 */

expect(str, fn, timeout)
char *str;
int fn;
{
	static char rdvec[MR];
	char *rp = rdvec;
	char nextch = 0;        /* was int /jrl */
	int kr;

	if (*str == '\0')
		return(0);
	*rp = 0;
	if (setjmp(Sjbuf)) {
		return(FAIL);
	}
	signal(SIGALRM, alarmtr);
	while (notin(str, rdvec)) {
		alarm(timeout);
		kr = read(fn, &nextch, 1);
		if (kr <= 0) {
			DEBUG(4, "kr - %d\n", kr);
			alarm(0);
			DEBUG(4, "lost line kr - %d, ", kr);
			DEBUG(4, "fn - %d\n", fn);
			logent("LOGIN", "LOST LINE");
			return(FAIL);
		}
		{
		int c;
		c = nextch & 0177;
		DEBUG(4, "%c", c > 040 ? c : '_');
		}
		if ((*rp = nextch & 0177) != '\0')
			rp++;
		if (rp >= rdvec + MR) {
			alarm(0);
			return(FAIL);
		}
		*rp = '\0';
	}
	alarm(0);
	return(0);
}


/***
 *	alarmtr()  -  catch alarm routine for "expect".
 */

alarmtr()
{
	longjmp(Sjbuf, 1);
}

/***
 *	sendthem(str, fn)	send line of login sequence
 *	char *str;
 *
 *	return codes:  none
 */

sendthem(str, fn)
char *str;
int fn;
{
	int nw, nulls, crw = 1;
	char *strptr;
	register char *outp;

	DEBUG(5, "send %s\n", str);
	if (prefix("BREAK", str)) {
		sscanf(&str[5], "%1d", &nulls);
		if (nulls <= 0 || nulls > 10)
			nulls = 3;
		/* send break */
		genbrk(fn, nulls);
		return;
	}

	if (strcmp(str, "EOT") == SAME) {
		write(fn, EOTMSG, (unsigned) strlen(EOTMSG));
		return;
	}

	for (strptr = str; *strptr; strptr++) {
		if (*(outp = strptr) == '\\')
			switch(*++strptr) {
			case '\\':
				break;
			case 'd':
				DEBUG(5, "DELAY\n", "");
				sleep(1);
				continue;
			case 'r':
				DEBUG(5, "RETURN\n", "");
				outp = "\r";
				break;
			case 'n':
				DEBUG(5, "NEWLINE\n", "");
				outp = "\n";
				break;
			case 'b':
				if (isdigit(*(strptr+1))) {
					nulls = (*++strptr - '0');
					if (nulls <= 0 || nulls > 10)
						nulls = 3;
				} else
					nulls = 3;
				/* send break */
#ifdef	UCBNEWTTY
				DEBUG(5, "BREAK - Hardware Break\n", nulls);
#else
				DEBUG(5, "BREAK (%d nulls)\n", nulls);
#endif
				genbrk(fn, nulls);
				continue;
			case 'c':
				if (*(strptr+1) == '\0') {
				DEBUG(5, "NO CR\n", "");
					crw = 0;
					continue;
				}
				DEBUG(5, "NO CR - MIDDLE (IGNORED)\n", "");
				break;
			default:
				DEBUG(5, "UNKNOWN ESCAPE\n", "");
				strptr--;
		}
		nw = write(fn, outp, 1);
		if (nw != 1) {
			DEBUG(4, "WRITE ERROR nw == %d wanted (1)\n", nw);
		}
		ASSERT(nw == 1, "BAD WRITE %s\n", str);
	}
	if (crw)
		write(fn, "\r", 1);
	return;
}


#define BSPEED B150

/***
 *	genbrk		send a break
 *
 *	return codes;  none
 */

genbrk(fn, bnulls)
{
	struct sgttyb ttbuf;
	int ret, sospeed;

#ifdef	UCBNEWTTY
#define	BRK_WAIT	1	/* Wait one full second before clearing the break line */

	ret = ioctl(fn, TIOCSBRK, 0);
	DEBUG(5, "ioctl ret %d\n", ret);
	sleep(BRK_WAIT);
	ret = ioctl(fn, TIOCCBRK, 0);
	DEBUG(5, "ioctl ret %d\n", ret);
	DEBUG(4, "sent BREAK Using Hardware - %d second wait\n", BRK_WAIT);
#else
	ret = ioctl(fn, TIOCGETP, &ttbuf);
	DEBUG(5, "ioctl ret %d\n", ret);
	sospeed = ttbuf.sg_ospeed;
	ttbuf.sg_ospeed = BSPEED;
	ret = ioctl(fn, TIOCSETP, &ttbuf);
	DEBUG(5, "ioctl ret %d\n", ret);
	ret = write(fn, "\0\0\0\0\0\0\0\0\0\0\0\0", bnulls);
	ASSERT(ret > 0, "BAD WRITE genbrk %d", ret);
	ttbuf.sg_ospeed = sospeed;
	ret = ioctl(fn, TIOCSETP, &ttbuf);
	ret = write(fn, "@", 1);
	ASSERT(ret > 0, "BAD WRITE genbrk %d", ret);
	DEBUG(4, "sent BREAK nulls - %d\n", bnulls);
#endif
	return;
}


/***
 *	notin(sh, lg)	check for occurrence of substring "sh"
 *	char *sh, *lg;
 *
 *	return codes:
 *		0  -  found the string
 *		1  -  not in the string
 */

notin(sh, lg)
char *sh, *lg;
{
	while (*lg != '\0') {
		if (wprefix(sh, lg))
			return(0);
		else
			lg++;
	}
	return(1);
}


/*******
 *	ifdate(s)
 *	char *s;
 *
 *	ifdate  -  this routine will check a string (s)
 *	like "MoTu0800-1730" to see if the present
 *	time is within the given limits.
 *	A time which crosses midnight like 2300-0800
 *	is interpreted as after 2300 or before 0800 on any
 *	suitable day, which may not be what you expected.
 *	(Mo2300-0800 is equivalent to Mo2300-2400,Mo0000-0800
 *	and not to Mo2300-2400,Tu0000-0800)
 *
 *      Now can be a list: Wk0000-0745,Wk1700-2400,Sa,Su
 *
 *	String alternatives:
 *		Wk - Mo thru Fr
 *		zero or one time means all day
 *		Any - any day
 *
 *	return codes:
 *		0  -  not within limits
 *		1  -  within limits
 */

ifdate(xs)
char *xs;
{
	static char *days[]={
		"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa", 0
	};
	char sbuf[80];
	long clock;
	int i, tl, th, tn, dayok=0;
	struct tm *localtime();
	register struct tm *tp;
	register char *s;
	char *tptr;


	time(&clock);
	tp = localtime(&clock);
	tn = tp->tm_hour * 100 + tp->tm_min;
	strcpy(sbuf, xs);

	for(s = sbuf; tptr; s = tptr) {
		dayok = 0;
		if(tptr = index(s, ','))
			*tptr++ = 0;

		while (isalpha(*s)) {
			for (i = 0; days[i]; i++) {
				if (prefix(days[i], s))
					if (tp->tm_wday == i)
						dayok = 1;
			}

			if (prefix("Wk", s))
				if (tp->tm_wday >= 1 && tp->tm_wday <= 5)
					dayok = 1;
			if (prefix("Any", s))
				dayok = 1;
			s++;
		}

		if (dayok == 0)
			continue;
		i = sscanf(s, "%d-%d", &tl, &th);
		if (i < 2)
			return(1);
		if (tl <= th) {
			if (tl <= tn && tn < th)
				return(1);
		} else if (tl <= tn || tn < th)
			return(1);
	}
	return(0);
}
