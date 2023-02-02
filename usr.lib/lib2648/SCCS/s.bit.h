h45119
s 00018/00000/00000
d D 4.1 83/03/09 16:24:51 ralph 1 0
c date and time created 83/03/09 16:24:51 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Bit matrix manipulations for font editor.
 *
 * General structure of a bit matrix: each row is packed into as few
 * bytes as possible, taking the bits from left to right within bytes.
 * The matrix is a sequence of such rows, i.e. up to 7 bits are wasted
 * at the end of each row.
 */

#include <stdio.h>
typedef char *	bitmat;
#ifdef TRACE
	FILE *trace;
#endif

#define max(x,y)	((x) > (y) ?   (x)  : (y))
#define min(x,y)	((x) < (y) ?   (x)  : (y))
E 1
