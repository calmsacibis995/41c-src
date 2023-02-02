char version[] = "@(#)fsck.c	2.22	(Berkeley)	3/8/83";

#include <stdio.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <dir.h>
#include <sys/stat.h>
#include <fstab.h>

/* RECONSTRUCT ONLY BAD CG IN PASS 6 */

typedef	int	(*SIG_TYP)();

#define	MAXNINDIR	(MAXBSIZE / sizeof (daddr_t))
#define	MAXINOPB	(MAXBSIZE / sizeof (struct dinode))
#define	SPERB		(MAXBSIZE / sizeof(short))

#define	MAXDUP	10		/* limit on dup blks (per inode) */
#define	MAXBAD	10		/* limit on bad blks (per inode) */

#define	USTATE	0		/* inode not allocated */
#define	FSTATE	01		/* inode is file */
#define	DSTATE	02		/* inode is directory */
#define	CLEAR	03		/* inode is to be cleared */

typedef struct dinode	DINODE;
typedef struct direct	DIRECT;

#define	ALLOC	((dp->di_mode & IFMT) != 0)
#define	DIRCT	((dp->di_mode & IFMT) == IFDIR)
#define	REG	((dp->di_mode & IFMT) == IFREG)
#define	BLK	((dp->di_mode & IFMT) == IFBLK)
#define	CHR	((dp->di_mode & IFMT) == IFCHR)
#define	LNK	((dp->di_mode & IFMT) == IFLNK)
#define	SOCK	((dp->di_mode & IFMT) == IFSOCK)
#define	BADBLK	((dp->di_mode & IFMT) == IFMT)
#define	SPECIAL	(BLK || CHR)

ino_t	startinum;		/* blk num of first in raw area */

struct bufarea {
	struct bufarea	*b_next;		/* must be first */
	daddr_t	b_bno;
	int	b_size;
	union {
		char	b_buf[MAXBSIZE];	/* buffer space */
		short	b_lnks[SPERB];		/* link counts */
		daddr_t	b_indir[MAXNINDIR];	/* indirect block */
		struct	fs b_fs;		/* super block */
		struct	cg b_cg;		/* cylinder group */
		struct dinode b_dinode[MAXINOPB]; /* inode block */
	} b_un;
	char	b_dirty;
};

typedef struct bufarea BUFAREA;

BUFAREA	inoblk;			/* inode blocks */
BUFAREA	fileblk;		/* other blks in filesys */
BUFAREA	sblk;			/* file system superblock */
BUFAREA	cgblk;

#define	initbarea(x)	(x)->b_dirty = 0;(x)->b_bno = (daddr_t)-1
#define	dirty(x)	(x)->b_dirty = 1
#define	inodirty()	inoblk.b_dirty = 1
#define	sbdirty()	sblk.b_dirty = 1
#define	cgdirty()	cgblk.b_dirty = 1

#define	dirblk		fileblk.b_un
#define	sblock		sblk.b_un.b_fs
#define	cgrp		cgblk.b_un.b_cg

struct filecntl {
	int	rfdes;
	int	wfdes;
	int	mod;
} dfile;			/* file descriptors for filesys */

#define	DUPTBLSIZE	100	/* num of dup blocks to remember */
daddr_t	duplist[DUPTBLSIZE];	/* dup block table */
daddr_t	*enddup;		/* next entry in dup table */
daddr_t	*muldup;		/* multiple dups part of table */

#define	MAXLNCNT	500	/* num zero link cnts to remember */
ino_t	badlncnt[MAXLNCNT];	/* table of inos with zero link cnts */
ino_t	*badlnp;		/* next entry in table */

char	rawflg;
char	nflag;			/* assume a no response */
char	yflag;			/* assume a yes response */
int	bflag;			/* location of alternate super block */
int	debug;			/* output debugging info */
char	preen;			/* just fix normal inconsistencies */
char	rplyflag;		/* any questions asked? */
char	hotroot;		/* checking root device */
char	fixcg;			/* corrupted free list bit maps */

char	*blockmap;		/* ptr to primary blk allocation map */
char	*freemap;		/* ptr to secondary blk allocation map */
char	*statemap;		/* ptr to inode state table */
short	*lncntp;		/* ptr to link count table */

char	*pathp;			/* pointer to pathname position */
char	*thisname;		/* ptr to current pathname component */
char	*srchname;		/* name being searched for in dir */
char	pathname[BUFSIZ];
char	*endpathname = &pathname[BUFSIZ - 2];

char	*lfname = "lost+found";

ino_t	inum;			/* inode we are currently working on */
ino_t	dnum;			/* directory inode currently being worked on */
ino_t	imax;			/* number of inodes */
ino_t	parentdir;		/* i number of parent directory */
ino_t	lastino;		/* hiwater mark of inodes */
ino_t	lfdir;			/* lost & found directory */
ino_t	orphan;			/* orphaned inode */

off_t	filsize;		/* num blks seen in file */
off_t	maxblk;			/* largest logical blk in file */
off_t	bmapsz;			/* num chars in blockmap */

daddr_t	n_ffree;		/* number of small free blocks */
daddr_t	n_bfree;		/* number of large free blocks */
daddr_t	n_blks;			/* number of blocks used */
daddr_t	n_files;		/* number of files seen */
daddr_t	n_index;
daddr_t	n_bad;
daddr_t	fmax;			/* number of blocks in the volume */

daddr_t	badblk;
daddr_t	dupblk;

int	inosumbad;
int	offsumbad;
int	frsumbad;
int	sbsumbad;

#define	zapino(x)	(*(x) = zino)
struct	dinode zino;

#define	setbmap(x)	setbit(blockmap, x)
#define	getbmap(x)	isset(blockmap, x)
#define	clrbmap(x)	clrbit(blockmap, x)

#define	setfmap(x)	setbit(freemap, x)
#define	getfmap(x)	isset(freemap, x)
#define	clrfmap(x)	clrbit(freemap, x)

#define	DATA	1
#define	ADDR	0

#define	ALTERD	010
#define	KEEPON	04
#define	SKIP	02
#define	STOP	01

int	(*signal())();
long	lseek();
time_t	time();
DINODE	*ginode();
BUFAREA	*getblk();
int	dirscan();
int	findino();
int	catch();
int	mkentry();
int	chgdd();
int	pass1check(), pass1bcheck(), pass2check(), pass4check(), pass5check();
int	(*pfunc)();
char	*rawname(), *rindex(), *unrawname();
extern int inside[], around[];
extern unsigned char *fragtbl[];

char	*devname;

main(argc, argv)
	int	argc;
	char	*argv[];
{
	struct fstab *fsp;
	int pid, passno, anygtr, sumstatus;

	sync();
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {

		case 'p':
			preen++;
			break;

		case 'b':
			bflag = atoi(argv[0]+1);
			printf("Alternate super block location: %d\n", bflag);
			break;

		case 'd':
			debug++;
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		default:
			errexit("%c option?\n", **argv);
		}
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if (argc) {
		while (argc-- > 0) {
			hotroot = 0;
			checkfilesys(*argv++);
		}
		exit(0);
	}
	sumstatus = 0;
	passno = 1;
	do {
		anygtr = 0;
		if (setfsent() == 0)
			errexit("Can't open checklist file: %s\n", FSTAB);
		while ((fsp = getfsent()) != 0) {
			if (strcmp(fsp->fs_type, FSTAB_RW) &&
			    strcmp(fsp->fs_type, FSTAB_RO))
				continue;
			if (preen == 0 ||
			    passno == 1 && fsp->fs_passno == passno) {
				if (blockcheck(fsp->fs_spec) == 0 && preen)
					exit(8);
			} else if (fsp->fs_passno > passno)
				anygtr = 1;
			else if (fsp->fs_passno == passno) {
				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(8);
				}
				if (pid == 0)
					if (blockcheck(fsp->fs_spec)==0)
						exit(8);
					else
						exit(0);
			}
		}
		if (preen) {
			int status;
			while (wait(&status) != -1)
				sumstatus |= status;
		}
		passno++;
	} while (anygtr);
	if (sumstatus)
		exit(8);
	endfsent();
	exit(0);
}

blockcheck(name)
	char *name;
{
	struct stat stslash, stblock, stchar;
	char *raw;
	int looped = 0;

	hotroot = 0;
	if (stat("/", &stslash) < 0){
		error("Can't stat root\n");
		return (0);
	}
retry:
	if (stat(name, &stblock) < 0){
		error("Can't stat %s\n", name);
		return (0);
	}
	if (stblock.st_mode & S_IFBLK) {
		raw = rawname(name);
		if (stat(raw, &stchar) < 0){
			error("Can't stat %s\n", raw);
			return (0);
		}
		if (stchar.st_mode & S_IFCHR) {
			if (stslash.st_dev == stblock.st_rdev) {
				hotroot++;
				raw = rawname(name);
			}
			checkfilesys(raw);
			return (1);
		} else {
			error("%s is not a character device\n", raw);
			return (0);
		}
	} else if (stblock.st_mode & S_IFCHR) {
		if (looped) {
			error("Can't make sense out of name %s\n", name);
			return (0);
		}
		name = unrawname(name);
		looped++;
		goto retry;
	}
	error("Can't make sense out of name %s\n", name);
	return (0);
}

checkfilesys(filesys)
	char *filesys;
{
	register DINODE *dp;
	register ino_t *blp;
	register int i, n;
	ino_t savino;
	int b, c, j, partial, ndb;
	daddr_t d, s;

	devname = filesys;
	if (setup(filesys) == 0) {
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
		return;
	}
/* 1: scan inodes tallying blocks used */
	if (preen == 0) {
		printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
		if (hotroot)
			printf("** Root file system\n");
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

/* 1b: locate first references to duplicates, if any */
	if (enddup != &duplist[0]) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

/* 2: traverse directories to check reference counts */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

/* 3 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

/* 4 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

/* 5 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	if (fixcg) {
		if (preen == 0)
			printf("** Phase 6 - Salvage Cylinder Groups\n");
		makecg();
		n_ffree = sblock.fs_cstotal.cs_nffree;
		n_bfree = sblock.fs_cstotal.cs_nbfree;
	}

	pwarn("%d files, %d used, %d free (%d frags, %d blocks)\n",
	    n_files, n_blks - howmany(sblock.fs_cssize, sblock.fs_fsize),
	    n_ffree + sblock.fs_frag * n_bfree, n_ffree, n_bfree);
	if (dfile.mod) {
		time(&sblock.fs_time);
		sbdirty();
	}
	ckfini();
	free(blockmap);
	free(freemap);
	free(statemap);
	free(lncntp);
	if (dfile.mod) {
		if (preen) {
			if (hotroot)
				exit(4);
		} else {
			printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
			if (hotroot) {
				printf("\n***** BOOT UNIX (NO SYNC!) *****\n");
				exit(4);
			}
		}
	}
	sync();			/* ??? */
}

setup(dev)
	char *dev;
{
	dev_t rootdev;
	struct stat statb;
	int super = bflag ? bflag : SBLOCK;
	int i, j, size;
	int c, d, cgd;

	bflag = 0;
	if (stat("/", &statb) < 0)
		errexit("Can't stat root\n");
	rootdev = statb.st_dev;
	if (stat(dev, &statb) < 0) {
		error("Can't stat %s\n", dev);
		return (0);
	}
	rawflg = 0;
	if ((statb.st_mode & S_IFMT) == S_IFBLK)
		;
	else if ((statb.st_mode & S_IFMT) == S_IFCHR)
		rawflg++;
	else {
		if (reply("file is not a block or character device; OK") == 0)
			return (0);
	}
	if (rootdev == statb.st_rdev)
		hotroot++;
	if ((dfile.rfdes = open(dev, 0)) < 0) {
		error("Can't open %s\n", dev);
		return (0);
	}
	if (preen == 0)
		printf("** %s", dev);
	if (nflag || (dfile.wfdes = open(dev, 1)) < 0) {
		dfile.wfdes = -1;
		if (preen)
			pfatal("NO WRITE ACCESS");
		printf(" (NO WRITE)");
	}
	if (preen == 0)
		printf("\n");
	fixcg = 0; inosumbad = 0; offsumbad = 0; frsumbad = 0; sbsumbad = 0;
	dfile.mod = 0;
	n_files = n_blks = n_ffree = n_bfree = 0;
	muldup = enddup = &duplist[0];
	badlnp = &badlncnt[0];
	lfdir = 0;
	rplyflag = 0;
	initbarea(&sblk);
	initbarea(&fileblk);
	initbarea(&inoblk);
	initbarea(&cgblk);
	/*
	 * Read in the super block and its summary info.
	 */
	if (bread(&dfile, &sblock, super, SBSIZE) == 0)
		return (0);
	sblk.b_bno = super;
	sblk.b_size = SBSIZE;
	/*
	 * run a few consistency checks of the super block
	 */
	if (sblock.fs_magic != FS_MAGIC)
		{ badsb("MAGIC NUMBER WRONG"); return (0); }
	if (sblock.fs_ncg < 1)
		{ badsb("NCG OUT OF RANGE"); return (0); }
	if (sblock.fs_cpg < 1 || sblock.fs_cpg > MAXCPG)
		{ badsb("CPG OUT OF RANGE"); return (0); }
	if (sblock.fs_nsect < 1)
		{ badsb("NSECT < 1"); return (0); }
	if (sblock.fs_ntrak < 1)
		{ badsb("NTRAK < 1"); return (0); }
	if (sblock.fs_spc != sblock.fs_nsect * sblock.fs_ntrak)
		{ badsb("SPC DOES NOT JIVE w/NTRAK*NSECT"); return (0); }
	if (sblock.fs_ipg % INOPB(&sblock))
		{ badsb("INODES NOT MULTIPLE OF A BLOCK"); return (0); }
	if (cgdmin(&sblock, 0) >= sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock))
		{ badsb("IMPLIES MORE INODE THAN DATA BLOCKS"); return (0); }
	if (sblock.fs_ncg * sblock.fs_cpg < sblock.fs_ncyl ||
	    (sblock.fs_ncg - 1) * sblock.fs_cpg >= sblock.fs_ncyl)
		{ badsb("NCYL DOES NOT JIVE WITH NCG*CPG"); return (0); }
	if (sblock.fs_fpg != sblock.fs_cpg * sblock.fs_spc / NSPF(&sblock))
		{ badsb("FPG DOES NOT JIVE WITH CPG & SPC"); return (0); }
	if (sblock.fs_size * NSPF(&sblock) <=
	    (sblock.fs_ncyl - 1) * sblock.fs_spc)
		{ badsb("SIZE PREPOSTEROUSLY SMALL"); return (0); }
	if (sblock.fs_size * NSPF(&sblock) > sblock.fs_ncyl * sblock.fs_spc)
		{ badsb("SIZE PREPOSTEROUSLY LARGE"); return (0); }
	/* rest we COULD repair... */
	if (sblock.fs_cgsize != fragroundup(&sblock,
	    sizeof(struct cg) + howmany(sblock.fs_fpg, NBBY)))
		{ badsb("CGSIZE INCORRECT"); return (0); }
	if (sblock.fs_cssize !=
	    fragroundup(&sblock, sblock.fs_ncg * sizeof(struct csum)))
		{ badsb("CSSIZE INCORRECT"); return (0); }
	fmax = sblock.fs_size;
	imax = sblock.fs_ncg * sblock.fs_ipg;
	n_bad = cgsblock(&sblock, 0); /* boot block plus dedicated sblock */
	/*
	 * read in the summary info.
	 */
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		size = sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize;
		sblock.fs_csp[j] = (struct csum *)calloc(1, size);
		bread(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    size);
	}
	/*
	 * allocate and initialize the necessary maps
	 */
	bmapsz = roundup(howmany(fmax, NBBY), sizeof(short));
	blockmap = (char *)calloc(bmapsz, sizeof (char));
	if (blockmap == NULL) {
		printf("cannot alloc %d bytes for blockmap\n", bmapsz);
		exit(1);
	}
	freemap = (char *)calloc(bmapsz, sizeof (char));
	if (freemap == NULL) {
		printf("cannot alloc %d bytes for freemap\n", bmapsz);
		exit(1);
	}
	statemap = (char *)calloc(imax+1, sizeof(char));
	if (statemap == NULL) {
		printf("cannot alloc %d bytes for statemap\n", imax + 1);
		exit(1);
	}
	lncntp = (short *)calloc(imax+1, sizeof(short));
	if (lncntp == NULL) {
		printf("cannot alloc %d bytes for lncntp\n", 
		    (imax + 1) * sizeof(short));
		exit(1);
	}
	for (c = 0; c < sblock.fs_ncg; c++) {
		cgd = cgdmin(&sblock, c);
		if (c == 0) {
			d = cgbase(&sblock, c);
			cgd += howmany(sblock.fs_cssize, sblock.fs_fsize);
		} else
			d = cgsblock(&sblock, c);
		for (; d < cgd; d++)
			setbmap(d);
	}

	startinum = imax + 1;
	return (1);

badsb:
	ckfini();
	return (0);
}

pass1()
{
	register int c, i, n, j;
	register DINODE *dp;
	int savino, ndb, partial;

	pfunc = pass1check;
	inum = 0;
	n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
	for (c = 0; c < sblock.fs_ncg; c++) {
		if (getblk(&cgblk, cgtod(&sblock, c), sblock.fs_cgsize) == 0)
			continue;
		if (cgrp.cg_magic != CG_MAGIC) {
			pfatal("cg %d: bad magic number\n", c);
			bzero((caddr_t)&cgrp, sblock.fs_cgsize);
		}
		n = 0;
		for (i = 0; i < sblock.fs_ipg; i++, inum++) {
			dp = ginode();
			if (dp == NULL)
				continue;
			n++;
			if (ALLOC) {
				if (!isset(cgrp.cg_iused, i)) {
					if (debug)
						printf("%d bad, not used\n",
						    inum);
					inosumbad++;
				}
				n--;
				lastino = inum;
				if (!preen && BADBLK &&
				    reply("HOLD BAD BLOCK") == 1) {
					dp->di_size = sblock.fs_fsize;
					dp->di_mode = IFREG|0600;
					inodirty();
				} else if (ftypeok(dp) == 0)
					goto unknown;
				if (dp->di_size < 0) {
					if (debug)
						printf("bad size %d:",
							dp->di_size);
					goto unknown;
				}
				ndb = howmany(dp->di_size, sblock.fs_bsize);
				if (SPECIAL)
					ndb++;
				for (j = ndb; j < NDADDR; j++)
					if (dp->di_db[j] != 0) {
						if (debug)
							printf("bad direct addr: %d\n",
								dp->di_db[j]);
						goto unknown;
					}
				for (j = 0, ndb -= NDADDR; ndb > 0; j++)
					ndb /= NINDIR(&sblock);
				for (; j < NIADDR; j++)
					if (dp->di_ib[j] != 0) {
						if (debug)
							printf("bad indirect addr: %d\n",
								dp->di_ib[j]);
						goto unknown;
					}
				n_files++;
				lncntp[inum] = dp->di_nlink;
				if (dp->di_nlink <= 0) {
					if (badlnp < &badlncnt[MAXLNCNT])
						*badlnp++ = inum;
					else {
						pfatal("LINK COUNT TABLE OVERFLOW");
						if (reply("CONTINUE") == 0)
							errexit("");
					}
				}
				statemap[inum] = DIRCT ? DSTATE : FSTATE;
				badblk = dupblk = 0; filsize = 0; maxblk = 0;
				ckinode(dp, ADDR);
				continue;
		unknown:
				if (!SOCK)
					pfatal("UNKNOWN FILE TYPE I=%u", inum);
				if ((preen && SOCK) || reply("CLEAR") == 1) {
					zapino(dp);
					inodirty();
					inosumbad++;
				}
			} else {
				if (isset(cgrp.cg_iused, i)) {
					if (debug)
						printf("%d bad, marked used\n",
						    inum);
					inosumbad++;
					n--;
				}
				partial = 0;
				for (j = 0; j < NDADDR; j++)
					if (dp->di_db[j] != 0)
						partial++;
				for (j = 0; j < NIADDR; j++)
					if (dp->di_ib[j] != 0)
						partial++;
				if (partial || dp->di_mode != 0 ||
				    dp->di_size != 0) {
					pfatal("PARTIALLY ALLOCATED INODE I=%u", inum);
					if (reply("CLEAR") == 1) {
						zapino(dp);
						inodirty();
						inosumbad++;
					}
				}
			}
		}
		if (n != cgrp.cg_cs.cs_nifree) {
			if (debug)
				printf("cg[%d].cg_cs.cs_nifree is %d; calc %d\n",
				    c, cgrp.cg_cs.cs_nifree, n);
			inosumbad++;
		}
		if (cgrp.cg_cs.cs_nbfree != sblock.fs_cs(&sblock, c).cs_nbfree
		  || cgrp.cg_cs.cs_nffree != sblock.fs_cs(&sblock, c).cs_nffree
		  || cgrp.cg_cs.cs_nifree != sblock.fs_cs(&sblock, c).cs_nifree
		  || cgrp.cg_cs.cs_ndir != sblock.fs_cs(&sblock, c).cs_ndir)
			sbsumbad++;
	}
}

pass1check(blk, size)
	daddr_t blk;
	int size;
{
	register daddr_t *dlp;
	int res = KEEPON;
	int anyout;

	anyout = outrange(blk, size);
	for (; size > 0; blk++, size--) {
		if (anyout && outrange(blk, 1)) {
			blkerr("BAD", blk);
			if (++badblk >= MAXBAD) {
				pwarn("EXCESSIVE BAD BLKS I=%u", inum);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			res = SKIP;
		} else if (getbmap(blk)) {
			blkerr("DUP", blk);
			if (++dupblk >= MAXDUP) {
				pwarn("EXCESSIVE DUP BLKS I=%u", inum);
				if (preen)
					printf(" (SKIPPING)\n");
				else if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			if (enddup >= &duplist[DUPTBLSIZE]) {
				pfatal("DUP TABLE OVERFLOW.");
				if (reply("CONTINUE") == 0)
					errexit("");
				return (STOP);
			}
			for (dlp = duplist; dlp < muldup; dlp++)
				if (*dlp == blk) {
					*enddup++ = blk;
					break;
				}
			if (dlp >= muldup) {
				*enddup++ = *muldup;
				*muldup++ = blk;
			}
		} else {
			n_blks++;
			setbmap(blk);
		}
		filsize++;
	}
	return (res);
}

pass1b()
{
	register int c, i;
	register DINODE *dp;

	pfunc = pass1bcheck;
	inum = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		for (i = 0; i < sblock.fs_ipg; i++, inum++) {
			dp = ginode();
			if (dp == NULL)
				continue;
			if (statemap[inum] != USTATE &&
			    (ckinode(dp, ADDR) & STOP))
				goto out1b;
		}
	}
out1b:
	flush(&dfile, &inoblk);
}

pass1bcheck(blk, size)
	daddr_t blk;
	int size;
{
	register daddr_t *dlp;
	int res = KEEPON;

	for (; size > 0; blk++, size--) {
		if (outrange(blk, 1))
			res = SKIP;
		for (dlp = duplist; dlp < muldup; dlp++)
			if (*dlp == blk) {
				blkerr("DUP", blk);
				*dlp = *--muldup;
				*muldup = blk;
				if (muldup == duplist)
					return (STOP);
			}
	}
	return (res);
}

pass2()
{
	register DINODE *dp;

	inum = ROOTINO;
	thisname = pathp = pathname;
	pfunc = pass2check;
	switch (statemap[inum]) {

	case USTATE:
		errexit("ROOT INODE UNALLOCATED. TERMINATING.\n");

	case FSTATE:
		pfatal("ROOT INODE NOT DIRECTORY");
		if (reply("FIX") == 0 || (dp = ginode()) == NULL)
			errexit("");
		dp->di_mode &= ~IFMT;
		dp->di_mode |= IFDIR;
		inodirty();
		inosumbad++;
		statemap[inum] = DSTATE;
		/* fall into ... */

	case DSTATE:
		descend();
		break;

	case CLEAR:
		pfatal("DUPS/BAD IN ROOT INODE");
		printf("\n");
		if (reply("CONTINUE") == 0)
			errexit("");
		statemap[inum] = DSTATE;
		descend();
	}
}

pass2check(dirp)
	register DIRECT *dirp;
{
	register char *p;
	register n;
	DINODE *dp;

	if ((inum = dirp->d_ino) == 0)
		return (KEEPON);
	thisname = pathp;
	if (pathp + dirp->d_namlen >= endpathname) {
		*pathp = '\0';
		errexit("NAME TOO LONG %s%s\n", pathname, dirp->d_name);
	}
	for (p = dirp->d_name; p < &dirp->d_name[MAXNAMLEN]; )
		if ((*pathp++ = *p++) == 0) {
			--pathp;
			break;
		}
	*pathp = 0;
	n = 0;
	if (inum > imax || inum <= 0)
		n = direrr("I OUT OF RANGE");
	else {
again:
		switch (statemap[inum]) {
		case USTATE:
			n = direrr("UNALLOCATED");
			break;

		case CLEAR:
			if ((n = direrr("DUP/BAD")) == 1)
				break;
			if ((dp = ginode()) == NULL)
				break;
			statemap[inum] = DIRCT ? DSTATE : FSTATE;
			goto again;

		case FSTATE:
			lncntp[inum]--;
			break;

		case DSTATE:
			lncntp[inum]--;
			descend();
			break;
		}
	}
	pathp = thisname;
	if (n == 0)
		return (KEEPON);
	dirp->d_ino = 0;
	return (KEEPON|ALTERD);
}

pass3()
{
	ino_t savino;
	register DINODE *dp;

	for (inum = ROOTINO; inum <= lastino; inum++) {
		if (statemap[inum] == DSTATE) {
			pfunc = findino;
			srchname = "..";
			savino = inum;
			do {
				orphan = inum;
				if ((dp = ginode()) == NULL)
					break;
				filsize = dp->di_size;
				parentdir = 0;
				ckinode(dp, DATA);
				if ((inum = parentdir) == 0)
					break;
			} while (statemap[inum] == DSTATE);
			inum = orphan;
			if (linkup() == 1) {
				thisname = pathp = pathname;
				*pathp++ = '?';
				pfunc = pass2check;
				descend();
			}
			inum = savino;
		}
	}
}

pass4()
{
	register int n;
	register ino_t *blp;

	pfunc = pass4check;
	for (inum = ROOTINO; inum <= lastino; inum++) {
		switch (statemap[inum]) {

		case FSTATE:
			n = lncntp[inum];
			if (n)
				adjust((short)n);
			else {
				for (blp = badlncnt;blp < badlnp; blp++)
					if (*blp == inum) {
						clri("UNREF", 1);
						break;
					}
			}
			break;

		case DSTATE:
			clri("UNREF", 1);
			break;

		case CLEAR:
			clri("BAD/DUP", 1);
			break;
		}
	}
	if (imax - ROOTINO - n_files != sblock.fs_cstotal.cs_nifree) {
		pwarn("FREE INODE COUNT WRONG IN SUPERBLK");
		if (preen)
			printf(" (FIXED)\n");
		if (preen || reply("FIX") == 1) {
			sblock.fs_cstotal.cs_nifree = imax - ROOTINO - n_files;
			sbdirty();
		}
	}
	flush(&dfile, &fileblk);
}

pass4check(blk, size)
	daddr_t blk;
{
	register daddr_t *dlp;
	int res = KEEPON;

	for (; size > 0; blk++, size--) {
		if (outrange(blk, 1))
			res = SKIP;
		else if (getbmap(blk)) {
			for (dlp = duplist; dlp < enddup; dlp++)
				if (*dlp == blk) {
					*dlp = *--enddup;
					return (KEEPON);
				}
			clrbmap(blk);
			n_blks--;
		}
	}
	return (res);
}

pass5()
{
	register int c, n, i, b, d;
	short bo[MAXCPG][NRPOS];
	long botot[MAXCPG];
	long frsum[MAXFRAG];
	int blk;
	daddr_t cbase;
	int blockbits = (1<<sblock.fs_frag)-1;

	bcopy(blockmap, freemap, (unsigned)bmapsz);
	dupblk = 0;
	n_index = sblock.fs_ncg * (cgdmin(&sblock, 0) - cgtod(&sblock, 0));
	for (c = 0; c < sblock.fs_ncg; c++) {
		cbase = cgbase(&sblock, c);
		bzero(botot, sizeof (botot));
		bzero(bo, sizeof (bo));
		bzero(frsum, sizeof (frsum));
		/*
		 * need to account for the super blocks
		 * which appear (inaccurately) bad
		 */
		n_bad += cgtod(&sblock, c) - cgsblock(&sblock, c);
		if (getblk(&cgblk, cgtod(&sblock, c), sblock.fs_cgsize) == 0)
			continue;
		if (cgrp.cg_magic != CG_MAGIC) {
			pfatal("cg %d: bad magic number\n", c);
			bzero((caddr_t)&cgrp, sblock.fs_cgsize);
		}
		for (b = 0; b < sblock.fs_fpg; b += sblock.fs_frag) {
			blk = blkmap(&sblock, cgrp.cg_free, b);
			if (blk == 0)
				continue;
			if (blk == blockbits) {
				if (pass5check(cbase+b, sblock.fs_frag) == STOP)
					goto out5;
				/* this is clumsy ... */
				n_ffree -= sblock.fs_frag;
				n_bfree++;
				botot[cbtocylno(&sblock, b)]++;
				bo[cbtocylno(&sblock, b)]
				    [cbtorpos(&sblock, b)]++;
				continue;
			}
			for (d = 0; d < sblock.fs_frag; d++)
				if ((blk & (1<<d)) &&
				    pass5check(cbase+b+d,1) == STOP)
					goto out5;
			fragacct(&sblock, blk, frsum, 1);
		}
		if (bcmp(cgrp.cg_frsum, frsum, sizeof (frsum))) {
			if (debug)
			for (i = 0; i < sblock.fs_frag; i++)
				if (cgrp.cg_frsum[i] != frsum[i])
				printf("cg[%d].cg_frsum[%d] have %d calc %d\n",
				    c, i, cgrp.cg_frsum[i], frsum[i]);
			frsumbad++;
		}
		if (bcmp(cgrp.cg_btot, botot, sizeof (botot))) {
			if (debug)
			for (n = 0; n < sblock.fs_cpg; n++)
				if (botot[n] != cgrp.cg_btot[n])
				printf("cg[%d].cg_btot[%d] have %d calc %d\n",
				    c, n, cgrp.cg_btot[n], botot[n]);
			offsumbad++;
		}
		if (bcmp(cgrp.cg_b, bo, sizeof (bo))) {
			if (debug)
			for (i = 0; i < NRPOS; i++)
				if (bo[n][i] != cgrp.cg_b[n][i])
				printf("cg[%d].cg_b[%d][%d] have %d calc %d\n",
				    c, n, i, cgrp.cg_b[n][i], bo[n][i]);
			offsumbad++;
		}
	}
out5:
	if (dupblk)
		pwarn("%d DUP BLKS IN BIT MAPS\n", dupblk);
	if (fixcg == 0) {
		if ((b = n_blks+n_ffree+sblock.fs_frag*n_bfree+n_index+n_bad) != fmax) {
			pwarn("%ld BLK(S) MISSING\n", fmax - b);
			fixcg = 1;
		} else if (inosumbad + offsumbad + frsumbad + sbsumbad) {
			pwarn("SUMMARY INFORMATION %s%s%s%sBAD\n",
			    inosumbad ? "(INODE FREE) " : "",
			    offsumbad ? "(BLOCK OFFSETS) " : "",
			    frsumbad ? "(FRAG SUMMARIES) " : "",
			    sbsumbad ? "(SUPER BLOCK SUMMARIES) " : "");
			fixcg = 1;
		} else if (n_ffree != sblock.fs_cstotal.cs_nffree ||
		    n_bfree != sblock.fs_cstotal.cs_nbfree) {
			pwarn("FREE BLK COUNT(S) WRONG IN SUPERBLK");
			if (preen)
				printf(" (FIXED)\n");
			if (preen || reply("FIX") == 1) {
				sblock.fs_cstotal.cs_nffree = n_ffree;
				sblock.fs_cstotal.cs_nbfree = n_bfree;
				sbdirty();
			}
		}
	}
	if (fixcg) {
		pwarn("BAD CYLINDER GROUPS");
		if (preen)
			printf(" (SALVAGED)\n");
		else if (reply("SALVAGE") == 0)
			fixcg = 0;
	}
}

pass5check(blk, size)
	daddr_t blk;
	int size;
{

	if (outrange(blk, size)) {
		fixcg = 1;
		if (preen)
			pfatal("BAD BLOCKS IN BIT MAPS.");
		if (++badblk >= MAXBAD) {
			printf("EXCESSIVE BAD BLKS IN BIT MAPS.");
			if (reply("CONTINUE") == 0)
				errexit("");
			return (STOP);
		}
	}
	for (; size > 0; blk++, size--)
		if (getfmap(blk)) {
			fixcg = 1;
			++dupblk;
		} else {
			n_ffree++;
			setfmap(blk);
		}
	return (KEEPON);
}

ckinode(dp, flg)
	DINODE *dp;
	register flg;
{
	register daddr_t *ap;
	register ret;
	int (*func)(), n, ndb, size, offset;
	ino_t number = inum;
	DINODE dino;

	if (SPECIAL)
		return (KEEPON);
	dino = *dp;
	func = (flg == ADDR) ? pfunc : dirscan;
	ndb = howmany(dino.di_size, sblock.fs_bsize);
	for (ap = &dino.di_db[0]; ap < &dino.di_db[NDADDR]; ap++) {
		if (--ndb == 0 && (offset = blkoff(&sblock, dino.di_size)) != 0)
			size = numfrags(&sblock, fragroundup(&sblock, offset));
		else
			size = sblock.fs_frag;
		dnum = number;
		if (*ap && (ret = (*func)(*ap, size)) & STOP)
			return (ret);
	}
	for (ap = &dino.di_ib[0], n = 1; n <= 2; ap++, n++) {
		dnum = number;
		if (*ap) {
			ret = iblock(*ap, n, flg,
			    dino.di_size - sblock.fs_bsize * NDADDR);
			if (ret & STOP)
				return (ret);
		}
	}
	return (KEEPON);
}

iblock(blk, ilevel, flg, isize)
	daddr_t blk;
	register ilevel;
	int isize;
{
	register daddr_t *ap;
	register daddr_t *aplim;
	register int i, n;
	int (*func)(), nif;
	BUFAREA ib;

	if (flg == ADDR) {
		func = pfunc;
		if (((n = (*func)(blk, sblock.fs_frag)) & KEEPON) == 0)
			return (n);
	} else
		func = dirscan;
	if (outrange(blk, sblock.fs_frag))		/* protect thyself */
		return (SKIP);
	initbarea(&ib);
	if (getblk(&ib, blk, sblock.fs_bsize) == NULL)
		return (SKIP);
	ilevel--;
	if (ilevel == 0) {
		nif = lblkno(&sblock, isize) + 1;
	} else /* ilevel == 1 */ {
		nif = isize / (sblock.fs_bsize * NINDIR(&sblock)) + 1;
	}
	if (nif > NINDIR(&sblock))
		nif = NINDIR(&sblock);
	aplim = &ib.b_un.b_indir[nif];
	for (ap = ib.b_un.b_indir, i = 1; ap < aplim; ap++, i++)
		if (*ap) {
			if (ilevel > 0)
				n = iblock(*ap, ilevel, flg,
				    isize - i*NINDIR(&sblock)*sblock.fs_bsize);
			else
				n = (*func)(*ap, sblock.fs_frag);
			if (n & STOP)
				return (n);
		}
	return (KEEPON);
}

outrange(blk, cnt)
	daddr_t blk;
	int cnt;
{
	register int c;

	if ((unsigned)(blk+cnt) > fmax)
		return (1);
	c = dtog(&sblock, blk);
	if (blk < cgdmin(&sblock, c)) {
		if ((blk+cnt) > cgsblock(&sblock, c)) {
			if (debug) {
				printf("blk %d < cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > cgsbase %d\n",
				    blk+cnt, cgsblock(&sblock, c));
			}
			return (1);
		}
	} else {
		if ((blk+cnt) > cgbase(&sblock, c+1)) {
			if (debug)  {
				printf("blk %d >= cgdmin %d;",
				    blk, cgdmin(&sblock, c));
				printf(" blk+cnt %d > sblock.fs_fpg %d\n",
				    blk+cnt, sblock.fs_fpg);
			}
			return (1);
		}
	}
	return (0);
}

blkerr(s, blk)
	daddr_t blk;
	char *s;
{

	pfatal("%ld %s I=%u", blk, s, inum);
	printf("\n");
	statemap[inum] = CLEAR;
}

descend()
{
	register DINODE *dp;
	register char *savname;
	off_t savsize;

	statemap[inum] = FSTATE;
	if ((dp = ginode()) == NULL)
		return;
	savname = thisname;
	*pathp++ = '/';
	savsize = filsize;
	filsize = dp->di_size;
	ckinode(dp, DATA);
	thisname = savname;
	*--pathp = 0;
	filsize = savsize;
}

struct dirstuff {
	int loc;
	int blkno;
	int blksiz;
	ino_t number;
	enum {DONTKNOW, NOFIX, FIX} fix;
};

dirscan(blk, nf)
	daddr_t blk;
	int nf;
{
	register DIRECT *dp;
	struct dirstuff dirp;
	int blksiz, dsize, n;
	char dbuf[DIRBLKSIZ];

	if (outrange(blk, 1)) {
		filsize -= sblock.fs_bsize;
		return (SKIP);
	}
	blksiz = nf * sblock.fs_fsize;
	dirp.loc = 0;
	dirp.blkno = blk;
	dirp.blksiz = blksiz;
	if (dirp.number != dnum) {
		dirp.number = dnum;
		dirp.fix = DONTKNOW;
	}
	for (dp = readdir(&dirp); dp != NULL; dp = readdir(&dirp)) {
		dsize = dp->d_reclen;
		bcopy(dp, dbuf, dsize);
		if ((n = (*pfunc)(dbuf)) & ALTERD) {
			if (getblk(&fileblk, blk, blksiz) != NULL) {
				bcopy(dbuf, dp, dsize);
				dirty(&fileblk);
				sbdirty();
			} else
				n &= ~ALTERD;
		}
		if (n & STOP) 
			return (n);
	}
	return (filsize > 0 ? KEEPON : STOP);
}

/*
 * get next entry in a directory.
 */
DIRECT *
readdir(dirp)
	register struct dirstuff *dirp;
{
	register DIRECT *dp, *ndp;
	long size;

	if (getblk(&fileblk, dirp->blkno, dirp->blksiz) == NULL) {
		filsize -= dirp->blksiz - dirp->loc;
		return NULL;
	}
	while (dirp->loc % DIRBLKSIZ == 0 && filsize > 0 &&
	    dirp->loc < dirp->blksiz) {
		dp = (DIRECT *)(dirblk.b_buf + dirp->loc);
	   	if (dp->d_ino < imax &&
		    dp->d_namlen <= MAXNAMLEN && dp->d_namlen >= 0 &&
		    dp->d_reclen > 0 && dp->d_reclen <= DIRBLKSIZ)
			break;
		dirp->loc += DIRBLKSIZ;
		filsize -= DIRBLKSIZ;
		if (dirp->fix == DONTKNOW) {
			pfatal("DIRECTORY %D CORRUPTED", dirp->number);
			dirp->fix = NOFIX;
			if (reply("SALVAGE") != 0)
				dirp->fix = FIX;
		}
		if (dirp->fix != FIX)
			continue;
		dp->d_reclen = DIRBLKSIZ;
		dp->d_ino = 0;
		dp->d_namlen = 0;
		dirty(&fileblk);
	}
	if (filsize <= 0 || dirp->loc >= dirp->blksiz)
		return NULL;
	dp = (DIRECT *)(dirblk.b_buf + dirp->loc);
	dirp->loc += dp->d_reclen;
	filsize -= dp->d_reclen;
	ndp = (DIRECT *)(dirblk.b_buf + dirp->loc);
	if ((filsize <= 0 && dirp->loc % DIRBLKSIZ != 0) ||
	    (dirp->loc < dirp->blksiz && filsize > 0 &&
	    (ndp->d_ino >= imax ||
	    ndp->d_namlen > MAXNAMLEN || ndp->d_namlen < 0 ||
	    ndp->d_reclen <= 0 || 
	    ndp->d_reclen > DIRBLKSIZ - (dirp->loc % DIRBLKSIZ)))) {
		size = DIRBLKSIZ - (dirp->loc % DIRBLKSIZ);
		dirp->loc += size;
		filsize -= size;
		if (dirp->fix == DONTKNOW) {
			pfatal("DIRECTORY %D CORRUPTED", dirp->number);
			dirp->fix = NOFIX;
			if (reply("SALVAGE") != 0)
				dirp->fix = FIX;
		}
		if (dirp->fix == FIX) {
			dp->d_reclen += size;
			dirty(&fileblk);
		}
	}
	return (dp);
}

direrr(s)
	char *s;
{
	register DINODE *dp;

	pwarn("%s ", s);
	pinode();
	printf("\n");
	if ((dp = ginode()) != NULL && ftypeok(dp))
		pfatal("%s=%s", DIRCT?"DIR":"FILE", pathname);
	else
		pfatal("NAME=%s", pathname);
	return (reply("REMOVE"));
}

adjust(lcnt)
	register short lcnt;
{
	register DINODE *dp;

	if ((dp = ginode()) == NULL)
		return;
	if (dp->di_nlink == lcnt) {
		if (linkup() == 0)
			clri("UNREF", 0);
	}
	else {
		pwarn("LINK COUNT %s",
			(lfdir==inum)?lfname:(DIRCT?"DIR":"FILE"));
		pinode();
		printf(" COUNT %d SHOULD BE %d",
			dp->di_nlink, dp->di_nlink-lcnt);
		if (preen) {
			if (lcnt < 0) {
				printf("\n");
				preendie();
			}
			printf(" (ADJUSTED)\n");
		}
		if (preen || reply("ADJUST") == 1) {
			dp->di_nlink -= lcnt;
			inodirty();
		}
	}
}

clri(s, flg)
	char *s;
{
	register DINODE *dp;

	if ((dp = ginode()) == NULL)
		return;
	if (flg == 1) {
		pwarn("%s %s", s, DIRCT?"DIR":"FILE");
		pinode();
	}
	if (preen || reply("CLEAR") == 1) {
		if (preen)
			printf(" (CLEARED)\n");
		n_files--;
		pfunc = pass4check;
		ckinode(dp, ADDR);
		zapino(dp);
		statemap[inum] = USTATE;
		inodirty();
		inosumbad++;
	}
}

badsb(s)
	char *s;
{

	if (preen)
		printf("%s: ", devname);
	printf("BAD SUPER BLOCK: %s\n", s);
	pwarn("USE -b OPTION TO FSCK TO SPECIFY LOCATION OF AN ALTERNATE\n");
	pfatal("SUPER-BLOCK TO SUPPLY NEEDED INFORMATION; SEE fsck(8).\n");
}

DINODE *
ginode()
{
	daddr_t iblk;

	if (inum < ROOTINO || inum > imax) {
		if (debug && (inum < 0 || inum > imax))
			printf("inum out of range (%d)\n", inum);
		return (NULL);
	}
	if (inum < startinum || inum >= startinum + INOPB(&sblock)) {
		iblk = itod(&sblock, inum);
		if (getblk(&inoblk, iblk, sblock.fs_bsize) == NULL) {
			return (NULL);
		}
		startinum = (inum / INOPB(&sblock)) * INOPB(&sblock);
	}
	return (&inoblk.b_un.b_dinode[inum % INOPB(&sblock)]);
}

ftypeok(dp)
	DINODE *dp;
{
	switch (dp->di_mode & IFMT) {

	case IFDIR:
	case IFREG:
	case IFBLK:
	case IFCHR:
	case IFLNK:
		return (1);

	default:
		if (debug)
			printf("bad file type 0%o\n", dp->di_mode);
		return (0);
	}
}

reply(s)
	char *s;
{
	char line[80];

	if (preen)
		pfatal("INTERNAL ERROR: GOT TO reply()");
	rplyflag = 1;
	printf("\n%s? ", s);
	if (nflag || dfile.wfdes < 0) {
		printf(" no\n\n");
		return (0);
	}
	if (yflag) {
		printf(" yes\n\n");
		return (1);
	}
	if (getline(stdin, line, sizeof(line)) == EOF)
		errexit("\n");
	printf("\n");
	if (line[0] == 'y' || line[0] == 'Y')
		return (1);
	else
		return (0);
}

getline(fp, loc, maxlen)
	FILE *fp;
	char *loc;
{
	register n;
	register char *p, *lastloc;

	p = loc;
	lastloc = &p[maxlen-1];
	while ((n = getc(fp)) != '\n') {
		if (n == EOF)
			return (EOF);
		if (!isspace(n) && p < lastloc)
			*p++ = n;
	}
	*p = 0;
	return (p - loc);
}

BUFAREA *
getblk(bp, blk, size)
	daddr_t blk;
	register BUFAREA *bp;
	int size;
{
	register struct filecntl *fcp;
	daddr_t dblk;

	fcp = &dfile;
	dblk = fsbtodb(&sblock, blk);
	if (bp->b_bno == dblk)
		return (bp);
	flush(fcp, bp);
	if (bread(fcp, bp->b_un.b_buf, dblk, size) != 0) {
		bp->b_bno = dblk;
		bp->b_size = size;
		return (bp);
	}
	bp->b_bno = (daddr_t)-1;
	return (NULL);
}

flush(fcp, bp)
	struct filecntl *fcp;
	register BUFAREA *bp;
{

	if (bp->b_dirty)
		bwrite(fcp, bp->b_un.b_buf, bp->b_bno, bp->b_size);
	bp->b_dirty = 0;
}

rwerr(s, blk)
	char *s;
	daddr_t blk;
{

	if (preen == 0)
		printf("\n");
	pfatal("CANNOT %s: BLK %ld", s, blk);
	if (reply("CONTINUE") == 0)
		errexit("Program terminated\n");
}

ckfini()
{

	flush(&dfile, &fileblk);
	flush(&dfile, &sblk);
	if (sblk.b_bno != SBLOCK) {
		sblk.b_bno = SBLOCK;
		sbdirty();
		flush(&dfile, &sblk);
	}
	flush(&dfile, &inoblk);
	close(dfile.rfdes);
	close(dfile.wfdes);
}

pinode()
{
	register DINODE *dp;
	register char *p;
	char uidbuf[BUFSIZ];
	char *ctime();

	printf(" I=%u ", inum);
	if ((dp = ginode()) == NULL)
		return;
	printf(" OWNER=");
	if (getpw((int)dp->di_uid, uidbuf) == 0) {
		for (p = uidbuf; *p != ':'; p++);
		*p = 0;
		printf("%s ", uidbuf);
	}
	else {
		printf("%d ", dp->di_uid);
	}
	printf("MODE=%o\n", dp->di_mode);
	if (preen)
		printf("%s: ", devname);
	printf("SIZE=%ld ", dp->di_size);
	p = ctime(&dp->di_mtime);
	printf("MTIME=%12.12s %4.4s ", p+4, p+20);
}

makecg()
{
	int c, blk;
	daddr_t dbase, d, dlower, dupper, dmax;
	long i, j, s;
	register struct csum *cs;
	register DINODE *dp;

	sblock.fs_cstotal.cs_nbfree = 0;
	sblock.fs_cstotal.cs_nffree = 0;
	sblock.fs_cstotal.cs_nifree = 0;
	sblock.fs_cstotal.cs_ndir = 0;
	for (c = 0; c < sblock.fs_ncg; c++) {
		dbase = cgbase(&sblock, c);
		dmax = dbase + sblock.fs_fpg;
		if (dmax > sblock.fs_size) {
			for ( ; dmax >= sblock.fs_size; dmax--)
				clrbit(cgrp.cg_free, dmax - dbase);
			dmax++;
		}
		dlower = cgsblock(&sblock, c) - dbase;
		dupper = cgdmin(&sblock, c) - dbase;
		cs = &sblock.fs_cs(&sblock, c);
		cgrp.cg_time = time(0);
		cgrp.cg_magic = CG_MAGIC;
		cgrp.cg_cgx = c;
		if (c == sblock.fs_ncg - 1)
			cgrp.cg_ncyl = sblock.fs_ncyl % sblock.fs_cpg;
		else
			cgrp.cg_ncyl = sblock.fs_cpg;
		cgrp.cg_niblk = sblock.fs_ipg;
		cgrp.cg_ndblk = dmax - dbase;
		cgrp.cg_cs.cs_ndir = 0;
		cgrp.cg_cs.cs_nffree = 0;
		cgrp.cg_cs.cs_nbfree = 0;
		cgrp.cg_cs.cs_nifree = 0;
		cgrp.cg_rotor = 0;
		cgrp.cg_frotor = 0;
		cgrp.cg_irotor = 0;
		for (i = 0; i < sblock.fs_frag; i++)
			cgrp.cg_frsum[i] = 0;
		inum = sblock.fs_ipg * c;
		for (i = 0; i < sblock.fs_ipg; inum++, i++) {
			cgrp.cg_cs.cs_nifree++;
			clrbit(cgrp.cg_iused, i);
			dp = ginode();
			if (dp == NULL)
				continue;
			if (ALLOC) {
				if (DIRCT)
					cgrp.cg_cs.cs_ndir++;
				cgrp.cg_cs.cs_nifree--;
				setbit(cgrp.cg_iused, i);
				continue;
			}
		}
		while (i < MAXIPG) {
			clrbit(cgrp.cg_iused, i);
			i++;
		}
		if (c == 0)
			for (i = 0; i < ROOTINO; i++) {
				setbit(cgrp.cg_iused, i);
				cgrp.cg_cs.cs_nifree--;
			}
		for (s = 0; s < MAXCPG; s++) {
			cgrp.cg_btot[s] = 0;
			for (i = 0; i < NRPOS; i++)
				cgrp.cg_b[s][i] = 0;
		}
		if (c == 0) {
			dupper += howmany(sblock.fs_cssize, sblock.fs_fsize);
		}
		for (d = dlower; d < dupper; d++)
			clrbit(cgrp.cg_free, d);
		for (d = 0; (d + sblock.fs_frag) <= dmax - dbase;
		    d += sblock.fs_frag) {
			j = 0;
			for (i = 0; i < sblock.fs_frag; i++) {
				if (!getbmap(dbase + d + i)) {
					setbit(cgrp.cg_free, d + i);
					j++;
				} else
					clrbit(cgrp.cg_free, d+i);
			}
			if (j == sblock.fs_frag) {
				cgrp.cg_cs.cs_nbfree++;
				cgrp.cg_btot[cbtocylno(&sblock, d)]++;
				cgrp.cg_b[cbtocylno(&sblock, d)]
				    [cbtorpos(&sblock, d)]++;
			} else if (j > 0) {
				cgrp.cg_cs.cs_nffree += j;
				blk = blkmap(&sblock, cgrp.cg_free, d);
				fragacct(&sblock, blk, cgrp.cg_frsum, 1);
			}
		}
		for (j = d; d < dmax - dbase; d++) {
			if (!getbmap(dbase + d)) {
				setbit(cgrp.cg_free, d);
				cgrp.cg_cs.cs_nffree++;
			} else
				clrbit(cgrp.cg_free, d);
		}
		for (; d % sblock.fs_frag != 0; d++)
			clrbit(cgrp.cg_free, d);
		if (j != d) {
			blk = blkmap(&sblock, cgrp.cg_free, j);
			fragacct(&sblock, blk, cgrp.cg_frsum, 1);
		}
		for (d /= sblock.fs_frag; d < MAXBPG(&sblock); d ++)
			clrblock(&sblock, cgrp.cg_free, d);
		sblock.fs_cstotal.cs_nffree += cgrp.cg_cs.cs_nffree;
		sblock.fs_cstotal.cs_nbfree += cgrp.cg_cs.cs_nbfree;
		sblock.fs_cstotal.cs_nifree += cgrp.cg_cs.cs_nifree;
		sblock.fs_cstotal.cs_ndir += cgrp.cg_cs.cs_ndir;
		*cs = cgrp.cg_cs;
		bwrite(&dfile, &cgrp, fsbtodb(&sblock, cgtod(&sblock, c)),
		    sblock.fs_cgsize);
	}
	for (i = 0, j = 0; i < sblock.fs_cssize; i += sblock.fs_bsize, j++) {
		bwrite(&dfile, (char *)sblock.fs_csp[j],
		    fsbtodb(&sblock, sblock.fs_csaddr + j * sblock.fs_frag),
		    sblock.fs_cssize - i < sblock.fs_bsize ?
		    sblock.fs_cssize - i : sblock.fs_bsize);
	}
	sblock.fs_ronly = 0;
	sblock.fs_fmod = 0;
	sbdirty();
}

findino(dirp)
	register DIRECT *dirp;
{
	if (dirp->d_ino == 0)
		return (KEEPON);
	if (!strcmp(dirp->d_name, srchname)) {
		if (dirp->d_ino >= ROOTINO && dirp->d_ino <= imax)
			parentdir = dirp->d_ino;
		return (STOP);
	}
	return (KEEPON);
}

mkentry(dirp)
	register DIRECT *dirp;
{
	register ino_t in;
	register char *p;
	DIRECT newent;
	int newlen, oldlen;

	newent.d_namlen = 11;
	newlen = DIRSIZ(&newent);
	if (dirp->d_ino != 0)
		oldlen = DIRSIZ(dirp);
	else
		oldlen = 0;
	if (dirp->d_reclen - oldlen < newlen)
		return (KEEPON);
	newent.d_reclen = dirp->d_reclen - oldlen;
	dirp->d_reclen = oldlen;
	dirp = (struct direct *)(((char *)dirp) + oldlen);
	dirp->d_ino = orphan;
	dirp->d_reclen = newent.d_reclen;
	p = &dirp->d_name[2];
	for (in = imax; in > 0; in /= 10)
		p++;
	*--p = 0;
	dirp->d_namlen = p - dirp->d_name;
	in = orphan;
	while (p > dirp->d_name) {
		*--p = (in % 10) + '0';
		in /= 10;
	}
	*p = '#';
	return (ALTERD|STOP);
}

chgdd(dirp)
	register DIRECT *dirp;
{
	if (dirp->d_name[0] == '.' && dirp->d_name[1] == '.' &&
	dirp->d_name[2] == 0) {
		dirp->d_ino = lfdir;
		return (ALTERD|STOP);
	}
	return (KEEPON);
}

linkup()
{
	register DINODE *dp;
	register lostdir;
	register ino_t pdir;

	if ((dp = ginode()) == NULL)
		return (0);
	lostdir = DIRCT;
	pdir = parentdir;
	pwarn("UNREF %s ", lostdir ? "DIR" : "FILE");
	pinode();
	if (preen && dp->di_size == 0)
		return (0);
	if (preen)
		printf(" (RECONNECTED)\n");
	else
		if (reply("RECONNECT") == 0)
			return (0);
	orphan = inum;
	if (lfdir == 0) {
		inum = ROOTINO;
		if ((dp = ginode()) == NULL) {
			inum = orphan;
			return (0);
		}
		pfunc = findino;
		srchname = lfname;
		filsize = dp->di_size;
		parentdir = 0;
		ckinode(dp, DATA);
		inum = orphan;
		if ((lfdir = parentdir) == 0) {
			pfatal("SORRY. NO lost+found DIRECTORY");
			printf("\n\n");
			return (0);
		}
	}
	inum = lfdir;
	if ((dp = ginode()) == NULL || !DIRCT || statemap[inum] != FSTATE) {
		inum = orphan;
		pfatal("SORRY. NO lost+found DIRECTORY");
		printf("\n\n");
		return (0);
	}
	if (fragoff(&sblock, dp->di_size)) {
		dp->di_size = fragroundup(&sblock, dp->di_size);
		inodirty();
	}
	filsize = dp->di_size;
	inum = orphan;
	pfunc = mkentry;
	if ((ckinode(dp, DATA) & ALTERD) == 0) {
		pfatal("SORRY. NO SPACE IN lost+found DIRECTORY");
		printf("\n\n");
		return (0);
	}
	lncntp[inum]--;
	if (lostdir) {
		pfunc = chgdd;
		dp = ginode();
		filsize = dp->di_size;
		ckinode(dp, DATA);
		inum = lfdir;
		if ((dp = ginode()) != NULL) {
			dp->di_nlink++;
			inodirty();
			lncntp[inum]++;
		}
		inum = orphan;
		pwarn("DIR I=%u CONNECTED. ", orphan);
		printf("PARENT WAS I=%u\n", pdir);
		if (preen == 0)
			printf("\n");
	}
	return (1);
}

bread(fcp, buf, blk, size)
	daddr_t blk;
	register struct filecntl *fcp;
	register size;
	char *buf;
{
	if (lseek(fcp->rfdes, blk * DEV_BSIZE, 0) < 0)
		rwerr("SEEK", blk);
	else if (read(fcp->rfdes, buf, size) == size)
		return (1);
	rwerr("READ", blk);
	return (0);
}

bwrite(fcp, buf, blk, size)
	daddr_t blk;
	register struct filecntl *fcp;
	register size;
	char *buf;
{

	if (fcp->wfdes < 0)
		return (0);
	if (lseek(fcp->wfdes, blk * DEV_BSIZE, 0) < 0)
		rwerr("SEEK", blk);
	else if (write(fcp->wfdes, buf, size) == size) {
		fcp->mod = 1;
		return (1);
	}
	rwerr("WRITE", blk);
	return (0);
}

catch()
{

	ckfini();
	exit(12);
}

char *
unrawname(cp)
	char *cp;
{
	char *dp = rindex(cp, '/');
	struct stat stb;

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);
	if (*(dp+1) != 'r')
		return (cp);
	strcpy(dp+1, dp+2);
	return (cp);
}

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[32];
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	strcpy(rawbuf, cp);
	*dp = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp+1);
	return (rawbuf);
}

/* VARARGS1 */
error(s1, s2, s3, s4)
	char *s1;
{

	printf(s1, s2, s3, s4);
}

/* VARARGS1 */
errexit(s1, s2, s3, s4)
	char *s1;
{
	error(s1, s2, s3, s4);
	exit(8);
}

/*
 * An inconsistency occured which shouldn't during normal operations.
 * Die if preening, otherwise just printf.
 */
/* VARARGS1 */
pfatal(s, a1, a2, a3)
	char *s;
{

	if (preen) {
		printf("%s: ", devname);
		printf(s, a1, a2, a3);
		printf("\n");
		preendie();
	}
	printf(s, a1, a2, a3);
}

preendie()
{

	printf("%s: UNEXPECTED INCONSISTENCY; RUN fsck MANUALLY.\n", devname);
	exit(8);
}

/*
 * Pwarn is like printf when not preening,
 * or a warning (preceded by filename) when preening.
 */
/* VARARGS1 */
pwarn(s, a1, a2, a3, a4, a5, a6)
	char *s;
{

	if (preen)
		printf("%s: ", devname);
	printf(s, a1, a2, a3, a4, a5, a6);
}

panic(s)
	char *s;
{

	pfatal("internal inconsistency: %s\n");
	exit(12);
}
