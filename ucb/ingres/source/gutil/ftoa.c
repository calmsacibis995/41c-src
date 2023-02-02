# include	<sccs.h>

SCCSID(@(#)ftoa.c	7.1	2/5/81)

# define	MAXDIG		25

/*
**  FLOATING POINT TO ASCII CONVERSION
**
**	'Value' is converted to an ascii character string and stored
**	into 'ascii'.  Ascii should have room for at least 'width' + 1
**	characters.  'Width' is the width of the output field (max).
**	'Prec' is the number of characters to put after the decimal
**	point.  The format of the output string is controlled by
**	'format'.
**
**	'Format' can be:
**		e or E: "E" format output
**		f or F:  "F" format output
**		g or G:  "F" format output if it will fit, otherwise
**			use "E" format.
**		n or N:  same as G, but decimal points will not always
**			be aligned.
**
**	If 'format' is upper case, the "E" comes out in upper case;
**	otherwise it comes out in lower case.
**
**	When the field width is not big enough, it fills the field with
**	stars ("*****") and returns zero.  Normal return is the width
**	of the output field (sometimes shorter than 'width').
*/

ftoa(value, ascii, width, prec1, format)
double	value;
char	*ascii;
int	width;
int	prec1;
char	format;
{
	auto int	expon;
	auto int	sign;
	register int	avail;
	register char	*a;
	register char	*p;
	char		mode;
	int		lowercase;
	int		prec;
	extern char	*ecvt(), *fcvt();

	prec = prec1;
	mode = format;
	lowercase = 'a' - 'A';
	if (mode >= 'a')
		mode -= 'a' - 'A';
	else
		lowercase = 0;

	if (mode != 'E')
	{
		/* try 'F' style output */
		p = fcvt(value, prec, &expon, &sign);
		avail = width;
		a = ascii;

		/* output sign */
		if (sign)
		{
			avail--;
			*a++ = '-';
		}

		/* output '0' before the decimal point */
		if (expon <= 0)
		{
			*a++ = '0';
			avail--;
		}

		/* compute space length left after dec pt and fraction */
		avail -= prec + 1;
		if (mode == 'G')
			avail -= 4;

		if (avail >= expon)
		{

			/* it fits.  output */
			while (expon > 0)
			{
				/* output left of dp */
				expon--;
				if (*p)
				{
					*a++ = *p++;
				}
				else
					*a++ = '0';
			}

			/* output fraction (right of dec pt) */
			avail = expon;
			goto frac_out;
		}
		/* won't fit; let's hope for G format */
	}

	if (mode != 'F')
	{
		/* try to do E style output */
		p = ecvt(value, prec + 1, &expon, &sign);
		avail = width - 5;
		a = ascii;

		/* output the sign */
		if (sign)
		{
			*a++ = '-';
			avail--;
		}
	}

	/* check for field too small */
	if (mode == 'F' || avail < prec)
	{
		/* sorry joker, you lose */
		a = ascii;
		for (avail = width; avail > 0; avail--)
			*a++ = '*';
		*a = 0;
		return (0);
	}

	/* it fits; output the number */
	mode = 'E';

	/* output the LHS single digit */
	*a++ = *p++;
	expon--;

	/* output the rhs */
	avail = 1;

  frac_out:
	*a++ = '.';
	while (prec > 0)
	{
		prec--;
		if (avail < 0)
		{
			avail++;
			*a++ = '0';
		}
		else
		{
			if (*p)
				*a++ = *p++;
			else
				*a++ = '0';
		}
	}

	/* output the exponent */
	if (mode == 'E')
	{
		*a++ = 'E' + lowercase;
		if (expon < 0)
		{
			*a++ = '-';
			expon = -expon;
		}
		else
			*a++ = '+';
		*a++ = (expon / 10) % 10 + '0';
		*a++ = expon % 10 + '0';
	}

	/* output spaces on the end in G format */
	if (mode == 'G')
	{
		*a++ = ' ';
		*a++ = ' ';
		*a++ = ' ';
		*a++ = ' ';
	}

	/* finally, we can return */
	*a = 0;
	avail = a - ascii;
	return (avail);
}
