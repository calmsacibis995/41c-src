#include "../h/param.h"
#include "../h/conf.h"
/*
 * Single storage module (R80) configuration
 *	root on rb0a
 *	paging on rb0b
 */
dev_t	rootdev	= makedev(11, 0);
dev_t	pipedev	= makedev(11, 0);
dev_t	argdev	= makedev(11, 1);
dev_t	dumpdev	= makedev(11, 1);
long	dumplo	= 33440 - 10 * 2048;

/*
 * Nswap is the basic number of blocks of swap per
 * swap device, and is multiplied by nswdev after
 * nswdev is determined at boot.
 */
int	nswap = 33440;

struct	swdevt swdevt[] =
{
	makedev(11, 1),	0,		/* rb0b */
	0,		0,
};
