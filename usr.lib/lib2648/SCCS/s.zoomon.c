h16179
s 00015/00000/00000
d D 4.1 83/03/09 16:23:34 ralph 1 0
c date and time created 83/03/09 16:23:34 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

zoomon()
{
	escseq(ESCD);
	outchar('g');
}

zoomoff()
{
	escseq(ESCD);
	outchar('h');
}
E 1
