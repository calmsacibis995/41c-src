/*	rl.c	4.1	83/02/08	*/

#include "rl.h"
#if NHL > 0
/*
 * UNIBUS RL02 disk driver
 * (not yet converted to 4.1c)
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/cpu.h"
#include "../h/nexus.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/pte.h"
#include "../h/mtpr.h"
#include "../h/vm.h"
#include "../h/ubavar.h"
#include "../h/ubareg.h"
#include "../h/cmap.h"

#include "../h/rlreg.h"

/* Pending Controller items and statistics */
struct	rl_softc {
	int	rl_softas;	/* Attention sumary, (seeks pending) */
	int	rl_ndrive;	/* Number of drives on controller */
	int	rl_wticks;	/* Monitor time for function */
} rl_softc[NHL];

/* 
 * this struct is used to keep the state of the controller for the last
 * transfer done.  Since only one transfer can be done at a time per
 * controller, only allocate one for each controller.
 */
struct	rl_stat {
	short	rl_cyl[4];	/* Current cylinder for each drive */
	short	rl_dn;		/* drive number currently transferring */
	short	rl_cylnhd;	/* current cylinder and head of transfer */
	u_short	rl_bleft;	/* bytes left to transfer */
	u_short	rl_bpart;	/* bytes transferred */
} rl_stat[NHL];

/* THIS SHOULD BE READ OFF THE PACK, PER DRIVE */
struct	size {
	daddr_t	nblocks;
	int	cyloff;
} rl02_sizes[8] = {
	14440,  	0,		/* A=cyl   0 thru 360 */
	 6040,        361,		/* B=cyl 361 thru 511 */
	20480,	        0,		/* C=cyl   0 thru 511 */
	    0,          0,		/* D= Not Defined     */
	    0,          0,		/* E= Not Defined     */
	    0,          0,		/* F= Not Defined     */
	    0,          0,		/* G= Not Defined     */
	    0,          0,		/* H= Not Defined     */
};
/* END OF STUFF WHICH SHOULD BE READ IN PER DISK */

int	rlprobe(), rlslave(), rlattach(), rldgo(), rlintr();
struct	uba_ctlr	*rlminfo[NHL];
struct	uba_device	*rldinfo[NRL];
struct	uba_device	*rlip[NHL][4];

/* RL02 driver structure */
u_short	rlstd[] = { 0174400 };
struct	uba_driver hldriver =
    { rlprobe, rlslave, rlattach, rldgo, rlstd, "rl", rldinfo, "hl", rlminfo };

/* User table per controller */
struct	buf	rlutab[NRL];

/* RL02 drive structure */
struct	RL02 {
	short	nbpt;		/* Number of 512 byte blocks/track */
	short	ntrak;
	short	nbpc;		/* Number of 512 byte blocks/cylinder */
	short	ncyl;
	short	btrak;		/* Number of bytes/track */
	struct	size *sizes;
} rl02 = {
	20,	2,	40,	512,	20*512,	rl02_sizes /* rl02/DEC*/
};

struct	buf	rrlbuf[NRL];

#define	b_cylin b_resid		/* Last seek as CYL<<1 | HD */

#ifdef INTRLVE
daddr_t dkblock();
#endif

int	rlwstart, rlwatch();		/* Have started guardian */

/* Check that controller exists */
/*ARGSUSED*/
rlprobe(reg)
	caddr_t reg;
{
	register int br, cvec;

#ifdef lint	
	br = 0; cvec = br; br = cvec;
#endif
	((struct rldevice *)reg)->rlcs = RL_IE | RL_NOOP;  /* Enable intrpt */
	DELAY(10);	/* Ensure interrupt takes place (10 microsec ) */
	((struct rldevice *)reg)->rlcs &= ~RL_IE;  /* Disable intrpt */
	return (1);
}

/* Check that drive exists and is functional*/
rlslave(ui, reg)
	struct uba_device *ui;
	caddr_t reg;
{
	register struct rldevice *rladdr = (struct rldevice *)reg;
	short ctr = 0;

	/*
	 * DEC reports that:
	 * For some unknown reason the RL02 (seems to be only drive 1)
	 * does not return a valid drive status the first time that a
	 * GET STATUS request is issued for the drive, in fact it can
	 * take up to three or more GET STATUS requests to obtain the
	 * correct status.
	 * In order to overcome this, the driver has been modified to
	 * issue a GET STATUS request and validate the drive status
	 * returned.  If a valid status is not returned after eight
	 * attempts, then an error message is printed.
	 */
	do {
		rladdr->rlda.getstat = RL_RESET;
		rladdr->rlcs = (ui->ui_slave <<8) | RL_GETSTAT; /* Get status*/
		rlwait(rladdr);
	} while( (rladdr->rlmp.getstat&RLMP_STATUS) != RLMP_STATOK && ++ctr<8 );


	if ((rladdr->rlcs & RL_DE) || (ctr >= 8))
		return (0);			 /* Error return */
	if ((rladdr->rlmp.getstat & RLMP_DT) == 0 ) {	/* NO RL01'S */
		printf("rl01 drives not supported (drive %d)\n", ui->ui_slave );
		return(0);
	}
	return (1);
}

/* Initialize controller */
rlattach(ui)
	register struct uba_device *ui;
{
	register struct rldevice *rladdr;

	if (rlwstart == 0) {
		timeout(rlwatch, (caddr_t)0, hz);   /* Watch for lost intr */
		rlwstart++;
	}
	/* Initialize iostat values */
	if (ui->ui_dk >= 0)
		dk_mspw[ui->ui_dk] = .000003906;   /* 16bit transfer time? */
	rlip[ui->ui_ctlr][ui->ui_slave] = ui;
	rl_softc[ui->ui_ctlr].rl_ndrive++;	   /* increment device/ctrl */
	rladdr = (struct rldevice *)ui->ui_addr;

	/* reset controller */
	rladdr->rlda.getstat = RL_RESET;	/* SHOULD BE REPEATED? */
	rladdr->rlcs = (ui->ui_slave << 8) | RL_GETSTAT; /* Reset DE bit */
	rlwait(rladdr);

	/* Determine disk posistion */
	rladdr->rlcs = (ui->ui_slave << 8) | RL_RHDR;
	rlwait(rladdr);

	/* save disk drive posistion */
	rl_stat[ui->ui_ctlr].rl_cyl[ui->ui_slave] =
		(rladdr->rlmp.readhdr & 0177700) >> 6;
	rl_stat[ui->ui_ctlr].rl_dn = -1;
}
 
rlstrategy(bp)
	register struct buf *bp;
{
	register struct uba_device *ui;
	register int drive;
	register struct buf *dp;
	int partition = minor(bp->b_dev) & 07;
	long bn, sz;

	sz = (bp->b_bcount+511) >> 9;	/* Blocks to transfer */

	drive = dkunit(bp);		/* Drive number */
	if (drive >= NRL)
		goto bad;
	ui = rldinfo[drive];		/* Controller uba_device */
	if (ui == 0 || ui->ui_alive == 0)
		goto bad;
	if (bp->b_blkno < 0 ||
	    (bn = dkblock(bp))+sz > rl02.sizes[partition].nblocks)
		goto bad;

	/* bn is in 512 byte block size */
	bp->b_cylin = bn/rl02.nbpc + rl02.sizes[partition].cyloff;
	(void) spl5();
	dp = &rlutab[ui->ui_unit];
	disksort(dp, bp);
	if (dp->b_active == 0) {
		(void) rlustart(ui);
		bp = &ui->ui_mi->um_tab;
		if (bp->b_actf && bp->b_active == 0)
			(void) rlstart(ui->ui_mi);
	}
	(void) spl0();
	return;

bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*
 * Unit start routine.
 * Seek the drive to be where the data is
 * and then generate another interrupt
 * to actually start the transfer.
 */
rlustart(ui)
	register struct uba_device *ui;
{
	register struct buf *bp, *dp;
	register struct uba_ctlr *um;
	register struct rldevice *rladdr;
	daddr_t bn;
	short cyl, sn, hd, diff;

	if (ui == 0)
		return (0);
	um = ui->ui_mi;
	dk_busy &= ~(1<<ui->ui_dk);	/* Kernel define, drives busy */
	dp = &rlutab[ui->ui_unit];
	if ((bp = dp->b_actf) == NULL)
		goto out;
	/*
	 * If the controller is active, just remember
	 * that this device has to be positioned...
	 */
	if (um->um_tab.b_active) {
		rl_softc[um->um_ctlr].rl_softas |=  1<<ui->ui_slave;
		return (0);
	}
	/*
	 * If we have already positioned this drive,
	 * then just put it on the ready queue.
	 */
	if (dp->b_active)
		goto done;
	dp->b_active = 1;	/* Posistioning drive */
	rladdr = (struct rldevice *)um->um_addr;

	/*
	 * Figure out where this transfer is going to
	 * and see if we are seeked correctly.
	 */
	bn = dkblock(bp);		/* Block # desired */
	/*
	 * these next two look funky... but we need to map
	 * 512 byte logical disk blocks to 256 byte sectors.
	 * (rl02's are stupid).
	 */
	sn = (bn % rl02.nbpt) << 1;	/* Sector # desired */
	hd = (bn / rl02.nbpt) & 1;	/* Get head required */

	diff = (rl_stat[um->um_ctlr].rl_cyl[ui->ui_slave] >> 1) - bp->b_cylin;
	if ( diff == 0 && (rl_stat[um->um_ctlr].rl_cyl[ui->ui_slave] & 1) == hd)
		goto done;		/* on cylinder and head */

search:
	/*
	 * Not at correct position.
	 */

	rl_stat[um->um_ctlr].rl_cyl[ui->ui_slave] = (bp->b_cylin << 1) | hd;

	if (diff < 0)
		rladdr->rlda.seek = -diff << 7 | RLDA_HGH | hd << 4;
	else
		rladdr->rlda.seek = diff << 7 | RLDA_LOW | hd << 4;
	rladdr->rlcs = (ui->ui_slave << 8) | RL_SEEK;

	/*
	 * Mark unit busy for iostat.
	 */
	if (ui->ui_dk >= 0) {
		dk_busy |= 1<<ui->ui_dk;
		dk_seek[ui->ui_dk]++;
	}
	rlwait( rladdr );

	/*
	 * fall through since we are now at the correct cylinder
	 */
done:
	/*
	 * Device is ready to go.
	 * Put it on the ready queue for the controller
	 * (unless its already there.)
	 */
	if (dp->b_active != 2) {
		dp->b_forw = NULL;
		if (um->um_tab.b_actf == NULL)
			um->um_tab.b_actf = dp;
		else
			um->um_tab.b_actl->b_forw = dp;
		um->um_tab.b_actl = dp;
		dp->b_active = 2;	/* Request on ready queue */
	}
out:
	return (0);
}

/*
 * Start up a transfer on a drive.
 */
rlstart(um)
	register struct uba_ctlr *um;
{
	register struct buf *bp, *dp;
	register struct uba_device *ui;
	register struct rldevice *rladdr;
	register struct rl_stat *st = &rl_stat[um->um_ctlr];
	daddr_t bn;
	short sn, cyl, cmd;

loop:
	/*
	 * Pull a request off the controller queue
	 */
	if ((dp = um->um_tab.b_actf) == NULL) {
		st->rl_dn = -1;
		st->rl_cylnhd = 0;
		st->rl_bleft = 0;
		st->rl_bpart = 0;
		return (0);
	}
	if ((bp = dp->b_actf) == NULL) {
		um->um_tab.b_actf = dp->b_forw;
		goto loop;
	}
	/*
	 * Mark controller busy, and
	 * determine destinationst.
	 */
	um->um_tab.b_active++;
	ui = rldinfo[dkunit(bp)];		/* Controller */
	bn = dkblock(bp);			/* 512 byte Block number */
	cyl = bp->b_cylin << 1;			/* Cylinder */
	cyl |= (bn / rl02.nbpt) & 1;		/* Get head required */
	sn = (bn % rl02.nbpt) << 1;		/* Sector number */
	rladdr = (struct rldevice *)ui->ui_addr;

	/*
	 * Check that controller is ready
	 */
	rlwait( rladdr );

	/*
	 * Setup for the transfer, and get in the
	 * UNIBUS adaptor queue.
	 */
	rladdr->rlda.rw = cyl<<6 | sn;

	/* save away current transfers drive status */
	st->rl_dn = ui->ui_slave;
	st->rl_cylnhd = cyl;
	st->rl_bleft = bp->b_bcount;
	st->rl_bpart = rl02.btrak - (sn * NRLBPSC);

	/* RL02 must seek between cylinders and between tracks */
	/* Determine maximum data transfer at this time */
	if( st->rl_bleft < st->rl_bpart)
		st->rl_bpart = st->rl_bleft;

	rladdr->rlmp.rw = -(st->rl_bpart >> 1);
	if (bp->b_flags & B_READ)
		cmd = RL_IE | RL_READ | (ui->ui_slave << 8);
	else
		cmd = RL_IE | RL_WRITE | (ui->ui_slave << 8);
	um->um_cmd = cmd;
	(void) ubago(ui);
	return (1);
}

/*
 * Now all ready to go, stuff the registers.
 */
rldgo(um)
	register struct uba_ctlr *um;
{
	register struct rldevice *rladdr = (struct rldevice *)um->um_addr;


	/* Place in unibus address for transfer (lower 18 bits of um_ubinfo) */
	/* Then execute instruction */
	rladdr->rlba = um->um_ubinfo;
	rladdr->rlcs = um->um_cmd|((um->um_ubinfo>>12)&RL_BAE);
}

/*
 * Handle a disk interrupt.
 */
rlintr(rl21)
	register rl21;
{
	register struct buf *bp, *dp;
	register struct uba_ctlr *um = rlminfo[rl21];
	register struct uba_device *ui;
	register struct rldevice *rladdr = (struct rldevice *)um->um_addr;
	register unit;
	struct rl_softc *rl = &rl_softc[um->um_ctlr];
	struct rl_stat *st = &rl_stat[um->um_ctlr];
	int as = rl->rl_softas;
	int needie = 1, waitdry, status;

	rl->rl_wticks = 0;
	rl->rl_softas = 0;

	/*
	 * Get device and block structures, and a pointer
	 * to the uba_device for the drive.
	 */
	dp = um->um_tab.b_actf;
	bp = dp->b_actf;
	ui = rldinfo[dkunit(bp)];
	dk_busy &= ~(1 << ui->ui_dk);	/* Clear busy bit */

	/*
	 * Check for and process errors on
	 * either the drive or the controller.
	 */
	if (rladdr->rlcs & RL_ERR) {
		u_short err;
		rlwait( rladdr );

		err = rladdr->rlcs;

		/* get staus and reset controller */
		rladdr->rlda.getstat = RL_GSTAT;
		rladdr->rlcs = (ui->ui_slave << 8) | RL_GETSTAT;
		rlwait(rladdr);
		status = rladdr->rlmp.getstat;

		/* reset drive */
		rladdr->rlda.getstat = RL_RESET;
		rladdr->rlcs = (ui->ui_slave <<8) | RL_GETSTAT; /* Get status*/
		rlwait(rladdr);

		if ( (status & RLMP_WL) == RLMP_WL ) {
			/*
			 * Give up on write protected devices
			 * immediately.
			 */
			printf("rl%d: write protected\n", dkunit(bp));
			bp->b_flags |= B_ERROR;
		} else if (++um->um_tab.b_errcnt > 10) {
			/*
			 * After 10 retries give up.
			 */
			harderr(bp, "rl");
			printf("cs=%b mp=%b\n",
				err, RLCS_BITS, status, RLER_BITS);

			bp->b_flags |= B_ERROR;
		} else
			um->um_tab.b_active = 0;	 /* force retry */

		/* Determine disk posistion */
		rladdr->rlcs = (ui->ui_slave << 8) | RL_RHDR;
		rlwait(rladdr);

		/* save disk drive posistion */
		st->rl_cyl[ui->ui_slave] = (rladdr->rlmp.readhdr & 0177700) >> 6;
	}

	/*
	 * If still ``active'', then don't need any more retries.
	 */
	if (um->um_tab.b_active) {
		/* RL02 check if more data from previous request */
		if ( (bp->b_flags & B_ERROR) == 0 &&
		     (st->rl_bleft -= st->rl_bpart) > 0 ) {
			/*
			 * the following code was modeled from the rk07
			 * driver when an ECC error occured.  It has to
			 * fix the bits then restart the transfer which is
			 * what we have to do (restart transfer).
			 */
			int reg, npf, o, cmd, ubaddr, diff, head;


			/* seek to next head/track */

			/* increment head and/or cylinder */
			st->rl_cylnhd++;
			diff = (st->rl_cyl[ui->ui_slave] >> 1) -
				(st->rl_cylnhd >> 1);
			st->rl_cyl[ui->ui_slave] = st->rl_cylnhd;
			head = st->rl_cylnhd & 1;
			rlwait( rladdr );

			if ( diff < 0 )
				rladdr->rlda.seek = -diff << 7 | RLDA_HGH | head << 4;
			else
				rladdr->rlda.seek = diff << 7 | RLDA_LOW | head << 4;
			rladdr->rlcs = (ui->ui_slave << 8) | RL_SEEK;

			npf = btop( bp->b_bcount - st->rl_bleft );
			reg = btop(um->um_ubinfo&0x3ffff) + npf;
			o = (int)bp->b_un.b_addr & PGOFSET;
			ubapurge(um);
			um->um_tab.b_active++;

			rlwait( rladdr );
			rladdr->rlda.rw = st->rl_cylnhd << 6;
			if ( st->rl_bleft < (st->rl_bpart = rl02.btrak) )
				st->rl_bpart = st->rl_bleft;
			rladdr->rlmp.rw = -(st->rl_bpart >> 1);
			cmd = (bp->b_flags&B_READ ? RL_READ : RL_WRITE) |
				RL_IE | (ui->ui_slave << 8);
			ubaddr = (int)ptob(reg) + o;
			cmd |= ((ubaddr >> 12) & RL_BAE);

			rladdr->rlba = ubaddr;
			rladdr->rlcs = cmd;
			return;
		}

		um->um_tab.b_active = 0;
		um->um_tab.b_errcnt = 0;
		dp->b_active = 0;
		dp->b_errcnt = 0;

		/* "b_resid" words remaining after error */
		bp->b_resid = st->rl_bleft;
		um->um_tab.b_actf = dp->b_forw;
		dp->b_actf = bp->av_forw;

retry:
		st->rl_dn = -1;
		st->rl_bpart = st->rl_bleft = 0;
		iodone(bp);
		/*
		 * If this unit has more work to do,
		 * then start it up right away.
		 */
		if (dp->b_actf)
			if (rlustart(ui))
				needie = 0;
		as &= ~(1<<ui->ui_slave);
	} else
		as |= (1<<ui->ui_slave);
	/*
	 * Release unibus resources and flush data paths.
	 */
	ubadone(um);

	/* reset state info */
	st->rl_dn = -1;
	st->rl_cylnhd = st->rl_bpart = st->rl_bleft = 0;

doattn:
	/*
	 * Process other units which need attention.
	 * For each unit which needs attention, call
	 * the unit start routine to place the slave
	 * on the controller device queue.
	 */
	while (unit = ffs(as)) {
		unit--;		/* was 1 origin */
		as &= ~(1<<unit);
		(void) rlustart(rlip[rl21][unit]);
	}
	/*
	 * If the controller is not transferring, but
	 * there are devices ready to transfer, start
	 * the controller.
	 */
	if (um->um_tab.b_actf && um->um_tab.b_active == 0)
		(void) rlstart(um);
}

rlwait( rladdr )
register struct rldevice *rladdr;
{
	while ((rladdr->rlcs & RL_CRDY) == 0)
		continue;         /* Wait */
}

rlread(dev)
	dev_t dev;
{
	register int unit = minor(dev) >> 3;

	if (unit >= NRL)
		u.u_error = ENXIO;
	else
		physio(rlstrategy, &rrlbuf[unit], dev, B_READ, minphys);
}

rlwrite(dev)
	dev_t dev;
{
	register int unit = minor(dev) >> 3;

	if (unit >= NRL)
		u.u_error = ENXIO;
	else
		physio(rlstrategy, &rrlbuf[unit], dev, B_WRITE, minphys);
}

/*
 * Reset driver after UBA init.
 * Cancel software state of all pending transfers
 * and restart all units and the controller.
 */
rlreset(uban)
	int uban;
{
	register struct uba_ctlr *um;
	register struct uba_device *ui;
	register struct rldevice *rladdr;
	register struct rl_stat *st;
	register int	rl21, unit;

	for (rl21 = 0; rl21 < NHL; rl21++) {
		if ((um = rlminfo[rl21]) == 0 || um->um_ubanum != uban ||
		    um->um_alive == 0)
			continue;
		printf(" Reset hl%d", rl21);
		rladdr = (struct rldevice *)um->um_addr;
		st = &rl_stat[rl21];
		um->um_tab.b_active = 0;
		um->um_tab.b_actf = um->um_tab.b_actl = 0;
		if (um->um_ubinfo) {
			printf("<%d>", (um->um_ubinfo>>28)&0xf);
			ubadone(um);
		}

		/* reset controller */
		st->rl_dn = -1;
		st->rl_cylnhd = 0;
		st->rl_bleft = 0;
		st->rl_bpart = 0;
		rlwait( rladdr );

		for (unit = 0; unit < NRL; unit++) {
			rladdr->rlcs = (unit << 8) | RL_GETSTAT;
			rlwait( rladdr );

			/* Determine disk posistion */
			rladdr->rlcs = (unit << 8) | RL_RHDR;
			rlwait(rladdr);

			/* save disk drive posistion */
			st->rl_cyl[unit] =
				(rladdr->rlmp.readhdr & 0177700) >> 6;

			if ((ui = rldinfo[unit]) == 0)
				continue;
			if (ui->ui_alive == 0 || ui->ui_mi != um)
				continue;
			rlutab[unit].b_active = 0;
			(void) rlustart(ui);
		}
		(void) rlstart(um);
	}
}

/*
 * Wake up every second and if an interrupt is pending
 * but nothing has happened increment a counter.
 * If nothing happens for 20 seconds, reset the UNIBUS
 * and begin anew.
 */
rlwatch()
{
	register struct uba_ctlr *um;
	register rl21, unit;
	register struct rl_softc *rl;

	timeout(rlwatch, (caddr_t)0, hz);
	for (rl21 = 0; rl21 < NHL; rl21++) {
		um = rlminfo[rl21];
		if (um == 0 || um->um_alive == 0)
			continue;
		rl = &rl_softc[rl21];
		if (um->um_tab.b_active == 0) {
			for (unit = 0; unit < NRL; unit++)
				if (rlutab[unit].b_active &&
				    rldinfo[unit]->ui_mi == um)
					goto active;
			rl->rl_wticks = 0;
			continue;
		}
active:
		rl->rl_wticks++;
		if (rl->rl_wticks >= 20) {
			rl->rl_wticks = 0;
			printf("hl%d: lost interrupt\n", rl21);
			ubareset(um->um_ubanum);
		}
	}
}

rldump(dev)
	dev_t dev;
{
	/* don't think there is room on swap for it anyway. */
}
