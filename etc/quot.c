static char *sccsid = "@(#)quot.c	4.3 (Berkeley) 82/10/24";

/*
 * quot
 */

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/inode.h>
#include <sys/fs.h>

#define	ISIZ	(MAXBSIZE/sizeof(struct dinode))
#define	NUID	1000
union {
	struct fs u_sblock;
	char dummy[SBSIZE];
} sb_un;
#define sblock sb_un.u_sblock
struct dinode itab[MAXBSIZE/sizeof(struct dinode)];
struct du
{
	long	blocks;
	long	blocks30,blocks60,blocks90;
	long	nfiles;
	int	uid;
	char	*name;
} du[NUID];
#define	TSIZE	500
int	sizes[TSIZE];
long	overflow;

int	nflg;
int	fflg;
int	cflg;
int	vflg;
int	hflg;
long	now;

int	fi;
unsigned	ino;
unsigned	nfiles;

struct	passwd	*getpwent();
char	*malloc();
char	*copy();

main(argc, argv)
char **argv;
{
	register int n;
	register struct passwd *lp;
	register char **p;

	now = time(0);
	for(n=0; n<NUID; n++)
		du[n].uid = n;
	while((lp=getpwent()) != 0) {
		n = lp->pw_uid;
		if (n>NUID)
			continue;
		if(du[n].name)
			continue;
		du[n].name = copy(lp->pw_name);
	}
	if (argc == 1) {
		fprintf(stderr, "usage: df device ...\n");
		exit(1);
	}
	while (--argc) {
		argv++;
		if (argv[0][0]=='-') {
			if (argv[0][1]=='n')
				nflg++;
			else if (argv[0][1]=='f')
				fflg++;
			else if (argv[0][1]=='c')
				cflg++;
			else if (argv[0][1]=='v')
				vflg++;
			else if (argv[0][1]=='h')
				hflg++;
		} else {
			check(*argv);
			report();
		}
	}
	return(0);
}

check(file)
char *file;
{
	register unsigned i, j;
	daddr_t iblk;
	int c;

	fi = open(file, 0);
	if (fi < 0) {
		printf("cannot open %s\n", file);
		return;
	}
	printf("%s:\n", file);
	sync();
	bread(SBLOCK, (char *)&sblock, SBSIZE);
	if (nflg) {
		if (isdigit(c = getchar()))
			ungetc(c, stdin);
		else while (c!='\n' && c != EOF)
			c = getchar();
	}
	nfiles = sblock.fs_ipg * sblock.fs_ncg;
	for (ino = 0; ino < nfiles; ) {
		iblk = fsbtodb(&sblock, itod(&sblock, ino));
		bread(iblk, (char *)itab, sblock.fs_bsize);
		for (j = 0; j < INOPB(&sblock) && ino < nfiles; j++) {
			if (ino++ < ROOTINO)
				continue;
			acct(&itab[j]);
		}
	}
}

acct(ip)
	register struct dinode *ip;
{
	register char *np;
	long blks, frags, size;
	char n;
	static fino;

	if ((ip->di_mode&IFMT) == 0)
		return;
	if (!hflg) {
		/*
		 * Assume that there are no holes in files.
		 */
		blks = lblkno(&sblock, ip->di_size);
		frags = blks * sblock.fs_frag +
			numfrags(&sblock, dblksize(&sblock, ip, blks));
	} else {
		/*
		 * Actually go out and count the number of allocated blocks.
		 */
		printf("Sorry, hard way not implemented yet...\n");
		exit(1);
	}
	size = frags * sblock.fs_fsize / 1024;
	if (cflg) {
		if ((ip->di_mode&IFMT)!=IFDIR && (ip->di_mode&IFMT)!=IFREG)
			return;
		if (size >= TSIZE) {
			overflow += size;
			size = TSIZE-1;
		}
		sizes[size]++;
		return;
	}
	if (ip->di_uid >= NUID)
		return;
	du[ip->di_uid].blocks += size;
#define	DAY (60 * 60 * 24)	/* seconds per day */
	if (now - ip->di_atime > 30 * DAY)
		du[ip->di_uid].blocks30 += size;
	if (now - ip->di_atime > 60 * DAY)
		du[ip->di_uid].blocks60 += size;
	if (now - ip->di_atime > 90 * DAY)
		du[ip->di_uid].blocks90 += size;
	du[ip->di_uid].nfiles++;
	if (nflg) {
	tryagain:
		if (fino==0)
			if (scanf("%d", &fino)<=0)
				return;
		if (fino > ino)
			return;
		if (fino<ino) {
			while ((n=getchar())!='\n' && n!=EOF)
				;
			fino = 0;
			goto tryagain;
		}
		if (np = du[ip->di_uid].name)
			printf("%.7s	", np);
		else
			printf("%d	", ip->di_uid);
		while ((n = getchar())==' ' || n=='\t')
			;
		putchar(n);
		while (n!=EOF && n!='\n') {
			n = getchar();
			putchar(n);
		}
		fino = 0;
	}
}

bread(bno, buf, cnt)
unsigned bno;
char *buf;
{

	lseek(fi, (long)bno*DEV_BSIZE, 0);
	if (read(fi, buf, cnt) != cnt) {
		printf("read error %u\n", bno);
		exit(1);
	}
}

qcmp(p1, p2)
register struct du *p1, *p2;
{
	if (p1->blocks > p2->blocks)
		return(-1);
	if (p1->blocks < p2->blocks)
		return(1);
	return(strcmp(p1->name, p2->name));
}

report()
{
	register i;

	if (nflg)
		return;
	if (cflg) {
		long t = 0;
		for (i=0; i<TSIZE-1; i++)
			if (sizes[i]) {
				t += i*sizes[i];
				printf("%d	%d	%D\n", i, sizes[i], t);
			}
		printf("%d	%d	%D\n", TSIZE-1, sizes[TSIZE-1], overflow+t);
		return;
	}
	qsort(du, NUID, sizeof(du[0]), qcmp);
	for (i=0; i<NUID; i++) {
		if (du[i].blocks==0)
			return;
		printf("%5D\t", du[i].blocks);
		if (fflg)
			printf("%5D\t", du[i].nfiles);
		if (du[i].name)
			printf("%-8.8s", du[i].name);
		else
			printf("#%-8d", du[i].uid);
		if (vflg)
			printf("\t%5D\t%5D\t%5D",
			    du[i].blocks30, du[i].blocks60, du[i].blocks90);
		printf("\n");
	}
}

char *
copy(s)
char *s;
{
	register char *p;
	register n;

	for(n=0; s[n]; n++)
		;
	p = malloc((unsigned)n+1);
	for(n=0; p[n] = s[n]; n++)
		;
	return(p);
}
