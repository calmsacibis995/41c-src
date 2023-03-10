/*	tty.c	4.40	83/02/10	*/

#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/uio.h"
#include "../h/kernel.h"

/*
 * Table giving parity for characters and indicating
 * character classes to tty driver.  In particular,
 * if the low 6 bits are 0, then the character needs
 * no special processing on output.
 */

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0201,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201,

	/*
	 * 7 bit ascii ends with the last character above,
	 * but we contine through all 256 codes for the sake
	 * of the tty output routines which use special vax
	 * instructions which need a 256 character trt table.
	 */

	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007
};

/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */
char	maptab[] ={
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,'|',000,000,000,000,000,'`',
	'{','}',000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,'~',000,
	000,'A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W',
	'X','Y','Z',000,000,000,000,000,
};

short	tthiwat[16] =
   { 100,100,100,100,100,100,100,200,200,400,400,400,650,650,1300,2000 };
short	ttlowat[16] =
   {  30, 30, 30, 30, 30, 30, 30, 50, 50,120,120,120,125,125,125,125 };

struct	ttychars ttydefaults = {
	CERASE,	CKILL,	CINTR,	CQUIT,	CSTART,	CSTOP,	CEOF,
	CBRK,	CSUSP,	CDSUSP, CRPRNT, CFLUSH, CWERASE,CLNEXT
};

ttychars(tp)
	struct tty *tp;
{

	tp->t_chars = ttydefaults;
}

/*
 * Wait for output to drain, then flush input waiting.
 */
wflushtty(tp)
	register struct tty *tp;
{
	int s;

	s = spl5();
	while (tp->t_outq.c_cc && tp->t_state&TS_CARR_ON
	    && tp->t_oproc) {		/* kludge for pty */
		(*tp->t_oproc)(tp);
		tp->t_state |= TS_ASLEEP;
		sleep((caddr_t)&tp->t_outq, TTOPRI);
	}
	flushtty(tp, FREAD);
	splx(s);
}

/*
 * Flush all TTY queues
 */
flushtty(tp, rw)
	register struct tty *tp;
{
	register s;

	s = spl6();
	if (rw & FREAD) {
		while (getc(&tp->t_canq) >= 0)
			;
		wakeup((caddr_t)&tp->t_rawq);
	}
	if (rw & FWRITE) {
		wakeup((caddr_t)&tp->t_outq);
		tp->t_state &= ~TS_TTSTOP;
		(*cdevsw[major(tp->t_dev)].d_stop)(tp, rw);
		while (getc(&tp->t_outq) >= 0)
			;
	}
	if (rw & FREAD) {
		while (getc(&tp->t_rawq) >= 0)
			;
		tp->t_delct = 0;
		tp->t_rocount = 0;
		tp->t_rocol = 0;
		tp->t_state &= ~TS_LOCAL;
	}
	splx(s);
}

/*
 * Send stop character on input overflow.
 */
ttyblock(tp)
	register struct tty *tp;
{
	register x;

	x = tp->t_rawq.c_cc + tp->t_canq.c_cc;
	if (tp->t_rawq.c_cc > TTYHOG) {
		flushtty(tp, FREAD|FWRITE);
		tp->t_state &= ~TS_TBLOCK;
	}
	if (x >= TTYHOG/2 && putc(tp->t_stopc, &tp->t_outq) == 0) {
		tp->t_state |= TS_TBLOCK;
		ttstart(tp);
	}
}

/*
 * Restart typewriter output following a delay
 * timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
ttrstrt(tp)
	register struct tty *tp;
{

	if (tp == 0)
		panic("ttrstrt");
	tp->t_state &= ~TS_TIMEOUT;
	ttstart(tp);
}

/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 */
ttstart(tp)
	register struct tty *tp;
{
	register s;

	s = spl5();
	if ((tp->t_state & (TS_TIMEOUT|TS_TTSTOP|TS_BUSY)) == 0 &&
	    tp->t_oproc)		/* kludge for pty */
		(*tp->t_oproc)(tp);
	splx(s);
}

/*
 * Common code for tty ioctls.
 */
/*ARGSUSED*/
ttioctl(tp, com, data, flag)
	register struct tty *tp;
	caddr_t data;
{
	int dev = tp->t_dev;
	extern int nldisp;
	int s;

	/*
	 * If the ioctl involves modification,
	 * insist on being able to write the device,
	 * and hang if in the background.
	 */
	switch (com) {

	case TIOCSETD:
	case TIOCSETP:
	case TIOCSETN:
	case TIOCFLUSH:
	case TIOCSETC:
	case TIOCSLTC:
	case TIOCSPGRP:
	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
	case TIOCBIS:
	case TIOCBIC:
	case TIOCSET:
	case TIOCSTI:
		while (tp->t_line == NTTYDISC &&
		   u.u_procp->p_pgrp != tp->t_pgrp && tp == u.u_ttyp &&
		   (u.u_procp->p_flag&SVFORK) == 0 &&
		   u.u_signal[SIGTTOU] != SIG_IGN &&
		   u.u_signal[SIGTTOU] != SIG_HOLD) {
			gsignal(u.u_procp->p_pgrp, SIGTTOU);
			sleep((caddr_t)&lbolt, TTOPRI);
		}
		break;
	}

	/*
	 * Process the ioctl.
	 */
	switch (com) {

	/* get discipline number */
	case TIOCGETD:
		*(int *)data = tp->t_line;
		break;

	/* set line discipline */
	case TIOCSETD: {
		register int t = *(int *)data;
		int error = 0;

		if (t >= nldisp)
			return (ENXIO);
		s = spl5();
		if (tp->t_line)
			(*linesw[tp->t_line].l_close)(tp);
		if (t)
			error = (*linesw[t].l_open)(dev, tp);
		splx(s);
		if (error) {
			s = spl5();
			if (tp->t_line)
				(void) (*linesw[tp->t_line].l_open)(dev, tp);
			splx(s);
			return (error);
		}
		tp->t_line = t;
		break;
	}

	/* prevent more opens on channel */
	case TIOCEXCL:
		tp->t_state |= TS_XCLUDE;
		break;

	case TIOCNXCL:
		tp->t_state &= ~TS_XCLUDE;
		break;

	case TIOCSET:
	case TIOCBIS: {
		u_long newflags = *(u_long *)data;

		s = spl5();
		if (tp->t_flags&RAW || newflags&RAW)
			wflushtty(tp);
		else if ((tp->t_flags&CBREAK) != (newflags&CBREAK))
			if (newflags&CBREAK) {
				struct clist tq;

				catq(&tp->t_rawq, &tp->t_canq);
				tq = tp->t_rawq;
				tp->t_rawq = tp->t_canq;
				tp->t_canq = tq;
			} else {
				tp->t_flags |= PENDIN;
				ttwakeup(tp);
			}
		if (com == TIOCSET)
			tp->t_flags = newflags;
		else
			tp->t_flags |= newflags;
		if (tp->t_flags&RAW) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart(tp);
		}
		splx(s);
		break;
	}

	case TIOCBIC: {
		u_long newflags = *(long *)data;

		if (tp->t_flags&RAW)
			wflushtty(tp);
		else if ((tp->t_flags&CBREAK) != (CBREAK&~newflags))
			if (newflags&CBREAK) {
				tp->t_flags |= PENDIN;
				ttwakeup(tp);
			} else {
				struct clist tq;

				catq(&tp->t_rawq, &tp->t_canq);
				tq = tp->t_rawq;
				tp->t_rawq = tp->t_canq;
				tp->t_canq = tq;
			}
		if (tp->t_flags&RAW) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart(tp);
		}
		splx(s);
		break;
	}

	case TIOCGET:
		*(long *)data = tp->t_flags;
		break;

	case TIOCCGET:
		bcopy((caddr_t)&tp->t_chars, data, sizeof (struct ttychars));
		break;

	case TIOCCSET:
		bcopy(data, (caddr_t)&tp->t_chars, sizeof (struct ttychars));
		break;

	/* hang up line on last close */
	case TIOCHPCL:
		tp->t_state |= TS_HUPCLS;
		break;

	case TIOCFLUSH: {
		register int flags = *(int *)data;

		if (flags == 0)
			flags = FREAD|FWRITE;
		else
			flags &= FREAD|FWRITE;
		flushtty(tp, flags);
		break;
	}

	/* return number of characters immediately available */
	case FIONREAD:
		*(off_t *)data = ttnread(tp);
		break;

	case TIOCSTOP:
		s = spl5();
		if ((tp->t_state&TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdevsw[major(tp->t_dev)].d_stop)(tp, 0);
		}
		splx(s);
		break;

	case TIOCSTART:
		s = spl5();
		if ((tp->t_state&TS_TTSTOP) || (tp->t_flags&FLUSHO)) {
			tp->t_state &= ~TS_TTSTOP;
			tp->t_flags &= ~FLUSHO;
			ttstart(tp);
		}
		splx(s);
		break;

	/*
	 * Simulate typing of a character at the terminal.
	 */
	case TIOCSTI:
		if (u.u_uid && u.u_ttyp != tp)
			return (EACCES);
		(*linesw[tp->t_line].l_rint)(*(char *)data, tp);
		break;

	default:
#ifndef NOCOMPAT
		return (ottioctl(tp, com, data, flag));
#else
		return (-1);
#endif
	}
	return (0);
}

ttnread(tp)
	struct tty *tp;
{
	int nread = 0;

	if (tp->t_flags & PENDIN)
		ttypend(tp);
	nread = tp->t_canq.c_cc;
	if (tp->t_flags & (RAW|CBREAK))
		nread += tp->t_rawq.c_cc;
	return (nread);
}

ttselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp = &cdevsw[major(dev)].d_ttys[minor(dev)];
	int nread;
	int s = spl5();

	switch (rw) {

	case FREAD:
		nread = ttnread(tp);
		if (nread > 0)
			goto win;
		if (tp->t_rsel && tp->t_rsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= TS_RCOLL;
		else
			tp->t_rsel = u.u_procp;
		break;

	case FWRITE:
		if (tp->t_outq.c_cc <= TTLOWAT(tp))
			goto win;
		if (tp->t_wsel && tp->t_wsel->p_wchan == (caddr_t)&selwait)
			tp->t_state |= TS_WCOLL;
		else
			tp->t_wsel = u.u_procp;
		break;
	}
	splx(s);
	return (0);
win:
	splx(s);
	return (1);
}

/*
 * Establish a process group for distribution of
 * quits and interrupts from the tty.
 */
ttyopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct proc *pp;

	pp = u.u_procp;
	tp->t_dev = dev;
	if (pp->p_pgrp == 0) {
		u.u_ttyp = tp;
		u.u_ttyd = dev;
		if (tp->t_pgrp == 0)
			tp->t_pgrp = pp->p_pid;
		pp->p_pgrp = tp->t_pgrp;
	}
	tp->t_state &= ~TS_WOPEN;
	tp->t_state |= TS_ISOPEN;
	if (tp->t_line != NTTYDISC)
		wflushtty(tp);
	return (0);
}

/*
 * clean tp on last close
 */
ttyclose(tp)
	register struct tty *tp;
{

	if (tp->t_line) {
		wflushtty(tp);
		tp->t_line = 0;
		return;
	}
	tp->t_pgrp = 0;
	wflushtty(tp);
	tp->t_state = 0;
}

/*
 * reinput pending characters after state switch
 * call at spl5().
 */
ttypend(tp)
	register struct tty *tp;
{
	struct clist tq;
	register c;

	tp->t_flags &= ~PENDIN;
	tp->t_state |= TS_TYPEN;
	tq = tp->t_rawq;
	tp->t_rawq.c_cc = 0;
	tp->t_rawq.c_cf = tp->t_rawq.c_cl = 0;
	while ((c = getc(&tq)) >= 0)
		ttyinput(c, tp);
	tp->t_state &= ~TS_TYPEN;
}

/*
 * Place a character on raw TTY input queue,
 * putting in delimiters and waking up top
 * half as needed.  Also echo if required.
 * The arguments are the character and the
 * appropriate tty structure.
 */
ttyinput(c, tp)
	register c;
	register struct tty *tp;
{
	register int t_flags = tp->t_flags;
	int i;

	/*
	 * If input is pending take it first.
	 */
	if (t_flags&PENDIN)
		ttypend(tp);
	tk_nin++;
	c &= 0377;

	/*
	 * In tandem mode, check high water mark.
	 */
	if (t_flags&TANDEM)
		ttyblock(tp);

	if (t_flags&RAW) {
		/*
		 * Raw mode, just put character
		 * in input q w/o interpretation.
		 */
		if (tp->t_rawq.c_cc > TTYHOG) 
			flushtty(tp, FREAD|FWRITE);
		else {
			if (putc(c, &tp->t_rawq) >= 0)
				ttwakeup(tp);
			ttyecho(c, tp);
		}
		goto endcase;
	}

	/*
	 * Ignore any high bit added during
	 * previous ttyinput processing.
	 */
	if ((tp->t_state&TS_TYPEN) == 0)
		c &= 0177;
	/*
	 * Check for literal nexting very first
	 */
	if (tp->t_state&TS_LNCH) {
		c |= 0200;
		tp->t_state &= ~TS_LNCH;
	}

	/*
	 * Scan for special characters.  This code
	 * is really just a big case statement with
	 * non-constant cases.  The bottom of the
	 * case statement is labeled ``endcase'', so goto
	 * it after a case match, or similar.
	 */
	if (tp->t_line == NTTYDISC) {
		if (c == tp->t_lnextc) {
			if (tp->t_flags&ECHO)
				ttyout("^\b", tp);
			tp->t_state |= TS_LNCH;
			goto endcase;
		}
		if (c == tp->t_flushc) {
			if (tp->t_flags&FLUSHO)
				tp->t_flags &= ~FLUSHO;
			else {
				flushtty(tp, FWRITE);
				ttyecho(c, tp);
				if (tp->t_rawq.c_cc + tp->t_canq.c_cc)
					ttyretype(tp);
				tp->t_flags |= FLUSHO;
			}
			goto startoutput;
		}
		if (c == tp->t_suspc) {
			if ((tp->t_flags&NOFLSH) == 0)
				flushtty(tp, FREAD);
			ttyecho(c, tp);
			gsignal(tp->t_pgrp, SIGTSTP);
			goto endcase;
		}
	}

	/*
	 * Handle start/stop characters.
	 */
	if (c == tp->t_stopc) {
		if ((tp->t_state&TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdevsw[major(tp->t_dev)].d_stop)(tp, 0);
			return;
		}
		if (c != tp->t_startc)
			return;
		goto endcase;
	}
	if (c == tp->t_startc)
		goto restartoutput;

	/*
	 * Look for interrupt/quit chars.
	 */
	if (c == tp->t_intrc || c == tp->t_quitc) {
		if ((tp->t_flags&NOFLSH) == 0)
			flushtty(tp, FREAD|FWRITE);
		ttyecho(c, tp);
		gsignal(tp->t_pgrp, c == tp->t_intrc ? SIGINT : SIGQUIT);
		goto endcase;
	}

	/*
	 * Cbreak mode, don't process line editing
	 * characters; check high water mark for wakeup.
	 */
	if (t_flags&CBREAK) {
		if (tp->t_rawq.c_cc > TTYHOG) {
			if (tp->t_outq.c_cc < TTHIWAT(tp) &&
			    tp->t_line == NTTYDISC)
				(void) ttyoutput(CTRL(g), tp);
		} else if (putc(c, &tp->t_rawq) >= 0) {
			ttwakeup(tp);
			ttyecho(c, tp);
		}
		goto endcase;
	}

	/*
	 * From here on down cooked mode character
	 * processing takes place.
	 */
	if ((tp->t_state&TS_QUOT) &&
	    (c == tp->t_erase || c == tp->t_kill)) {
		ttyrub(unputc(&tp->t_rawq), tp);
		c |= 0200;
	}
	if (c == tp->t_erase) {
		if (tp->t_rawq.c_cc)
			ttyrub(unputc(&tp->t_rawq), tp);
		goto endcase;
	}
	if (c == tp->t_kill) {
		if (tp->t_flags&CRTKIL &&
		    tp->t_rawq.c_cc == tp->t_rocount) {
			while (tp->t_rawq.c_cc)
				ttyrub(unputc(&tp->t_rawq), tp);
		} else {
			ttyecho(c, tp);
			ttyecho('\n', tp);
			while (getc(&tp->t_rawq) > 0)
				;
			tp->t_rocount = 0;
		}
		tp->t_state &= ~TS_LOCAL;
		goto endcase;
	}

	/*
	 * New line discipline,
	 * check word erase/reprint line.
	 */
	if (tp->t_line == NTTYDISC) {
		if (c == tp->t_werasc) {
			if (tp->t_rawq.c_cc == 0)
				goto endcase;
			do {
				c = unputc(&tp->t_rawq);
				if (c != ' ' && c != '\t')
					goto erasenb;
				ttyrub(c, tp);
			} while (tp->t_rawq.c_cc);
			goto endcase;
	erasenb:
			do {
				ttyrub(c, tp);
				if (tp->t_rawq.c_cc == 0)
					goto endcase;
				c = unputc(&tp->t_rawq);
			} while (c != ' ' && c != '\t');
			(void) putc(c, &tp->t_rawq);
			goto endcase;
		}
		if (c == tp->t_rprntc) {
			ttyretype(tp);
			goto endcase;
		}
	}

	/*
	 * Check for input buffer overflow
	 */
	if (tp->t_rawq.c_cc+tp->t_canq.c_cc >= TTYHOG) {
		if (tp->t_line == NTTYDISC)
			(void) ttyoutput(CTRL(g), tp);
		goto endcase;
	}

	/*
	 * Put data char in q for user and
	 * wakeup on seeing a line delimiter.
	 */
	if (putc(c, &tp->t_rawq) >= 0) {
		if (ttbreakc(c, tp)) {
			tp->t_rocount = 0;
			catq(&tp->t_rawq, &tp->t_canq);
			ttwakeup(tp);
		} else if (tp->t_rocount++ == 0)
			tp->t_rocol = tp->t_col;
		tp->t_state &= ~TS_QUOT;
		if (c == '\\')
			tp->t_state |= TS_QUOT;
		if (tp->t_state&TS_ERASE) {
			tp->t_state &= ~TS_ERASE;
			(void) ttyoutput('/', tp);
		}
		i = tp->t_col;
		ttyecho(c, tp);
		if (c == tp->t_eofc && tp->t_flags&ECHO) {
			i = MIN(2, tp->t_col - i);
			while (i > 0) {
				(void) ttyoutput('\b', tp);
				i--;
			}
		}
	}

endcase:
	/*
	 * If DEC-style start/stop is enabled don't restart
	 * output until seeing the start character.
	 */
	if (tp->t_flags&DECCTQ && tp->t_state&TS_TTSTOP &&
	    tp->t_startc != tp->t_stopc)
		return;

restartoutput:
	tp->t_state &= ~TS_TTSTOP;
	tp->t_flags &= ~FLUSHO;

startoutput:
	ttstart(tp);
}

/*
 * Put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * This is called both from the top half for output,
 * and from interrupt level for echoing.
 * The arguments are the character and the tty structure.
 * Returns < 0 if putc succeeds, otherwise returns char to resend
 * Must be recursive.
 */
ttyoutput(c, tp)
	register c;
	register struct tty *tp;
{
	register char *colp;
	register ctype;

	if (tp->t_flags & (RAW|LITOUT)) {
		if (tp->t_flags&FLUSHO)
			return (-1);
		if (putc(c, &tp->t_outq))
			return (c);
		tk_nout++;
		return (-1);
	}

	/*
	 * Ignore EOT in normal mode to avoid
	 * hanging up certain terminals.
	 */
	c &= 0177;
	if (c == CEOT && (tp->t_flags&CBREAK) == 0)
		return (-1);
	/*
	 * Turn tabs to spaces as required
	 */
	if (c == '\t' && (tp->t_flags&TBDELAY) == XTABS) {
		register int s;

		c = 8 - (tp->t_col&7);
		if ((tp->t_flags&FLUSHO) == 0) {
			s = spl5();		/* don't interrupt tabs */
			c -= b_to_q("        ", c, &tp->t_outq);
			tk_nout += c;
			splx(s);
		}
		tp->t_col += c;
		return (c ? -1 : '\t');
	}
	tk_nout++;
	/*
	 * for upper-case-only terminals,
	 * generate escapes.
	 */
	if (tp->t_flags&LCASE) {
		colp = "({)}!|^~'`";
		while (*colp++)
			if (c == *colp++) {
				if (ttyoutput('\\', tp) >= 0)
					return (c);
				c = colp[-2];
				break;
			}
		if ('A' <= c && c <= 'Z') {
			if (ttyoutput('\\', tp) >= 0)
				return (c);
		} else if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
	}

	/*
	 * turn <nl> to <cr><lf> if desired.
	 */
	if (c == '\n' && tp->t_flags&CRMOD)
		if (ttyoutput('\r', tp) >= 0)
			return (c);
	if (c == '~' && tp->t_flags&TILDE)
		c = '`';
	if ((tp->t_flags&FLUSHO) == 0 && putc(c, &tp->t_outq))
		return (c);
	/*
	 * Calculate delays.
	 * The numbers here represent clock ticks
	 * and are not necessarily optimal for all terminals.
	 * The delays are indicated by characters above 0200.
	 * In raw mode there are no delays and the
	 * transmission path is 8 bits wide.
	 *
	 * SHOULD JUST ALLOW USER TO SPECIFY DELAYS
	 */
	colp = &tp->t_col;
	ctype = partab[c];
	c = 0;
	switch (ctype&077) {

	case ORDINARY:
		(*colp)++;

	case CONTROL:
		break;

	case BACKSPACE:
		if (*colp)
			(*colp)--;
		break;

	case NEWLINE:
		ctype = (tp->t_flags >> 8) & 03;
		if (ctype == 1) { /* tty 37 */
			if (*colp)
				c = max(((unsigned)*colp>>4) + 3, (unsigned)6);
		} else if (ctype == 2) /* vt05 */
			c = 6;
		*colp = 0;
		break;

	case TAB:
		ctype = (tp->t_flags >> 10) & 03;
		if (ctype == 1) { /* tty 37 */
			c = 1 - (*colp | ~07);
			if (c < 5)
				c = 0;
		}
		*colp |= 07;
		(*colp)++;
		break;

	case VTAB:
		if (tp->t_flags&VTDELAY) /* tty 37 */
			c = 0177;
		break;

	case RETURN:
		ctype = (tp->t_flags >> 12) & 03;
		if (ctype == 1) /* tn 300 */
			c = 5;
		else if (ctype == 2) /* ti 700 */
			c = 10;
		else if (ctype == 3) { /* concept 100 */
			int i;

			if ((i = *colp) >= 0)
				for (; i < 9; i++)
					(void) putc(0177, &tp->t_outq);
		}
		*colp = 0;
	}
	if (c && (tp->t_flags&FLUSHO) == 0)
		(void) putc(c|0200, &tp->t_outq);
	return (-1);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 */
ttread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register struct clist *qp;
	register c, t_flags;
	int s, first, error = 0;

	if ((tp->t_state&TS_CARR_ON)==0)
		return (EIO);
loop:
	/*
	 * Take any pending input first.
	 */
	s = spl5();
	if (tp->t_flags&PENDIN)
		ttypend(tp);
	splx(s);

	/*
	 * Hang process if it's in the background.
	 */
	while (tp == u.u_ttyp && u.u_procp->p_pgrp != tp->t_pgrp) {
		if (u.u_signal[SIGTTIN] == SIG_IGN ||
		    u.u_signal[SIGTTIN] == SIG_HOLD ||
/*
		    (u.u_procp->p_flag&SDETACH) ||
*/
		    u.u_procp->p_flag&SVFORK)
			return (EIO);
		gsignal(u.u_procp->p_pgrp, SIGTTIN);
		sleep((caddr_t)&lbolt, TTIPRI);
	}
	t_flags = tp->t_flags;

	/*
	 * In raw mode take characters directly from the
	 * raw queue w/o processing.  Interlock against
	 * device interrupts when interrogating rawq.
	 */
	if (t_flags&RAW) {
		s = spl5();
		if (tp->t_rawq.c_cc <= 0) {
			if ((tp->t_state&TS_CARR_ON) == 0 ||
			    (tp->t_state&TS_NBIO)) {
				splx(s);
				return (0);
			}
			sleep((caddr_t)&tp->t_rawq, TTIPRI);
			splx(s);
			goto loop;
		}
		splx(s);
		while (!error && tp->t_rawq.c_cc && uio->uio_iovcnt)
			error = passuc(getc(&tp->t_rawq), uio);
		goto checktandem;
	}

	/*
	 * In cbreak mode use the rawq, otherwise
	 * take characters from the canonicalized q.
	 */
	qp = t_flags&CBREAK ? &tp->t_rawq : &tp->t_canq;

	/*
	 * No input, sleep on rawq awaiting hardware
	 * receipt and notification.
	 */
	s = spl5();
	if (qp->c_cc <= 0) {
		if ((tp->t_state&TS_CARR_ON) == 0 ||
		    (tp->t_state&TS_NBIO)) {
			splx(s);
			return (EWOULDBLOCK);
		}
		sleep((caddr_t)&tp->t_rawq, TTIPRI);
		splx(s);
		goto loop;
	}
	splx(s);

	/*
	 * Input present, perform input mapping
	 * and processing (we're not in raw mode).
	 */
	first = 1;
	while ((c = getc(qp)) >= 0) {
		if (t_flags&CRMOD && c == '\r')
			c = '\n';
		/*
		 * Hack lower case simulation on
		 * upper case only terminals.
		 */
		if (t_flags&LCASE && c <= 0177)
			if (tp->t_state&TS_BKSL) {
				if (maptab[c])
					c = maptab[c];
				tp->t_state &= ~TS_BKSL;
			} else if (c >= 'A' && c <= 'Z')
				c += 'a' - 'A';
			else if (c == '\\') {
				tp->t_state |= TS_BKSL;
				continue;
			}
		/*
		 * Check for delayed suspend character.
		 */
		if (tp->t_line == NTTYDISC && c == tp->t_dsuspc) {
			gsignal(tp->t_pgrp, SIGTSTP);
			if (first) {
				sleep((caddr_t)&lbolt, TTIPRI);
				goto loop;
			}
			break;
		}
		/*
		 * Interpret EOF only in cooked mode.
		 */
		if (c == tp->t_eofc && (t_flags&CBREAK) == 0)
			break;
		/*
		 * Give user character.
		 */
		error = passuc(c & 0177, uio);
		if (error)
			break;
		if (uio->uio_iovcnt == 0)
			break;
		/*
		 * In cooked mode check for a "break character"
		 * marking the end of a "line of input".
		 */
		if ((t_flags&CBREAK) == 0 && ttbreakc(c, tp))
			break;
		first = 0;
	}
	tp->t_state &= ~TS_BKSL;

checktandem:
	/*
	 * Look to unblock output now that (presumably)
	 * the input queue has gone down.
	 */
	if (tp->t_state&TS_TBLOCK && tp->t_rawq.c_cc < TTYHOG/5)
		if (putc(tp->t_startc, &tp->t_outq) == 0) {
			tp->t_state &= ~TS_TBLOCK;
			ttstart(tp);
		}
	return (error);
}

/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 */
ttwrite(tp, uio)
	register struct tty *tp;
	register struct uio *uio;
{
	register char *cp;
	register int cc, ce, c;
	int i, hiwat, cnt, error, s;
	char obuf[OBUFSIZ];

	if ((tp->t_state&TS_CARR_ON) == 0)
		return (EIO);
	hiwat = TTHIWAT(tp);
	cnt = uio->uio_resid;
	error = 0;
loop:
	/*
	 * Hang the process if it's in the background.
	 */
	while (u.u_procp->p_pgrp != tp->t_pgrp && tp == u.u_ttyp &&
	    (tp->t_flags&TOSTOP) && (u.u_procp->p_flag&SVFORK)==0 &&
	    u.u_signal[SIGTTOU] != SIG_IGN &&
	    u.u_signal[SIGTTOU] != SIG_HOLD
/*
					     &&
	    (u.u_procp->p_flag&SDETACH)==0) {
*/
	    ) {
		gsignal(u.u_procp->p_pgrp, SIGTTOU);
		sleep((caddr_t)&lbolt, TTIPRI);
	}

	/*
	 * Process the user's data in at most OBUFSIZ
	 * chunks.  Perform lower case simulation and
	 * similar hacks.  Keep track of high water
	 * mark, sleep on overflow awaiting device aid
	 * in acquiring new space.
	 */
	while (uio->uio_resid > 0) {
		/*
		 * Grab a hunk of data from the user.
		 */
		cc = uio->uio_iov->iov_len;
		if (cc == 0) {
			uio->uio_iovcnt--;
			uio->uio_iov++;
			if (uio->uio_iovcnt < 0)
				panic("ttwrite");
			continue;
		}
		if (cc > OBUFSIZ)
			cc = OBUFSIZ;
		cp = obuf;
		error = uiomove(cp, (unsigned)cc, UIO_WRITE, uio);
		if (error)
			break;
		if (tp->t_outq.c_cc > hiwat)
			goto ovhiwat;
		if (tp->t_flags&FLUSHO)
			continue;
		/*
		 * If we're mapping lower case or kludging tildes,
		 * then we've got to look at each character, so
		 * just feed the stuff to ttyoutput...
		 */
		if (tp->t_flags & (LCASE|TILDE)) {
			while (cc > 0) {
				c = *cp++;
				tp->t_rocount = 0;
				while ((c = ttyoutput(c, tp)) >= 0) {
					/* out of clists, wait a bit */
					ttstart(tp);
					sleep((caddr_t)&lbolt, TTOPRI);
					tp->t_rocount = 0;
				}
				--cc;
				if (tp->t_outq.c_cc > hiwat)
					goto ovhiwat;
			}
			continue;
		}
		/*
		 * If nothing fancy need be done, grab those characters we
		 * can handle without any of ttyoutput's processing and
		 * just transfer them to the output q.  For those chars
		 * which require special processing (as indicated by the
		 * bits in partab), call ttyoutput.  After processing
		 * a hunk of data, look for FLUSHO so ^O's will take effect
		 * immediately.
		 */
		while (cc > 0) {
			if (tp->t_flags & (RAW|LITOUT))
				ce = cc;
			else {
				ce = cc - scanc(cc, cp, partab, 077);
				/*
				 * If ce is zero, then we're processing
				 * a special character through ttyoutput.
				 */
				if (ce == 0) {
					tp->t_rocount = 0;
					if (ttyoutput(*cp, tp) >= 0) {
						/* no c-lists, wait a bit */
						ttstart(tp);
						sleep((caddr_t)&lbolt, TTOPRI);
						continue;
					}
					cp++, cc--;
					if (tp->t_flags&FLUSHO ||
					    tp->t_outq.c_cc > hiwat)
						goto ovhiwat;
					continue;
				}
			}
			/*
			 * A bunch of normal characters have been found,
			 * transfer them en masse to the output queue and
			 * continue processing at the top of the loop.
			 * If there are any further characters in this
			 * <= OBUFSIZ chunk, the first should be a character
			 * requiring special handling by ttyoutput.
			 */
			tp->t_rocount = 0;
			i = b_to_q(cp, ce, &tp->t_outq);
			ce -= i;
			tp->t_col += ce;
			cp += ce, cc -= ce, tk_nout += ce;
			if (i > 0) {
				/* out of c-lists, wait a bit */
				ttstart(tp);
				sleep((caddr_t)&lbolt, TTOPRI);
			}
			if (tp->t_flags&FLUSHO || tp->t_outq.c_cc > hiwat)
				goto ovhiwat;
		}
	}
	ttstart(tp);
	return (error);

ovhiwat:
	s = spl5();
	if (cc != 0) {
		uio->uio_iov->iov_base -= cc;
		uio->uio_iov->iov_len += cc;
		uio->uio_resid += cc;
		uio->uio_offset -= cc;
	}
	/*
	 * This can only occur if FLUSHO
	 * is also set in t_flags.
	 */
	if (tp->t_outq.c_cc <= hiwat) {
		splx(s);
		goto loop;
	}
	ttstart(tp);
	if (tp->t_state&TS_NBIO) {
		if (uio->uio_resid == cnt)
			return (EWOULDBLOCK);
		return (0);
	}
	tp->t_state |= TS_ASLEEP;
	sleep((caddr_t)&tp->t_outq, TTOPRI);
	splx(s);
	goto loop;
}

/*
 * Rubout one character from the rawq of tp
 * as cleanly as possible.
 */
ttyrub(c, tp)
	register c;
	register struct tty *tp;
{
	register char *cp;
	register int savecol;
	int s;
	char *nextc();

	if ((tp->t_flags&ECHO) == 0)
		return;
	tp->t_flags &= ~FLUSHO;
	c &= 0377;
	if (tp->t_flags&CRTBS) {
		if (tp->t_rocount == 0) {
			/*
			 * Screwed by ttwrite; retype
			 */
			ttyretype(tp);
			return;
		}
		if (c == ('\t'|0200) || c == ('\n'|0200))
			ttyrubo(tp, 2);
		else switch (partab[c&=0177]&0177) {

		case ORDINARY:
			if (tp->t_flags&LCASE && c >= 'A' && c <= 'Z')
				ttyrubo(tp, 2);
			else
				ttyrubo(tp, 1);
			break;

		case VTAB:
		case BACKSPACE:
		case CONTROL:
		case RETURN:
			if (tp->t_flags&CTLECH)
				ttyrubo(tp, 2);
			break;

		case TAB:
			if (tp->t_rocount < tp->t_rawq.c_cc) {
				ttyretype(tp);
				return;
			}
			s = spl5();
			savecol = tp->t_col;
			tp->t_state |= TS_CNTTB;
			tp->t_flags |= FLUSHO;
			tp->t_col = tp->t_rocol;
			cp = tp->t_rawq.c_cf;
			for (; cp; cp = nextc(&tp->t_rawq, cp))
				ttyecho(*cp, tp);
			tp->t_flags &= ~FLUSHO;
			tp->t_state &= ~TS_CNTTB;
			splx(s);
			/*
			 * savecol will now be length of the tab
			 */
			savecol -= tp->t_col;
			tp->t_col += savecol;
			if (savecol > 8)
				savecol = 8;		/* overflow screw */
			while (--savecol >= 0)
				(void) ttyoutput('\b', tp);
			break;

		default:
			panic("ttyrub");
		}
	} else if (tp->t_flags&PRTERA) {
		if ((tp->t_state&TS_ERASE) == 0) {
			(void) ttyoutput('\\', tp);
			tp->t_state |= TS_ERASE;
		}
		ttyecho(c, tp);
	} else
		ttyecho(tp->t_erase, tp);
	tp->t_rocount--;
}

/*
 * Crt back over cnt chars perhaps
 * erasing them.
 */
ttyrubo(tp, cnt)
	register struct tty *tp;
	int cnt;
{
	register char *rubostring = tp->t_flags&CRTERA ? "\b \b" : "\b";

	while (--cnt >= 0)
		ttyout(rubostring, tp);
}

/*
 * Reprint the rawq line.
 * We assume c_cc has already been checked.
 */
ttyretype(tp)
	register struct tty *tp;
{
	register char *cp;
	char *nextc();
	int s;

	if (tp->t_rprntc != 0377)
		ttyecho(tp->t_rprntc, tp);
	(void) ttyoutput('\n', tp);
	s = spl5();
	for (cp = tp->t_canq.c_cf; cp; cp = nextc(&tp->t_canq, cp))
		ttyecho(*cp, tp);
	for (cp = tp->t_rawq.c_cf; cp; cp = nextc(&tp->t_rawq, cp))
		ttyecho(*cp, tp);
	tp->t_state &= ~TS_ERASE;
	splx(s);
	tp->t_rocount = tp->t_rawq.c_cc;
	tp->t_rocol = 0;
}

/*
 * Echo a typed character to the terminal
 */
ttyecho(c, tp)
	register c;
	register struct tty *tp;
{

	if ((tp->t_state&TS_CNTTB) == 0)
		tp->t_flags &= ~FLUSHO;
	if ((tp->t_flags&ECHO) == 0)
		return;
	c &= 0377;
	if (tp->t_flags&RAW) {
		(void) ttyoutput(c, tp);
		return;
	}
	if (c == '\r' && tp->t_flags&CRMOD)
		c = '\n';
	if (tp->t_flags&CTLECH) {
		if ((c&0177) <= 037 && c!='\t' && c!='\n' || (c&0177)==0177) {
			(void) ttyoutput('^', tp);
			c &= 0177;
			if (c == 0177)
				c = '?';
			else if (tp->t_flags&LCASE)
				c += 'a' - 1;
			else
				c += 'A' - 1;
		}
	}
	if ((tp->t_flags&LCASE) && (c >= 'A' && c <= 'Z'))
		c += 'a' - 'A';
	(void) ttyoutput(c&0177, tp);
}

/*
 * Is c a break char for tp?
 */
ttbreakc(c, tp)
	register c;
	register struct tty *tp;
{
	return (c == '\n' || c == tp->t_eofc || c == tp->t_brkc ||
		c == '\r' && (tp->t_flags&CRMOD));
}

/*
 * send string cp to tp
 */
ttyout(cp, tp)
	register char *cp;
	register struct tty *tp;
{
	register char c;

	while (c = *cp++)
		(void) ttyoutput(c, tp);
}

ttwakeup(tp)
	struct tty *tp;
{

	if (tp->t_rsel) {
		selwakeup(tp->t_rsel, tp->t_state&TS_RCOLL);
		tp->t_state &= ~TS_RCOLL;
		tp->t_rsel = 0;
	}
	wakeup((caddr_t)&tp->t_rawq);
}

#if !defined(vax) && !defined(sun)
scanc(size, cp, table, mask)
	register int size;
	register char *cp, table[];
	register int mask;
{
	register int i = 0;

	while ((table[*(u_char *)(cp + i)]&mask) == 0 && i < size)
		i++;
	return (i);
}
#endif
