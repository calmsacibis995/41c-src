h25163
s 00017/00000/00000
d D 4.1 83/03/09 16:23:30 ralph 1 0
c date and time created 83/03/09 16:23:30 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * zermat: set a matrix to all zeros
 */

#include "bit.h"

zermat(m, rows, cols)
bitmat m;
int rows, cols;
{
	register int size = ((cols + 7) >> 3) * rows;
	register char *p;

	for (p = &m[size]; p>=m; )
		*--p = 0;
}
E 1
