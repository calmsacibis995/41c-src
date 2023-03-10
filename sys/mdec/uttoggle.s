/*	uttoggle.s	4.3	83/03/06	*/

/*
 * Prototype toggle in bootstrap code for ut type tapes.
 * If on anything but a 780 with the drive on uba0
 * this will have to be repaired by patching uba and umem.
 */
begin:
	movl	uba,r1
	movl	$0x80200000,0x800(r1)
	clrl	0x804(r1)
	movl	umem,r2
	bisl2	$0172440,r2
	movw	$0x04c0,26(r2)		/* set tape density & format */
	mnegw	$512,6(r2)		/* set frame count */
	mnegw	$256,2(r2)		/* set word count */
	clrw	4(r2)			/* set bus address */
	movw	$0x39,(r2)		/* set command and go */
	halt
	.align	2
uba:	.long	0x20006000
umem:	.long	0x2013e000
