# include "sendmail.h"

SCCSID(@(#)parseaddr.c	3.80		3/8/83);

/*
**  PARSEADDR -- Parse an address
**
**	Parses an address and breaks it up into three parts: a
**	net to transmit the message on, the host to transmit it
**	to, and a user on that host.  These are loaded into an
**	ADDRESS header with the values squirreled away if necessary.
**	The "user" part may not be a real user; the process may
**	just reoccur on that machine.  For example, on a machine
**	with an arpanet connection, the address
**		csvax.bill@berkeley
**	will break up to a "user" of 'csvax.bill' and a host
**	of 'berkeley' -- to be transmitted over the arpanet.
**
**	Parameters:
**		addr -- the address to parse.
**		a -- a pointer to the address descriptor buffer.
**			If NULL, a header will be created.
**		copyf -- determines what shall be copied:
**			-1 -- don't copy anything.  The printname
**				(q_paddr) is just addr, and the
**				user & host are allocated internally
**				to parse.
**			0 -- copy out the parsed user & host, but
**				don't copy the printname.
**			+1 -- copy everything.
**		delim -- the character to terminate the address, passed
**			to prescan.
**
**	Returns:
**		A pointer to the address descriptor header (`a' if
**			`a' is non-NULL).
**		NULL on error.
**
**	Side Effects:
**		none
*/

/* following delimiters are inherent to the internal algorithms */
# define DELIMCHARS	"$()<>,;\\\"\r\n"	/* word delimiters */

ADDRESS *
parseaddr(addr, a, copyf, delim)
	char *addr;
	register ADDRESS *a;
	int copyf;
	char delim;
{
	register char **pvp;
	register struct mailer *m;
	extern char **prescan();
	extern ADDRESS *buildaddr();

	/*
	**  Initialize and prescan address.
	*/

	CurEnv->e_to = addr;
# ifdef DEBUG
	if (tTd(20, 1))
		printf("\n--parseaddr(%s)\n", addr);
# endif DEBUG

	pvp = prescan(addr, delim);
	if (pvp == NULL)
		return (NULL);

	/*
	**  Apply rewriting rules.
	**	Ruleset 0 does basic parsing.  It must resolve.
	*/

	rewrite(pvp, 3);
	rewrite(pvp, 0);

	/*
	**  See if we resolved to a real mailer.
	*/

	if (pvp[0][0] != CANONNET)
	{
		setstat(EX_USAGE);
		usrerr("cannot resolve name");
		return (NULL);
	}

	/*
	**  Build canonical address from pvp.
	*/

	a = buildaddr(pvp, a);
	if (a == NULL)
		return (NULL);
	m = a->q_mailer;

	/*
	**  Make local copies of the host & user and then
	**  transport them out.
	*/

	if (copyf > 0)
	{
		extern char *DelimChar;
		char savec = *DelimChar;

		*DelimChar = '\0';
		a->q_paddr = newstr(addr);
		*DelimChar = savec;
	}
	else
		a->q_paddr = addr;
	if (copyf >= 0)
	{
		if (a->q_host != NULL)
			a->q_host = newstr(a->q_host);
		else
			a->q_host = "";
		if (a->q_user != a->q_paddr)
			a->q_user = newstr(a->q_user);
	}

	/*
	**  Do UPPER->lower case mapping unless inhibited.
	*/

	if (!bitnset(M_HST_UPPER, m->m_flags))
		makelower(a->q_host);
	if (!bitnset(M_USR_UPPER, m->m_flags))
		makelower(a->q_user);

	/*
	**  Compute return value.
	*/

# ifdef DEBUG
	if (tTd(20, 1))
	{
		printf("parseaddr-->");
		printaddr(a, FALSE);
	}
# endif DEBUG

	return (a);
}
/*
**  PRESCAN -- Prescan name and make it canonical
**
**	Scans a name and turns it into a set of tokens.  This process
**	deletes blanks and comments (in parentheses).
**
**	This routine knows about quoted strings and angle brackets.
**
**	There are certain subtleties to this routine.  The one that
**	comes to mind now is that backslashes on the ends of names
**	are silently stripped off; this is intentional.  The problem
**	is that some versions of sndmsg (like at LBL) set the kill
**	character to something other than @ when reading addresses;
**	so people type "csvax.eric\@berkeley" -- which screws up the
**	berknet mailer.
**
**	Parameters:
**		addr -- the name to chomp.
**		delim -- the delimiter for the address, normally
**			'\0' or ','; \0 is accepted in any case.
**
**	Returns:
**		A pointer to a vector of tokens.
**		NULL on error.
**
**	Side Effects:
**		none.
*/

/* states and character types */
# define OPR		0	/* operator */
# define ATM		1	/* atom */
# define QST		2	/* in quoted string */
# define SPC		3	/* chewing up spaces */
# define ONE		4	/* pick up one character */

# define NSTATES	5	/* number of states */
# define TYPE		017	/* mask to select state type */

/* meta bits for table */
# define M		020	/* meta character; don't pass through */
# define B		040	/* cause a break */
# define MB		M|B	/* meta-break */

static short StateTab[NSTATES][NSTATES] =
{
   /*	oldst	chtype>	OPR	ATM	QST	SPC	ONE	*/
	/*OPR*/		OPR|B,	ATM|B,	QST|B,	SPC|MB,	ONE|B,
	/*ATM*/		OPR|B,	ATM,	QST|B,	SPC|MB,	ONE|B,
	/*QST*/		QST,	QST,	OPR,	QST,	QST,
	/*SPC*/		OPR,	ATM,	QST,	SPC|M,	ONE,
	/*ONE*/		OPR,	OPR,	OPR,	OPR,	OPR,
};

# define NOCHAR		-1	/* signal nothing in lookahead token */

char	*DelimChar;		/* set to point to the delimiter */

char **
prescan(addr, delim)
	char *addr;
	char delim;
{
	register char *p;
	register char *q;
	register int c;
	char **avp;
	bool bslashmode;
	int cmntcnt;
	int anglecnt;
	char *tok;
	int state;
	int newstate;
	static char buf[MAXNAME+MAXATOM];
	static char *av[MAXATOM+1];

	q = buf;
	bslashmode = FALSE;
	cmntcnt = 0;
	anglecnt = 0;
	avp = av;
	state = OPR;
	c = NOCHAR;
	p = addr;
# ifdef DEBUG
	if (tTd(22, 45))
	{
		printf("prescan: ");
		xputs(p);
		putchar('\n');
	}
# endif DEBUG

	do
	{
		/* read a token */
		tok = q;
		for (;;)
		{
			/* store away any old lookahead character */
			if (c != NOCHAR)
			{
				/* squirrel it away */
				if (q >= &buf[sizeof buf - 5])
				{
					usrerr("Address too long");
					DelimChar = p;
					return (NULL);
				}
				*q++ = c;
			}

			/* read a new input character */
			c = *p++;
			if (c == '\0')
				break;
# ifdef DEBUG
			if (tTd(22, 101))
				printf("c=%c, s=%d; ", c, state);
# endif DEBUG

			/* chew up special characters */
			c &= ~0200;
			*q = '\0';
			if (bslashmode)
			{
				c |= 0200;
				bslashmode = FALSE;
			}
			else if (c == '\\')
			{
				bslashmode = TRUE;
				c = NOCHAR;
			}
			else if (state == QST)
			{
				/* do nothing, just avoid next clauses */
			}
			else if (c == '(')
			{
				cmntcnt++;
				c = NOCHAR;
			}
			else if (c == ')')
			{
				if (cmntcnt <= 0)
				{
					usrerr("Unbalanced ')'");
					DelimChar = p;
					return (NULL);
				}
				else
					cmntcnt--;
			}
			else if (cmntcnt > 0)
				c = NOCHAR;
			else if (c == '<')
				anglecnt++;
			else if (c == '>')
			{
				if (anglecnt <= 0)
				{
					usrerr("Unbalanced '>'");
					DelimChar = p;
					return (NULL);
				}
				anglecnt--;
			}
			else if (delim == ' ' && isspace(c))
				c = ' ';

			if (c == NOCHAR)
				continue;

			/* see if this is end of input */
			if (c == delim && anglecnt <= 0 && state != QST)
				break;

			newstate = StateTab[state][toktype(c)];
# ifdef DEBUG
			if (tTd(22, 101))
				printf("ns=%02o\n", newstate);
# endif DEBUG
			state = newstate & TYPE;
			if (bitset(M, newstate))
				c = NOCHAR;
			if (bitset(B, newstate))
				break;
		}

		/* new token */
		if (tok != q)
		{
			*q++ = '\0';
# ifdef DEBUG
			if (tTd(22, 36))
			{
				printf("tok=");
				xputs(tok);
				putchar('\n');
			}
# endif DEBUG
			if (avp >= &av[MAXATOM])
			{
				syserr("prescan: too many tokens");
				DelimChar = p;
				return (NULL);
			}
			*avp++ = tok;
		}
	} while (c != '\0' && (c != delim || anglecnt > 0));
	*avp = NULL;
	DelimChar = --p;
	if (cmntcnt > 0)
		usrerr("Unbalanced '('");
	else if (anglecnt > 0)
		usrerr("Unbalanced '<'");
	else if (state == QST)
		usrerr("Unbalanced '\"'");
	else if (av[0] != NULL)
		return (av);
	return (NULL);
}
/*
**  TOKTYPE -- return token type
**
**	Parameters:
**		c -- the character in question.
**
**	Returns:
**		Its type.
**
**	Side Effects:
**		none.
*/

toktype(c)
	register char c;
{
	static char buf[50];
	static bool firstime = TRUE;

	if (firstime)
	{
		firstime = FALSE;
		expand("$o", buf, &buf[sizeof buf - 1], CurEnv);
		(void) strcat(buf, DELIMCHARS);
	}
	if (c == MATCHCLASS || c == MATCHREPL || c == MATCHNCLASS)
		return (ONE);
	if (c == '"')
		return (QST);
	if (!isascii(c))
		return (ATM);
	if (isspace(c) || c == ')')
		return (SPC);
	if (iscntrl(c) || index(buf, c) != NULL)
		return (OPR);
	return (ATM);
}
/*
**  REWRITE -- apply rewrite rules to token vector.
**
**	This routine is an ordered production system.  Each rewrite
**	rule has a LHS (called the pattern) and a RHS (called the
**	rewrite); 'rwr' points the the current rewrite rule.
**
**	For each rewrite rule, 'avp' points the address vector we
**	are trying to match against, and 'pvp' points to the pattern.
**	If pvp points to a special match value (MATCHZANY, MATCHANY,
**	MATCHONE, MATCHCLASS, MATCHNCLASS) then the address in avp
**	matched is saved away in the match vector (pointed to by 'mvp').
**
**	When a match between avp & pvp does not match, we try to
**	back out.  If we back up over MATCHONE, MATCHCLASS, or MATCHNCLASS
**	we must also back out the match in mvp.  If we reach a
**	MATCHANY or MATCHZANY we just extend the match and start
**	over again.
**
**	When we finally match, we rewrite the address vector
**	and try over again.
**
**	Parameters:
**		pvp -- pointer to token vector.
**
**	Returns:
**		none.
**
**	Side Effects:
**		pvp is modified.
*/

struct match
{
	char	**first;	/* first token matched */
	char	**last;		/* last token matched */
};

# define MAXMATCH	9	/* max params per rewrite */


rewrite(pvp, ruleset)
	char **pvp;
	int ruleset;
{
	register char *ap;		/* address pointer */
	register char *rp;		/* rewrite pointer */
	register char **avp;		/* address vector pointer */
	register char **rvp;		/* rewrite vector pointer */
	register struct match *mlp;	/* cur ptr into mlist */
	register struct rewrite *rwr;	/* pointer to current rewrite rule */
	struct match mlist[MAXMATCH];	/* stores match on LHS */
	char *npvp[MAXATOM+1];		/* temporary space for rebuild */
	extern bool sameword();

	if (OpMode == MD_TEST || tTd(21, 2))
	{
		printf("rewrite: ruleset %2d   input:", ruleset);
		printav(pvp);
	}
	if (pvp == NULL)
		return;

	/*
	**  Run through the list of rewrite rules, applying
	**	any that match.
	*/

	for (rwr = RewriteRules[ruleset]; rwr != NULL; )
	{
# ifdef DEBUG
		if (tTd(21, 12))
		{
			printf("-----trying rule:");
			printav(rwr->r_lhs);
		}
# endif DEBUG

		/* try to match on this rule */
		mlp = mlist;
		rvp = rwr->r_lhs;
		avp = pvp;
		while ((ap = *avp) != NULL || *rvp != NULL)
		{
			rp = *rvp;
# ifdef DEBUG
			if (tTd(21, 35))
			{
				printf("ap=");
				xputs(ap);
				printf(", rp=");
				xputs(rp);
				printf("\n");
			}
# endif DEBUG
			if (rp == NULL)
			{
				/* end-of-pattern before end-of-address */
				goto backup;
			}
			if (ap == NULL && *rp != MATCHZANY)
			{
				/* end-of-input */
				break;
			}

			switch (*rp)
			{
				register STAB *s;

			  case MATCHCLASS:
			  case MATCHNCLASS:
				/* match any token in (not in) a class */
				s = stab(ap, ST_CLASS, ST_FIND);
				if (s == NULL || !bitnset(rp[1], s->s_class))
				{
					if (*rp == MATCHCLASS)
						goto backup;
				}
				else if (*rp == MATCHNCLASS)
					goto backup;

				/* explicit fall-through */

			  case MATCHONE:
			  case MATCHANY:
				/* match exactly one token */
				mlp->first = avp;
				mlp->last = avp++;
				mlp++;
				break;

			  case MATCHZANY:
				/* match zero or more tokens */
				mlp->first = avp;
				mlp->last = avp - 1;
				mlp++;
				break;

			  default:
				/* must have exact match */
				if (!sameword(rp, ap))
					goto backup;
				avp++;
				break;
			}

			/* successful match on this token */
			rvp++;
			continue;

		  backup:
			/* match failed -- back up */
			while (--rvp >= rwr->r_lhs)
			{
				rp = *rvp;
				if (*rp == MATCHANY || *rp == MATCHZANY)
				{
					/* extend binding and continue */
					avp = ++mlp[-1].last;
					avp++;
					rvp++;
					break;
				}
				avp--;
				if (*rp == MATCHONE || *rp == MATCHCLASS ||
				    *rp == MATCHNCLASS)
				{
					/* back out binding */
					mlp--;
				}
			}

			if (rvp < rwr->r_lhs)
			{
				/* total failure to match */
				break;
			}
		}

		/*
		**  See if we successfully matched
		*/

		if (rvp < rwr->r_lhs || *rvp != NULL)
		{
# ifdef DEBUG
			if (tTd(21, 10))
				printf("----- rule fails\n");
# endif DEBUG
			rwr = rwr->r_next;
			continue;
		}

		rvp = rwr->r_rhs;
# ifdef DEBUG
		if (tTd(21, 12))
		{
			printf("-----rule matches:");
			printav(rvp);
		}
# endif DEBUG

		rp = *rvp;
		if (*rp == CANONUSER)
		{
			rvp++;
			rwr = rwr->r_next;
		}
		else if (*rp == CANONHOST)
		{
			rvp++;
			rwr = NULL;
		}
		else if (*rp == CANONNET)
			rwr = NULL;

		/* substitute */
		for (avp = npvp; *rvp != NULL; rvp++)
		{
			register struct match *m;
			register char **pp;

			rp = *rvp;
			if (*rp != MATCHREPL)
			{
				if (avp >= &npvp[MAXATOM])
				{
					syserr("rewrite: expansion too long");
					return;
				}
				*avp++ = rp;
				continue;
			}

			/* substitute from LHS */
			m = &mlist[rp[1] - '1'];
# ifdef DEBUG
			if (tTd(21, 15))
			{
				printf("$%c:", rp[1]);
				pp = m->first;
				while (pp <= m->last)
				{
					printf(" %x=\"", *pp);
					(void) fflush(stdout);
					printf("%s\"", *pp++);
				}
				printf("\n");
			}
# endif DEBUG
			pp = m->first;
			while (pp <= m->last)
			{
				if (avp >= &npvp[MAXATOM])
				{
					syserr("rewrite: expansion too long");
					return;
				}
				*avp++ = *pp++;
			}
		}
		*avp++ = NULL;
		if (**npvp == CALLSUBR)
		{
			bmove((char *) &npvp[2], (char *) pvp,
				(avp - npvp - 2) * sizeof *avp);
# ifdef DEBUG
			if (tTd(21, 3))
				printf("-----callsubr %s\n", npvp[1]);
# endif DEBUG
			rewrite(pvp, atoi(npvp[1]));
		}
		else
		{
			bmove((char *) npvp, (char *) pvp,
				(avp - npvp) * sizeof *avp);
		}
# ifdef DEBUG
		if (tTd(21, 4))
		{
			printf("rewritten as:");
			printav(pvp);
		}
# endif DEBUG
	}

	if (OpMode == MD_TEST || tTd(21, 2))
	{
		printf("rewrite: ruleset %2d returns:", ruleset);
		printav(pvp);
	}
}
/*
**  BUILDADDR -- build address from token vector.
**
**	Parameters:
**		tv -- token vector.
**		a -- pointer to address descriptor to fill.
**			If NULL, one will be allocated.
**
**	Returns:
**		NULL if there was an error.
**		'a' otherwise.
**
**	Side Effects:
**		fills in 'a'
*/

ADDRESS *
buildaddr(tv, a)
	register char **tv;
	register ADDRESS *a;
{
	static char buf[MAXNAME];
	struct mailer **mp;
	register struct mailer *m;
	extern bool sameword();

	if (a == NULL)
		a = (ADDRESS *) xalloc(sizeof *a);
	clear((char *) a, sizeof *a);

	/* figure out what net/mailer to use */
	if (**tv != CANONNET)
	{
		syserr("buildaddr: no net");
		return (NULL);
	}
	tv++;
	if (sameword(*tv, "error"))
	{
		if (**++tv == CANONHOST)
		{
			setstat(atoi(*++tv));
			tv++;
		}
		if (**tv != CANONUSER)
			syserr("buildaddr: error: no user");
		buf[0] = '\0';
		while (*++tv != NULL)
		{
			if (buf[0] != '\0')
				(void) strcat(buf, " ");
			(void) strcat(buf, *tv);
		}
		usrerr(buf);
		return (NULL);
	}
	for (mp = Mailer; (m = *mp++) != NULL; )
	{
		if (sameword(m->m_name, *tv))
			break;
	}
	if (m == NULL)
	{
		syserr("buildaddr: unknown net %s", *tv);
		return (NULL);
	}
	a->q_mailer = m;

	/* figure out what host (if any) */
	tv++;
	if (!bitnset(M_LOCAL, m->m_flags))
	{
		if (**tv++ != CANONHOST)
		{
			syserr("buildaddr: no host");
			return (NULL);
		}
		buf[0] = '\0';
		while (*tv != NULL && **tv != CANONUSER)
			(void) strcat(buf, *tv++);
		a->q_host = newstr(buf);
	}
	else
		a->q_host = NULL;

	/* figure out the user */
	if (**tv != CANONUSER)
	{
		syserr("buildaddr: no user");
		return (NULL);
	}
	rewrite(++tv, 4);
	cataddr(tv, buf, sizeof buf);
	a->q_user = buf;

	return (a);
}
/*
**  CATADDR -- concatenate pieces of addresses (putting in <LWSP> subs)
**
**	Parameters:
**		pvp -- parameter vector to rebuild.
**		buf -- buffer to build the string into.
**		sz -- size of buf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Destroys buf.
*/

cataddr(pvp, buf, sz)
	char **pvp;
	char *buf;
	register int sz;
{
	bool oatomtok = FALSE;
	bool natomtok = FALSE;
	register int i;
	register char *p;

	if (pvp == NULL)
	{
		strcpy(buf, "");
		return;
	}
	p = buf;
	sz -= 2;
	while (*pvp != NULL && (i = strlen(*pvp)) < sz)
	{
		natomtok = (toktype(**pvp) == ATM);
		if (oatomtok && natomtok)
			*p++ = SpaceSub;
		(void) strcpy(p, *pvp);
		oatomtok = natomtok;
		p += i;
		sz -= i + 1;
		pvp++;
	}
	*p = '\0';
}
/*
**  SAMEADDR -- Determine if two addresses are the same
**
**	This is not just a straight comparison -- if the mailer doesn't
**	care about the host we just ignore it, etc.
**
**	Parameters:
**		a, b -- pointers to the internal forms to compare.
**
**	Returns:
**		TRUE -- they represent the same mailbox.
**		FALSE -- they don't.
**
**	Side Effects:
**		none.
*/

bool
sameaddr(a, b)
	register ADDRESS *a;
	register ADDRESS *b;
{
	/* if they don't have the same mailer, forget it */
	if (a->q_mailer != b->q_mailer)
		return (FALSE);

	/* if the user isn't the same, we can drop out */
	if (strcmp(a->q_user, b->q_user) != 0)
		return (FALSE);

	/* if the mailer ignores hosts, we have succeeded! */
	if (bitnset(M_LOCAL, a->q_mailer->m_flags))
		return (TRUE);

	/* otherwise compare hosts (but be careful for NULL ptrs) */
	if (a->q_host == NULL || b->q_host == NULL)
		return (FALSE);
	if (strcmp(a->q_host, b->q_host) != 0)
		return (FALSE);

	return (TRUE);
}
/*
**  PRINTADDR -- print address (for debugging)
**
**	Parameters:
**		a -- the address to print
**		follow -- follow the q_next chain.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
*/

# ifdef DEBUG

printaddr(a, follow)
	register ADDRESS *a;
	bool follow;
{
	bool first = TRUE;

	while (a != NULL)
	{
		first = FALSE;
		printf("%x=", a);
		(void) fflush(stdout);
		printf("%s: mailer %d (%s), host `%s', user `%s'\n", a->q_paddr,
		       a->q_mailer->m_mno, a->q_mailer->m_name, a->q_host,
		       a->q_user);
		printf("\tnext=%x, flags=%o, alias %x\n", a->q_next, a->q_flags,
		       a->q_alias);
		printf("\thome=\"%s\", fullname=\"%s\"\n", a->q_home,
		       a->q_fullname);

		if (!follow)
			return;
		a = a->q_next;
	}
	if (first)
		printf("[NULL]\n");
}

# endif DEBUG
/*
**  REMOTENAME -- return the name relative to the current mailer
**
**	Parameters:
**		name -- the name to translate.
**		m -- the mailer that we want to do rewriting relative
**			to.
**		senderaddress -- if set, uses the sender rewriting rules
**			rather than the recipient rewriting rules.
**		canonical -- if set, strip out any comment information,
**			etc.
**
**	Returns:
**		the text string representing this address relative to
**			the receiving mailer.
**
**	Side Effects:
**		none.
**
**	Warnings:
**		The text string returned is tucked away locally;
**			copy it if you intend to save it.
*/

char *
remotename(name, m, senderaddress, canonical)
	char *name;
	struct mailer *m;
	bool senderaddress;
	bool canonical;
{
	register char **pvp;
	char *fancy;
	extern char *macvalue();
	char *oldg = macvalue('g', CurEnv);
	static char buf[MAXNAME];
	char lbuf[MAXNAME];
	extern char **prescan();
	extern char *crackaddr();

# ifdef DEBUG
	if (tTd(12, 1))
		printf("remotename(%s)\n", name);
# endif DEBUG

	/* don't do anything if we are tagging it as special */
	if ((senderaddress ? m->m_s_rwset : m->m_r_rwset) < 0)
		return (name);

	/*
	**  Do a heuristic crack of this name to extract any comment info.
	**	This will leave the name as a comment and a $g macro.
	*/

	if (canonical)
		fancy = "$g";
	else
		fancy = crackaddr(name);

	/*
	**  Turn the name into canonical form.
	**	Normally this will be RFC 822 style, i.e., "user@domain".
	**	If this only resolves to "user", and the "C" flag is
	**	specified in the sending mailer, then the sender's
	**	domain will be appended.
	*/

	pvp = prescan(name, '\0');
	if (pvp == NULL)
		return (name);
	rewrite(pvp, 3);
	if (CurEnv->e_fromdomain != NULL)
	{
		/* append from domain to this address */
		register char **pxp = pvp;

		/* see if there is an "@domain" in the current name */
		while (*pxp != NULL && strcmp(*pxp, "@") != 0)
			pxp++;
		if (*pxp == NULL)
		{
			/* no.... append the "@domain" from the sender */
			register char **qxq = CurEnv->e_fromdomain;

			while ((*pxp++ = *qxq++) != NULL)
				continue;
		}
	}

	/*
	**  Do more specific rewriting.
	**	Rewrite using ruleset 1 or 2 depending on whether this is
	**		a sender address or not.
	**	Then run it through any receiving-mailer-specific rulesets.
	*/

	if (senderaddress)
	{
		rewrite(pvp, 1);
		if (m->m_s_rwset > 0)
			rewrite(pvp, m->m_s_rwset);
	}
	else
	{
		rewrite(pvp, 2);
		if (m->m_r_rwset > 0)
			rewrite(pvp, m->m_r_rwset);
	}

	/*
	**  Do any final sanitation the address may require.
	**	This will normally be used to turn internal forms
	**	(e.g., user@host.LOCAL) into external form.  This
	**	may be used as a default to the above rules.
	*/

	rewrite(pvp, 4);

	/*
	**  Now restore the comment information we had at the beginning.
	*/

	cataddr(pvp, lbuf, sizeof lbuf);
	define('g', lbuf, CurEnv);
	expand(fancy, buf, &buf[sizeof buf - 1], CurEnv);
	define('g', oldg, CurEnv);

# ifdef DEBUG
	if (tTd(12, 1))
		printf("remotename => `%s'\n", buf);
# endif DEBUG
	return (buf);
}
