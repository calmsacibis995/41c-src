h29824
s 00017/00000/00000
d D 4.1 83/03/09 16:22:58 ralph 1 0
c date and time created 83/03/09 16:22:58 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * move to (x, y).  Both the _pen and cursor are supposed to be moved.
 * We really just remember it for later, in case we move again.
 */

#include "2648.h"

move(x, y)
{
#ifdef TRACE
	if (trace)
		fprintf(trace, "\tmove(%d, %d), ", x, y);
#endif
	_supx = x;
	_supy = y;
}
E 1
