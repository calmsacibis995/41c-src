h17240
s 00013/00000/00000
d D 4.1 83/03/09 16:23:06 ralph 1 0
c date and time created 83/03/09 16:23:06 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Low level output routines
 */

#include "2648.h"

outstr(str)
char *str;
{
	while (*str)
		outchar(*str++);
}
E 1
