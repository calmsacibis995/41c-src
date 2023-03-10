/* Copyright (c) 1982 Regents of the University of California */

static char sccsid[] = "@(#)source.c 1.2 2/11/82";

/*
 * Source file management.
 */

#include "defs.h"
#include "source.h"

/*
 * Seektab is the data structure used for indexing source
 * seek addresses by line number.
 *
 * The constraints are:
 *
 *  we want an array so indexing is fast and easy
 *  we don't want to waste space for small files
 *  we don't want an upper bound on # of lines in a file
 *  we don't know how many lines there are
 *
 * The solution is a "dirty" hash table.  We have NSLOTS pointers to
 * arrays of NLINESPERSLOT addresses.  To find the source address of
 * a particular line we find the slot, allocate space if necessary,
 * and then find its location within the pointed to array.
 *
 * As a result, there is a limit of NSLOTS*NLINESPERSLOT lines per file
 * but this is plenty high and still fairly inexpensive.
 *
 * This implementation maintains only one source file at any given
 * so as to avoid consuming too much memory.  In an environment where
 * memory is less constrained and one expects to be changing between
 * files often enough, it would be reasonable to have multiple seek tables.
 */

typedef int SEEKADDR;

#define NSLOTS 20
#define NLINESPERSLOT 500

#define slotno(line)    ((line)/NLINESPERSLOT)
#define index(line) ((line)%NLINESPERSLOT)
#define slot_alloc()    alloc(NLINESPERSLOT, SEEKADDR)
#define srcaddr(line)   seektab[(line)/NLINESPERSLOT][(line)%NLINESPERSLOT]

LOCAL SEEKADDR *seektab[NSLOTS];

LOCAL FILE *srcfp;

/*
 * check to make sure a source line number is valid
 */

chkline(linenum)
register LINENO linenum;
{
    if (linenum < 1) {
	error("line number must be positive");
    }
    if (linenum > lastlinenum) {
	error("not that many lines");
    }
}

/*
 * print out the given lines from the source
 */

printlines(l1, l2)
LINENO l1, l2;
{
    register int c;
    register LINENO i;
    register FILE *fp;

    chkline(l1);
    chkline(l2);
    if (l2 < l1) {
	error("second line number less than first");
    }
    fp = srcfp;
    fseek(fp, (long) srcaddr(l1), 0);
    for (i = l1; i <= l2; i++) {
	printf("%5d   ", i);
	while ((c = getc(fp)) != '\n') {
	    putchar(c);
	}
	putchar('\n');
    }
}

/*
 * read the source file getting seek pointers for each line
 */

skimsource(file)
char *file;
{
    register int c;
    register LINENO count;
    register FILE *fp;
    register LINENO linenum;
    register SEEKADDR lastaddr;
    register int slot;

    if (file == NIL || file == cursource) {
	return;
    }
    if ((fp = fopen(file, "r")) == NULL) {
	panic("can't open \"%s\"", file);
    }
    if (cursource != NIL) {
	free_seektab();
    }
    cursource = file;
    linenum = 0, count = 0, lastaddr = 0;
    while ((c = getc(fp)) != EOF) {
	count++;
	if (c == '\n') {
	    slot = slotno(++linenum);
	    if (slot >= NSLOTS) {
		panic("skimsource: too many lines");
	    }
	    if (seektab[slot] == NIL) {
		seektab[slot] = slot_alloc();
	    }
	    seektab[slot][index(linenum)] = lastaddr;
	    lastaddr = count;
	}
    }
    lastlinenum = linenum;
    srcfp = fp;
}

/*
 * Erase information and release space in the current seektab.
 * This is in preparation for reading in seek pointers for a
 * new file.  It is possible that seek pointers for all files
 * should be kept around, but the current concern is space.
 */

LOCAL free_seektab()
{
    register int slot;

    for (slot = 0; slot < NSLOTS; slot++) {
	if (seektab[slot] != NIL) {
	    free(seektab[slot]);
	    seektab[slot] = NIL;
	}
    }
}
