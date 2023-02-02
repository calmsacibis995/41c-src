h37872
s 00022/00000/00000
d D 4.1 83/03/09 16:22:33 ralph 1 0
c date and time created 83/03/09 16:22:33 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Draw a box around a window.  The lower left corner of the box is at (r, c).
 * Color is 1 for drawing a box, 0 for erasing.
 * The box is nrow by ncol.
 */

#include "2648.h"

drawbox(r, c, color, nrow, ncol)
int r, c, color, nrow, ncol;
{
	if (color)
		setset();
	else
		setclear();
	move(c, r);
	draw(c+ncol-1, r);
	draw(c+ncol-1, r+nrow-1);
	draw(c, r+nrow-1);
	draw(c, r);
}
E 1
