static	char *sccsid = "@(#)tar.c	4.14 (Berkeley) 83/01/05";

/*
 * Tape Archival Program
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dir.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <signal.h>

#define TBLOCK	512
#define NBLOCK	20
#define NAMSIZ	100

union hblock {
	char dummy[TBLOCK];
	struct header {
		char name[NAMSIZ];
		char mode[8];
		char uid[8];
		char gid[8];
		char size[12];
		char mtime[12];
		char chksum[8];
		char linkflag;
		char linkname[NAMSIZ];
	} dbuf;
};

struct linkbuf {
	ino_t	inum;
	dev_t	devnum;
	int	count;
	char	pathname[NAMSIZ];
	struct	linkbuf *nextp;
};

union	hblock dblock;
union	hblock tbuf[NBLOCK];
struct	linkbuf *ihead;
struct	stat stbuf;

int	rflag;
int	xflag;
int	vflag;
int	tflag;
int	cflag;
int	mflag;
int	fflag;
int	oflag;
int	pflag;
int	wflag;
int	hflag;
int	Bflag;

int	mt;
int	term;
int	chksum;
int	recno;
int	first;
int	linkerrok;
int	freemem = 1;
int	nblock = NBLOCK;
int	onintr();
int	onquit();
int	onhup();
int	onterm();

daddr_t	low;
daddr_t	high;
daddr_t	bsrch();

FILE	*tfile;
char	tname[] = "/tmp/tarXXXXXX";
char	*usefile;
char	magtape[] = "/dev/rmt8";
char	*malloc();
char	*sprintf();
char	*strcat();
char	*getcwd();
char	*getwd();

main(argc, argv)
int	argc;
char	*argv[];
{
	char *cp;

	if (argc < 2)
		usage();

	tfile = NULL;
	usefile =  magtape;
	argv[argc] = 0;
	argv++;
	for (cp = *argv++; *cp; cp++) 
		switch(*cp) {

		case 'f':
			usefile = *argv++;
			fflag++;
			break;

		case 'c':
			cflag++;
			rflag++;
			break;

		case 'o':
			oflag++;
			break;

		case 'p':
			pflag++;
			break;
		
		case 'u':
			mktemp(tname);
			if ((tfile = fopen(tname, "w")) == NULL) {
				fprintf(stderr,
				 "Tar: cannot create temporary file (%s)\n",
				 tname);
				done(1);
			}
			fprintf(tfile, "!!!!!/!/!/!/!/!/!/! 000\n");
			/*FALL THRU*/

		case 'r':
			rflag++;
			break;

		case 'v':
			vflag++;
			break;

		case 'w':
			wflag++;
			break;

		case 'x':
			xflag++;
			break;

		case 't':
			tflag++;
			break;

		case 'm':
			mflag++;
			break;

		case '-':
			break;

		case '0':
		case '1':
		case '4':
		case '5':
		case '7':
		case '8':
			magtape[8] = *cp;
			usefile = magtape;
			break;

		case 'b':
			nblock = atoi(*argv++);
			if (nblock > NBLOCK || nblock <= 0) {
				fprintf(stderr, "Invalid blocksize. (Max %d)\n",
					NBLOCK);
				done(1);
			}
			break;

		case 'l':
			linkerrok++;
			break;

		case 'h':
			hflag++;
			break;

		case 'B':
			Bflag++;
			break;

		default:
			fprintf(stderr, "tar: %c: unknown option\n", *cp);
			usage();
		}

	if (!rflag && !xflag && !tflag)
		usage();
	if (rflag) {
		if (cflag && tfile != NULL)
			usage();
		if (signal(SIGINT, SIG_IGN) != SIG_IGN)
			signal(SIGINT, onintr);
		if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
			signal(SIGHUP, onhup);
		if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
			signal(SIGQUIT, onquit);
#ifdef notdef
		if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
			signal(SIGTERM, onterm);
#endif
		if (strcmp(usefile, "-") == 0) {
			if (cflag == 0) {
				fprintf(stderr,
				 "Can only create standard output archives\n");
				done(1);
			}
			mt = dup(1);
			nblock = 1;
		} else if ((mt = open(usefile, 2)) < 0) {
			if (cflag == 0 || (mt =  creat(usefile, 0666)) < 0) {
				fprintf(stderr,
					"tar: cannot open %s\n", usefile);
				done(1);
			}
		}
		dorep(argv);
		done(0);
	}
	if (strcmp(usefile, "-") == 0) {
		mt = dup(0);
		nblock = 1;
	} else if ((mt = open(usefile, 0)) < 0) {
		fprintf(stderr, "tar: cannot open %s\n", usefile);
		done(1);
	}
	if (xflag)
		doxtract(argv);
	else
		dotable();
	done(0);
}

usage()
{
	fprintf(stderr,
"tar: usage  tar -{txruB}[cvfblmh] [tapefile] [blocksize] file1 file2...\n");
	done(1);
}

dorep(argv)
	char *argv[];
{
	register char *cp, *cp2;
	char wdir[MAXPATHLEN], tempdir[MAXPATHLEN], *parent;

	if (!cflag) {
		getdir();
		do {
			passtape();
			if (term)
				done(0);
			getdir();
		} while (!endtape());
		if (tfile != NULL) {
			char buf[200];

			sprintf(buf,
"sort +0 -1 +1nr %s -o %s; awk '$1 != prev {print; prev=$1}' %s >%sX; mv %sX %s",
				tname, tname, tname, tname, tname, tname);
			fflush(tfile);
			system(buf);
			freopen(tname, "r", tfile);
			fstat(fileno(tfile), &stbuf);
			high = stbuf.st_size;
		}
	}

	(void) getcwd(wdir);
	while (*argv && ! term) {
		cp2 = *argv;
		if (!strcmp(cp2, "-C") && argv[1]) {
			argv++;
			if (chdir(*argv) < 0)
				perror(*argv);
			else
				(void) getcwd(wdir);
			argv++;
			continue;
		}
		parent = wdir;
		for (cp = *argv; *cp; cp++)
			if (*cp == '/')
				cp2 = cp;
		if (cp2 != *argv) {
			*cp2 = '\0';
			if (chdir(*argv) < 0) {
				perror(*argv);
				continue;
			}
			parent = getcwd(tempdir);
			*cp2 = '/';
			cp2++;
		}
		putfile(*argv++, cp2, parent);
		chdir(wdir);
	}
	putempty();
	putempty();
	flushtape();
	if (linkerrok == 0)
		return;
	for (; ihead != NULL; ihead = ihead->nextp) {
		if (ihead->count == 0)
			continue;
		fprintf(stderr, "Missing links to %s\n", ihead->pathname);
	}
}

endtape()
{
	if (dblock.dbuf.name[0] != '\0')
		return (0);
	backtape();
	return (1);
}

getdir()
{
	register struct stat *sp;
	int i;

	readtape((char *)&dblock);
	if (dblock.dbuf.name[0] == '\0')
		return;
	sp = &stbuf;
	sscanf(dblock.dbuf.mode, "%o", &i);
	sp->st_mode = i;
	sscanf(dblock.dbuf.uid, "%o", &i);
	sp->st_uid = i;
	sscanf(dblock.dbuf.gid, "%o", &i);
	sp->st_gid = i;
	sscanf(dblock.dbuf.size, "%lo", &sp->st_size);
	sscanf(dblock.dbuf.mtime, "%lo", &sp->st_mtime);
	sscanf(dblock.dbuf.chksum, "%o", &chksum);
	if (chksum != checksum()) {
		fprintf(stderr, "directory checksum error\n");
		done(2);
	}
	if (tfile != NULL)
		fprintf(tfile, "%s %s\n", dblock.dbuf.name, dblock.dbuf.mtime);
}

passtape()
{
	long blocks;
	char buf[TBLOCK];

	if (dblock.dbuf.linkflag == '1')
		return;
	blocks = stbuf.st_size;
	blocks += TBLOCK-1;
	blocks /= TBLOCK;

	while (blocks-- > 0)
		readtape(buf);
}

putfile(longname, shortname, parent)
	char *longname;
	char *shortname;
	char *parent;
{
	int infile;
	long blocks;
	char buf[TBLOCK];
	register char *cp, *cp2;
	struct direct *dp;
	DIR *dirp;
	int i, j;
	char newparent[NAMSIZ+64];

	infile = open(shortname, 0);
	if (infile < 0) {
		fprintf(stderr, "tar: %s: cannot open file\n", longname);
		return;
	}
	if (!hflag)
		lstat(shortname, &stbuf);
	else if (stat(shortname, &stbuf) < 0) {
		perror(longname);
		close(infile);
		return;
	}
	if (tfile != NULL && checkupdate(longname) == 0) {
		close(infile);
		return;
	}
	if (checkw('r', longname) == 0) {
		close(infile);
		return;
	}

	if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
		for (i = 0, cp = buf; *cp++ = longname[i++];)
			;
		*--cp = '/';
		*++cp = 0  ;
		if (!oflag) {
			if ((cp - buf) >= NAMSIZ) {
				fprintf(stderr, "%s: file name too long\n",
					longname);
				close(infile);
				return;
			}
			stbuf.st_size = 0;
			tomodes(&stbuf);
			strcpy(dblock.dbuf.name,buf);
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			writetape((char *)&dblock);
		}
		sprintf(newparent, "%s/%s", parent, shortname);
		chdir(shortname);
		close(infile);
		if ((dirp = opendir(".")) == NULL) {
			fprintf(stderr, "%s: directory read error\n", longname);
			chdir(parent);
			return;
		}
		while ((dp = readdir(dirp)) != NULL && !term) {
			if (dp->d_ino == 0)
				continue;
			if (!strcmp(".", dp->d_name) ||
			    !strcmp("..", dp->d_name))
				continue;
			strcpy(cp, dp->d_name);
			i = telldir(dirp);
			closedir(dirp);
			putfile(buf, cp, newparent);
			dirp = opendir(".");
			seekdir(dirp, i);
		}
		closedir(dirp);
		chdir(parent);
		return;
	}
	i = stbuf.st_mode & S_IFMT;
	if (i != S_IFREG && i != S_IFLNK) {
		fprintf(stderr, "tar: %s is not a file. Not dumped\n",
			longname);
		return;
	}
	tomodes(&stbuf);
	cp2 = longname; cp = dblock.dbuf.name; i = 0;
	while ((*cp++ = *cp2++) && i < NAMSIZ)
		i++;
	if (i >= NAMSIZ) {
		fprintf(stderr, "%s: file name too long\n", longname);
		close(infile);
		return;
	}
	if ((stbuf.st_mode & S_IFMT) == S_IFLNK) {
		if (stbuf.st_size + 1 >= NAMSIZ) {
			fprintf(stderr, "%s: symbolic link too long\n",
				longname);
			close(infile);
			return;
		}
		i = readlink(shortname, dblock.dbuf.linkname, NAMSIZ - 1);
		if (i < 0) {
			perror(longname);
			close(infile);
			return;
		}
		dblock.dbuf.linkname[i] = '\0';
		dblock.dbuf.linkflag = '2';
		if (vflag) {
			fprintf(stderr, "a %s ", longname);
			fprintf(stderr, "symbolic link to %s\n",
				dblock.dbuf.linkname);
		}
		sprintf(dblock.dbuf.size, "%11lo", 0);
		sprintf(dblock.dbuf.chksum, "%6o", checksum());
		writetape((char *)&dblock);
		close(infile);
		return;
	}
	if (stbuf.st_nlink > 1) {
		struct linkbuf *lp;
		int found = 0;

		for (lp = ihead; lp != NULL; lp = lp->nextp)
			if (lp->inum == stbuf.st_ino &&
			    lp->devnum == stbuf.st_dev) {
				found++;
				break;
			}
		if (found) {
			strcpy(dblock.dbuf.linkname, lp->pathname);
			dblock.dbuf.linkflag = '1';
			sprintf(dblock.dbuf.chksum, "%6o", checksum());
			writetape( (char *) &dblock);
			if (vflag) {
				fprintf(stderr, "a %s ", longname);
				fprintf(stderr, "link to %s\n", lp->pathname);
			}
			lp->count--;
			close(infile);
			return;
		}
		lp = (struct linkbuf *) malloc(sizeof(*lp));
		if (lp == NULL) {
			if (freemem) {
				fprintf(stderr,
				  "Out of memory. Link information lost\n");
				freemem = 0;
			}
		} else {
			lp->nextp = ihead;
			ihead = lp;
			lp->inum = stbuf.st_ino;
			lp->devnum = stbuf.st_dev;
			lp->count = stbuf.st_nlink - 1;
			strcpy(lp->pathname, longname);
		}
	}
	blocks = (stbuf.st_size + (TBLOCK-1)) / TBLOCK;
	if (vflag) {
		fprintf(stderr, "a %s ", longname);
		fprintf(stderr, "%ld blocks\n", blocks);
	}
	sprintf(dblock.dbuf.chksum, "%6o", checksum());
	writetape((char *)&dblock);

	while ((i = read(infile, buf, TBLOCK)) > 0 && blocks > 0) {
		writetape(buf);
		blocks--;
	}
	close(infile);
	if (blocks != 0 || i != 0)
		fprintf(stderr, "%s: file changed size\n", longname);
	while (--blocks >=  0)
		putempty();
}

doxtract(argv)
	char *argv[];
{
	long blocks, bytes;
	char buf[TBLOCK];
	char **cp;
	int ofile;

	for (;;) {
		getdir();
		if (endtape())
			break;
		if (*argv == 0)
			goto gotit;
		for (cp = argv; *cp; cp++)
			if (prefix(*cp, dblock.dbuf.name))
				goto gotit;
		passtape();
		continue;

gotit:
		if (checkw('x', dblock.dbuf.name) == 0) {
			passtape();
			continue;
		}
		if (checkdir(dblock.dbuf.name))
			continue;
		if (dblock.dbuf.linkflag == '2') {
			unlink(dblock.dbuf.name);
			if (symlink(dblock.dbuf.linkname, dblock.dbuf.name)<0) {
				fprintf(stderr, "%s: symbolic link failed\n",
					dblock.dbuf.name);
				continue;
			}
			if (vflag)
				fprintf(stderr, "x %s symbolic link to %s\n",
				  dblock.dbuf.name, dblock.dbuf.linkname);
#ifdef notdef
			/* ignore alien orders */
			chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);
			if (mflag == 0) {
				time_t timep[2];

				timep[0] = time(0);
				timep[1] = stbuf.st_mtime;
				utime(dblock.dbuf.name, timep);
			}
			if (pflag)
				chmod(dblock.dbuf.name, stbuf.st_mode & 07777);
#endif
			continue;
		}
		if (dblock.dbuf.linkflag == '1') {
			unlink(dblock.dbuf.name);
			if (link(dblock.dbuf.linkname, dblock.dbuf.name) < 0) {
				fprintf(stderr, "%s: cannot link\n",
					dblock.dbuf.name);
				continue;
			}
			if (vflag)
				fprintf(stderr, "%s linked to %s\n",
					dblock.dbuf.name, dblock.dbuf.linkname);
			continue;
		}
		if ((ofile = creat(dblock.dbuf.name,stbuf.st_mode&0xfff)) < 0) {
			fprintf(stderr, "tar: %s - cannot create\n",
				dblock.dbuf.name);
			passtape();
			continue;
		}
		chown(dblock.dbuf.name, stbuf.st_uid, stbuf.st_gid);
		blocks = ((bytes = stbuf.st_size) + TBLOCK-1)/TBLOCK;
		if (vflag)
			fprintf(stderr, "x %s, %ld bytes, %ld tape blocks\n",
				dblock.dbuf.name, bytes, blocks);
		for (; blocks-- > 0; bytes -= TBLOCK) {
			readtape(buf);
			if (bytes > TBLOCK) {
				if (write(ofile, buf, TBLOCK) < 0) {
					fprintf(stderr,
					"tar: %s: HELP - extract write error\n",
					 dblock.dbuf.name);
					done(2);
				}
				continue;
			}
			if (write(ofile, buf, (int) bytes) < 0) {
				fprintf(stderr,
					"tar: %s: HELP - extract write error\n",
					dblock.dbuf.name);
				done(2);
			}
		}
		close(ofile);
		if (mflag == 0) {
			time_t timep[2];

			timep[0] = time(NULL);
			timep[1] = stbuf.st_mtime;
			utime(dblock.dbuf.name, timep);
		}
		if (pflag)
			chmod(dblock.dbuf.name, stbuf.st_mode & 07777);
	}
}

dotable()
{
	for (;;) {
		getdir();
		if (endtape())
			break;
		if (vflag)
			longt(&stbuf);
		printf("%s", dblock.dbuf.name);
		if (dblock.dbuf.linkflag == '1')
			printf(" linked to %s", dblock.dbuf.linkname);
		if (dblock.dbuf.linkflag == '2')
			printf(" symbolic link to %s", dblock.dbuf.linkname);
		printf("\n");
		passtape();
	}
}

putempty()
{
	char buf[TBLOCK];
	char *cp;

	for (cp = buf; cp < &buf[TBLOCK]; )
		*cp++ = '\0';
	writetape(buf);
}

longt(st)
	register struct stat *st;
{
	register char *cp;
	char *ctime();

	pmode(st);
	printf("%3d/%1d", st->st_uid, st->st_gid);
	printf("%7D", st->st_size);
	cp = ctime(&st->st_mtime);
	printf(" %-12.12s %-4.4s ", cp+4, cp+20);
}

#define	SUID	04000
#define	SGID	02000
#define	ROWN	0400
#define	WOWN	0200
#define	XOWN	0100
#define	RGRP	040
#define	WGRP	020
#define	XGRP	010
#define	ROTH	04
#define	WOTH	02
#define	XOTH	01
#define	STXT	01000
int	m1[] = { 1, ROWN, 'r', '-' };
int	m2[] = { 1, WOWN, 'w', '-' };
int	m3[] = { 2, SUID, 's', XOWN, 'x', '-' };
int	m4[] = { 1, RGRP, 'r', '-' };
int	m5[] = { 1, WGRP, 'w', '-' };
int	m6[] = { 2, SGID, 's', XGRP, 'x', '-' };
int	m7[] = { 1, ROTH, 'r', '-' };
int	m8[] = { 1, WOTH, 'w', '-' };
int	m9[] = { 2, STXT, 't', XOTH, 'x', '-' };

int	*m[] = { m1, m2, m3, m4, m5, m6, m7, m8, m9};

pmode(st)
	register struct stat *st;
{
	register int **mp;

	for (mp = &m[0]; mp < &m[9];)
		select(*mp++, st);
}

select(pairp, st)
	int *pairp;
	struct stat *st;
{
	register int n, *ap;

	ap = pairp;
	n = *ap++;
	while (--n>=0 && (st->st_mode&*ap++)==0)
		ap++;
	printf("%c", *ap);
}

checkdir(name)
	register char *name;
{
	register char *cp;

	for (cp = name; *cp; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		if (access(name, 1) < 0) {
			if (mkdir(name, 0777) < 0) {
				perror(name);
				done(0);
			}
			chown(name, stbuf.st_uid, stbuf.st_gid);
			if (pflag)
				chmod(name, stbuf.st_mode & 0777);
		}
		*cp = '/';
	}
	return (cp[-1]=='/');
}

onintr()
{
	signal(SIGINT, SIG_IGN);
	term++;
}

onquit()
{
	signal(SIGQUIT, SIG_IGN);
	term++;
}

onhup()
{
	signal(SIGHUP, SIG_IGN);
	term++;
}

onterm()
{
	signal(SIGTERM, SIG_IGN);
	term++;
}

tomodes(sp)
register struct stat *sp;
{
	register char *cp;

	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		*cp = '\0';
	sprintf(dblock.dbuf.mode, "%6o ", sp->st_mode & 07777);
	sprintf(dblock.dbuf.uid, "%6o ", sp->st_uid);
	sprintf(dblock.dbuf.gid, "%6o ", sp->st_gid);
	sprintf(dblock.dbuf.size, "%11lo ", sp->st_size);
	sprintf(dblock.dbuf.mtime, "%11lo ", sp->st_mtime);
}

checksum()
{
	register i;
	register char *cp;

	for (cp = dblock.dbuf.chksum;
	     cp < &dblock.dbuf.chksum[sizeof(dblock.dbuf.chksum)]; cp++)
		*cp = ' ';
	i = 0;
	for (cp = dblock.dummy; cp < &dblock.dummy[TBLOCK]; cp++)
		i += *cp;
	return (i);
}

checkw(c, name)
	char *name;
{
	if (!wflag)
		return (1);
	printf("%c ", c);
	if (vflag)
		longt(&stbuf);
	printf("%s: ", name);
	return (response() == 'y');
}

response()
{
	char c;

	c = getchar();
	if (c != '\n')
		while (getchar() != '\n')
			;
	else
		c = 'n';
	return (c);
}

checkupdate(arg)
	char *arg;
{
	char name[100];
	long mtime;
	daddr_t seekp;
	daddr_t	lookup();

	rewind(tfile);
	for (;;) {
		if ((seekp = lookup(arg)) < 0)
			return (1);
		fseek(tfile, seekp, 0);
		fscanf(tfile, "%s %lo", name, &mtime);
		return (stbuf.st_mtime > mtime);
	}
}

done(n)
{
	unlink(tname);
	exit(n);
}

prefix(s1, s2)
	register char *s1, *s2;
{
	while (*s1)
		if (*s1++ != *s2++)
			return (0);
	if (*s2)
		return (*s2 == '/');
	return (1);
}

#define	N	200
int	njab;

daddr_t
lookup(s)
	char *s;
{
	register i;
	daddr_t a;

	for(i=0; s[i]; i++)
		if (s[i] == ' ')
			break;
	a = bsrch(s, i, low, high);
	return (a);
}

daddr_t
bsrch(s, n, l, h)
	daddr_t l, h;
	char *s;
{
	register i, j;
	char b[N];
	daddr_t m, m1;

	njab = 0;

loop:
	if (l >= h)
		return (-1L);
	m = l + (h-l)/2 - N/2;
	if (m < l)
		m = l;
	fseek(tfile, m, 0);
	fread(b, 1, N, tfile);
	njab++;
	for(i=0; i<N; i++) {
		if (b[i] == '\n')
			break;
		m++;
	}
	if (m >= h)
		return (-1L);
	m1 = m;
	j = i;
	for(i++; i<N; i++) {
		m1++;
		if (b[i] == '\n')
			break;
	}
	i = cmp(b+j, s, n);
	if (i < 0) {
		h = m;
		goto loop;
	}
	if (i > 0) {
		l = m1;
		goto loop;
	}
	return (m);
}

cmp(b, s, n)
	char *b, *s;
{
	register i;

	if (b[0] != '\n')
		exit(2);
	for(i=0; i<n; i++) {
		if (b[i+1] > s[i])
			return (-1);
		if (b[i+1] < s[i])
			return (1);
	}
	return (b[i+1] == ' '? 0 : -1);
}

readtape(buffer)
	char *buffer;
{
	register int i;

	if (recno >= nblock || first == 0) {
		if ((i = bread(mt, tbuf, TBLOCK*nblock)) < 0) {
			fprintf(stderr, "Tar: tape read error\n");
			done(3);
		}
		if (first == 0) {
			if ((i % TBLOCK) != 0) {
				fprintf(stderr, "Tar: tape blocksize error\n");
				done(3);
			}
			i /= TBLOCK;
			if (i != nblock) {
				fprintf(stderr, "Tar: blocksize = %d\n", i);
				nblock = i;
			}
		}
		recno = 0;
	}
	first = 1;
	copy(buffer, &tbuf[recno++]);
	return (TBLOCK);
}

writetape(buffer)
	char *buffer;
{
	first = 1;
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "Tar: tape write error\n");
			done(2);
		}
		recno = 0;
	}
	copy(&tbuf[recno++], buffer);
	if (recno >= nblock) {
		if (write(mt, tbuf, TBLOCK*nblock) < 0) {
			fprintf(stderr, "Tar: tape write error\n");
			done(2);
		}
		recno = 0;
	}
	return (TBLOCK);
}

backtape()
{
	static int mtdev = 1;
	static struct mtop mtop = {MTBSR, 1};
	struct mtget mtget;

	if (mtdev == 1)
		mtdev = ioctl(mt, MTIOCGET, &mtget);
	if (mtdev == 0) {
		if (ioctl(mt, MTIOCTOP, &mtop) < 0) {
			fprintf(stderr, "Tar: tape backspace error\n");
			done(4);
		}
	} else
		lseek(mt, (long) -TBLOCK*nblock, 1);
	recno--;
}

flushtape()
{
	write(mt, tbuf, TBLOCK*nblock);
}

copy(to, from)
	register char *to, *from;
{
	register i;

	i = TBLOCK;
	do {
		*to++ = *from++;
	} while (--i);
}

bread(fd, buf, size)
	int fd;
	char *buf;
	int size;
{
	int count;
	static int lastread = 0;

	if (!Bflag)
		return (read(fd, buf, size));
	for (count = 0; count < size; count += lastread) {
		if (lastread < 0) {
			if (count > 0)
				return (count);
			return (lastread);
		}
		lastread = read(fd, buf, size - count);
		buf += lastread;
	}
	return (count);
}

char *
getcwd(buf)
	char *buf;
{

	if (getwd(buf) == NULL) {
		fprintf(stderr, "tar: %s\n", buf);
		exit(1);
	}
	return (buf);
}
