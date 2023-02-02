h35394
s 00023/00000/00000
d D 4.1 83/03/09 16:22:37 ralph 1 0
c date and time created 83/03/09 16:22:37 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * emptyrow: returns true if row r of m is all zeros.
 *
 * Note that we assume the garbage at the end of the
 * row is all zeros.
 */

#include "bit.h"

emptyrow(m, rows, cols, r)
bitmat m;
int rows, cols, r;
{
	char *top, *bot;

	bot = &m[r*((cols+7)>>3)];
	top = bot + ((cols-1) >> 3);
	while (bot <= top)
		if (*bot++)
			return(0);
	return (1);
}
E 1
