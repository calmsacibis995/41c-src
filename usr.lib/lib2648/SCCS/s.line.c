h37860
s 00025/00000/00000
d D 4.1 83/03/09 16:22:49 ralph 1 0
c date and time created 83/03/09 16:22:49 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * line: draw a line from point 1 to point 2.
 */

#include "2648.h"

line(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
#ifdef TRACE
	if (trace)
		fprintf(trace, "line((%d, %d), (%d, %d)),", x1, y1, x2, y2);
#endif
	if (x1==_penx && y1==_peny) {
		/*
		 * Get around a bug in the HP terminal where one point
		 * lines don't get drawn more than once.
		 */
		move(x1, y1+1);
		sync();
	}
	move(x1, y1);
	draw(x2, y2);
}
E 1
