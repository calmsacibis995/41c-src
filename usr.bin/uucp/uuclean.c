static char sccsid[] = "@(#)uuclean.c	4.1	(Berkeley)	9/11/82";

#include "uucp.h"
#include "uucpdefs.h"
#include <signal.h>
#include <pwd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dir.h>

/*******
 *
 *	uuclean  -  this program will search through the spool
 *	directory (Spool) and delete all files with a requested
 *	prefix which are older than (nomtime) seconds.
 *	If the -m option is set, the program will try to
 *	send mail to the usid of the file.
 *
 *	options:
 *		-m  -  send mail for deleted file
 *		-d  -  directory to clean
 *		-n  -  time to age files before delete (in hours)
 *		-p  -  prefix for search
 *		-x  -  turn on debug outputs
 *	exit status:
 *		0  -  normal return
 *		1  -  can not read directory
 */

#define DPREFIX "U"
#define NOMTIME 72	/* hours to age files before deletion */

main(argc, argv)
char *argv[];
{
	DIR *pdirf;
	register struct direct *dirp;
	time_t nomtime, ptime;
	struct stat stbuf;
	int mflg=0;
	extern int onintr();

	nomtime = NOMTIME * 3600L;

	while (argc>1 && argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'd':
			Spool = &argv[1][2];
			break;
		case 'm':
			mflg = 1;
			break;
		case 'n':
			nomtime = atoi(&argv[1][2]) * 3600L;
			break;
		case 'p':
			if (&argv[1][2] != '\0')
				stpre(&argv[1][2]);
			break;
		case 'x':
			Debug = atoi(&argv[1][2]);
			if (Debug <= 0)
				Debug = 1;
			break;
		default:
			printf("unknown flag %s\n", argv[1]); break;
		}
		--argc;  argv++;
	}

	DEBUG(4, "DEBUG# %s\n", "START");
	chdir(Spool);

	if ((pdirf = opendir(Spool, "r")) == NULL) {
		printf("%s directory unreadable\n", Spool);
		exit(1);
	}

	time(&ptime);
	while ((dirp = readdir(pdirf)) != NULL) {
		if (!chkpre(dirp->d_name))
			continue;

		if (stat(dirp->d_name, &stbuf) == -1) {
		DEBUG(4, "stat on %s failed\n", dirp->d_name);
			continue;
		}


		if ((stbuf.st_mode & S_IFMT) == S_IFDIR)
			continue;
		if ((ptime - stbuf.st_ctime) < nomtime)
			continue;
		DEBUG(4, "unlink file %s\n", dirp->d_name);
		unlink(dirp->d_name);
		if (mflg) sdmail(dirp->d_name, stbuf.st_uid);
	}

	closedir(pdirf);
	exit(0);
}


#define MAXPRE 10
char Pre[MAXPRE][MAXNAMLEN];
int Npre = 0;
/***
 *	chkpre(file)	check for prefix
 *	char *file;
 *
 *	return codes:
 *		0  -  not prefix
 *		1  -  is prefix
 */

chkpre(file)
char *file;
{
	int i;

	for (i = 0; i < Npre; i++) {
		if (prefix(Pre[i], file))
			return(1);
		}
	return(0);
}

/***
 *	stpre(p)	store prefix
 *	char *p;
 *
 *	return codes:  none
 */

stpre(p)
char *p;
{
	if (Npre < MAXPRE - 2)
		strcpy(Pre[Npre++], p);
	return;
}
