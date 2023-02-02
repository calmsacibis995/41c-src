h17234
s 00012/00000/00000
d D 4.1 83/03/09 16:22:43 ralph 1 0
c date and time created 83/03/09 16:22:43 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * reset terminal to default graphics state
 */

#include "2648.h"

gdefault()
{
	escseq(ESCM);
	outstr("r");
}
E 1
