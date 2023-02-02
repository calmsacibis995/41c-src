h15609
s 00007/00001/00182
d D 1.2 83/02/23 14:33:15 cooper 2 1
c added LongUnspecified
e
s 00183/00000/00000
d D 1.1 83/02/23 12:56:29 cooper 1 0
c date and time created 83/02/23 12:56:29 by cooper
e
u
U
t
T
I 1
/*
 * %M% %I% %G%
 */
#include "Courier.h"
#include <errno.h>

char *input_file;
char hfile[MAXSTR], cfile1[MAXSTR], cfile2[MAXSTR];
char ufile[MAXSTR], sfile[MAXSTR];
FILE *hf, *cf1, *cf2, *uf, *sf;
list Values, Types;
int errs;

/*
 * Predefined types.
 */
struct object
	*Boolean_type,
	*Cardinal_type, *LongCardinal_type,
	*Integer_type, *LongInteger_type,
	*String_type,
D 2
	*Unspecified_type;
E 2
I 2
	*Unspecified_type, *LongUnspecified_type;
E 2

struct object
	*Undefined_constant;

main(argc, argv)
	int argc;
	char **argv;
{
	if (argc == 1) {
		fprintf(stderr, "Usage: %s input_file ...\n", *argv);
		exit(1);
	}
	setup_predefs();
	while (--argc > 0) {
		errs = 0;
		input_file = *++argv;
		if (freopen(input_file, "r", stdin) == NULL) {
			perror(input_file);
			continue;
		}
		tempname(hfile);
		if ((hf = fopen(hfile, "w")) == NULL) {
			perror(hfile);
			continue;
		}
		tempname(ufile);
		if ((uf = fopen(ufile, "w")) == NULL) {
			perror(ufile);
			continue;
		}
		tempname(sfile);
		if ((sf = fopen(sfile, "w")) == NULL) {
			perror(sfile);
			continue;
		}
		tempname(cfile1);
		if ((cf1 = fopen(cfile1, "w")) == NULL) {
			perror(cfile1);
			continue;
		}
		tempname(cfile2);
		if ((cf2 = fopen(cfile2, "w")) == NULL) {
			perror(cfile2);
			continue;
		}
		(void) yyparse();
		fclose(hf); fclose(uf); fclose(sf);
		if (! errs) {
			int c;

			freopen(cfile2, "r", cf2);
			while ((c = getc(cf2)) != EOF)
				putc(c, cf1);
			fclose(cf1); fclose(cf2);
			unlink(cfile2);
			rename(cfile1, program_name, "_stubs.c");
			rename(hfile, program_name, ".h");
			rename(ufile, program_name, "_client.c");
			rename(sfile, program_name, "_server.c");
		} else {
			fclose(cf1); fclose(cf2);
		}
	}
}

rename(source, dest, suffix)
	char *source, *dest, *suffix;
{
	char newname[MAXSTR], backup[MAXSTR];
	extern int errno;

	sprintf(newname, "%s%s", dest, suffix);
	for (;;) {
		if (link(source, newname) == 0) {
			if (unlink(source) != 0)
				break;
			return (0);
		}
		if (errno != EEXIST || rename(newname, "#", newname) != 0)
			break;
	}
	perror(newname);
	return (-1);
}

/* VARARGS1 */
yyerror(s, args)
	char *s;
{
	extern int yylineno;

	errs = 1;
	fprintf(stderr, "%d: ", yylineno);
	_doprnt(s, &args, stderr);
	putc('\n', stderr);
	unlink(hfile); unlink(ufile); unlink(sfile);
	unlink(cfile1); unlink(cfile2);
}

tempname(buf)
	char *buf;
{
	static int n = 0;

	sprintf(buf, "tmp%d.%d", n, getpid());
	n++;
}

/*
 * This mess is needed because C doesn't handle initialization of unions.
 */
setup_predefs()
{
	struct object *t;

	Boolean_type = make(O_SYMBOL, "Boolean");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackBoolean";
	t->t_ufname = "UnpackBoolean";
	declare(&Values, Boolean_type, t);

	Cardinal_type = make(O_SYMBOL, "Cardinal");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackCardinal";
	t->t_ufname = "UnpackCardinal";
	declare(&Values, Cardinal_type, t);

	LongCardinal_type = make(O_SYMBOL, "LongCardinal");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackLongCardinal";
	t->t_ufname = "Unpack_LongCardinal";
	declare(&Values, LongCardinal_type, t);

	Integer_type = make(O_SYMBOL, "Integer");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackInteger";
	t->t_ufname = "UnpackInteger";
	declare(&Values, Integer_type, t);

	LongInteger_type = make(O_SYMBOL, "LongInteger");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackLongInteger";
	t->t_ufname = "UnpackLongInteger";
	declare(&Values, LongInteger_type, t);

	String_type = make(O_SYMBOL, "String");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackString";
	t->t_ufname = "UnpackString";
	declare(&Values, String_type, t);

	Unspecified_type = make(O_SYMBOL, "Unspecified");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackUnspecified";
	t->t_ufname = "UnpackUnspecified";
	declare(&Values, Unspecified_type, t);
I 2

	LongUnspecified_type = make(O_SYMBOL, "LongUnspecified");
	t = make(O_TYPE, C_PREDEF);
	t->t_pfname = "PackLongUnspecified";
	t->t_ufname = "UnpackLongUnspecified";
	declare(&Values, LongUnspecified_type, t);
E 2

	Undefined_constant = make(O_SYMBOL, "?undefined?");
	declare(&Types, Undefined_constant, Unspecified_type);
	declare(&Values, Undefined_constant, make(O_CONSTANT, 0));
}
E 1
