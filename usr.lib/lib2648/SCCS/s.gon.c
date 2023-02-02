h16683
s 00017/00000/00000
d D 4.1 83/03/09 16:22:45 ralph 1 0
c date and time created 83/03/09 16:22:45 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

gon()
{
	sync();
	escseq(ESCD);
	outchar('c');
}

goff()
{
	sync();
	escseq(ESCD);
	outchar('d');
}
E 1
