h22356
s 00018/00000/00000
d D 4.1 83/03/09 16:22:46 ralph 1 0
c date and time created 83/03/09 16:22:46 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Turn on keypad, so it sends codes instead of doing them in local.
 */

#include "2648.h"

kon()
{
	escseq(NONE);
	outstr("\33&s1A");
}

koff()
{
	escseq(NONE);
	outstr("\33&s0A");
}
E 1
