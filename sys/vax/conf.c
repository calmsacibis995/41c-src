/*	conf.c	4.62	82/10/31	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/tty.h"
#include "../h/conf.h"

int	nulldev();
int	nodev();

#include "hp.h"
#if NHP > 0
int	hpopen(),hpstrategy(),hpread(),hpwrite(),hpdump(),hpioctl();
#else
#define	hpopen		nodev
#define	hpstrategy	nodev
#define	hpread		nodev
#define	hpwrite		nodev
#define	hpdump		nodev
#define	hpioctl		nodev
#endif
 
#include "tu.h"
#if NHT > 0
int	htopen(),htclose(),htstrategy(),htread(),htwrite(),htdump(),htioctl();
#else
#define	htopen		nodev
#define	htclose		nodev
#define	htstrategy	nodev
#define	htread		nodev
#define	htwrite		nodev
#define	htdump		nodev
#define	htioctl		nodev
#endif

#include "rk.h"
#if NHK > 0
int	rkopen(),rkstrategy(),rkread(),rkwrite(),rkintr(),rkdump(),rkreset();
#else
#define	rkopen		nodev
#define	rkstrategy	nodev
#define	rkread		nodev
#define	rkwrite		nodev
#define	rkintr		nodev
#define	rkdump		nodev
#define	rkreset		nodev
#endif

#include "te.h"
#if NTE > 0
int	tmopen(),tmclose(),tmstrategy(),tmread(),tmwrite();
int	tmioctl(),tmdump(),tmreset();
#else
#define	tmopen		nodev
#define	tmclose		nodev
#define	tmstrategy	nodev
#define	tmread		nodev
#define	tmwrite		nodev
#define	tmioctl		nodev
#define	tmdump		nodev
#define	tmreset		nodev
#endif

#include "ts.h"
#if NTS > 0
int	tsopen(),tsclose(),tsstrategy(),tsread(),tswrite();
int	tsioctl(),tsdump(),tsreset();
#else
#define	tsopen		nodev
#define	tsclose		nodev
#define	tsstrategy	nodev
#define	tsread		nodev
#define	tswrite		nodev
#define	tsioctl		nodev
#define	tsdump		nodev
#define	tsreset		nodev
#endif

#include "mu.h"
#if NMT > 0
int	mtopen(),mtclose(),mtstrategy(),mtread(),mtwrite();
int	mtioctl(),mtdump();
#else
#define	mtopen		nodev
#define	mtclose		nodev
#define	mtstrategy	nodev
#define	mtread		nodev
#define	mtwrite		nodev
#define	mtioctl		nodev
#define	mtdump		nodev
#endif

#include "ra.h"
#if NUDA > 0
int	udopen(),udstrategy(),udread(),udwrite(),udreset(),uddump();
#else
#define	udopen		nodev
#define	udstrategy	nodev
#define	udread		nodev
#define	udwrite		nodev
#define	udreset		nulldev
#define	uddump		nodev
#endif

#include "up.h"
#if NSC > 0
int	upopen(),upstrategy(),upread(),upwrite(),upreset(),updump();
#else
#define	upopen		nodev
#define	upstrategy	nodev
#define	upread		nodev
#define	upwrite		nodev
#define	upreset		nulldev
#define	updump		nodev
#endif

#include "tj.h"
#if NUT > 0
int	utopen(),utclose(),utstrategy(),utread(),utwrite(),utioctl();
int	utreset(),utdump();
#else
#define	utopen		nodev
#define	utclose		nodev
#define	utread		nodev
#define	utstrategy	nodev
#define	utwrite		nodev
#define	utreset		nulldev
#define	utioctl		nodev
#define	utdump		nodev
#endif

#include "rb.h"
#if NIDC > 0
int	idcopen(),idcstrategy(),idcread(),idcwrite(),idcreset(),idcdump();
#else
#define	idcopen		nodev
#define	idcstrategy	nodev
#define	idcread		nodev
#define	idcwrite	nodev
#define	idcreset	nulldev
#define	idcdump		nodev
#endif

#if defined(VAX750) || defined(VAX730)
int	tuopen(),tuclose(),tustrategy();
#else
#define	tuopen		nodev
#define	tuclose		nodev
#define	tustrategy	nodev
#endif

int	swstrategy(),swread(),swwrite();

struct bdevsw	bdevsw[] =
{
	hpopen,		nulldev,	hpstrategy,	hpdump,	0,	/*0*/
	htopen,		htclose,	htstrategy,	htdump,	B_TAPE,	/*1*/
	upopen,		nulldev,	upstrategy,	updump,	0,	/*2*/
	rkopen,		nulldev,	rkstrategy,	rkdump,	0,	/*3*/
	nodev,		nodev,		swstrategy,	nodev,	0,	/*4*/
	tmopen,		tmclose,	tmstrategy,	tmdump,	B_TAPE,	/*5*/
	tsopen,		tsclose,	tsstrategy,	tsdump,	B_TAPE,	/*6*/
	mtopen,		mtclose,	mtstrategy,	mtdump,	B_TAPE,	/*7*/
	tuopen,		tuclose,	tustrategy,	nodev,	B_TAPE,	/*8*/
	udopen,		nulldev,	udstrategy,	uddump,	0,	/*9*/
	utopen,		utclose,	utstrategy,	utdump,	B_TAPE,	/*10*/
	idcopen,	nodev,		idcstrategy,	idcdump,0,	/*11*/
};
int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);

int	cnopen(),cnclose(),cnread(),cnwrite(),cnioctl();
struct tty cons;

#include "acc.h"
#if NACC > 0
int     accreset();
#else
#define accreset nulldev
#endif

#include "ct.h"
#if NCT > 0
int	ctopen(),ctclose(),ctwrite();
#else
#define	ctopen	nulldev
#define	ctclose	nulldev
#define	ctwrite	nulldev
#endif

#include "dh.h"
#if NDH == 0
#define	dhopen	nodev
#define	dhclose	nodev
#define	dhread	nodev
#define	dhwrite	nodev
#define	dhioctl	nodev
#define	dhstop	nodev
#define	dhreset	nulldev
#define	dh11	0
#else
int	dhopen(),dhclose(),dhread(),dhwrite(),dhioctl(),dhstop(),dhreset();
struct	tty dh11[];
#endif

#include "dmf.h"
#if NDMF == 0
#define	dmfopen	nodev
#define	dmfclose	nodev
#define	dmfread	nodev
#define	dmfwrite	nodev
#define	dmfioctl	nodev
#define	dmfstop	nodev
#define	dmfreset	nulldev
#define	dmf11	0
#else
int	dmfopen(),dmfclose(),dmfread(),dmfwrite(),dmfioctl(),dmfstop(),dmfreset();
struct	tty dmf_tty[];
#endif

#if VAX780
int	flopen(),flclose(),flread(),flwrite();
#endif

#include "dz.h"
#if NDZ == 0
#define	dzopen	nodev
#define	dzclose	nodev
#define	dzread	nodev
#define	dzwrite	nodev
#define	dzioctl	nodev
#define	dzstop	nodev
#define	dzreset	nulldev
#define	dz_tty	0
#else
int	dzopen(),dzclose(),dzread(),dzwrite(),dzioctl(),dzstop(),dzreset();
struct	tty dz_tty[];
#endif

#include "lp.h"
#if NLP > 0
int	lpopen(),lpclose(),lpwrite(),lpreset();
#else
#define	lpopen		nodev
#define	lpclose		nodev
#define	lpwrite		nodev
#define	lpreset		nulldev
#endif

int	syopen(),syread(),sywrite(),syioctl(),syselect();

int 	mmread(),mmwrite();
#define	mmselect	seltrue

#include "va.h"
#if NVA > 0
int	vaopen(),vaclose(),vawrite(),vaioctl(),vareset(),vaselect();
#else
#define	vaopen		nodev
#define	vaclose		nodev
#define	vawrite		nodev
#define	vaopen		nodev
#define	vaioctl		nodev
#define	vareset		nulldev
#define	vaselect	nodev
#endif

#include "vp.h"
#if NVP > 0
int	vpopen(),vpclose(),vpwrite(),vpioctl(),vpreset(),vpselect();
#else
#define	vpopen		nodev
#define	vpclose		nodev
#define	vpwrite		nodev
#define	vpioctl		nodev
#define	vpreset		nulldev
#define	vpselect	nodev
#endif

#include "pty.h"
#if NPTY > 0
int	ptsopen(),ptsclose(),ptsread(),ptswrite(),ptsstop();
int	ptcopen(),ptcclose(),ptcread(),ptcwrite(),ptcselect();
int	ptyioctl();
struct	tty pt_tty[];
#else
#define ptsopen		nodev
#define ptsclose	nodev
#define ptsread		nodev
#define ptswrite	nodev
#define ptcopen		nodev
#define ptcclose	nodev
#define ptcread		nodev
#define ptcwrite	nodev
#define ptyioctl	nodev
#define	pt_tty		0
#define	ptcselect	nodev
#define	ptsstop		nulldev
#endif

#include "lpa.h"
#if NLPA > 0
int	lpaopen(),lpaclose(),lparead(),lpawrite(),lpaioctl();
#else
#define	lpaopen		nodev
#define	lpaclose	nodev
#define	lparead		nodev
#define	lpawrite	nodev
#define	lpaioctl	nodev
#endif

#include "dn.h"
#if NDN > 0
int	dnopen(),dnclose(),dnwrite();
#else
#define	dnopen		nodev
#define	dnclose		nodev
#define	dnwrite		nodev
#endif

#include "gpib.h"
#if NGPIB > 0
int	gpibopen(),gpibclose(),gpibread(),gpibwrite(),gpibioctl();
#else
#define	gpibopen	nodev
#define	gpibclose	nodev
#define	gpibread	nodev
#define	gpibwrite	nodev
#define	gpibioctl	nodev
#endif

#include "ik.h"
#if NIK > 0
int	ikopen(),ikclose(),ikread(),ikwrite(),ikioctl(),ikreset();
#else
#define ikopen nodev
#define ikclose nodev
#define ikread nodev
#define ikwrite nodev
#define ikioctl nodev
#define ikreset nodev
#endif

#include "ps.h"
#if NPS > 0
int	psopen(),psclose(),psread(),pswrite(),psioctl(),psreset();
#else
#define psopen nodev
#define psclose nodev
#define psread nodev
#define pswrite nodev
#define psopen nodev
#define psioctl nodev
#define psreset nodev
#endif

#include "ib.h"
#if NIB > 0
int	ibopen(),ibclose(),ibread(),ibwrite(),ibioctl();
#else
#define	ibopen	nodev
#define	ibclose	nodev
#define	ibread	nodev
#define	ibwrite	nodev
#define	ibioctl	nodev
#endif

#include "ad.h"
#if NAD > 0
int	adopen(),adclose(),adioctl(),adreset();
#else
#define adopen nodev
#define adclose nodev
#define adioctl nodev
#define adreset nodev
#endif

/* #include "efs.h" */
#if NEFS > 0
int	efsopen(),efsclose(),efsread(),efswrite(),efsioctl(),efsreset();
#else
#define efsopen nodev
#define efsclose nodev
#define efsread nodev
#define efswrite nodev
#define efsioctl nodev
#define efsreset nodev
#endif

int	ttselect(), seltrue();

struct cdevsw	cdevsw[] =
{
	cnopen,		cnclose,	cnread,		cnwrite,	/*0*/
	cnioctl,	nulldev,	nulldev,	&cons,
	ttselect,	nodev,
	dzopen,		dzclose,	dzread,		dzwrite,	/*1*/
	dzioctl,	dzstop,		dzreset,	dz_tty,
	ttselect,	nodev,
	syopen,		nulldev,	syread,		sywrite,	/*2*/
	syioctl,	nulldev,	nulldev,	0,
	syselect,	nodev,
	nulldev,	nulldev,	mmread,		mmwrite,	/*3*/
	nodev,		nulldev,	nulldev,	0,
	mmselect,	nodev,
	hpopen,		nulldev,	hpread,		hpwrite,	/*4*/
	hpioctl,	nodev,		nulldev,	0,
	seltrue,	nodev,
	htopen,		htclose,	htread,		htwrite,	/*5*/
	htioctl,	nodev,		nulldev,	0,
	seltrue,	nodev,
	vpopen,		vpclose,	nodev,		vpwrite,	/*6*/
	vpioctl,	nulldev,	vpreset,	0,
	vpselect,	nodev,
	nulldev,	nulldev,	swread,		swwrite,	/*7*/
	nodev,		nodev,		nulldev,	0,
	nodev,		nodev,
#if VAX780
	flopen,		flclose,	flread,		flwrite,	/*8*/
	nodev,		nodev,		nulldev,	0,
	seltrue,	nodev,
#else
	nodev,		nodev,		nodev,		nodev,		/*8*/
	nodev,		nodev,		nodev,		0,
	nodev,		nodev,
#endif
	udopen,		nulldev,	udread,		udwrite,	/*9*/
	nodev,		nodev,		udreset,		0,
	seltrue,	nodev,
	vaopen,		vaclose,	nodev,		vawrite,	/*10*/
	vaioctl,	nulldev,	vareset,	0,
	vaselect,	nodev,
	rkopen,		nulldev,	rkread,		rkwrite,	/*11*/
	nodev,		nodev,		rkreset,	0,
	seltrue,	nodev,
	dhopen,		dhclose,	dhread,		dhwrite,	/*12*/
	dhioctl,	dhstop,		dhreset,	dh11,
	ttselect,	nodev,
	upopen,		nulldev,	upread,		upwrite,	/*13*/
	nodev,		nodev,		upreset,	0,
	seltrue,	nodev,
	tmopen,		tmclose,	tmread,		tmwrite,	/*14*/
	tmioctl,	nodev,		tmreset,	0,
	seltrue,	nodev,
	lpopen,		lpclose,	nodev,		lpwrite,	/*15*/
	nodev,		nodev,		lpreset,	0,
	seltrue,	nodev,
	tsopen,		tsclose,	tsread,		tswrite,	/*16*/
	tsioctl,	nodev,		tsreset,	0,
	seltrue,	nodev,
	utopen,		utclose,	utread,		utwrite,	/*17*/
	utioctl,	nodev,		utreset,	0,
	seltrue,	nodev,
	ctopen,		ctclose,	nodev,		ctwrite,	/*18*/
	nodev,		nodev,		nulldev,	0,
	seltrue,	nodev,
	mtopen,		mtclose,	mtread,		mtwrite,	/*19*/
	mtioctl,	nodev,		nodev,		0,
	seltrue,	nodev,
	ptsopen,	ptsclose,	ptsread,	ptswrite,	/*20*/
	ptyioctl,	ptsstop,	nodev,		pt_tty,
	ttselect,	nodev,
	ptcopen,	ptcclose,	ptcread,	ptcwrite,	/*21*/
	ptyioctl,	nulldev,	nodev,		pt_tty,
	ptcselect,	nodev,
	dmfopen,	dmfclose,	dmfread,	dmfwrite,	/*22*/
	dmfioctl,	dmfstop,	dmfreset,	0,
	ttselect,	nodev,
	idcopen,	nulldev,	idcread,	idcwrite,	/*23*/
	nodev,		nodev,		idcreset,	0,
	seltrue,	nodev,
	dnopen,		dnclose,	nodev,		dnwrite,	/*24*/
	nodev,		nodev,		nodev,		0,
	seltrue,	nodev,
/* 25-29 reserved to local sites */
	gpibopen,	gpibclose,	gpibread,	gpibwrite,	/*25*/
	gpibioctl,	nulldev,	nodev,		0,
	seltrue,	nodev,
	lpaopen,	lpaclose,	lparead,	lpawrite,	/*26*/
	lpaioctl,	nodev,		nulldev,	0,
	seltrue,	nodev,
	psopen,		psclose,	psread,		pswrite,	/*27*/
	psioctl,	nodev,		psreset,	0,
	seltrue,	nodev,
	ibopen,		ibclose,	ibread,		ibwrite,	/*28*/
	ibioctl,	nodev,		nodev,		0,
	seltrue,	nodev,
	adopen,		adclose,	nodev,		nodev,		/*29*/
	adioctl,	nodev,		adreset,	0,
	seltrue,	nodev,
	efsopen,	efsclose,	efsread,	efswrite,	/*30*/
	efsioctl,	nodev,		efsreset,	0,
	seltrue,	nodev,
	ikopen,		ikclose,	ikread,		ikwrite,	/*31*/
	ikioctl,	nodev,		ikreset,	0,
	seltrue,	nodev,
};
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

int	mem_no = 3; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(4, 0);
