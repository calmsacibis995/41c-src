h15703
s 00057/00000/00000
d D 4.1 83/03/09 16:23:24 ralph 1 0
c date and time created 83/03/09 16:23:24 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * sgtty stuff
 */

#include <sgtty.h>

struct	sgttyb	_ttyb;
struct	tchars	_otch, _ntch;
int	_normf;

/*
 * Routines for dealing with the unix tty modes
 */

#include "2648.h"

ttyinit()
{
	if (strcmp(getenv("TERM"), "hp2648") == 0)
		_on2648 = 1;
	ioctl(fileno(stdin), TIOCGETP, &_ttyb);
	ioctl(fileno(stdin), TIOCGETC, &_otch);
	_ntch = _otch;
	_ntch.t_quitc = _ntch.t_startc = _ntch.t_stopc = -1;
	_normf = _ttyb.sg_flags;
	_ttyb.sg_flags |= CBREAK;
	_ttyb.sg_flags &= ~(ECHO|CRMOD);
	ioctl(fileno(stdin), TIOCSETN, &_ttyb);
	ioctl(fileno(stdin), TIOCSETC, &_ntch);
	gdefault();
	zoomn(1);
	zoomon();
	kon();
	rboff();
	_cursoron = 1;	/* to force it off */
	_escmode = NONE;
	curoff();
	clearg();
	gon();
	aoff();
}

done()
{
	goff();
	koff();
	aon();
	sync();
	escseq(NONE);
	lowleft();
	printf("\n");
	fflush(stdout);
	_ttyb.sg_flags = _normf;
	ioctl(fileno(stdin), TIOCSETN, &_ttyb);
	ioctl(fileno(stdin), TIOCSETC, &_otch);
}
E 1
