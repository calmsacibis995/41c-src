h46858
s 00028/00000/00000
d D 4.1 83/03/09 16:23:19 ralph 1 0
c date and time created 83/03/09 16:23:19 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * setmat: set the value in m[r, c] to nval.
 */

#include "bit.h"

setmat(m, rows, cols, r, c, nval)
bitmat m;
int rows, cols, r, c, nval;
{
	register int offset, thisbit;

	if (r<0 || c<0 || r>=rows || c>=cols) {
#ifdef TRACE
		if (trace)
			fprintf(trace, "setmat range error: (%d, %d) <- %d in a (%d, %d) matrix %x\n", r, c, nval, rows, cols, m);
#endif

		return;
	}
	offset = r*((cols+7)>>3) + (c>>3);
	thisbit = 0x80 >> (c&7);
	if (nval)
		m[offset] |= thisbit;
	else
		m[offset] &= ~thisbit;
}
E 1
