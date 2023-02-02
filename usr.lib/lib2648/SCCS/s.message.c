h18965
s 00012/00000/00000
d D 4.1 83/03/09 16:22:53 ralph 1 0
c date and time created 83/03/09 16:22:53 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * message: print str on the screen in the message area.
 */

#include "2648.h"

message(str)
char *str;
{
	dispmsg(str, 4, 4, 100);
}
E 1
