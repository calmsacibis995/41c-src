/* Copyright (c) 1982 Regents of the University of California */

#ifndef lint
char version[] = "@(#)restor.c 2.17 1/8/83";
#endif

/*	Modified to include h option (recursively extract all files within
 *	a subtree) and m option (recreate the heirarchical structure of
 *	that subtree and move extracted files to their proper homes).
 *	8/29/80		by Mike Litzkow
 *
 *	Modified to work on the new file system
 *	1/19/82		by Kirk McKusick
 *
 *	Includes the s (skip files) option for use with multiple dumps on
 *	a single tape.
 */

#define MAXINO	3000
#define BITS	8
#define NCACHE	3
#define SIZEINC 10

#include <stdio.h>
#include <signal.h>
#include <fstab.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>
#include <dir.h>
#include <stat.h>
#include <dumprestor.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>

#define ODIRSIZ 14
struct odirect {
	u_short	d_ino;
	char	d_name[ODIRSIZ];
};

#define	MWORD(m,i) (m[(unsigned)(i-1)/NBBY])
#define	MBIT(i)	(1<<((unsigned)(i-1)%NBBY))
#define	BIS(i,w)	(MWORD(w,i) |=  MBIT(i))
#define	BIC(i,w)	(MWORD(w,i) &= ~MBIT(i))
#define	BIT(i,w)	(MWORD(w,i) & MBIT(i))

ino_t	ino;

int	eflag = 0, hflag = 1, mflag = 1, tflag = 0, vflag = 0, yflag = 0;
int	cvtflag = 0;
#define vprintf		if (vflag) fprintf

long	fssize;
char	tapename[] = "/dev/rmt8";
char	*magtape = tapename;
int	mt;
int	dumpnum = 1;
int	volno = 1;
int	curblk = 0;
int	bct = NTREC+1;
char	tbf[NTREC*TP_BSIZE];

daddr_t	seekpt;
FILE	*df;
DIR	*dirp;
int	ofile;
char	dirfile[] = "/tmp/rstaXXXXXX";
char	lnkbuf[MAXPATHLEN + 1];
int	pathlen;

#define INOHASH(val) (val % MAXINO)
struct inotab {
	struct inotab *t_next;
	ino_t	t_ino;
	daddr_t	t_seekpt;
	time_t t_timep[2];
} *inotab[MAXINO];
int maxino = 0;

#define XISDIR	1
#define XTRACTD	2
#define XINUSE	4
#define XLINKED	8
struct xtrlist {
	struct xtrlist	*x_next;
	struct xtrlist	*x_linkedto;
	ino_t		x_ino;
	char		x_flags;
	char 		x_name[1];
	/* actually longer */
} *xtrlist[MAXINO];
int xtrcnt = 0;
struct xtrlist *entry;
struct xtrlist *unknown;
struct xtrlist *allocxtr();

char	*dumpmap;
char	*clrimap;

char	clearedbuf[MAXBSIZE];

extern char *ctime();
extern int seek();
ino_t search();
int dirwrite();
#ifdef RRESTOR
char *host;
#endif

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *cp;
	char command;
	int (*signal())();
	int done();

	if (signal(SIGINT, done) == SIG_IGN)
		signal(SIGINT, SIG_IGN);
	if (signal(SIGTERM, done) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	mktemp(dirfile);
	if (argc < 2) {
usage:
		fprintf(stderr, "Usage: restor xfhmsvy file file... or restor tf\n");
		done(1);
	}
	argv++;
	argc -= 2;
	command = '\0';
	for (cp = *argv++; *cp; cp++) {
		switch (*cp) {
		case '-':
			break;
		case 'c':
			cvtflag++;
			break;
		case 'f':
			magtape = *argv++;
#ifdef RRESTOR
		{ char *index();
		  host = magtape;
		  magtape = index(host, ':');
		  if (magtape == 0) {
nohost:
			msg("need keyletter ``f'' and device ``host:tape''");
			done(1);
		  }
		  *magtape++ = 0;
		  if (rmthost(host) == 0)
			done(1);
		}
#endif
			argc--;
			break;
		/* s dumpnum (skip to) for multifile dump tapes */
		case 's':
			dumpnum = atoi(*argv++);
			if (dumpnum <= 0) {
				fprintf(stderr, "Dump number must be a positive integer\n");
				done(1);
			}
			argc--;
			break;
		case 'h':
			hflag = 0;
			break;
		case 'm':
			mflag = 0;
			break;
		case 'x':
			if (command != '\0') {
				fprintf(stderr, "x and t are exclusive\n");
				goto usage;
			}
			command = 'x';
			break;
		case 'v':
			vflag++;
			break;
		case 'y':
			yflag++;
			break;
		case 't':
			if (command != '\0') {
				fprintf(stderr, "x and t are exclusive\n");
				goto usage;
			}
			command = 't';
			tflag++;		
			vflag++;
			break;
		default:
			fprintf(stderr, "Bad key character %c\n", *cp);
			goto usage;
		}
	}
	if (command == '\0') {
		fprintf(stderr, "must specify x or t\n");
		goto usage;
	}
#ifdef RRESTOR
	if (host == 0)
		goto nohost;
#endif
	setuid(getuid());	/* no longer need or want root privileges */
	doit(command, argc, argv);
	done(0);
}

doit(command, argc, argv)
	char	command;
	int	argc;
	char	*argv[]; 
{
	struct mtop tcom;
	char	*ststore();
	register struct inotab *itp;
	register struct xtrlist *xp;
	struct xtrlist **xpp;
	ino_t	d;
	int	xtrfile(), xtrskip(), xtrcvtdir(), xtrcvtskip(),
		xtrlnkfile(), xtrlnkskip(), null();
	int	mode, uid, gid, i;
	struct	stat stbuf;
	time_t	timep[2];
	char	name[BUFSIZ + 1];

	if (argc == 0) {
		argc++;
		*--argv = ".";
	}
#ifdef RRESTOR
	if ((mt = rmtopen(magtape, 0)) < 0) {
#else
	if ((mt = open(magtape, 0)) < 0) {
#endif
		fprintf(stderr, "%s: cannot open tape\n", magtape);
		done(1);
	}
	if (dumpnum != 1) {
		tcom.mt_op = MTFSF;
		tcom.mt_count = dumpnum -1;
#ifdef RRESTOR
		rmtioctl(MTFSF,dumpnum - 1);
#else
		if (ioctl(mt,MTIOCTOP,&tcom) < 0)
			perror("ioctl MTFSF");
#endif
	}
	bzero(clearedbuf, (long)MAXBSIZE);
	if (readhdr(&spcl) == 0) {
		bct--; /* push back this block */
		cvtflag++;
		if (readhdr(&spcl) == 0) {
			fprintf(stderr, "Tape is not a dump tape\n");
			done(1);
		}
		fprintf(stderr, "Converting to new file system format.\n");
	}
	if (vflag) {
		fprintf(stdout, "Dump   date: %s", ctime(&spcl.c_date));
		fprintf(stdout, "Dumped from: %s", ctime(&spcl.c_ddate));
	}
	df = fopen(dirfile, "w");
	if (df == 0) {
		fprintf(stderr,
		    "restor: %s - cannot create directory temporary\n",
		    dirfile);
		done(1);
	}
	if (stat(".", &stbuf) < 0) {
		fprintf(stderr, "cannot stat .\n");
		done(1);
	}
	fssize = stbuf.st_blksize;
	if (fssize <= 0 || ((fssize - 1) & fssize) != 0) {
		fprintf(stderr, "bad block size %d\n", fssize);
		done(1);
	}
	if (checkvol(&spcl, 1) == 0) {
		fprintf(stderr, "Tape is not volume 1 of the dump\n");
	}
	clrimap = 0;
	dumpmap = 0;
	unknown = allocxtr(0, "name unknown - not extracted", 0);
	pass1();  /* This sets the various maps on the way by */
	while (argc--) {
		if ((d = psearch(*argv)) == 0 || BIT(d,dumpmap) == 0) {
			fprintf(stderr, "%s: not on tape\n", *argv++);
			continue;
		}
		if (mflag && !tflag)
			checkdir(*argv);
		if (hflag)
			getleaves(d, *argv++);
		else
			(void)allocxtr(d, *argv++, XINUSE);
	}
	if (dumpnum > 1) {
		/*
		 * if this is a multi-dump tape we always start with 
		 * volume 1, so as to avoid accidentally restoring
		 * from a different dump!
		 */
		resetmt();
		dumpnum = 1;
		volno = 1;
		readhdr(&spcl);
		goto rbits;
	}
newvol:
#ifdef RRESTOR
	rmtclose();
#else
	close(mt);
#endif
	if (tflag)
		return;
getvol:
	fprintf(stderr, "Mount desired tape volume; Specify volume #: ");
	if (gets(tbf) == NULL)
		return;
	volno = atoi(tbf);
	if (volno <= 0) {
		fprintf(stderr, "Volume numbers are positive numerics\n");
		goto getvol;
	}
#ifdef RRESTOR
	if ((mt = rmtopen(magtape, 0)) == -1) {
#else
	if ((mt = open(magtape, 0)) == -1) {
#endif
		fprintf(stderr, "Cannot open tape!\n");
		goto getvol;
	}
	if (dumpnum > 1)
		resetmt();
	if (readhdr(&spcl) == 0) {
		fprintf(stderr, "tape is not dump tape\n");
		goto newvol;
	}
	if (checkvol(&spcl, volno) == 0) {
		fprintf(stderr, "Wrong volume (%d)\n", spcl.c_volume);
		goto newvol;
	}
rbits:
	while (gethead(&spcl) == 0)
		;
	if (checktype(&spcl, TS_INODE) == 1) {
		fprintf(stderr, "Can't find inode mask!\n");
		goto newvol;
	}
	if (checktype(&spcl, TS_BITS) == 0)
		goto rbits;
	readbits(&dumpmap);
	while (xtrcnt > 0) {
again:
		if (ishead(&spcl) == 0) {
			i = 0;
			while (gethead(&spcl) == 0)
				i++;
			fprintf(stderr, "resync restor, skipped %d blocks\n",
			    i);
		}
		if (checktype(&spcl, TS_END) == 1) {
			fprintf(stderr, "end of tape\n");
			break;
		}
		if (checktype(&spcl, TS_INODE) == 0) {
			gethead(&spcl);
			goto again;
		}
		d = spcl.c_inumber;
		entry = unknown;
		entry->x_ino = d;
		for (xp = xtrlist[INOHASH(d)]; xp; xp = xp->x_next) {
			if (d != xp->x_ino || (xp->x_flags & XLINKED))
				continue;
			entry = xp;
			timep[0] = spcl.c_dinode.di_atime;
			timep[1] = spcl.c_dinode.di_mtime;
			mode = spcl.c_dinode.di_mode;
			if (mflag)
				strcpy(name, xp->x_name);
			else
				sprintf(name, "%u", xp->x_ino);
			switch (mode & IFMT) {
			default:
				fprintf(stderr, "%s: unknown file mode 0%o\n",
				    name, mode);
				xp->x_flags |= XTRACTD;
				xtrcnt--;
				goto skipfile;
			case IFCHR:
			case IFBLK:
				vprintf(stdout, "extract special file %s\n", name);
				if (mknod(name, mode, spcl.c_dinode.di_rdev)) {
					fprintf(stderr, "%s: cannot create special file\n", name);
					xp->x_flags |= XTRACTD;
					xtrcnt--;
					goto skipfile;
				}
				getfile(null, null, spcl.c_dinode.di_size);
				break;
			case IFDIR:
				if (mflag) {
					for (itp = inotab[INOHASH(d)];
					     itp; itp = itp->t_next) {
						if (itp->t_ino != d)
							continue;
						itp->t_timep[0] = timep[0];
						itp->t_timep[1] = timep[1];
						break;
					}
					if (itp == 0) 
						fprintf(stderr, "missing directory entry\n");
					vprintf(stdout, "extract directory %s\n", name);
					strncat(name, "/.", BUFSIZ);
					checkdir(name);
					chown(xp->x_name, spcl.c_dinode.di_uid, spcl.c_dinode.di_gid);
					getfile(null, null, spcl.c_dinode.di_size);
					break;
				}
				vprintf(stdout, "extract file %s\n", name);
				if ((ofile = creat(name, 0666)) < 0) {
					fprintf(stderr, "%s: cannot create file\n", name);
					xp->x_flags |= XTRACTD;
					xtrcnt--;
					goto skipfile;
				}
				chown(name, spcl.c_dinode.di_uid, spcl.c_dinode.di_gid);
				if (cvtflag) {
					getfile(xtrcvtdir, xtrcvtskip,
					    spcl.c_dinode.di_size);
					flushent(xtrfile);
				} else
					getfile(xtrfile, xtrskip,
					    spcl.c_dinode.di_size);
				close(ofile);
				break;
			case IFLNK:
				/*
				 * Some early dump tapes have symbolic links
				 * present without the associated data blocks.
				 * This hack avoids trashing a file system with
				 * inodes with missing data blocks.
				 */
				if (spcl.c_count == 0) {
					vprintf(stdout, "%s: 0 length symbolic link (ignored)\n", name);
					xp->x_flags |= XTRACTD;
					xtrcnt--;
					goto skipfile;
				}
				vprintf(stdout, "extract symbolic link %s\n", name);
				uid = spcl.c_dinode.di_uid;
				gid = spcl.c_dinode.di_gid;
				lnkbuf[0] = '\0';
				pathlen = 0;
				getfile(xtrlnkfile, xtrlnkskip, spcl.c_dinode.di_size);
				if (symlink(lnkbuf, name) < 0) {
					fprintf(stderr, "%s: cannot create symbolic link\n", name);
					xp->x_flags |= XTRACTD;
					xtrcnt--;
					goto finished;
				}
				chown(name, uid, gid);
				break;
			case IFREG:
				vprintf(stdout, "extract file %s\n", name);
				if ((ofile = creat(name, 0666)) < 0) {
					fprintf(stderr, "%s: cannot create file\n", name);
					xp->x_flags |= XTRACTD;
					xtrcnt--;
					goto skipfile;
				}
				chown(name, spcl.c_dinode.di_uid, spcl.c_dinode.di_gid);
				getfile(xtrfile, xtrskip, spcl.c_dinode.di_size);
				close(ofile);
				break;
			}
			chmod(name, mode);
			utime(name, timep);
			xp->x_flags |= XTRACTD;
			xtrcnt--;
			goto finished;
		}
skipfile:
		getfile(null, null, spcl.c_dinode.di_size);
finished:
		;
	}
	if (xtrcnt == 0 && !mflag)
		return;
	for (xpp = xtrlist; xpp < &xtrlist[MAXINO]; xpp++) {
		for (xp = *xpp; xp; xp = xp->x_next) {
			if (mflag && (xp->x_flags & XISDIR)) {
				for (itp = inotab[INOHASH(xp->x_ino)];
				     itp; itp = itp->t_next) {
					if (itp->t_ino != xp->x_ino)
						continue;
					utime(xp->x_name, itp->t_timep);
					break;
				}
				if (itp == 0)
					fprintf(stderr, "missing dir entry\n");
			}
			if (xp->x_flags & XTRACTD)
				continue;
			if ((xp->x_flags & XLINKED) == 0) {
				fprintf(stderr, "cannot find file %s\n",
					xp->x_name);
				continue;
			}
			if (!mflag)
				continue;
			vprintf(stdout, "link %s to %s\n",
					xp->x_linkedto->x_name, xp->x_name);
			if (link(xp->x_linkedto->x_name, xp->x_name) < 0)
				fprintf(stderr, "link %s to %s failed\n",
					xp->x_linkedto->x_name, xp->x_name);
		}
	}
}

/*
 * Read the tape, bulding up a directory structure for extraction
 * by name
 */
pass1()
{
	register int i;
	register struct dinode *ip;
	struct direct nulldir;
	char buf[TP_BSIZE];
	int putdir(), null(), dirwrite();

	nulldir.d_ino = 1;
	nulldir.d_namlen = 1;
	strncpy(nulldir.d_name, "/", nulldir.d_namlen);
	nulldir.d_reclen = DIRSIZ(&nulldir);
	while (gethead(&spcl) == 0) {
		fprintf(stderr, "Can't find directory header!\n");
	}
	for (;;) {
		if (checktype(&spcl, TS_BITS) == 1) {
			readbits(&dumpmap);
			continue;
		}
		if (checktype(&spcl, TS_CLRI) == 1) {
			readbits(&clrimap);
			continue;
		}
		if (checktype(&spcl, TS_INODE) == 0) {
finish:
			fclose(df);
			dirp = opendir(dirfile);
			if (dirp == NULL)
				perror("opendir");
			resetmt();
			return;
		}
		ip = &spcl.c_dinode;
		i = ip->di_mode & IFMT;
		if (i != IFDIR) {
			goto finish;
		}
		allocinotab(spcl.c_inumber, seekpt);
		getfile(putdir, null, spcl.c_dinode.di_size);
		putent(&nulldir, dirwrite);
		flushent(dirwrite);
	}
}

/*
 * Put the directory entries in the directory file
 */
putdir(buf, size)
	char *buf;
	int size;
{
	struct direct cvtbuf;
	register struct odirect *odp;
	struct odirect *eodp;
	register struct direct *dp;
	long loc, i;

	if (cvtflag) {
		eodp = (struct odirect *)&buf[size];
		for (odp = (struct odirect *)buf; odp < eodp; odp++)
			if (odp->d_ino != 0) {
				dcvt(odp, &cvtbuf);
				putent(&cvtbuf, dirwrite);
			}
	} else {
		for (loc = 0; loc < size; ) {
			dp = (struct direct *)(buf + loc);
			i = DIRBLKSIZ - (loc & (DIRBLKSIZ - 1));
			if (dp->d_reclen <= 0 || dp->d_reclen > i) {
				loc += i;
				continue;
			}
			loc += dp->d_reclen;
			if (dp->d_ino != 0) {
				putent(dp, dirwrite);
			}
		}
	}
}

/*
 *	Recursively find names and inumbers of all files in subtree 
 *	pname and put them in xtrlist[]
 */
getleaves(ino, pname)
	ino_t ino;
	char *pname;
{
	register struct inotab *itp;
	int namelen;
	daddr_t bpt;
	register struct direct *dp;
	char locname[BUFSIZ + 1];

	if (BIT(ino, dumpmap) == 0) {
		vprintf(stdout, "%s: not on the tape\n", pname);
		return;
	}
	for (itp = inotab[INOHASH(ino)]; itp; itp = itp->t_next) {
		if (itp->t_ino != ino)
			continue;
		/*
		 * pname is a directory name 
		 */
		(void)allocxtr(ino, pname, XISDIR);
		/*
		 * begin search through the directory
		 * skipping over "." and ".."
		 */
		strncpy(locname, pname, BUFSIZ);
		strncat(locname, "/", BUFSIZ);
		namelen = strlen(locname);
		seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
		dp = readdir(dirp);
		dp = readdir(dirp);
		dp = readdir(dirp);
		bpt = telldir(dirp);
		/*
		 * "/" signals end of directory
		 */
		while (dp != NULL && 
		    !(dp->d_namlen == 1 && dp->d_name[0] == '/')) {
			locname[namelen] = '\0';
			if (namelen + dp->d_namlen >= BUFSIZ) {
				fprintf(stderr, "%s%s: name exceeds %d char\n",
					locname, dp->d_name, BUFSIZ);
			} else {
				strncat(locname, dp->d_name, dp->d_namlen);
				getleaves(dp->d_ino, locname);
				seekdir(dirp, bpt, itp->t_seekpt);
			}
			dp = readdir(dirp);
			bpt = telldir(dirp);
		}
		return;
	}
	/*
	 * locname is name of a simple file 
	 */
	(void)allocxtr(ino, pname, XINUSE);
}

/*
 * Search the directory tree rooted at inode ROOTINO
 * for the path pointed at by n
 */
psearch(n)
	char	*n;
{
	register char *cp, *cp1;
	char c;

	ino = ROOTINO;
	if (*(cp = n) == '/')
		cp++;
next:
	cp1 = cp + 1;
	while (*cp1 != '/' && *cp1)
		cp1++;
	c = *cp1;
	*cp1 = 0;
	ino = search(ino, cp);
	if (ino == 0) {
		*cp1 = c;
		return(0);
	}
	*cp1 = c;
	if (c == '/') {
		cp = cp1+1;
		goto next;
	}
	return(ino);
}

/*
 * search the directory inode ino
 * looking for entry cp
 */
ino_t
search(inum, cp)
	ino_t	inum;
	char	*cp;
{
	register struct direct *dp;
	register struct inotab *itp;
	int len;

	for (itp = inotab[INOHASH(inum)]; itp; itp = itp->t_next)
		if (itp->t_ino == inum)
			goto found;
	return(0);
found:
	seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	len = strlen(cp);
	do {
		dp = readdir(dirp);
		if (dp->d_namlen == 1 && dp->d_name[0] == '/')
			return(0);
	} while (dp->d_namlen != len || strncmp(dp->d_name, cp, len));
	return(dp->d_ino);
}

/*
 * Do the file extraction, calling the supplied functions
 * with the blocks
 */
getfile(f1, f2, size)
	int	(*f2)(), (*f1)();
	off_t	size;
{
	register int i;
	char buf[MAXBSIZE / TP_BSIZE][TP_BSIZE];
	union u_spcl addrblk;
#	define addrblock addrblk.s_spcl

	addrblock = spcl;
	for (;;) {
		for (i = 0; i < addrblock.c_count; i++) {
			if (addrblock.c_addr[i]) {
				readtape(&buf[curblk++][0]);
				if (curblk == fssize / TP_BSIZE) {
					(*f1)(buf, size > TP_BSIZE ?
					     (long) (fssize) :
					     (curblk - 1) * TP_BSIZE + size);
					curblk = 0;
				}
			} else {
				if (curblk > 0) {
					(*f1)(buf, size > TP_BSIZE ?
					     (long) (curblk * TP_BSIZE) :
					     (curblk - 1) * TP_BSIZE + size);
					curblk = 0;
				}
				(*f2)(clearedbuf, size > TP_BSIZE ?
					(long) TP_BSIZE : size);
			}
			if ((size -= TP_BSIZE) <= 0) {
				gethead(&spcl);
				goto out;
			}
		}
		if (gethead(&addrblock) == 0) {
			fprintf(stderr, "Missing address (header) block for %s\n",
				entry->x_name);
			spcl.c_magic = 0;
			goto out;
		}
		if (checktype(&addrblock, TS_ADDR) == 0) {
			spcl = addrblock;
			goto out;
		}
	}
out:
	if (curblk > 0) {
		(*f1)(buf, (curblk * TP_BSIZE) + size);
		curblk = 0;
	}
}

/*
 * The next routines are called during file extraction to
 * put the data into the right form and place.
 */
xtrfile(buf, size)
	char	*buf;
	long	size;
{

	if (write(ofile, buf, (int) size) == -1) {
		perror("extract write");
		done(1);
	}
}

xtrskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf;
#endif
	if (lseek(ofile, size, 1) == -1) {
		perror("extract seek");
		done(1);
	}
}

xtrcvtdir(buf, size)
	struct odirect *buf;
	long size;
{
	struct odirect *odp, *edp;
	struct direct cvtbuf;

	edp = &buf[size / sizeof(struct odirect)];
	for (odp = buf; odp < edp; odp++) {
		dcvt(odp, &cvtbuf);
		putent(&cvtbuf, xtrfile);
	}
}

xtrcvtskip(buf, size)
	char *buf;
	long size;
{

	fprintf(stderr, "unallocated block in directory %s\n",
		entry->x_name);
	xtrskip(buf, size);
}

xtrlnkfile(buf, size)
	char	*buf;
	long	size;
{

	pathlen += size;
	if (pathlen > MAXPATHLEN) {
		fprintf(stderr, "symbolic link name: %s; too long %d\n",
		    buf, size);
		done(1);
	}
	strcat(lnkbuf, buf);
}

xtrlnkskip(buf, size)
	char *buf;
	long size;
{

#ifdef lint
	buf = buf, size = size;
#endif
	fprintf(stderr, "unallocated block in symbolic link %s\n",
		entry->x_name);
	done(1);
}

null() {;}

/*
 * Do the tape i/o, dealing with volume changes
 * etc..
 */
readtape(b)
	char *b;
{
	register long i;
	struct u_spcl tmpbuf;
	char c;

	if (bct >= NTREC) {
		for (i = 0; i < NTREC; i++)
			((struct s_spcl *)&tbf[i*TP_BSIZE])->c_magic = 0;
		bct = 0;
#ifdef RRESTOR
		if ((i = rmtread(tbf, NTREC*TP_BSIZE)) < 0) {
#else
		if ((i = read(mt, tbf, NTREC*TP_BSIZE)) < 0) {
#endif
			fprintf(stderr, "Tape read error while restoring %s\n",
				entry->x_name);
			if (!yflag) {
				fprintf(stderr, "continue? ");
				do	{
					fprintf(stderr, "[yn] ");
					c = getchar();
					while (getchar() != '\n')
						/* void */;
				} while (c != 'y' && c != 'n');
				if (c == 'n')
					done(1);
			}
			eflag++;
			i = NTREC*TP_BSIZE;
			bzero(tbf, i);
#ifdef RRESTOR
			if (rmtseek(i, 1) < 0) {
#else
			if (lseek(mt, i, 1) < 0) {
#endif
				fprintf(stderr, "continuation failed\n");
				done(1);
			}
		}
		if (i == 0) {
			bct = NTREC + 1;
			volno++;
loop:
			flsht();
#ifdef RRESTOR
			rmtclose();
#else
			close(mt);
#endif
			fprintf(stderr, "Mount volume %d\n", volno);
			while (getchar() != '\n')
				;
#ifdef RRESTOR
			if ((mt = rmtopen(magtape, 0)) == -1) {
#else
			if ((mt = open(magtape, 0)) == -1) {
#endif
				fprintf(stderr, "Cannot open tape!\n");
				goto loop;
			}
			if (readhdr(&tmpbuf) == 0) {
				fprintf(stderr, "Not a dump tape.Try again\n");
				goto loop;
			}
			if (checkvol(&tmpbuf, volno) == 0) {
				fprintf(stderr, "Wrong tape. Try again\n");
				goto loop;
			}
			readtape(b);
			return;
		}
	}
	bcopy(&tbf[(bct++*TP_BSIZE)], b, (long)TP_BSIZE);
}

flsht()
{

	bct = NTREC+1;
}

resetmt()
{
	struct mtop tcom;

	if (dumpnum > 1)
		tcom.mt_op = MTBSF;
	else
		tcom.mt_op = MTREW;
	tcom.mt_count = 1;
	flsht();
#ifdef RRESTOR
	if (rmtioctl(tcom.mt_op, 1) == -1) {
		/* kludge for disk dumps */
		rmtseek((long)0, 0);
	}
#else
	if (ioctl(mt,MTIOCTOP,&tcom) == -1) {
		/* kludge for disk dumps */
		lseek(mt, (long)0, 0);
	}
#endif
	if (dumpnum > 1) {
#ifdef RRESTOR
		rmtioctl(MTFSF, 1);
#else
		tcom.mt_op = MTFSF;
		tcom.mt_count = 1;
		ioctl(mt,MTIOCTOP,&tcom);
#endif
	}
}

checkvol(b, t)
	struct s_spcl *b;
	int t;
{

	if (b->c_volume == t)
		return(1);
	return(0);
}

readhdr(b)
	struct s_spcl *b;
{

	if (gethead(b) == 0)
		return(0);
	if (checktype(b, TS_TAPE) == 0)
		return(0);
	return(1);
}

/*
 * read the tape into buf, then return whether or
 * or not it is a header block.
 */
gethead(buf)
	struct s_spcl *buf;
{
	union u_ospcl {
		char dummy[TP_BSIZE];
		struct	s_ospcl {
			int	c_type;
			time_t	c_date;
			time_t	c_ddate;
			int	c_volume;
			daddr_t	c_tapea;
			ino_t	c_inumber;
			int	c_magic;
			int	c_checksum;
			struct odinode {
				unsigned short odi_mode;
				short	odi_nlink;
				short	odi_uid;
				short	odi_gid;
				off_t	odi_size;
				daddr_t	odi_rdev;
				char	odi_addr[36];
				time_t	odi_atime;
				time_t	odi_mtime;
				time_t	odi_ctime;
			} c_dinode;
			int	c_count;
			char	c_addr[TP_NINDIR];
		} s_ospcl;
	} u_ospcl;

	if (!cvtflag) {
		readtape((char *)buf);
		if (buf->c_magic != NFS_MAGIC || checksum((int *)buf) == 0)
			return(0);
		return(1);
	}
	readtape((char *)(&u_ospcl.s_ospcl));
	bzero((char *)buf, TP_BSIZE);
	buf->c_type = u_ospcl.s_ospcl.c_type;
	buf->c_date = u_ospcl.s_ospcl.c_date;
	buf->c_ddate = u_ospcl.s_ospcl.c_ddate;
	buf->c_volume = u_ospcl.s_ospcl.c_volume;
	buf->c_tapea = u_ospcl.s_ospcl.c_tapea;
	buf->c_inumber = u_ospcl.s_ospcl.c_inumber;
	buf->c_checksum = u_ospcl.s_ospcl.c_checksum;
	buf->c_magic = u_ospcl.s_ospcl.c_magic;
	buf->c_dinode.di_mode = u_ospcl.s_ospcl.c_dinode.odi_mode;
	buf->c_dinode.di_nlink = u_ospcl.s_ospcl.c_dinode.odi_nlink;
	buf->c_dinode.di_uid = u_ospcl.s_ospcl.c_dinode.odi_uid;
	buf->c_dinode.di_gid = u_ospcl.s_ospcl.c_dinode.odi_gid;
	buf->c_dinode.di_size = u_ospcl.s_ospcl.c_dinode.odi_size;
	buf->c_dinode.di_rdev = u_ospcl.s_ospcl.c_dinode.odi_rdev;
	buf->c_dinode.di_atime = u_ospcl.s_ospcl.c_dinode.odi_atime;
	buf->c_dinode.di_mtime = u_ospcl.s_ospcl.c_dinode.odi_mtime;
	buf->c_dinode.di_ctime = u_ospcl.s_ospcl.c_dinode.odi_ctime;
	buf->c_count = u_ospcl.s_ospcl.c_count;
	bcopy(u_ospcl.s_ospcl.c_addr, buf->c_addr, TP_NINDIR);
	if (u_ospcl.s_ospcl.c_magic != OFS_MAGIC ||
	    checksum((int *)(&u_ospcl.s_ospcl)) == 0)
		return(0);
	buf->c_magic = NFS_MAGIC;
	return(1);
}

/*
 * return whether or not the buffer contains a header block
 */
ishead(buf)
	struct s_spcl *buf;
{

	if (buf->c_magic != NFS_MAGIC)
		return(0);
	return(1);
}

checktype(b, t)
	struct s_spcl *b;
	int	t;
{

	return(b->c_type == t);
}

/*
 * read a bit mask from the tape into m.
 */
readbits(mapp)
	char **mapp;
{
	register int i;
	char	*m;

	i = spcl.c_count;

	if (*mapp == 0)
		*mapp = (char *)calloc(i, (TP_BSIZE/(NBBY/BITS)));
	m = *mapp;
	while (i--) {
		readtape((char *) m);
		m += (TP_BSIZE/(NBBY/BITS));
	}
	while (gethead(&spcl) == 0)
		;
}

checksum(b)
	register int *b;
{
	register int i, j;

	j = sizeof(union u_spcl) / sizeof(int);
	i = 0;
	do
		i += *b++;
	while (--j);
	if (i != CHECKSUM) {
		fprintf(stderr, "Checksum error %o, file %s\n", i,
			entry->x_name);
		return(0);
	}
	return(1);
}

/*
 *	Check for access into each directory in the pathname of an extracted
 *	file and create such a directory if needed in preparation for moving 
 *	the file to its proper home.
 */
checkdir(name)
	register char *name;
{
	register char *cp;
	int i;

	for (cp = name; *cp; cp++) {
		if (*cp != '/')
			continue;
		*cp = '\0';
		if (access(name, 01) < 0 && mkdir(name, 0777) < 0)
			fprintf(stderr, "restor: "), perror(name);
		*cp = '/';
	}
}

setdir(dev)
	char *dev;
{
	struct fstab *fsp;

	if (setfsent() == 0) {
		fprintf(stderr, "Can't open checklist file: %s\n", FSTAB);
		done(1);
	}
	while ((fsp = getfsent()) != 0) {
		if (strcmp(fsp->fs_spec, dev) == 0) {
			fprintf(stderr, "%s mounted on %s\n", dev, fsp->fs_file);
			if (chdir(fsp->fs_file) >= 0)
				return;
			fprintf(stderr, "%s cannot chdir to %s\n",
			    fsp->fs_file);
			done(1);
		}
	}
	fprintf(stderr, "%s not mounted\n", dev);
	done(1);
}

/*
 * These variables are "local" to the following two functions.
 */
char dirbuf[DIRBLKSIZ];
long dirloc = 0;
long prev = 0;

/*
 * add a new directory entry to a file.
 */
putent(dp, wrtfunc)
	struct direct *dp;
	int (*wrtfunc)();
{

	if (dp->d_ino == 0)
		return;
	if (dirloc + dp->d_reclen > DIRBLKSIZ) {
		((struct direct *)(dirbuf + prev))->d_reclen =
		    DIRBLKSIZ - prev;
		(*wrtfunc)(dirbuf, DIRBLKSIZ);
		dirloc = 0;
	}
	bcopy((char *)dp, dirbuf + dirloc, (long)dp->d_reclen);
	prev = dirloc;
	dirloc += dp->d_reclen;
}

/*
 * flush out a directory that is finished.
 */
flushent(wrtfunc)
	int (*wrtfunc)();
{

	((struct direct *)(dirbuf + prev))->d_reclen = DIRBLKSIZ - prev;
	(*wrtfunc)(dirbuf, dirloc);
	dirloc = 0;
}

dirwrite(buf, size)
	char *buf;
	int size;
{

	fwrite(buf, 1, size, df);
	seekpt = ftell(df);
}

dcvt(odp, ndp)
	register struct odirect *odp;
	register struct direct *ndp;
{

	bzero((char *)ndp, (long)(sizeof *ndp));
	ndp->d_ino =  odp->d_ino;
	strncpy(ndp->d_name, odp->d_name, ODIRSIZ);
	ndp->d_namlen = strlen(ndp->d_name);
	ndp->d_reclen = DIRSIZ(ndp);
	/*
	 * this quickly calculates if this inode is a directory.
	 * Currently not maintained.
	 *
	for (itp = inotab[INOHASH(odp->d_ino)]; itp; itp = itp->t_next) {
		if (itp->t_ino != odp->d_ino)
			continue;
		ndp->d_fmt = IFDIR;
		break;
	}
	 */
}

/*
 * Open a directory.
 * Modified to allow any random file to be a legal directory.
 */
DIR *
opendir(name)
	char *name;
{
	register DIR *dirp;

	dirp = (DIR *)malloc(sizeof(DIR));
	dirp->dd_fd = open(name, 0);
	if (dirp->dd_fd == -1) {
		free((char *)dirp);
		return NULL;
	}
	dirp->dd_loc = 0;
	return dirp;
}

/*
 * Seek to an entry in a directory.
 * Only values returned by ``telldir'' should be passed to seekdir.
 * Modified to have many directories based in one file.
 */
void
seekdir(dirp, loc, base)
	register DIR *dirp;
	daddr_t loc, base;
{

	if (loc == telldir(dirp))
		return;
	loc -= base;
	if (loc < 0)
		fprintf(stderr, "bad seek pointer to seekdir %d\n", loc);
	(void)lseek(dirp->dd_fd, base + (loc & ~(DIRBLKSIZ - 1)), 0);
	dirp->dd_loc = loc & (DIRBLKSIZ - 1);
	if (dirp->dd_loc != 0)
		dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, DIRBLKSIZ);
}

/*
 * get next entry in a directory.
 */
struct direct *
readdir(dirp)
	register DIR *dirp;
{
	register struct direct *dp;

	for (;;) {
		if (dirp->dd_loc == 0) {
			dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, 
			    DIRBLKSIZ);
			if (dirp->dd_size <= 0)
				return NULL;
		}
		if (dirp->dd_loc >= dirp->dd_size) {
			dirp->dd_loc = 0;
			continue;
		}
		dp = (struct direct *)(dirp->dd_buf + dirp->dd_loc);
		if (dp->d_reclen <= 0 ||
		    dp->d_reclen > DIRBLKSIZ + 1 - dirp->dd_loc)
			return NULL;
		dirp->dd_loc += dp->d_reclen;
		if (dp->d_ino == 0)
			continue;
		return (dp);
	}
}

allocinotab(ino, seekpt)
	ino_t ino;
	daddr_t seekpt;
{
	register struct inotab	*itp;

	itp = (struct inotab *)calloc(1, sizeof(struct inotab));
	itp->t_next = inotab[INOHASH(ino)];
	inotab[INOHASH(ino)] = itp;
	itp->t_ino = ino;
	itp->t_seekpt = seekpt;
}

struct xtrlist *
allocxtr(ino, name, flags)
	ino_t ino;
	char *name;
	char flags;
{
	register struct xtrlist	*xp, *pxp;
	int size;

	size = sizeof(struct xtrlist) + strlen(name);
	xp = (struct xtrlist *)calloc(1, size);
	xp->x_ino = ino;
	xp->x_flags = flags;
	strcpy(xp->x_name, name);
	if (flags == 0)
		return (xp);
	xp->x_next = xtrlist[INOHASH(ino)];
	xtrlist[INOHASH(ino)] = xp;
	xtrcnt++;
	for (pxp = xp->x_next; pxp; pxp = pxp->x_next)
		if (pxp->x_ino == ino && (pxp->x_flags & XLINKED) == 0) {
			xp->x_flags |= XLINKED;
			xp->x_linkedto = pxp;
			xtrcnt--;
			break;
		}
	if (!vflag && !tflag)
		return (xp);
	if (xp->x_flags & XLINKED)
		fprintf(stdout, "%s: linked to %s\n", xp->x_name, pxp->x_name);
	else if (xp->x_flags & XISDIR)
		fprintf(stdout, "%s: directory inode %u\n", xp->x_name, ino);
	else
		fprintf(stdout, "%s: inode %u\n", xp->x_name, ino);
	return (xp);
}

#ifdef RRESTOR
msg(cp, a1, a2, a3)
	char *cp;
{

	fprintf(stderr, cp, a1, a2, a3);
}
#endif RRESTOR

done(exitcode)
	int exitcode;
{

	unlink(dirfile);
	exit(exitcode);
}
