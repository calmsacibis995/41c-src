h26515
s 00022/00000/00000
d D 4.1 83/03/09 16:22:13 ralph 1 0
c date and time created 83/03/09 16:22:13 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * position the alphanumeric cursor to (x, y).
 */

#include "2648.h"

agoto(x, y)
int x, y;
{
	char mes[20];
	sprintf(mes, "\33*dE\33&a%dr%dC", x, y);
	outstr(mes);
}

/*
 * lower left corner of screen.
 */
lowleft()
{
	outstr("\33F");
}
E 1
