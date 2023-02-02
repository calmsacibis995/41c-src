h21764
s 00020/00000/00000
d D 4.1 83/03/09 16:23:28 ralph 1 0
c date and time created 83/03/09 16:23:28 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

vidnorm()
{
	_video = NORMAL;
}

vidinv()
{
	_video = INVERSE;
}

togvid()
{
	_video = (_video==NORMAL) ? INVERSE : NORMAL;
	escseq(ESCM);
	outstr("3a1b0 0 719 359e");
}
E 1
