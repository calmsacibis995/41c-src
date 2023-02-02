h15703
s 00063/00000/00000
d D 4.1 83/03/09 16:23:14 ralph 1 0
c date and time created 83/03/09 16:23:14 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * Read a line from the keyboard in the message line.  The line
 * goes into caller provided buffer msg, whos size is maxlen bytes.
 */

#include "2648.h"

readline(prompt, msg, maxlen)
char *prompt;
char *msg;
int maxlen;
{
	register char c;
	register char *cp;
	int oldx, oldy;
	int oldcuron;
	int oldquiet;
	extern int QUIET;

	oldx = _curx; oldy = _cury;
	oldcuron = _cursoron;
	areaclear(4, 4, 4+8, 719);
	setset();
	zoomout();
	curon();
	movecurs(4, 4);
	texton();

	oldquiet = QUIET;
	QUIET = 0;
	outstr(prompt);
	if (oldquiet)
		outstr("\r\n");
	QUIET = oldquiet;

	for (cp=msg; ; cp) {
		fflush(stdout);
		c = getchar();
		switch (c) {
		case '\n':
		case '\r':
		case ESC:
			*cp++ = 0;
			textoff();
			movecurs(oldx, oldy);
			if (oldcuron == 0)
				curoff();
			return;
		case '\b':
			if (--cp >= msg)
				outchar(c);
			else
				cp = msg;
			break;
		default:
			*cp++ = c;
			outstr(rdchar(c));
			if (cp-msg >= maxlen)
				error("line too long");
		}
	}
}
E 1
