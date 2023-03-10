static char sccsid[] = "@(#)index.c	4.1	(Berkeley)	9/11/82";

#include <stdio.h>


/*******
 *	char *
 *	index(str, c)	return pointer to character c
 *	char c, *str;
 *
 *	return codes:
 *		NULL  -  character not found
 *		pointer  -  pointer to character
 */

char *
index(str, c)
char c, *str;
{
	for (; *str != '\0'; str++) {
		if (*str == c)
			return(str);
	}

	return(NULL);
}
