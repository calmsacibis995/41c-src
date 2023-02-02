h35831
s 00020/00000/00000
d D 4.1 83/03/09 16:22:51 ralph 1 0
c date and time created 83/03/09 16:22:51 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * mat: retrieve the value in m[r, c].
 * rows and cols are the size of the matrix in all these routines.
 */

#include "bit.h"

int
mat(m, rows, cols, r, c)
register bitmat m;
register int c;
int rows, cols, r;
{
	register int thisbyte;

	thisbyte = m[r*((cols+7)>>3) + (c>>3)] & 0xff;
	thisbyte &= 0x80 >> (c&7);
	return (thisbyte != 0);
}
E 1
