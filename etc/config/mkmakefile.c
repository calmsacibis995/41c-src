/*	mkmakefile.c	1.26	83/02/21	*/

/*
 * Build the makefile for the system, from
 * the information in the files files and the
 * additional files for the machine being compiled to.
 */

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define next_word(fp, wd) \
	{ register char *word = get_word(fp); \
	  if (word == (char *)EOF) \
		return; \
	  else \
		wd = word; \
	}

static	struct file_list *fcur;

/*
 * Lookup a file, by make.
 */
struct file_list *
fl_lookup(file)
	register char *file;
{
	register struct file_list *fp;

	for (fp = ftab ; fp != 0; fp = fp->f_next) {
		if (eq(fp->f_fn, file))
			return (fp);
	}
	return (0);
}

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
	register struct file_list *fp;

	fp = (struct file_list *) malloc(sizeof *fp);
	fp->f_needs = 0;
	fp->f_next = 0;
	if (fcur == 0)
		fcur = ftab = fp;
	else
		fcur->f_next = fp;
	fcur = fp;
	return (fp);
}

char	*COPTS;

/*
 * Build the makefile from the skeleton
 */
makefile()
{
	FILE *ifp, *ofp;
	char line[BUFSIZ];
	struct opt *op;

	read_files();
	strcpy(line, "../conf/makefile.");
	(void) strcat(line, machinename);
	ifp = fopen(line, "r");
	if (ifp == 0) {
		perror(line);
		exit(1);
	}
	ofp = fopen(path("makefile"), "w");
	if (ofp == 0) {
		perror(path("makefile"));
		exit(1);
	}
#ifdef vax
	if (maxusers == 0) {
		printf("maxusers not specified; 24 assumed\n");
		maxusers = 24;
	} else if (maxusers < 8) {
		printf("minimum of 8 maxusers assumed\n");
		maxusers = 8;
	} else if (maxusers > 128) {
		printf("maxusers truncated to 128\n");
		maxusers = 128;
	}
#endif
#ifdef sun
	if (maxusers == 0) {
		printf("maxusers not specified; 8 assumed\n");
		maxusers = 8;
	} else if (maxusers < 2) {
		printf("minimum of 2 maxusers assumed\n");
		maxusers = 2;
	} else if (maxusers > 32) {
		printf("maxusers truncated to 32\n");
		maxusers = 32;
	}
#endif
	fprintf(ofp, "IDENT=-D%s", raise(ident));
	if (profiling)
		fprintf(ofp, " -DGPROF");
	fprintf(ofp, " -DMAXUSERS=%d", maxusers);
	if (cputype == 0) {
		printf("cpu type must be specified\n");
		exit(1);
	}
	{ struct cputype *cp;
	  for (cp = cputype; cp; cp = cp->cpu_next)
		fprintf(ofp, " -D%s", cp->cpu_name);
	}
	for (op = opt; op; op = op->op_next)
		if (op->op_value)
			fprintf(ofp, " -D%s=\"%s\"", op->op_name, op->op_value);
		else
			fprintf(ofp, " -D%s", op->op_name);
	fprintf(ofp, "\n");
	if (hadtz == 0)
		printf("timezone not specified; gmt assumed\n");
	fprintf(ofp, "PARAM=-DTIMEZONE=%d -DDST=%d\n",
	    timezone, dst);
	while (fgets(line, BUFSIZ, ifp) != 0) {
		if (*line == '%')
			goto percent;
		if (profiling && strncmp(line, "COPTS=", 6) == 0) {
			register char *cp;

			fprintf(ofp, "GPROF.EX=/usr/src/lib/libc/csu/gmon.ex\n");
			cp = index(line, '\n');
			if (cp)
				*cp = 0;
			cp = line + 6;
			while (*cp && (*cp == ' ' || *cp == '\t'))
				cp++;
			COPTS = malloc((unsigned)(strlen(cp) + 1));
			if (COPTS == 0) {
				printf("config: out of memory\n");
				exit(1);
			}
			strcpy(COPTS, cp);
			fprintf(ofp, "%s -pg\n", line);
			continue;
		}
		fprintf(ofp, "%s", line);
		continue;
	percent:
		if (eq(line, "%OBJS\n"))
			do_objs(ofp);
		else if (eq(line, "%CFILES\n"))
			do_cfiles(ofp);
		else if (eq(line, "%RULES\n"))
			do_rules(ofp);
		else if (eq(line, "%LOAD\n"))
			do_load(ofp);
		else
			fprintf(stderr,
			    "Unknown %% construct in generic makefile: %s",
			    line);
	}
	(void) fclose(ifp);
	(void) fclose(ofp);
}

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
read_files()
{
	FILE *fp;
	register struct file_list *tp;
	register struct device *dp;
	char *wd, *this, *needs, *devorprof;
	char fname[32];
	int nreqs;
	int first = 1;

	ftab = 0;
	(void) strcpy(fname, "files");
openit:
	fp = fopen(fname, "r");
	if (fp == 0) {
		perror(fname);
		exit(1);
	}
next:
	/* filename	[ standard | optional dev* ] [ device-driver ] */
	wd = get_word(fp);
	if (wd == (char *)EOF) {
		(void) fclose(fp);
		if (first) {
			(void) sprintf(fname, "files.%s", machinename);
			first = 0;
			goto openit;
		}
		return;
	}
	if (wd == 0)
		goto next;
	this = ns(wd);
	next_word(fp, wd);
	if (wd == 0) {
		printf("%s: No type for %s.\n",
		    fname, this);
		exit(1);
	}
	if (fl_lookup(this)) {
		printf("%s: Duplicate file %s.\n",
		    fname, this);
		exit(1);
	}
	nreqs = 0;
	devorprof = "";
	needs = 0;
	if (eq(wd, "standard"))
		goto checkdev;
	if (!eq(wd, "optional")) {
		printf("%s: %s must be optional or standard\n",
		    fname, this);
		exit(1);
	}
nextopt:
	next_word(fp, wd);
	if (wd == 0)
		goto doneopt;
	devorprof = wd;
	if (eq(wd, "device-driver") || eq(wd, "profiling-routine")) {
		next_word(fp, wd);
		goto save;
	}
	nreqs++;
	if (needs == 0)
		needs = ns(wd);
	for (dp = dtab; dp != 0; dp = dp->d_next)
		if (eq(dp->d_name, wd))
			goto nextopt;
	while ((wd = get_word(fp)) != 0)
		;
	tp = new_fent();
	tp->f_fn = this;
	tp->f_type = INVISIBLE;
	tp->f_needs = needs;
	goto next;
doneopt:
	if (nreqs == 0) {
		printf("%s: what is %s optional on?\n",
		    fname, this);
		exit(1);
	}
checkdev:
	if (wd) {
		next_word(fp, wd);
		if (wd != 0) {
			devorprof = wd;
			next_word(fp, wd);
		}
	}
save:
	if (wd != 0) {
		printf("%s: syntax error describing %s\n",
		    fname, this);
		exit(1);
	}
	if (eq(devorprof, "profiling-routine") && profiling == 0)
		goto next;
	tp = new_fent();
	tp->f_fn = this;
	if (eq(devorprof, "device-driver"))
		tp->f_type = DEVICE;
	else if (eq(devorprof, "profiling-routine"))
		tp->f_type = PROFILING;
	else
		tp->f_type = NORMAL;
	tp->f_needs = needs;
	goto next;
}

do_objs(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;
	register char *cp, och, *sp;
	char *tail();

	fprintf(fp, "OBJS=");
	lpos = 6;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		sp = tail(tp->f_fn);
		cp = sp + (len = strlen(sp)) - 1;
		och = *cp;
		*cp = 'o';
		if (len + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "%s ", sp);
		lpos += len + 1;
		*cp = och;
	}
	if (lpos != 8)
		putc('\n', fp);
}

do_cfiles(fp)
	FILE *fp;
{
	register struct file_list *tp;
	register int lpos, len;

	fprintf(fp, "CFILES=");
	lpos = 8;
	for (tp = ftab; tp != 0; tp = tp->f_next) {
		if (tp->f_type == INVISIBLE)
			continue;
		if (tp->f_fn[strlen(tp->f_fn)-1] != 'c')
			continue;
		if ((len = 3 + strlen(tp->f_fn)) + lpos > 72) {
			lpos = 8;
			fprintf(fp, "\\\n\t");
		}
		fprintf(fp, "../%s ", tp->f_fn);
		lpos += len + 1;
	}
	if (lpos != 8)
		putc('\n', fp);
}

char *
tail(fn)
	char *fn;
{
	register char *cp;

	cp = rindex(fn, '/');
	return (cp+1);
}

/*
 * Create the makerules for each file
 * which is part of the system.
 * Devices are processed with the special c2 option -i
 * which avoids any problem areas with i/o addressing
 * (e.g. for the VAX); assembler files are processed by as.
 */
do_rules(f)
	FILE *f;
{
	register char *cp, *np, och, *tp;
	register struct file_list *ftp;

for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
	if (ftp->f_type == INVISIBLE)
		continue;
	cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
	och = *cp;
	*cp = '\0';
	fprintf(f, "%so: ../%s%c\n", tail(np), np, och);
	tp = tail(np);
	if (och == 's') {
		fprintf(f, "\t${AS} -o %so ../%ss\n\n", tp, np);
		continue;
	}
	switch (ftp->f_type) {

	case NORMAL:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S ${COPTS} ../%sc\n", np);
			fprintf(f, "\t${C2} %ss | sed -f ../%s/asm.sed |",
			    tp, machinename);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} ../%sc\n\n", np);
			break;
		}
		break;

	case DEVICE:
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S ${COPTS} ../%sc\n", np);
			fprintf(f,"\t${C2} -i %ss | sed -f ../%s/asm.sed |",
			    tp, machinename);
			fprintf(f, " ${AS} -o %so\n", tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} ../%sc\n\n", np);
		}
		break;

	case PROFILING:
		if (!profiling)
			continue;
		if (COPTS == 0) {
			fprintf(stderr,
			    "config: COPTS undefined in generic makefile");
			COPTS = "";
		}
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S %s ../%sc\n", COPTS, np);
			fprintf(f, "\tex - %ss < ${GPROF.EX}\n", tp);
			fprintf(f,
			    "\tsed -f ../vax/asm.sed %ss | ${AS} -o %so\n",
			    tp, tp);
			fprintf(f, "\trm -f %ss\n\n", tp);
			break;

		case MACHINE_SUN:
			fprintf(stderr,
			    "config: don't know how to profile kernel on sun\n");
			break;
		}
		break;

	default:
		printf("Don't know rules for %s", np);
		break;
	}
	*cp = och;
}
}

/*
 * Create the load strings
 */
do_load(f)
	register FILE *f;
{
	register struct file_list *fl;
	int first = 1;

	for (fl = conf_list; fl != 0; fl = fl->f_next) {
		fprintf(f, "%s: makefile locore.o ${OBJS} param.o",
		    fl->f_needs);
		fprintf(f, " ioconf.o swap%s.o\n", fl->f_fn);
		fprintf(f, "\t@echo loading %s\n\t@rm -f %s\n",
		    fl->f_needs, fl->f_needs);
		if (first) {
			first = 0;
			fprintf(f, "\t@sh ../conf/newvers.sh\n");
			fprintf(f, "\t@${CC} $(CFLAGS) -c vers.c\n");
		}
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t@${LD} -n -o %s -e start -x -T 80000000 ",
				fl->f_needs);
			fprintf(f, "locore.o ${OBJS} vers.o ioconf.o param.o ");
			fprintf(f, "swap%s.o\n", fl->f_fn);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t@${LD} -o %s -e start -x -T 4000 ",
				fl->f_needs);
			fprintf(f, "locore.o ${OBJS} vers.o ioconf.o param.o ");
			fprintf(f, "swap%s.o\n", fl->f_fn);
			break;
		}
		fprintf(f, "\t@echo rearranging symbols\n");
		fprintf(f, "\t@-symorder ../%s/symbols.sort %s\n",
		    machinename, fl->f_needs);
		fprintf(f, "\t@size %s\n", fl->f_needs);
		fprintf(f, "\t@chmod 755 %s\n\n", fl->f_needs);
	}
	for (fl = conf_list; fl != 0; fl = fl->f_next) {
		fprintf(f, "swap%s.o: ../%s/swap%s.c\n",
		    fl->f_fn, machinename, fl->f_fn);
		switch (machine) {

		case MACHINE_VAX:
			fprintf(f, "\t${CC} -I. -c -S ${COPTS}");
			fprintf(f, " ../%s/swap%s.c\n", machinename, fl->f_fn);
			fprintf(f, "\t${C2} swap%s.s | sed -f ../%s/asm.sed",
			    fl->f_fn, machinename);
			fprintf(f, " | ${AS} -o swap%s.o\n",
			    fl->f_fn);
			fprintf(f, "\trm -f swap%s.s\n\n", fl->f_fn);
			break;

		case MACHINE_SUN:
			fprintf(f, "\t${CC} -I. -c -O ${COPTS} ");
			fprintf(f, "../%s/swap%s.c\n\n", machinename, fl->f_fn);
			break;
		}
	}
	fprintf(f, "all:");
	for (fl = conf_list; fl != 0; fl = fl->f_next)
		fprintf(f, " %s", fl->f_needs);
	fprintf(f, "\n");
}

char *
raise(str)
	register char *str;
{
	register char *cp = str;

	while (*str) {
		if (islower(*str))
			*str = toupper(*str);
		str++;
	}
	return (cp);
}
