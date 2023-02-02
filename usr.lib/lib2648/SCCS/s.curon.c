h23828
s 00023/00000/00000
d D 4.1 83/03/09 16:22:28 ralph 1 0
c date and time created 83/03/09 16:22:28 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

curon()
{
	if (_cursoron)
		return;
	sync();
	escseq(ESCD);
	outchar('k');
	_cursoron = 1;
}

curoff()
{
	if (!_cursoron)
		return;
	sync();
	escseq(ESCD);
	outchar('l');
	_cursoron = 0;
}
E 1
