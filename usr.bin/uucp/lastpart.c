static char sccsid[] = "@(#)lastpart.c	4.1	(Berkeley)	9/11/82";



/*******
 *	char *
 *	lastpart(file)	find last part of file name
 *	char *file;
 *
 *	return - pointer to last part
 */

char *
lastpart(file)
char *file;
{
	char *c;

	c = file + strlen(file);
	while (c >= file)
		if (*(--c) == '/')
			break;
	return(++c);
}
