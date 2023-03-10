/*	format.c	4.3	83/03/02	*/

/* 
 * Standalone program to do media checking
 * and record bad block information on any 
 * disk with the appropriate driver.
 */
#include "../h/param.h"
#include "../h/fs.h"
#include "../h/inode.h"
#include "../h/dkbad.h"
#include "../h/vmmac.h"

#include "saio.h"
#include "savax.h"

#define MAXBADDESC	126		/* size of bad block table */
#define CHUNK		48		/* max # of sectors/io operation */
#define SECTSIZ		512		/* standard sector size */
#define HDRSIZ		4		/* number of bytes in sector header */

#define SSERR		0
#define BSERR		1

#define SSDEV		((ioctl(iob[fd-3], SAIOSSDEV, (char *)0) == 0))

struct sector {
	u_short	header1;
	u_short header2;
	char	buf[SECTSIZ];
};

struct	dkbad dkbad;		/* bad sector table */
struct	dkbad sstab;		/* skip sector table */

#define	NERRORS		6
static char *
errornames[NERRORS] = {
#define	FE_WCE		0
	"Write check",
#define	FE_BSE		1
	"Bad sector",
#define	FE_ECC		2
	"ECC",
#define	FE_HARD		3
	"Other hard",
#define	FE_TOTAL	4
	"Total",
#define	FE_SSE		5
	"Skip sector",
};

int	errors[NERRORS];	/* histogram of errors */
int	pattern;

char	*malloc();
char	*prompt();
extern	int end;

main()
{
	register int sector, sn;
	int lastsector, tracksize;
	int unit, fd, resid, i, trk, cyl, debug;
	struct st st;
	struct sector *bp, *cbp;
	char *cp;

	printf("Disk format/check utility\n\n");

again:
	cp = prompt("Enable debugging (1=bse, 2=ecc, 3=bse+ecc)? ");
	debug = atoi(cp);
	if (debug < 0)
		debug = 0;
	for (i = 0; i < NERRORS; i++)
		errors[i] = 0;
	fd = getdevice();
	ioctl(fd, SAIODEVDATA, &st);
	printf("Device data: #cylinders=%d, #tracks=%d, #sectors=%d\n",
	  st.ncyl, st.ntrak, st.nsect);
	if (getpattern())
		goto again;
	printf("Start formatting...make sure the drive is online\n");
	ioctl(fd, SAIONOBAD, (char *)0);
	ioctl(fd, SAIOECCLIM, (char *)0);
	ioctl(fd, SAIODEBUG, (char *)debug);
	if (SSDEV) {
		ioctl(fd, SAIOSSI, (char *)0);	/* set skip sector inhibit */
		st.nsect++;
		st.nspc += st.ntrak;
	}
	tracksize = sizeof (struct sector) * st.nsect;
	bp = (struct sector *)malloc(tracksize);
	bufinit(bp, tracksize);
	/*
	 * Begin check, for each track,
	 *
	 * 1) Write header and test pattern.
	 * 2) Write check header and data.
	 */
	lastsector = st.nspc * st.ncyl;
	for (sector = 0; sector < lastsector; sector += st.nsect) {
		cyl = sector / st.nspc;
		trk = (sector % st.nspc) / st.nsect;
		for (i = 0; i < st.nsect; i++) {
			bp[i].header1 =
				(u_short) cyl | HDR1_FMT22 | HDR1_OKSCT;
			bp[i].header2 = ((u_short)trk << 8) + i;
		}
		if (sector && (sector % (st.nspc * 10)) == 0)
			printf("sector %d\n", sector);
		/*
		 * Try and write the headers and data patterns into
		 * each sector in the track.  Continue until such
		 * we're done, or until there's less than a sector's
		 * worth of data to transfer.
		 *
		 * The lseek call is necessary because of
		 * the odd sector size (516 bytes)
		 */
		for (resid = tracksize, cbp = bp, sn = sector;;) {
			int cc;

			lseek(fd, sn * SECTSIZ, 0);
			ioctl(fd, SAIOHDR, (char *)0);
			cc = write(fd, cbp, resid);
			if (cc == resid)
				break;
			/*
			 * Don't record errors during write,
			 * all errors will be found during
			 * writecheck performed below.
			 */
			sn = iob[fd - 3].i_errblk;
			cbp += sn - sector;
			resid -= (sn - sector) * sizeof (struct sector);
			if (resid < sizeof (struct sector)) 
				break;
		}
		/*
		 * Write check headers and test patterns.
		 * Retry remainder of track on error until
		 * we're done, or until there's less than a
		 * sector to verify.
		 */
		for (resid = tracksize, cbp = bp, sn = sector;;) {
			int cc;

			lseek(fd, sn * SECTSIZ, 0);
			ioctl(fd, SAIOHCHECK, (char *)0);
			cc = read(fd, cbp, resid);
			if (cc == resid)
				break;
			sn = iob[fd-3].i_errblk;
			printf("sector %d, write check error\n", sn);
			recorderror(fd, sn, &st);
			/* advance past bad sector */
			sn++;
			cbp += sn - sector;
			resid -= (sn - sector) * sizeof (struct sector);
			if (resid < sizeof (struct sector)) 
				break;
		}
	}
	/*
	 * Checking finished.
	 */
	if (errors[FE_TOTAL] || errors[FE_SSE]) {
		printf("Errors:\n");
		for (i = 0; i < NERRORS; i++)
			printf("%s: %d\n", errornames[i], errors[i]);
		printf("Total of %d hard errors found\n",
			errors[FE_TOTAL] + errors[FE_SSE]);
		/* change the headers of all the bad sectors */
		writebb(fd, errors[FE_SSE], &sstab, &st, SSERR);
		writebb(fd, errors[FE_TOTAL], &dkbad, &st, BSERR);
	}
	while (errors[FE_TOTAL] < MAXBADDESC) {
		int i = errors[FE_TOTAL]++;

		dkbad.bt_bad[i].bt_cyl = -1;
		dkbad.bt_bad[i].bt_trksec = -1;
	}
	printf("\nWriting bad sector table at sector #%d\n",
		st.ncyl * st.nspc - st.nsect);
	/* place on disk */
	for (i = 0; i < 10; i += 2) {
		lseek(fd, SECTSIZ * (st.ncyl * st.nspc - st.nsect + i), 0);
		write(fd, &dkbad, sizeof (dkbad));
	}
	printf("Done\n");
	ioctl(fd,SAIONOSSI,(char *)0);
	close(fd);
#ifndef JUSTEXIT
	goto again;
#endif
}

/*
 * Write out the bad blocks.
 */
writebb(fd, nsects, dbad, st, sw)
	int nsects, fd;
	struct dkbad *dbad;
	register struct st *st;
{
	struct sector bb_buf; /* buffer for one sector plus 4 byte header */
	register int i;
	int bn, j;
	struct bt_bad *btp;

	for (i = 0; i < nsects; i++) {
		btp = &dbad->bt_bad[i];
		if (sw == BSERR) {
			bb_buf.header1 = HDR1_FMT22|btp->bt_cyl;
			if (SSDEV)
				bb_buf.header1 |= HDR1_SSF;
		} else
			bb_buf.header1 =
			       btp->bt_cyl | HDR1_FMT22 | HDR1_SSF | HDR1_OKSCT;
		bb_buf.header2 = btp->bt_trksec;
		bn = st->nspc * btp->bt_cyl +
		     st->nsect * (btp->bt_trksec >> 8) +
		     (btp->bt_trksec & 0x1f);
		lseek(fd, bn * SECTSIZ, 0);
		ioctl(fd, SAIOHDR, (char *)0);
		write(fd, &bb_buf, sizeof (bb_buf));
		if (!SSDEV)
			continue;
		/*
		 * If skip sector, mark all remaining
		 * sectors on the track.
		 */
		for (j = (btp->bt_trksec & 0x1f) + 1; j < st->nsect; j++) {
			bb_buf.header1 = j | HDR1_FMT22 | HDR1_SSF;
			ioctl(fd, SAIOHDR, (char *)0);
			write(fd, &bb_buf, sizeof (bb_buf));
		}
	}
}

/*
 * Record an error, and if there's room, put
 * it in the appropriate bad sector table.
 */
recorderror(fd, bn, st)
	int fd, bn;
	register struct st *st;
{
	int cn, tn, sn, strk;

	if (errors[FE_TOTAL] >= MAXBADDESC) {
		printf("Too many bad sectors\n");
		return;
	}
	if (errors[FE_SSE] >= MAXBADDESC) {
		printf("Too many skip sector errors\n");
		return;
	}
	if (errno <= ECMD || errno > EHER)
		return;
	errors[errno]++;
	cn = bn / st->nspc;
	sn = bn % st->nspc;
	tn = sn / st->nsect;
	sn %= st->nsect;
	if (SSDEV) {		/* if drive has skip sector capability */
		int ss = errors[FE_SSE]++;

		if (ss)
			strk = sstab.bt_bad[ss - 1].bt_trksec >> 8;
		else
			strk = -1;
		if (tn != strk) {	  /* only one skip sector/track */
			sstab.bt_bad[ss].bt_cyl = cn;
			sstab.bt_bad[ss].bt_trksec = (tn<<8) + sn;
			return;
		}
		cn = -cn;
	}
	/* record the bad sector address and continue */
	dkbad.bt_bad[errors[FE_TOTAL]++].bt_cyl = cn;
	dkbad.bt_bad[errors[FE_TOTAL]++].bt_trksec = (tn << 8) + sn;
}

/*
 * Allocate memory on a page-aligned address.
 * Round allocated chunk to a page multiple to
 * ease next request.
 */
char *
malloc(size)
	int size;
{
	char *result;
	static caddr_t last = 0;

	if (last == 0)
		last = (caddr_t)(((int)&end + 511) & ~0x1ff);
	size = (size + 511) & ~0x1ff;
	result = (char *)last;
	last += size;
	return (result);
}

/*
 * Prompt and verify a device name from the user.
 */
getdevice()
{
	register char *cp;
	register struct devsw *dp;
	int fd;

top:
	cp = prompt("Device to format? ");
	if ((fd = open(cp, 2)) < 0) {
		printf("Known devices are: ");
		for (dp = devsw; dp->dv_name; dp++)
			printf("%s ",dp->dv_name);
		printf("\n");
		goto top;
	}
	printf("Formatting drive %d on %c%c%d ",
		iob[fd - 3].i_unit % 8, cp[0], cp[1], iob[fd - 3].i_unit / 8);
	cp = prompt("verify (yes/no)? ");
	while (*cp != 'y' && *cp != 'n')
		cp = prompt("Huh, yes or no? ");
	if (*cp == 'y')
		return (fd);
	goto top;
}

static struct pattern {
	long	pa_value;
	char	*pa_name;
} pat[] = {
	{ 0xf00ff00f, 	"RH750 worst case" },
	{ 0xec6dec6d,	"media worst case" },
	{ 0xa5a5a5a5,	"alternate 1's and 0's" },
	{ 0, 0 },
};

getpattern()
{
	register struct pattern *p;
	int npatterns;
	char *cp;

	printf("Available test patterns are:\n");
	for (p = pat; p->pa_value; p++)
		printf("\t%d - (%x) %s\n", (p - pat) + 1,
		  p->pa_value & 0xffff, p->pa_name);
	npatterns = p - pat;
	cp = prompt("Pattern (one of the above, other to restart)? ");
	pattern = atoi(cp) - 1;
	return (pattern < 0 || pattern >= npatterns);
}

struct xsect {
	u_short	hd1;
	u_short	hd2;
	long	buf[128];
};

/*
 * Initialize the buffer with the requested pattern. 
 */
bufinit(bp, size)
	register struct xsect *bp;
	int size;
{
	register struct pattern *pptr;
	register long *pp, *last;
	register struct xsect *lastbuf;

	size /= sizeof (struct sector);
	lastbuf = bp + size;
	pptr = &pat[pattern];
	while (bp < lastbuf) {
		last = &bp->buf[128];
		for (pp = bp->buf; pp < last; pp++)
			*pp = pptr->pa_value;
		bp++;
	}
}

char *
prompt(msg)
	char *msg;
{
	static char buf[132];

	printf("%s", msg);
	gets(buf);
	return (buf);
}
