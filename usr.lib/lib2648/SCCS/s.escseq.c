h61045
s 00049/00000/00000
d D 4.1 83/03/09 16:22:41 ralph 1 0
c date and time created 83/03/09 16:22:41 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * escseq: get us out of any escape sequence we are in the middle of
 * and put us into the requested kind of escape sequence.
 */

#include "2648.h"

escseq(mode)
int mode;
{
	if (mode == _escmode)
		return;
	/* Get out of previous mode */
	switch (_escmode) {
	case NONE:
		break;
	case ESCD:
		if (mode == TEXT) {
			outchar('s');
			_escmode = mode;
			return;
		}
	case ESCP:
	case ESCM:
		outchar('Z');	/* no-op */
		break;
	case TEXT:
		outstr("\33*dT");
		break;
	}
	/* Get into new mode */
	switch (_escmode = mode) {
	case NONE:
		break;
	case ESCD:
		outstr("\33*d");
		break;
	case ESCP:
		outstr("\33*p");
		break;
	case ESCM:
		outstr("\33*m");
		break;
	case TEXT:
		outstr("\33*dS");
		break;
	}
}
E 1
