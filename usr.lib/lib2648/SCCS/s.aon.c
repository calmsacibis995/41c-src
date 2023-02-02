h16673
s 00017/00000/00000
d D 4.1 83/03/09 16:22:17 ralph 1 0
c date and time created 83/03/09 16:22:17 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

aon()
{
	sync();
	escseq(ESCD);
	outchar('e');
}

aoff()
{
	sync();
	escseq(ESCD);
	outchar('f');
}
E 1
