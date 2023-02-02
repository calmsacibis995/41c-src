h57336
s 00037/00000/00000
d D 4.1 83/03/09 16:23:09 ralph 1 0
c date and time created 83/03/09 16:23:09 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * get a character from the terminal, with no line buffering.
 */

#include "2648.h"

rawchar()
{
	char c;

	sync();
	escseq(NONE);
	fflush(stdout);
	if (_pb_front && _on2648) {
		c = *_pb_front++;
#ifdef TRACE
		if (trace)
			fprintf(trace, "%s from queue, front=%d, back=%d\n", rdchar(c), _pb_front-_pushback, _pb_back-_pushback);
#endif
		if (_pb_front > _pb_back) {
			_pb_front = _pb_back = NULL;
#ifdef TRACE
			if (trace)
				fprintf(trace, "reset pushback to null\n");
#endif
		}
		return (c);
	}
	_outcount = 0;
	c = getchar();
#ifdef TRACE
	if (trace)
		fprintf(trace, "rawchar '%s'\n", rdchar(c));
#endif
	return (c);
}
E 1
