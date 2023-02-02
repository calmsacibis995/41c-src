h23117
s 00021/00000/00000
d D 4.1 83/03/09 16:23:16 ralph 1 0
c date and time created 83/03/09 16:23:16 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Routines to set line type.
 */

#include "2648.h"

setxor()
{
	_supsmode = MX;
}

setclear()
{
	_supsmode = _video==INVERSE ? MS : MC;
}

setset()
{
	_supsmode = _video==INVERSE ? MC : MS;
}
E 1
