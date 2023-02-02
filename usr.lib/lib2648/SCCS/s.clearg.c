h15192
s 00010/00000/00000
d D 4.1 83/03/09 16:22:26 ralph 1 0
c date and time created 83/03/09 16:22:26 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

clearg()
{
	sync();
	escseq(ESCD);
	outchar((_video==INVERSE) ? 'b' : 'a');
}
E 1
