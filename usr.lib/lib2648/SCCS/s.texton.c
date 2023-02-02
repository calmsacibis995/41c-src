h31990
s 00024/00000/00000
d D 4.1 83/03/09 16:23:23 ralph 1 0
c date and time created 83/03/09 16:23:23 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

texton()
{
	sync();
	escseq(TEXT);
}

textoff()
{
	sync();

	/*
	 * The following is needed because going into text mode
	 * leaves the pen where the cursor last was.
	 */
	_penx = -40; _peny = 40;
	escseq(ESCP);
	outchar('a');
	motion(_supx, _supy);
	_penx = _supx; _peny = _supy;
}
E 1
