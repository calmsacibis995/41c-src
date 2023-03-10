h63760
s 00036/00000/00000
d D 4.1 83/03/09 16:22:56 ralph 1 0
c date and time created 83/03/09 16:22:56 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Move the pen to x, y.  We assume we are already in ESCP mode.
 */

#include "2648.h"

motion(x, y)
{
	char lox, loy, hix, hiy;
	int delx, dely;

	delx = x-_penx; dely = y-_peny;
	if (-16 <= delx && delx <= 15 && -16 <= dely && dely <= 15) {
		/*
		 * Optimization: if within 15 in both directions, can use
		 * HP short incremental mode, only 3 bytes.
		 */
		outchar('j');
		outchar(32 + (delx & 31));
		outchar(32 + (dely & 31));
	} else {
		/*
		 * Otherwise must use binary absolute mode, 5 bytes.
		 * We never use ascii mode or binary incremental, since
		 * those both take many more bytes.
		 */
		outchar('i');
		outchar(32+ ((x>>5) & 31));
		outchar(32+ (x&31));
		outchar(32+ ((y>>5) & 31));
		outchar(32+ (y&31));
	}
	_penx = x;
	_peny = y;
}
E 1
