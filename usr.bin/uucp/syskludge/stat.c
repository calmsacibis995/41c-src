/* @(#)stat.c	4.1 (Berkeley) 1/1/83 */

#include "SYS.h"

ENTRY(stat)
	pushl	4(ap)
	CALL(1,_fixf);
	movl	r0,4(ap)
	chmk	$SYS_stat
	jcs 	err
	ret
err:
	jmp 	cerror
