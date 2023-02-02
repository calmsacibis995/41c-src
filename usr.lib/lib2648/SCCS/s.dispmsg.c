h47810
s 00035/00000/00000
d D 4.1 83/03/09 16:22:30 ralph 1 0
c date and time created 83/03/09 16:22:30 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * display a message, str, starting at (x, y).
 */

#include "2648.h"

dispmsg(str, x, y, maxlen)
char *str;
int x, y;
{
	int oldx, oldy;
	int oldcuron;
	int oldquiet;
	extern int QUIET;

	oldx = _curx; oldy = _cury;
	oldcuron = _cursoron;
	zoomout();
	areaclear(y, x, y+8, x+6*maxlen);
	setset();
	curon();
	movecurs(x, y);
	texton();
	oldquiet = QUIET;
	QUIET = 0;
	outstr(str);
	if (oldquiet)
		outstr("\r\n");
	QUIET = oldquiet;
	textoff();
	movecurs(oldx, oldy);
	if (oldcuron == 0)
		curoff();
}
E 1
