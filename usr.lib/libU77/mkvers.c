char id_mkvers[] = "%W%";
/*
 * extract sccs id strings from source files
 * first arg is lib name.
 * Put them in Version.c
 */

#include	<stdio.h>

#define SCCS_ID		"@(#)"
#define VERSION		"Version.c"

main(argc, argv)
int argc; char **argv;
{
	char buf[256];
	char *s, *e;
	char *index(), *ctime();
	long t;
	FILE *V, *fdopen();

	V = stdout; /* fdopen(creat(VERSION, 0644), "w"); */
	if (!V)
	{
		perror("mkvers");
		exit(1);
	}
	fprintf(V, "char *sccs_id[] = {\n");
	if (argc-- > 1)
	{
		time(&t);
		s = ctime(&t) + 4;
		s[20] = '\0';
		fprintf(V, "\t\"%s%s\t%s\",\n", SCCS_ID, *++argv, s);
	}
	while (--argc)
	{
		if (freopen(*++argv, "r", stdin) == NULL)
		{
			perror(*argv);
			continue;
		}
		while(gets(buf))
		{
			s = buf;
			while(s = index(s, '@'))
				if (strncmp(s, SCCS_ID, 4) == 0)
					break;
			if (s)
			{
				e = index(s, '"');
				if (e)
				{
					*e = '\0';
					fprintf(V, "\t\"%s\",\n", s);
					break;
				}
			}
		}
		if (feof(stdin))
			fprintf(stderr, "%s: no sccs id string\n", *argv);
	}
	fprintf(V, "};\n");
	fclose(V);
	fflush(stdout);
	fflush(stderr);
}
