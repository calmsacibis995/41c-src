h42084
s 00033/00000/00000
d D 4.1 83/03/09 16:23:07 ralph 1 0
c date and time created 83/03/09 16:23:07 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

printg()
{
	int oldvid = _video;
	int c, c2;

	if (oldvid==INVERSE)
		togvid();
	sync();
	escseq(NONE);
	outstr("\33&p4d5u0C");
	outchar('\21');	/* test handshaking fix */

	/*
	 * The terminal sometimes sends back S<cr> or F<cr>.
	 * Ignore them.
	 */
	fflush(stdout);
	c = getchar();
	if (c=='F' || c=='S') {
		c2 = getchar();
		if (c2 != '\r' && c2 != '\n')
			ungetc(c2, stdin);
	} else {
		ungetc(c, stdin);
	}

	if (oldvid==INVERSE)
		togvid();
}
E 1
