
# line 2 "ingres.y"

# include	<stdio.h>
# include	<ctype.h>
# include	<ingres.h>
# include	<aux.h>
# include	<version.h>
# include	<access.h>
# include	<lock.h>
# include	<opsys.h>
# include	<ctlmod.h>
# include	<sccs.h>


/*
**  INGRES -- INGRES startup
**
**	This program starts up the entire system.
**
**	Parameters:
**		1 -- database name
**		2 -- optional process table name
**		x -- flags of the form +x or -x may be freely inter-
**			sperced in the argument list.
**
**	Return:
**		none if successful
**		1 -- user error (no database, etc)
**		-1 -- system error
**
**	Flags:
**		-&xxxx -- EQUEL flag: xxxx are file descriptors for the
**			status return pipe, the command write pipe, the
**			data return pipe, and the data transfer pipe
**			respectively.
**		-@xxxx -- xxxx is same as EQUEL flag, but no flags
**			are set.
**		-*?? -- Masterid flag. Gives the siteid of the master
**			site in a distributed ingres. (Used in dist.
**			ingres' initproc() function.)
**		-|xxxx -- Network flag.  This flag is just passed to
**			the other processes, to be processed by the
**			DBU's.
**		-uusername -- set effective user to be username.  You
**			must be INGRES or the DBA for the database to
**			use this option.
**		-cN -- set minimum character field output width to be
**			N, default 6.  This is the fewest number of
**			characters which may be output in any "c" type
**			field.
**		-inN -- integer output width.  this is the width of
**			an integer field.  The small "n" is the size
**			of the internal field ("1", "2", or "4") and
**			N is the width of the field for that flag.
**			The defaults are -i16, -i26, and -i413.
**		-fnxN.M -- floating point output width and precision.
**			Small "n" is the internal width in bytes ("4"
**			or "8"), x is the format (f, F, g, G, e, E,
**			n, or N), N is the field width, and M is the
**			precision (number of digits after the decimal
**			point).  The formats are:
**			"f" or "F": FORTRAN-style F format: digits,
**				decimal point, digits, no exponent.
**			"e" or "E": FORTRAN-style E format: digits,
**				decimal point, digits, letter "e" (or
**				"E", depending on "x" in the param-
**				eter), and an exponent.  The scaling
**				factor is always one, that is, there
**				is always one digit before the decimal
**				point.
**			"g" or "G": F format if it will fit in the
**				field, otherwise E format.  Space is
**				always left at the right of the field
**				for the exponent, so that decimal
**				points will align.
**			"n" or "N": like G, except that space is not
**				left for the decimal point in F style
**				format (useful if you expect everything
**				to fit, but you're not sure).
**			The default is -fn10.3.
**		-vx -- set vertical seperator for print operations
**			and retrieves to the terminal to be "x".  The
**			default is vertical bar ("|").
**		+w -- database wait state.  If set ("+w"), you will
**			wait until the database is not busy.  If clear,
**			you will be informed if the database is busy.
**			If not specified, the same operations take
**			place depending on whether or not you are
**			running in background (determined by whether
**			or not your input is a teletype).  If in fore-
**			ground, you are informed; if in background,
**			you wait.
**		-M -- monitor trace flag
**		-P -- parser trace flag
**		-O -- ovqp trace flag
**		-Q -- qrymod trace flag
**		-D -- decomp trace flag
**		-Z -- dbu trace flag.  These flags require the 020 bit
**			in the status field of the users file to be
**			set.  The syntax is loose and is described
**			elsewhere.  Briefly, "-Z" sets all flags except
**			the last 20, "-Z4" sets flag 4, and "-Z5/7"
**			sets all flags from 5 through 7.
**		+L -- enable/disable upper to lower case mapping in the
**			parser.  Used for debugging.
**		-rmode -- retrieve into mode
**		-nmode -- index mode.  These flags give the default
**			modify mode for retrieve into and index.  They
**			default to cheapsort and isam.  "Mode" can be
**			any mode to modify except "truncated".
**		+a -- enable/disable autoclear function in monitor.
**			Default on.
**		+b -- enable/disable batch update.  Default on.
**			The 02 bit is needed to clear this flag.
**		+d -- enable/disable printing of the dayfile.  Default
**			on.
**		+s -- enable/disable printing of almost everything from
**			the monitor.
**		+U -- enable/disable direct update of system catalogs.
**			Default off.  The 04 bit is needed to set this
**			option.
**
**	Files:
**		.../files/usage -- to print a "usage: ..." message.
**		.../data/base/<database>/admin -- to determine
**			existance and some info about <database>.
**		.../files/dayfile<VERSION> -- dayfile (printed by
**			monitor).
**		.../files/users -- file with UNIX uid -> INGRES code
**			mapping, plus a pile of other information about
**			the user.
**		.../files/proctab<VERSION> -- default process table
*/

SCCSID(@(#)ingres.y	7.1	2/5/81)

# define	MAXOPTNS	10		/* maximum number of options you can specify */
# define	MAXPROCS	10		/* maximum number of processes in the system */
# define	EQUELFLAG	'&'
# define	NETFLAG		'|'		/* network slave flag */
# define	CLOSED		'?'

char		Fileset[10];
char		*Database;
extern char	*Dbpath;		/* defined in initucode */
struct admin	Admin;			/* set in initucode */
struct lockreq	Lock;
FILE		*ProcFile;		/* fildes for the process table */
char		*DefProcTab = NULL;	/* default process table name */
char		*Opt[MAXOPTNS + 1];
int		Nopts;
int		No_exec;		/* if set, don't execute */
int		NumProcs;		/* number of processes this system */


/*
**  Internal form of process descriptions.
*/

struct proc
{
	short		prstat;		/* status bits, see below */
	char		prmpipe;	/* initial pipe to this process */
	char		prtflag;	/* trace flag for CM this proc */
	char		prpath[50];	/* pathname of this process */
	struct _cm_t	prcm;		/* cm info passed to this proc */
};

/* bits for prstat */
# define PR_REALUID	0001		/* run as the user, not INGRES */
# define PR_NOCHDIR	0002		/* don't chdir into database */
# define PR_CLSSIN	0004		/* close standard input */
# define PR_CLSDOUT	0010		/* close diagnostic output */

struct proc	ProcTab[CM_MAXPROC];


/*
**  Open pipe info.
*/

struct pipeinfo
{
	char	pip_rfd;	/* read file descriptor */
	char	pip_wfd;	/* write file descriptor */
	short	pip_rcnt;	/* read reference count */
	short	pip_wcnt;	/* write reference count */
};

struct pipeinfo Pipe[128];


/*
**  Macro definitions
*/

char	Macro[26][80];


/* system error messages, etc. */
extern int	sys_nerr;
extern char	*sys_errlist[];
extern int	errno;

/* globals used by the grammar. */
struct proc	*Proc;
state_t		*StateP;
proc_t		*ProcP;
int		ProcNo;
int		RemStat;



# line 215 "ingres.y"
typedef union 
{
	int	yyint;		/* integer */
	char	*yystr;		/* string */
	char	yypip;		/* pipe id */
	char	yychar;		/* single character */
} YYSTYPE;
# define INT 257
# define STR 258
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
extern short yyerrflag;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif
YYSTYPE yylval, yyval;
# define YYERRCODE 256

# line 416 "ingres.y"


main(argc, argv)
int	argc;
char	**argv;
{
	register int	i;
	register int	j;
	extern char	*Proc_name;
	int		fd;
	extern int	Status;
	char		*proctab;
	register char	*p;
	char		*ptr;
	extern char	*Flagvect[];	/* defined in initucode.c */
	extern char	*Parmvect[];	/* ditto */
	char		*uservect[4];
	char		buf[MAXLINE+1];

	Proc_name = "INGRES";
	itoa(getpid(), Fileset);
	proctab = NULL;
	Database = NULL;

	/*
	**  Initialize everything, like Flagvect, Parmvect, Usercode,
	**	etc.
	*/

	for (i = 0; i < 128; i++)
		Pipe[i].pip_rfd = Pipe[i].pip_wfd = -1;

	i = initucode(argc, argv, TRUE, uservect, -1);
	switch (i)
	{
	  case 0:	/* ok */
	  case 5:
		break;

	  case 1:	/* database does not exist */
	  case 6:
		printf("Database %s does not exist\n", Parmvect[0]);
		goto usage;

	  case 2:	/* you are not authorized */
		printf("You may not access database %s\n", Database);
		goto usage;

	  case 3:	/* not a valid user */
		printf("You are not a valid INGRES user\n");
		goto usage;

	  case 4:	/* no database name specified */
		printf("No database name specified\n");
		goto usage;

	  default:
		syserr("initucode %d", i);
	}

	/*
	**  Extract database name and process table name from
	**	parameter vector.
	**  Initialize the $P macro.
	*/

	Database = Parmvect[0];
	proctab = Parmvect[1];
	smove(Pathname, Macro['P' - 'A']);

	/* scan flags in users file */
	for (p = uservect[0]; *p != '\0'; p++)
	{
		/* skip initial blanks and tabs */
		if (*p == ' ' || *p == '\t')
			continue;
		ptr = p;

		/* find end of flag and null-terminate it */
		while (*p != '\0' && *p != ' ' && *p != '\t')
			p++;
		i = *p;
		*p = '\0';

		/* process the flag */
		doflag(ptr, 1);
		if (i == '\0')
			break;
	}

	/* scan flags on command line */
	for (i = 0; (p = Flagvect[i]) != NULL; i++)
		doflag(p, 0);

	/* check for query modification specified for this database */
	if ((Admin.adhdr.adflags & A_QRYMOD) == 0)
		doflag("-q", -1);

	/* close any extraneous files, being careful not to close anything we need */
	for (i = 3; i < NOFILE; i++)
	{
		for (j = '0'; j <= '9'; j++)
		{
			if (Pipe[j].pip_wfd == i || Pipe[j].pip_rfd == i)
				break;
		}
		if (j > '9')
			close(i);
	}

	/* determine process table */
	if (proctab == NULL)
	{
		/* use default proctab */
		if (DefProcTab == NULL)
		{
			if (argv[0][length(argv[0]) - 1] == 'x')
				DefProcTab = "=procx";
			else
				DefProcTab = "=proctab";
			proctab = uservect[1];
		}
		if (proctab == NULL || proctab[0] == 0)
		{
			/* no proctab in users file */
			concat(DefProcTab, VERSION, buf);
			proctab = buf;
		}
	}
	else
	{
		/* proctab specified; check permissions */
		if ((Status & (proctab[0] == '=' ? U_EPROCTAB : U_APROCTAB)) == 0)
		{
			printf("You may not specify this process table\n");
			goto usage;
		}
	}

	/* expand process table name */
	if (proctab[0] == '=')
	{
		smove(ztack(ztack(Pathname, "/files/"), &proctab[1]), buf);
		proctab = buf;
	}

	/* open and read the process table */
	if ((ProcFile = fopen(proctab, "r")) == NULL)
	{
		printf("Proctab %s: %s\n", proctab, sys_errlist[errno]);
		goto usage;
	}

	/* build internal form of the process table */
	if (yyparse())
		No_exec++;

	/* don't bother executing if we have found errors */
	if (No_exec)
	{
	  usage:
		/* cat .../files/usage */
		cat(ztack(Pathname, "/files/usage"));
		exit(1);
	}

	fclose(ProcFile);

	/* set locks on the database */
	dolocks();

	/* satisfy process table (never returns) */
	satisfypt();
}



/*
**  Process rubouts (just exit)
*/

rubproc()
{
	exit(2);
}
/*
**  DOFLAG -- process flag
**
**	Parameters:
**		flag -- the flag (as a string)
**		where -- where it is called from
**			-1 -- internally inserted
**			0 -- on user command line
**			1 -- from users file
**
**	Return:
**		none
**
**	Side effects:
**		All flags are inserted on the end of the
**		"Flaglist" vector for passing to the processes.
**		The "No_exec" flag is set if the flag is bad or you
**		are not authorized to use it.
**
**	Requires:
**		Status -- to get the status bits set for this user.
**		syserr -- for the obvious
**		printf -- to print errors
**		atoi -- to check syntax on numerically-valued flags
**
**	Defines:
**		doflag()
**		Flagok -- a list of legal flags and attributes (for
**			local use only).
**		Relmode -- a list of legal relation modes.
**
**	Called by:
**		main
**
**	History:
**		11/6/79 (6.2/8) (eric) -- -u flag processing dropped,
**			since initucode does it anyhow.  -E flag
**			removed (what is it?).  F_USER code dropped.
**			F_DROP is still around; we may need it some-
**			day.  Also, test of U_SUPER flag and/or DBA
**			status was wrong.
**		7/5/78 (eric) -- NETFLAG added to list.
**		3/27/78 (eric) -- EQUELFLAG added to the list.
**		1/29/78 -- do_u_flag broken off by eric
**		1/4/78 -- written by eric
*/

struct flag
{
	char	flagname;	/* name of the flag */
	char	flagstat;	/* status of flag (see below) */
	int	flagsyntx;	/* syntax code for this flag */
	int	flagperm;	/* status bits needed to use this flag */
	char	*flagpt;	/* default proctab to use with this flag */
};

/* status bits for flag */
# define	F_PLOK		01	/* allow +x form */
# define	F_PLD		02	/* defaults to +x */
# define	F_DBA		04	/* must be the DBA to use */
# define	F_DROP		010	/* don't save in Flaglist */

/* syntax codes */
# define	F_ACCPT		1	/* always accept */
# define	F_C_SPEC	3	/* -cN spec */
# define	F_I_SPEC	4	/* -inN spec */
# define	F_F_SPEC	5	/* -fnxN.M spec */
# define	F_CHAR		6	/* single character */
# define	F_MODE		7	/* a modify mode */
# define	F_INTERNAL	8	/* internal flag, e.g., -q */
# define	F_EQUEL		9	/* EQUEL flag */

struct flag	Flagok[] =
{
	'a',	F_PLD|F_PLOK,	F_ACCPT,	0,		NULL,
	'b',	F_PLD|F_PLOK,	F_ACCPT,	U_DRCTUPDT,	NULL,
	'c',	0,		F_C_SPEC,	0,		NULL,
	'd',	F_PLD|F_PLOK,	F_ACCPT,	0,		NULL,
	'f',	0,		F_F_SPEC,	0,		NULL,
	'i',	0,		F_I_SPEC,	0,		NULL,
	'n',	0,		F_MODE,		0,		NULL,
	'q',	F_PLD|F_PLOK,	F_INTERNAL,	0,		NULL,
	'r',	0,		F_MODE,		0,		NULL,
	's',	F_PLD|F_PLOK,	F_ACCPT,	0,		NULL,
	'v',	0,		F_CHAR,		0,		NULL,
	'w',	F_PLOK|F_DROP,	F_ACCPT,	0,		NULL,
	'D',	0,		F_ACCPT,	U_TRACE,	NULL,
	'L',	F_PLOK,		F_ACCPT,	0,		NULL,
	'M',	0,		F_ACCPT,	U_TRACE,	NULL,
	'O',	0,		F_ACCPT,	U_TRACE,	NULL,
	'P',	0,		F_ACCPT,	U_TRACE,	NULL,
	'Q',	0,		F_ACCPT,	U_TRACE,	NULL,
	'T',	0,		F_ACCPT,	U_TRACE,	NULL,
	'U',	F_PLOK,		F_ACCPT,	U_UPSYSCAT,	NULL,
	'Z',	0,		F_ACCPT,	U_TRACE,	NULL,
	EQUELFLAG, 0,		F_EQUEL,	0,		"=equel",
	NETFLAG, 0,		F_EQUEL,	0,		"=slave",
	'@',	0,		F_EQUEL,	0,		NULL,
	'*',	0,		F_ACCPT,	0,		NULL,
	0,	0,		0,		0,		NULL,
};

/* list of valid retrieve into or index modes */
char	*Relmode[] =
{
	"isam",
	"cisam",
	"hash",
	"chash",
	"heap",
	"cheap",
	"heapsort",
	"cheapsort",
	NULL
};


doflag(flag, where)
char	*flag;
int	where;
{
	register char		*p;
	register struct flag	*f;
	auto int		intxx;
	register char		*ptr;
	int			i;
	int			j;
	extern int		Status;

	p = flag;

	/* check for valid flag format (begin with + or -) */
	if (p[0] != '+' && p[0] != '-')
		goto badflag;

	/* check for flag in table */
	for (f = Flagok; f->flagname != p[1]; f++)
	{
		if (f->flagname == 0)
			goto badflag;
	}

	/* check for +x form allowed */
	if (p[0] == '+' && (f->flagstat & F_PLOK) == 0)
		goto badflag;

	/* check for permission to use the flag */
	if ((f->flagperm != 0 && (Status & f->flagperm) == 0 &&
	     (((f->flagstat & F_PLD) == 0) ? (p[0] == '+') : (p[0] == '-'))) ||
	    ((f->flagstat & F_DBA) != 0 && (Status & U_SUPER) == 0 &&
	     !bequal(Usercode, Admin.adhdr.adowner, 2)))
	{
		printf("You are not authorized to use the %s flag\n", p);
		No_exec++;
	}

	/* check syntax */
	switch (f->flagsyntx)
	{
	  case F_ACCPT:
		break;

	  case F_C_SPEC:
		if (atoi(&p[2], &intxx) || intxx > MAXFIELD)
			goto badflag;
		break;

	  case F_I_SPEC:
		if (p[2] != '1' && p[2] != '2' && p[2] != '4')
			goto badflag;
		if (atoi(&p[3], &intxx) || intxx > MAXFIELD)
			goto badflag;
		break;

	  case F_F_SPEC:
		if (p[2] != '4' && p[2] != '8')
			goto badflag;
		switch (p[3])
		{
		  case 'e':
		  case 'E':
		  case 'f':
		  case 'F':
		  case 'g':
		  case 'G':
		  case 'n':
		  case 'N':
			break;

		  default:
			goto badflag;

		}
		ptr = &p[4];
		while (*ptr != '.')
			if (*ptr == 0)
				goto badflag;
			else
				ptr++;
		*ptr = 0;
		if (atoi(&p[4], &intxx) || intxx > MAXFIELD)
			goto badflag;
		*ptr++ = '.';
		if (atoi(ptr, &intxx) || intxx > MAXFIELD)
			goto badflag;
		break;

	  case F_CHAR:
		if (p[2] == 0 || p[3] != 0)
			goto badflag;
		break;

	  case F_MODE:
		for (i = 0; (ptr = Relmode[i]) != NULL; i++)
		{
			if (sequal(&p[2], ptr))
				break;
		}
		if (ptr == NULL)
			goto badflag;
		break;

	  case F_INTERNAL:
		if (where >= 0)
			goto badflag;
		break;

	  case F_EQUEL:
		ptr = &p[2];
		for (i = 0; i < 20; i++, ptr++)
		{
			if (*ptr == CLOSED)
				continue;
			if (*ptr < 0100 || *ptr >= 0100 + NOFILE)
				break;
			j = (i / 2) + '0';
			if ((i & 01) == 0)
			{
				Pipe[j].pip_rfd = *ptr & 077;
			}
			else
			{
				Pipe[j].pip_wfd = *ptr & 077;
			}
		}
		break;

	  default:
		syserr("doflag: syntx %d", f->flagsyntx);

	}

	/* save flag */
	if (Nopts >= MAXOPTNS)
	{
		printf("Too many options to INGRES\n");
		exit(1);
	}
	if ((f->flagstat & F_DROP) == 0)
		Opt[Nopts++] = p;

	/* change to new process table as appropriate */
	if (f->flagpt != NULL)
		DefProcTab = f->flagpt;

	return;

  badflag:
	printf("Bad flag format: %s\n", p);
	No_exec++;
	return;
}
/*
**  DOLOCKS -- set database lock
**
**	A lock is set on the database.
*/

dolocks()
{
	db_lock(flagval('E') > 0 ? M_EXCL : M_SHARE);
	close(Alockdes);
}
/*
**  FLAGVAL -- return value of flag
**
**	Parameter:
**		flag -- the name of the flag
**
**	Return:
**		-1 -- flag is de-asserted (-x)
**		0 -- flag is not specified
**		1 -- flag is asserted (+x)
**
**	Requires:
**		Opt -- to scan the flags
**
**	Defines:
**		flagval
**
**	Called by:
**		buildint
**		dolocks
**
**	History:
**		3/27/78 (eric) -- changed to handle EQUEL flag
**			normally.
**		1/4/78 -- written by eric
*/

flagval(flag)
char	flag;
{
	register char	f;
	register char	**p;
	register char	*o;

	f = flag;

	/* start scanning option list */
	for (p = Opt; (o = *p) != 0; p++)
	{
		if (o[1] == f)
			if (o[0] == '+')
				return (1);
			else
				return (-1);
	}
	return (0);
}
/*
**  SATISFYPT -- satisfy the process table
**
**	Well folks, now that you've read this far, this is it!!!  I
**	mean, this one really does it!!!  It takes the internal form
**	built by the parser and creates pipes as necessary, forks, and
**	execs the INGRES processes.  Isn't that neat?
**
**	Parameters:
**		none
**
**	Returns:
**		never
**
**	Requires:
**		Proctab -- the internal form
**		ingexec -- to actually exec the process
**		pipe -- to create the pipe
**		syserr -- for the obvious
**		fillpipe -- to extend a newly opened pipe through all
**			further references to it.
**		checkpipes -- to see if a given pipe will ever be
**			referenced again.
**		fork -- to create a new process
**
**	Defines:
**		satisfypt
**
**	Called by:
**		main
**
**	History:
**		3/14/80 (eric) -- changed for version 7.0.
**		7/24/78 (eric) -- Actual file descriptors stored in
**			'prpipes' are changed to have the 0100 bit
**			set internally (as well as externally), so
**			fd 0 will work correctly.
**		1/4/78 -- written by eric
*/

satisfypt()
{
	register struct proc	*pr;
	register proc_t		*pp;
	register int		i;
	int			procno;
	register int		pip;

	/* scan the process table */
	for (procno = CM_MAXPROC-1; procno >= 0; procno--)
	{
		pr = &ProcTab[procno];
		if (pr->prpath[0] == '\0')
			continue;

		/* scan pipe vector, creating new pipes as needed */
		pipeopen(pr->prmpipe, TRUE);
		pipeopen(pr->prcm.cm_input, FALSE);
		pipeopen(pr->prcm.cm_rinput, FALSE);
		for (i = 0; i < CM_MAXPROC; i++)
		{
			pp = &pr->prcm.cm_proc[i];
			pipeopen(pp->pr_file, TRUE);
			pipeopen(pp->pr_ninput, FALSE);
		}

		/* substitute real file descriptors throughout */
		pipexlat(&pr->prmpipe, TRUE);
		pipexlat(&pr->prcm.cm_input, FALSE);
		pipexlat(&pr->prcm.cm_rinput, FALSE);
		for (i = 0; i < CM_MAXPROC; i++)
		{
			pp = &pr->prcm.cm_proc[i];
			pipexlat(&pp->pr_file, TRUE);
			pipexlat(&pp->pr_ninput, FALSE);
		}

		/* fork if necessary */
		if (--NumProcs <= 0 || (i = fork()) == 0)
		{
			/* child!! */
			ingexec(procno);
		}

		/* parent */
		if (i < 0)
			syserr("satisfypt: fork");

		/* scan pipes.  close all not used in the future */
		for (i = 0; i < 128; i++)
		{
			if (i == CLOSED)
				continue;
			if (Pipe[i].pip_rcnt <= 0 && Pipe[i].pip_rfd >= 0)
			{
				if (close(Pipe[i].pip_rfd) < 0)
					syserr("satisfypt: close-r(%d)", Pipe[i].pip_rfd);
				Pipe[i].pip_rfd = -1;
			}
			if (Pipe[i].pip_wcnt <= 0 && Pipe[i].pip_wfd >= 0)
			{
				if (close(Pipe[i].pip_wfd) < 0)
					syserr("satisfypt: close-w(%d)", Pipe[i].pip_wfd);
				Pipe[i].pip_wfd = -1;
			}
		}
	}
	syserr("satisfypt: fell out");
}
/*
**  PIPEOPEN -- open pipe if necessary.
*/

pipeopen(pipid, rw)
	char pipid;
	int rw;
{
	register struct pipeinfo *pi;
	int pipex[2];

	if (pipid == '\0')
		return;

	pi = &Pipe[pipid];

	if ((rw ? pi->pip_wfd : pi->pip_rfd) >= 0)
		return;
	if (pi->pip_rfd >= 0 || pi->pip_wfd >= 0)
		syserr("pipeopen %o %d: rfd=%d, wfd=%d", pipid, rw, pi->pip_rfd, pi->pip_wfd);
	if (pipid == CLOSED)
		pi->pip_rfd = pi->pip_wfd = CLOSED;
	else
	{
		if (pipe(pipex) < 0)
			syserr("pipeopen: pipe");
		pi->pip_rfd = pipex[0];
		pi->pip_wfd = pipex[1];
	}
}
/*
**  CHECKPIPES -- check for pipe referenced in the future
**
**	Parameters:
**		proc -- point in the process table to start looking
**			from.
**		fd -- the file descriptor to look for.
**
**	Return:
**		zero -- it will be referenced in the future.
**		one -- it is never again referenced.
**
**	Requires:
**		nothing
**
**	Defines:
**		checkpipes
**
**	Called by:
**		satisfypt
**
**	History:
**		7/24/78 (eric) -- 0100 bit on file descriptors handled.
**		1/4/78 -- written by eric
*/

checkpipes(proc, fd)
struct proc	*proc;
register int	fd;
{
	register struct proc	*pr;
	register proc_t		*pp;
	register int		i;

	for (pr = proc; pr < &ProcTab[CM_MAXPROC]; pr++)
	{
		if (pr->prpath[0] == '\0')
			continue;
		for (i = 0; i < CM_MAXPROC; i++)
		{
			pp = &pr->prcm.cm_proc[i];
			if (pp->pr_file == fd || pp->pr_ninput == fd)
				return (0);
		}
	}
	return (1);
}
/*
**  INGEXEC -- execute INGRES process
**
**	This routine handles all the setup of the argument vector
**	and then executes a process.
**
**	Parameters:
**		process -- a pointer to the process table entry which
**			describes this process.
**
**	Returns:
**		never
**
**	Side Effects:
**		never returns, but starts up a new overlay.  Notice
**			that it does NOT fork.
**
**	Requires:
**		none
**
**	Called By:
**		satisfypt
**
**	Trace Flags:
**		none
**
**	Diagnostics:
**		none
**
**	Syserrs:
**		chdir %s -- could not change directory into the data-
**			base.
**		creat %s -- could not create the redirected standard
**			output file.
**		%s not executable -- could not execute the process.
**
**	History:
**		8/9/78 (eric) -- changed "prparam" to be a colon-
**			separated list of parameters (so the number
**			is variable); also, moved parameter expansion
**			into this routine from buildint() so that
**			the colons in the dbu part of the proctab
**			would not confuse things.
**		7/24/78 (eric) -- changed the technique of closing
**			files 0 & 2 so that they will never be closed
**			(even if requested in the status field)
**			if they are mentioned in the pipe vector.
**			Also, some fiddling is done to handle the
**			0100 bit on file descriptors correctly.
*/

ingexec(procno)
	int procno;
{
	char			*vect[30];
	register char		**v;
	char			**opt;
	int			i;
	register struct proc	*pr;
	register proc_t		*pp;
	register char		*p;
	int			outfd;
	char			closeit[NOFILE];
	char			fdbuf[3];

	v = vect;
	pr = &ProcTab[procno];

	*v++ = pr->prpath;
	fdbuf[0] = pr->prcm.cm_rinput | 0100;
	fdbuf[1] = pr->prtflag;
	fdbuf[2] = '\0';
	*v++ = fdbuf;
	*v++ = Fileset;
	*v++ = Usercode;
	*v++ = Database;
	*v++ = Pathname;

	/* insert flag parameters */
	for (opt = Opt; *opt; opt++)
		*v++ = *opt;
	*v = 0;

	/* set up 'closeit' to tell which pipes to close */
	for (i = 0; i < NOFILE; i++)
		closeit[i] = TRUE;
	closeit[pr->prmpipe & 077] = FALSE;
	closeit[pr->prcm.cm_input & 077] = FALSE;
	closeit[pr->prcm.cm_rinput & 077] = FALSE;
	for (i = 0; i < CM_MAXPROC; i++)
	{
		pp = &pr->prcm.cm_proc[i];
		if (pp->pr_ninput != CLOSED)
			closeit[pp->pr_ninput & 077] = FALSE;
		if (pp->pr_file != CLOSED)
			closeit[pp->pr_file & 077] = FALSE;
	}
	closeit[1] = FALSE;
	if ((pr->prstat & PR_CLSSIN) == 0)
		closeit[0] = FALSE;
	if ((pr->prstat & PR_CLSDOUT) == 0)
		closeit[2] = FALSE;

	/* close extra pipes (those not used by this process) */
	for (i = 0; i < NOFILE; i++)
	{
		if (closeit[i])
			close(i);
	}

	/* change to the correct directory */
	if ((pr->prstat & PR_NOCHDIR) == 0)
	{
		if (chdir(Dbpath))
			syserr("ingexec: chdir %s", Dbpath);
	}

	/* change to normal userid/groupid if a non-dangerous process */
	if ((pr->prstat & PR_REALUID) != 0)
	{
		setuid(getuid());
#		ifndef xB_UNIX
		setgid(getgid());
#		endif
	}

# ifdef LEAVEOUT
	/* change standard output if specified in proctab */
	p = pr->prstdout;
	if (*p != 0)
	{
		/* chew up fd 0 (just in case) */
		outfd = dup(1);
		close(1);
		if (creat(p, 0666) != 1)
		{
			/* restore standard output and print error */
			close(1);
			dup(outfd);	/* better go into slot 1 */
			syserr("ingexec: creat %s", p);
		}
		close(outfd);
	}
# endif LEAVEOUT

	/*
	**  PLEASE NOTE THE TRICKERY USED HERE.
	**	In this code I depend on UNIX buffering pipes at least
	**	enough to handle one "CM" struct.  If not, the following
	**	write will hang before the exec will call the process
	**	that will read it.
	**
	**	The "correct" way to do this is to fork & have the
	**	parent write the CM struct.  But how do I handle the
	**	last one (that does not fork)?  I could also do an
	**	extra fork of a process to do the write.  But some
	**	systems have a limit on processes, and besides, it
	**	seems like a lot of overhead for such a little thing.
	**
	**	Perhaps I should encode the CM struct into argv
	**	instead & do it "right".
	*/

	/* output the control structure to the awaiting process... */
	write(pr->prmpipe & 077, &pr->prcm, sizeof pr->prcm);
	close(pr->prmpipe & 077);

	/* give it the old college (or in this case, University) try */
	execv(vect[0], vect);
	syserr("\"%s\" not executable", vect[0]);
}



pipexlat(ppip, rw)
	char *ppip;
	int rw;
{
	register struct pipeinfo *pi;
	int cnt;
	int fd;

	if (*ppip == '\0' || *ppip == CLOSED)
		return;
	pi = &Pipe[*ppip];

	if (rw)
	{
		cnt = --(pi->pip_wcnt);
		fd = pi->pip_wfd;
	}
	else
	{
		cnt = --(pi->pip_rcnt);
		fd = pi->pip_rfd;
	}

	if (cnt < 0)
		syserr("pipexlat: cnt=%d: %o %d", cnt, *ppip, rw);
	if (fd < 0 || fd > NOFILE)
		syserr("pipexlat: fd=%d: %o %d", fd, *ppip, rw);

	*ppip = fd;
}
/*
**  YYLEX -- Return next token from proctab
**
**	Parameters:
**		none
**
**	Returns:
**		Next token
**
**	Side Effects:
**		Input from proctab
*/

# define BOLSTATE	0	/* beginning of line */
# define NORMSTATE	1	/* normal token */
# define EOFSTATE	2	/* end of file */
int	LineNo;			/* current line number */

yylex()
{
	static int state;
	static char *ptp;
	auto int ix;
	static char line[MAXLINE];
	register int c;
	register char *p;

	switch (state)
	{
	  case EOFSTATE:
		return (0);

	  case BOLSTATE:
		ptp = line;
		for (;;)
		{
			LineNo++;
			c = getc(ProcFile);
			if (c < 0)
			{
				state = EOFSTATE;
				return (0);
			}
			switch (c)
			{
			  case '*':
			  case '#':
			  case '\n':
				while (c != '\n' && (c = getc(ProcFile)) > 0)
					continue;
				break;

			  case ':':
				while (c != '\n' && (c = getc(ProcFile)) > 0)
					putchar(c);
				break;

			  default:
				/* regular line, return header */
				state = NORMSTATE;
				return (c);
			}
		}
	
	  case NORMSTATE:
		yylval.yystr = ptp;
		while ((c = getc(ProcFile)) != ':' && c != '\n' && c > 0)
		{
			*ptp++ = c;
			if (c == '$')
			{
				c = getc(ProcFile);
				if (c < 'A' || c > 'Z')
					*ptp++ = c;
				else
				{
					ptp--;
					for (p = Macro[c - 'A']; (*ptp++ = *p++) != '\0'; )
						continue;
					ptp--;
				}
			}
		}
		
		/* compute next state */
		if (c != ':')
			state = BOLSTATE;

		*ptp++ = '\0';
		if (atoi(yylval.yystr, &ix) == 0)
		{
			if (yylval.yystr[0] == '0')
				ix = oatoi(yylval.yystr);
			yylval.yyint = ix;
			return (INT);
		}
		else
			return (STR);
	
	  default:
		syserr("yylex: state %d", state);
	}
}




yyerror(s)
	char *s;
{
	syserr("Line %d: Yacc error: %s", LineNo, s);
}


/*VARARGS1*/
usrerr(f, p1, p2, p3)
	char *f;
{
	printf("Line %d: ", LineNo);
	printf(f, p1, p2, p3);
	printf("\n");
}
short yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
	};
# define YYNPROD 27
# define YYLAST 55
short yyact[]={

  45,  38,  34,  27,  26,  19,  29,  22,  36,  17,
  13,   7,  28,  37,  21,  16,  15,  10,   3,  30,
  23,   8,  14,   6,  12,  11,   5,  20,   9,   4,
   2,  24,   1,  44,  18,  33,  25,  32,  31,  35,
   0,   0,   0,   0,   0,   0,   0,  39,   0,   0,
  40,  41,  42,   0,  43 };
short yypact[]={

 -57,-1000, -57,-1000, -66,-1000,-248,-253,-1000, -66,
-1000,-1000,-1000,-250,-1000,-248,-254,-1000,-255,-1000,
-1000,-251,-1000,-250,-251,-256,-1000,-1000,-249,-1000,
-1000,-1000,-257,-257,-1000,-250,-1000,-257,-1000,-251,
-1000,-251,-258,-1000,-1000,-1000 };
short yypgo[]={

   0,  15,  14,  39,  12,  36,  35,  13,  34,  33,
  32,  30,  18,  29,  28,  26,  17,  25,  24,  22,
  20,  19 };
short yyr1[]={

   0,  10,  11,  11,  12,  12,  13,  14,  14,  16,
  16,  17,  18,  19,  20,  20,  21,  15,   1,   2,
   3,   5,   6,   7,   9,   4,   8 };
short yyr2[]={

   0,   1,   1,   2,   2,   1,   7,   1,   2,   1,
   1,   5,   2,   6,   0,   2,   1,   3,   1,   1,
   1,   1,   1,   1,   1,   1,   1 };
short yychk[]={

-1000, -10, -11, -12, -13, -15,  80,  68, -12, -14,
 -16, -17, -18,  76, -19,  82,  -1, 257,  -8, 258,
 -16,  -2, 257, -20,  -1,  -5, 258, 258,  -4, 257,
 -21,  -2,  -4,  -6, 258,  -3, 257,  -7, 258,  -7,
  -2,  -7,  -4,  -4,  -9, 258 };
short yydef[]={

   0,  -2,   1,   2,   0,   5,   0,   0,   3,   4,
   7,   9,  10,   0,  14,   0,   0,  18,   0,  26,
   8,   0,  19,  12,   0,   0,  21,  17,   0,  25,
  15,  16,   0,   0,  22,   0,  20,   0,  23,   0,
  11,   0,   0,  13,   6,  24 };
#
# define YYFLAG -1000
# define YYERROR goto yyerrlab
# define YYACCEPT return(0)
# define YYABORT return(1)

/*	parser for yacc output	*/

#ifdef YYDEBUG
int yydebug = 0; /* 1 for debugging */
#endif
YYSTYPE yyv[YYMAXDEPTH]; /* where the values are stored */
int yychar = -1; /* current input token number */
int yynerrs = 0;  /* number of errors */
short yyerrflag = 0;  /* error recovery flag */

yyparse() {

	short yys[YYMAXDEPTH];
	short yyj, yym;
	register YYSTYPE *yypvt;
	register short yystate, *yyps, yyn;
	register YYSTYPE *yypv;
	register short *yyxi;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyps= &yys[-1];
	yypv= &yyv[-1];

 yystack:    /* put a state and value onto the stack */

#ifdef YYDEBUG
	if( yydebug  ) printf( "state %d, char 0%o\n", yystate, yychar );
#endif
		if( ++yyps> &yys[YYMAXDEPTH] ) { yyerror( "yacc stack overflow" ); return(1); }
		*yyps = yystate;
		++yypv;
		*yypv = yyval;

 yynewstate:

	yyn = yypact[yystate];

	if( yyn<= YYFLAG ) goto yydefault; /* simple state */

	if( yychar<0 ) if( (yychar=yylex())<0 ) yychar=0;
	if( (yyn += yychar)<0 || yyn >= YYLAST ) goto yydefault;

	if( yychk[ yyn=yyact[ yyn ] ] == yychar ){ /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if( yyerrflag > 0 ) --yyerrflag;
		goto yystack;
		}

 yydefault:
	/* default state action */

	if( (yyn=yydef[yystate]) == -2 ) {
		if( yychar<0 ) if( (yychar=yylex())<0 ) yychar = 0;
		/* look through exception table */

		for( yyxi=yyexca; (*yyxi!= (-1)) || (yyxi[1]!=yystate) ; yyxi += 2 ) ; /* VOID */

		while( *(yyxi+=2) >= 0 ){
			if( *yyxi == yychar ) break;
			}
		if( (yyn = yyxi[1]) < 0 ) return(0);   /* accept */
		}

	if( yyn == 0 ){ /* error */
		/* error ... attempt to resume parsing */

		switch( yyerrflag ){

		case 0:   /* brand new error */

			yyerror( "syntax error" );
		yyerrlab:
			++yynerrs;

		case 1:
		case 2: /* incompletely recovered error ... try again */

			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */

			while ( yyps >= yys ) {
			   yyn = yypact[*yyps] + YYERRCODE;
			   if( yyn>= 0 && yyn < YYLAST && yychk[yyact[yyn]] == YYERRCODE ){
			      yystate = yyact[yyn];  /* simulate a shift of "error" */
			      goto yystack;
			      }
			   yyn = yypact[*yyps];

			   /* the current yyps has no shift onn "error", pop stack */

#ifdef YYDEBUG
			   if( yydebug ) printf( "error recovery pops state %d, uncovers %d\n", *yyps, yyps[-1] );
#endif
			   --yyps;
			   --yypv;
			   }

			/* there is no state on the stack with an error shift ... abort */

	yyabort:
			return(1);


		case 3:  /* no shift yet; clobber input char */

#ifdef YYDEBUG
			if( yydebug ) printf( "error recovery discards char %d\n", yychar );
#endif

			if( yychar == 0 ) goto yyabort; /* don't discard EOF, quit */
			yychar = -1;
			goto yynewstate;   /* try again in the same state */

			}

		}

	/* reduction by production yyn */

#ifdef YYDEBUG
		if( yydebug ) printf("reduce %d\n",yyn);
#endif
		yyps -= yyr2[yyn];
		yypvt = yypv;
		yypv -= yyr2[yyn];
		yyval = yypv[1];
		yym=yyn;
			/* consult goto table to find next state */
		yyn = yyr1[yyn];
		yyj = yypgo[yyn] + *yyps + 1;
		if( yyj>=YYLAST || yychk[ yystate = yyact[yyj] ] != -yyn ) yystate = yyact[yypgo[yyn]];
		switch(yym){
			
case 6:
# line 265 "ingres.y"
{
			NumProcs++;
			Proc = &ProcTab[yypvt[-5].yyint];
			smove(yypvt[-4].yystr, Proc->prpath);
			Proc->prmpipe = Proc->prcm.cm_input = Proc->prcm.cm_rinput
			    = Proc->prcm.cm_proc[yypvt[-5].yyint].pr_ninput = yypvt[-2].yypip;
			smove(yypvt[-3].yystr, Proc->prcm.cm_myname);
			Proc->prcm.cm_myproc = yypvt[-5].yyint;
			Proc->prstat = yypvt[-1].yyint;
			Proc->prtflag = yypvt[-0].yychar;
			Pipe[yypvt[-2].yypip].pip_rcnt += 3;
			Pipe[yypvt[-2].yypip].pip_wcnt++;
		} break;
case 11:
# line 289 "ingres.y"
{
			StateP = &Proc->prcm.cm_state[yypvt[-3].yyint];
			StateP->st_type = ST_LOCAL;
			StateP->st_stat = yypvt[-2].yyint;
			StateP->st_v.st_loc.st_funcno = yypvt[-1].yyint;
			StateP->st_v.st_loc.st_next = yypvt[-0].yyint;
		} break;
case 13:
# line 302 "ingres.y"
{
			ProcNo = yypvt[-4].yyint;
			ProcP = &Proc->prcm.cm_proc[ProcNo];
			RemStat = yypvt[-3].yyint;
			ProcP->pr_file = yypvt[-2].yypip;
			ProcP->pr_ninput = yypvt[-1].yypip;
			ProcP->pr_stat = yypvt[-0].yyint;
			Pipe[yypvt[-2].yypip].pip_wcnt++;
			Pipe[yypvt[-1].yypip].pip_rcnt++;
		} break;
case 16:
# line 319 "ingres.y"
{
			StateP = &Proc->prcm.cm_state[yypvt[-0].yyint];
			StateP->st_type = ST_REMOT;
			StateP->st_stat = RemStat;
			StateP->st_v.st_rem.st_proc = ProcNo;
		} break;
case 17:
# line 332 "ingres.y"
{
			smove(yypvt[-0].yystr, Macro[yypvt[-1].yychar - 'A']);
		} break;
case 18:
# line 341 "ingres.y"
{
			if (yypvt[-0].yyint < 0 || yypvt[-0].yyint >= CM_MAXPROC)
			{
				usrerr("Illegal proc number %d", yypvt[-0].yyint);
				YYERROR;
			}
		} break;
case 19:
# line 351 "ingres.y"
{
			if (yypvt[-0].yyint < 0 || yypvt[-0].yyint >= CM_MAXST)
			{
				usrerr("Illegal state number %d", yypvt[-0].yyint);
				YYERROR;
			}
		} break;
case 20:
# line 361 "ingres.y"
{
			if (yypvt[-0].yyint < 0)
			{
				usrerr("Illegal funcno %d", yypvt[-0].yyint);
				YYERROR;
			}
		} break;
case 23:
# line 377 "ingres.y"
{
			if ((islower(yypvt[-0].yystr[0]) || yypvt[-0].yystr[0] == CLOSED) && yypvt[-0].yystr[1] == '\0')
				yyval.yypip = yypvt[-0].yystr[0];
			else if (yypvt[-0].yystr[0] == '|' && isdigit(yypvt[-0].yystr[1]) && yypvt[-0].yystr[2] == '\0')
				yyval.yypip = yypvt[-0].yystr[1];
			else
			{
				usrerr("Invalid pipe id \"%s\"", yypvt[-0].yystr);
				YYERROR;
			}
		} break;
case 24:
# line 391 "ingres.y"
{
			if (yypvt[-0].yystr[1] != '\0')
			{
				usrerr("Invalid trace flag \"%s\"", yypvt[-0].yystr);
				YYERROR;
			}
			else
				yyval.yychar = yypvt[-0].yystr[0];
		} break;
case 26:
# line 406 "ingres.y"
{
			if (yypvt[-0].yystr[0] < 'A' || yypvt[-0].yystr[0] > 'Z' || yypvt[-0].yystr[1] != '\0')
			{
				usrerr("Invalid macro name \"%s\"", yypvt[-0].yystr);
				YYERROR;
			}
			else
				yyval.yychar = yypvt[-0].yystr[0];
		} break;
		}
		goto yystack;  /* stack new state and value */

	}
