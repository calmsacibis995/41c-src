h22467
s 00017/00000/00000
d D 4.1 83/03/09 16:23:00 ralph 1 0
c date and time created 83/03/09 16:23:00 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

movecurs(x, y)
{
	char mes[20];

	if (x==_curx && y==_cury)
		return;
	sprintf(mes, "%d,%do", x, y);
	escseq(ESCD);
	outstr(mes);
	escseq(NONE);
	_curx = x;
	_cury = y;
}
E 1
