# include "sendmail.h"

SCCSID(@(#)macro.c	3.20		2/3/83);

/*
**  EXPAND -- macro expand a string using $x escapes.
**
**	Parameters:
**		s -- the string to expand.
**		buf -- the place to put the expansion.
**		buflim -- the buffer limit, i.e., the address
**			of the last usable position in buf.
**		e -- envelope in which to work.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Bugs:
**		The handling of $$ (to get one dollar) is rather bizarre,
**			especially if there should be another macro
**			expansion in the same string.
*/

expand(s, buf, buflim, e)
	register char *s;
	register char *buf;
	char *buflim;
	register ENVELOPE *e;
{
	register char *q;
	bool skipping;		/* set if conditionally skipping output */
	bool gotone = FALSE;	/* set if any expansion done */
	char xbuf[BUFSIZ];
	register char *xp = xbuf;
	extern char *macvalue();

# ifdef DEBUG
	if (tTd(35, 4))
	{
		printf("expand(");
		xputs(s);
		printf(")\n");
	}
# endif DEBUG

	skipping = FALSE;
	if (s == NULL)
		s = "";
	for (; *s != '\0'; s++)
	{
		char c;

		/*
		**  Check for non-ordinary (special?) character.
		**	'q' will be the interpolated quantity.
		*/

		q = NULL;
		c = *s;
		switch (c)
		{
		  case CONDIF:		/* see if var set */
			c = *++s;
			skipping = macvalue(c, e) == NULL;
			continue;

		  case CONDELSE:	/* change state of skipping */
			skipping = !skipping;
			continue;

		  case CONDFI:		/* stop skipping */
			skipping = FALSE;
			continue;

		  case '$':		/* macro interpolation */
			c = *++s;
			if (c == '$')
				break;
			q = macvalue(c & 0177, e);
			if (q == NULL)
				continue;
			gotone = TRUE;
			break;
		}

		/*
		**  Interpolate q or output one character
		*/

		if (skipping)
			continue;
		while (xp < &xbuf[sizeof xbuf])
		{
			if (q == NULL)
			{
				*xp++ = c;
				break;
			}
			if (*q == '\0')
				break;
			*xp++ = *q++;
		}
	}
	*xp = '\0';

# ifdef DEBUG
	if (tTd(35, 4))
	{
		printf("expand ==> ");
		xputs(xbuf);
		printf("\n");
	}
# endif DEBUG

	/* recurse as appropriate */
	if (gotone)
	{
		expand(xbuf, buf, buflim, e);
		return;
	}

	/* copy results out */
	for (q = buf, xp = xbuf; xp != '\0' && q < buflim-1; )
		*q++ = *xp++;
	*q = '\0';
}
/*
**  DEFINE -- define a macro.
**
**	this would be better done using a #define macro.
**
**	Parameters:
**		n -- the macro name.
**		v -- the macro value.
**		e -- the envelope to store the definition in.
**
**	Returns:
**		none.
**
**	Side Effects:
**		e->e_macro[n] is defined.
**
**	Notes:
**		There is one macro for each ASCII character,
**		although they are not all used.  The currently
**		defined macros are:
**
**		$a   date in ARPANET format (preferring the Date: line
**		     of the message)
**		$b   the current date (as opposed to the date as found
**		     the message) in ARPANET format
**		$c   hop count
**		$d   (current) date in UNIX (ctime) format
**		$e   the SMTP entry message+
**		$f   raw from address
**		$g   translated from address
**		$h   to host
**		$i   queue id
**		$j   official SMTP hostname, used in messages+
**		$l   UNIX-style from line+
**		$n   name of sendmail ("MAILER-DAEMON" on local
**		     net typically)+
**		$o   delimiters ("operators") for address tokens+
**		$p   my process id in decimal
**		$q   the string that becomes an address -- this is
**		     normally used to combine $g & $x.
**		$r   protocol used to talk to sender
**		$s   sender's host name
**		$t   the current time in seconds since 1/1/1970
**		$u   to user
**		$v   version number of sendmail
**		$w   our host name (if it can be determined)
**		$x   signature (full name) of from person
**		$y   the tty id of our terminal
**		$z   home directory of to person
**
**		Macros marked with + must be defined in the
**		configuration file and are used internally, but
**		are not set.
**
**		There are also some macros that can be used
**		arbitrarily to make the configuration file
**		cleaner.  In general all upper-case letters
**		are available.
*/

define(n, v, e)
	char n;
	char *v;
	register ENVELOPE *e;
{
# ifdef DEBUG
	if (tTd(35, 3))
	{
		printf("define(%c as ", n);
		xputs(v);
		printf(")\n");
	}
# endif DEBUG
	e->e_macro[n & 0177] = v;
}
/*
**  MACVALUE -- return uninterpreted value of a macro.
**
**	Parameters:
**		n -- the name of the macro.
**
**	Returns:
**		The value of n.
**
**	Side Effects:
**		none.
*/

char *
macvalue(n, e)
	char n;
	register ENVELOPE *e;
{
	n &= 0177;
	while (e != NULL)
	{
		register char *p = e->e_macro[n];

		if (p != NULL)
			return (p);
		e = e->e_parent;
	}
	return (NULL);
}
