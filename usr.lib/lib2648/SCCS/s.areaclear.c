h51355
s 00030/00000/00000
d D 4.1 83/03/09 16:22:19 ralph 1 0
c date and time created 83/03/09 16:22:19 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/

#include "2648.h"

areaclear(rmin, cmin, rmax, cmax)
int rmin, cmin, rmax, cmax;
{
	int osm;
	char mes[20];
	register int i;

#ifdef TRACE
	if (trace)
		fprintf(trace, "areaclear(%d, %d, %d, %d)\n", rmin, cmin, rmax, cmax);
#endif
	osm = _supsmode;
	setclear();
	sync();
#ifdef notdef
	/* old kludge because I couldn't get area fill to work */
	for (i=rmax; i>=rmin; i--) {
		move(cmin, i);
		draw(cmax, i);
	}
#endif
	sprintf(mes, "%da1b%d %d %d %de", (_video==NORMAL) ? 1 : 2, cmin, rmin, cmax, rmax);
	escseq(ESCM);
	outstr(mes);
	_supsmode = osm;
}
E 1
