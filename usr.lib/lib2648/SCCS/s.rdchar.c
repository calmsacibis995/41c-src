h45122
s 00026/00000/00000
d D 4.1 83/03/09 16:23:12 ralph 1 0
c date and time created 83/03/09 16:23:12 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * rdchar: returns a readable representation of an ASCII char, using ^ notation.
 */

#include <ctype.h>

char *rdchar(c)
char c;
{
	static char ret[4];
	register char *p;

	/*
	 * Due to a bug in isprint, this prints spaces as ^`, but this is OK
	 * because we want something to show up on the screen.
	 */
	ret[0] = ((c&0377) > 0177) ? '\'' : ' ';
	c &= 0177;
	ret[1] = isprint(c) ? ' ' : '^';
	ret[2] = isprint(c) ?  c  : c^0100;
	ret[3] = 0;
	for (p=ret; *p==' '; p++)
		;
	return (p);
}
E 1
