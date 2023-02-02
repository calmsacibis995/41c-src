h32764
s 00021/00000/00000
d D 4.1 83/03/09 16:22:31 ralph 1 0
c date and time created 83/03/09 16:22:31 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * draw a line from the current place to (x,y).  Such lines are
 * supposed to be horizontal, and are affected by the current mode.
 */

#include "2648.h"

draw(x, y)
{
#ifdef TRACE
	if (trace) {
		fprintf(trace, "draw(%d,%d)\n", x, y);
	}
#endif
	sync();
	escseq(ESCP);
	motion(x, y);
	_supx = x;
	_supy = y;
}
E 1
