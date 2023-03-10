/*	@(#)signal.c	4.3	(Berkeley)	12/28/82	*/

/*
 * C library -- sigsys
 *
 * sigsys(n, SIG_DFL);		default action on signal(n)
 * sigsys(n, SIG_HOLD);		block signal temporarily
 * sigsys(n, SIG_IGN);		ignore signal(n)
 * sigsys(n, label);		goto label on signal(n)
 * sigsys(n, DEFERSIG(label));	goto label with signal SIG_HOLD
 *
 * returns old label, only one level.
 */
#include "SYS.h"

#define	SIGDORTI	0x200
#define	SYS_signal	48
#define	SYS_sigsys	SYS_signal

SYSCALL(sigsys)
	ret

/*
 * sigpeel(n, newact)
 *
 * when called from routine which was called by system, peels
 * back frames to the last one, then calls the system to reenable
 * the signal with newact, arranging to clean the stack before the
 * signal can happen again
 */
#define	RETOFF	16	/* offset of return address in frame */

ENTRY(sigpeel)
	movl	4(ap),r0	/* get signal number value to set */
	movl	8(ap),r1
	movab	unw1,RETOFF(fp)
	ret			/* peel off our frame */
/* top frame is now frame of routine signal action called */
unw1:
	movab	unw2,RETOFF(fp)
	ret
/*
 * now frame from callg of ``locore.s/sigcode()'' is gone
 * if no value to set, can just ret now to go back to old code
 * with an ret->rei, else must make the frame passed to us into a
 * signal call frame by putting the new signal code in the
 * second word, and call the kernel which will pop off the frame.
 */
unw2:
	tstl	r1
	bneq	unw3
	ret			/* easy */
/* mark dorti wanted, and reenable signal */
unw3:
	bisl3	$SIGDORTI,r0,4(ap)
	movl	r1,8(ap)
	chmk	$SYS_signal
	halt			/* can't happen */
