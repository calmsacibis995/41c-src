h16271
s 00012/00000/00000
d D 4.1 83/03/09 16:23:32 ralph 1 0
c date and time created 83/03/09 16:23:32 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

zoomn(size)
char size;
{
	sync();
	escseq(ESCD);
	outchar(size+'0');
	outchar('i');
}
E 1
