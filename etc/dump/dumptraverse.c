static	char *sccsid = "@(#)dumptraverse.c	1.13 (Berkeley) 2/11/83";

#include "dump.h"

pass(fn, map)
	int (*fn)();
	char *map;
{
	struct dinode *dp;
	int bits;
	ino_t maxino;

	maxino = sblock->fs_ipg * sblock->fs_ncg - 1;
	for (ino = 0; ino < maxino; ) {
		if((ino % NBBY) == 0) {
			bits = ~0;
			if(map != NULL)
				bits = *map++;
		}
		ino++;
		if(bits & 1) {
			dp = getino(ino);
			(*fn)(dp);
		}
		bits >>= 1;
	}
}

mark(ip)
	struct dinode *ip;
{
	register f;

	f = ip->di_mode & IFMT;
	if(f == 0)
		return;
	BIS(ino, clrmap);
	if(f == IFDIR)
		BIS(ino, dirmap);
	if ((ip->di_mtime >= spcl.c_ddate || ip->di_ctime >= spcl.c_ddate) &&
	    !BIT(ino, nodmap)) {
		BIS(ino, nodmap);
		if (f != IFREG && f != IFDIR && f != IFLNK) {
			esize += 1;
			return;
		}
		est(ip);
	}
}

add(ip)
	register struct	dinode	*ip;
{
	register int i;

	if(BIT(ino, nodmap))
		return;
	nsubdir = 0;
	dadded = 0;
	for (i = 0; i < NDADDR; i++) {
		if (ip->di_db[i] != 0)
			dsrch(ip->di_db[i], dblksize(sblock, ip, i));
	}
	for (i = 0; i < NIADDR; i++) {
		if (ip->di_ib[i] != 0)
			indir(ip->di_ib[i], i);
	}
	if(dadded) {
		nadded++;
		if (!BIT(ino, nodmap)) {
			BIS(ino, nodmap);
			est(ip);
		}
	}
	if(nsubdir == 0)
		if(!BIT(ino, nodmap))
			BIC(ino, dirmap);
}

indir(d, n)
	daddr_t d;
	int n;
{
	register i;
	daddr_t	idblk[MAXNINDIR];

	bread(fsbtodb(sblock, d), (char *)idblk, sblock->fs_bsize);
	if(n <= 0) {
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				dsrch(d, sblock->fs_bsize);
		}
	} else {
		n--;
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				indir(d, n);
		}
	}
}

dump(ip)
	struct dinode *ip;
{
	register int i;
	long size;

	if(newtape) {
		newtape = 0;
		bitmap(nodmap, TS_BITS);
	}
	BIC(ino, nodmap);
	spcl.c_dinode = *ip;
	spcl.c_type = TS_INODE;
	spcl.c_count = 0;
	i = ip->di_mode & IFMT;
	if ((i != IFDIR && i != IFREG && i != IFLNK) || ip->di_size == 0) {
		spclrec();
		return;
	}
	if (ip->di_size > NDADDR * sblock->fs_bsize)
		i = NDADDR * sblock->fs_frag;
	else
		i = howmany(ip->di_size, sblock->fs_fsize);
	blksout(&ip->di_db[0], i);
	size = ip->di_size - NDADDR * sblock->fs_bsize;
	if (size <= 0)
		return;
	for (i = 0; i < NIADDR; i++) {
		dmpindir(ip->di_ib[i], i, &size);
		if (size <= 0)
			return;
	}
}

dmpindir(blk, lvl, size)
	daddr_t blk;
	int lvl;
	long *size;
{
	int i, cnt;
	daddr_t idblk[MAXNINDIR];

	if (blk != 0)
		bread(fsbtodb(sblock, blk), (char *)idblk, sblock->fs_bsize);
	else
		bzero(idblk, sblock->fs_bsize);
	if (lvl <= 0) {
		if (*size < NINDIR(sblock) * sblock->fs_bsize)
			cnt = howmany(*size, sblock->fs_fsize);
		else
			cnt = NINDIR(sblock) * sblock->fs_frag;
		*size -= NINDIR(sblock) * sblock->fs_bsize;
		blksout(&idblk[0], cnt);
		return;
	}
	lvl--;
	for (i = 0; i < NINDIR(sblock); i++) {
		dmpindir(idblk[i], lvl, size);
		if (*size <= 0)
			return;
	}
}

blksout(blkp, frags)
	daddr_t *blkp;
	int frags;
{
	int i, j, count, blks, tbperdb;

	blks = howmany(frags * sblock->fs_fsize, TP_BSIZE);
	tbperdb = sblock->fs_bsize / TP_BSIZE;
	for (i = 0; i < blks; i += TP_NINDIR) {
		if (i + TP_NINDIR > blks)
			count = blks;
		else
			count = i + TP_NINDIR;
		for (j = i; j < count; j++)
			if (blkp[j / tbperdb] != 0)
				spcl.c_addr[j - i] = 1;
			else
				spcl.c_addr[j - i] = 0;
		spcl.c_count = count - i;
		spclrec();
		for (j = i; j < count; j += tbperdb)
			if (blkp[j / tbperdb] != 0)
				if (j + tbperdb <= count)
					dmpblk(blkp[j / tbperdb],
					    sblock->fs_bsize);
				else
					dmpblk(blkp[j / tbperdb],
					    (count - j) * TP_BSIZE);
		spcl.c_type = TS_ADDR;
	}
}

bitmap(map, typ)
	char *map;
{
	register i, n;
	char *cp;

	n = -1;
	for (i = 0; i < msiz; i++)
		if(map[i])
			n = i;
	if (n < 0)
		return;
	n++;
	spcl.c_type = typ;
	spcl.c_count = howmany(n * sizeof(map[0]), TP_BSIZE);
	spclrec();
	for (i = 0, cp = map; i < spcl.c_count; i++, cp += TP_BSIZE)
		taprec(cp);
}

spclrec()
{
	register int s, i, *ip;

	spcl.c_inumber = ino;
	spcl.c_magic = NFS_MAGIC;
	spcl.c_checksum = 0;
	ip = (int *)&spcl;
	s = 0;
	for(i = 0; i < sizeof(union u_spcl)/sizeof(int); i++)
		s += *ip++;
	spcl.c_checksum = CHECKSUM - s;
	taprec((char *)&spcl);
}

dsrch(d, size)
	daddr_t d;
	int size;
{
	register struct direct *dp;
	long loc;
	char dblk[MAXBSIZE];

	if(dadded)
		return;
	bread(fsbtodb(sblock, d), dblk, size);
	for (loc = 0; loc < size; ) {
		dp = (struct direct *)(dblk + loc);
		if (dp->d_reclen == 0)
			break;
		loc += dp->d_reclen;
		if(dp->d_ino == 0)
			continue;
		if(dp->d_name[0] == '.') {
			if(dp->d_name[1] == '\0')
				continue;
			if(dp->d_name[1] == '.' && dp->d_name[2] == '\0')
				continue;
		}
		if(BIT(dp->d_ino, nodmap)) {
			dadded++;
			return;
		}
		if(BIT(dp->d_ino, dirmap))
			nsubdir++;
	}
}

struct dinode *
getino(ino)
	daddr_t ino;
{
	static daddr_t minino, maxino;
	static struct dinode itab[MAXINOPB];

	if (ino >= minino && ino < maxino) {
		return (&itab[ino - minino]);
	}
	bread(fsbtodb(sblock, itod(sblock, ino)), itab, sblock->fs_bsize);
	minino = ino - (ino % INOPB(sblock));
	maxino = minino + INOPB(sblock);
	return (&itab[ino - minino]);
}

int	breaderrors = 0;		
#define	BREADEMAX 32

bread(da, ba, c)
	daddr_t da;
	char *ba;
	int	c;	
{
	register n;
	register	regc;

	if (lseek(fi, (long)(da * DEV_BSIZE), 0) < 0){
		msg("bread: lseek fails\n");
	}
	regc = c;	/* put c someplace safe; it gets clobbered */
	n = read(fi, ba, c);
	if (n != c || regc != c) {
		msg("(This should not happen)bread from %s [block %d]: c=0x%x, regc=0x%x, &c=0x%x, n=0x%x\n",
			disk, da, c, regc, &c, n);
#ifdef ERNIE
		msg("Notify Robert Henry of this error.\n");
#endif
		if (++breaderrors > BREADEMAX){
			msg("More than %d block read errors from %d\n",
				BREADEMAX, disk);
			broadcast("DUMP IS AILING!\n");
			msg("This is an unrecoverable error.\n");
			if (!query("Do you want to attempt to continue?")){
				dumpabort();
				/*NOTREACHED*/
			} else
				breaderrors = 0;
		}
	}
}
