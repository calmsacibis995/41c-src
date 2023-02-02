h48834
s 00031/00000/00000
d D 4.1 83/03/09 16:23:02 ralph 1 0
c date and time created 83/03/09 16:23:02 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * newmat: return a brand new bitmat with the proper size.
 * To get rid of it just call free.
 */

#include "bit.h"

bitmat
newmat(rows, cols)
int rows, cols;
{
	int size = ((cols + 7) >> 3) * rows;
	char *m;

#ifdef TRACE
	if (size <= 0 && trace) {
		fprintf(trace, "newmat: rows=%d, cols=%d\n", rows, cols);
		abort();
	}
	if (trace)
		fprintf(trace, "newmat: malloc(%d) =", size);
#endif
	m = (char *) malloc(size);
#ifdef TRACE
	if (trace)
		fprintf(trace, "%x\n", m);
#endif
	zermat(m, rows, cols);
	return (m);
}
E 1
