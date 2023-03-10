static	char *sccsid = "@(#)dumpitime.c	1.7 (Berkeley) 12/2/82";
#include "dump.h"

char *prdate(d)
	time_t d;
{
	char *p;

	if(d == 0)
		return("the epoch");
	p = ctime(&d);
	p[24] = 0;
	return(p);
}

struct	idates	**idatev = 0;
int	nidates = 0;
int	idates_in = 0;
struct	itime	*ithead = 0;

inititimes()
{
			FILE	*df;
	register	int	i;
	register	struct	itime	*itwalk;

	if (idates_in)
		return;
	if ( (df = fopen(increm, "r")) == NULL){
		nidates = 0;
		ithead = 0;
	} else {
		do{
			itwalk=(struct itime *)calloc(1,sizeof (struct itime));
			if (getrecord(df, &(itwalk->it_value)) < 0)
				break;
			nidates++;
			itwalk->it_next = ithead;
			ithead = itwalk;
		} while (1);
		fclose(df);
	}

	idates_in = 1;
	/*
	 *	arrayify the list, leaving enough room for the additional
	 *	record that we may have to add to the idate structure
	 */
	idatev = (struct idates **)calloc(nidates + 1,sizeof (struct idates *));
	for (i = nidates-1, itwalk = ithead; i >= 0; i--, itwalk = itwalk->it_next)
		idatev[i] = &itwalk->it_value;
}

getitime()
{
	register	struct	idates	*ip;
	register	int	i;
			char	*fname;

	fname = disk;
#ifdef FDEBUG
	msg("Looking for name %s in increm = %s for delta = %c\n",
		fname, increm, incno);
#endif
	spcl.c_ddate = 0;

	inititimes();
	/*
	 *	Go find the entry with the same name for a lower increment
	 *	and older date
	 */
	ITITERATE(i, ip){
		if(strncmp(fname, ip->id_name,
				sizeof (ip->id_name)) != 0)
			continue;
		if (ip->id_incno >= incno)
			continue;
		if (ip->id_ddate <= spcl.c_ddate)
			continue;
		spcl.c_ddate = ip->id_ddate;
	} 
}

putitime()
{
	FILE		*df;
	register	struct	idates	*itwalk;
	register	int	i;
	char		*fname;

	if(uflag == 0)
		return;
	fname = disk;

	spcl.c_ddate = 0;
	ITITERATE(i, itwalk){
		if (strncmp(fname, itwalk->id_name,
				sizeof (itwalk->id_name)) != 0)
			continue;
		if (itwalk->id_incno != incno)
			continue;
		goto found;
	}
	/*
	 *	construct the new upper bound;
	 *	Enough room has been allocated.
	 */
	itwalk = idatev[nidates] =
		(struct idates *)calloc(1, sizeof(struct idates));
	nidates += 1;
  found:
	strncpy(itwalk->id_name, fname, sizeof (itwalk->id_name));
	itwalk->id_incno = incno;
	itwalk->id_ddate = spcl.c_date;

	if ( (df = fopen(increm, "w")) == NULL){
		msg("Cannot open %s\n", increm);
		dumpabort();
	}
	ITITERATE(i, itwalk){
		recout(df, itwalk);
	}
	fclose(df);
	msg("level %c dump on %s\n", incno, prdate(spcl.c_date));
}

recout(file, what)
	FILE	*file;
	struct	idates	*what;
{
	fprintf(file, DUMPOUTFMT,
		what->id_name,
		what->id_incno,
		ctime(&(what->id_ddate))
	);
}

int	recno;
int getrecord(df, idatep)
	FILE	*df;
	struct	idates	*idatep;
{
	char		buf[BUFSIZ];

	recno = 0;
	if ( (fgets(buf, BUFSIZ, df)) != buf)
		return(-1);
	recno++;
	if (makeidate(idatep, buf) < 0)
		msg("Unknown intermediate format in %s, line %d\n",
			NINCREM, recno);

#ifdef FDEBUG
	msg("getrecord: %s %c %s\n",
		idatep->id_name, idatep->id_incno, prdate(idatep->id_ddate));
#endif
	return(0);
}

/*
 *	Convert from old format to new format
 *	Convert from /etc/ddate to /etc/dumpdates format
 */
o_nconvert()
{
	FILE	*oldfile;
	FILE	*newfile;
	struct	idates	idate;
	struct	idates	idatecopy;

	if( (newfile = fopen(NINCREM, "w")) == NULL){
		msg("%s: Can not open %s to update.\n", processname, NINCREM);
		Exit(X_ABORT);
	}
	if ( (oldfile = fopen(OINCREM, "r")) != NULL){
		while(!feof(oldfile)){
			if (fread(&idate, sizeof(idate), 1, oldfile) != 1)
				break;
			/*
			 *	The old format ddate did not have
			 *	the full special path name on it;
			 *	we add the prefix /dev/ to the
			 *	special name, although this may not be
			 *	always the right thing to do.
			 */
			idatecopy = idate;
			strcpy(idatecopy.id_name, "/dev/");
			strncat(idatecopy.id_name, idate.id_name,
				sizeof(idate.id_name) - sizeof ("/dev/"));
			recout(newfile, &idatecopy);
		}
	}
	fclose(oldfile);
	fclose(newfile);
}

time_t	unctime();

int makeidate(ip, buf)
	struct	idates	*ip;
	char	*buf;
{
	char	un_buf[128];

	sscanf(buf, DUMPINFMT, ip->id_name, &ip->id_incno, un_buf);
	ip->id_ddate = unctime(un_buf);
	if (ip->id_ddate < 0)
		return(-1);
	return(0);
}

/*
 * This is an estimation of the number of TP_BSIZE blocks in the file.
 * It assumes that there are no unallocated blocks; hence
 * the estimate may be high
 */
est(ip)
	struct dinode *ip;
{
	long s;

	esize++;
	/* calc number of TP_BSIZE blocks */
	s = howmany(ip->di_size, TP_BSIZE);
	if (ip->di_size > sblock->fs_bsize * NDADDR) {
		/* calc number of indirect blocks on the dump tape */
		s += howmany(s - NDADDR * sblock->fs_bsize / TP_BSIZE,
			TP_NINDIR);
	}
	esize += s;
}

bmapest(map)
	char *map;
{
	register i, n;

	n = -1;
	for (i = 0; i < msiz; i++)
		if(map[i])
			n = i;
	if(n < 0)
		return;
	n++;
	esize++;
	esize += howmany(n * sizeof map[0], TP_BSIZE);
}
