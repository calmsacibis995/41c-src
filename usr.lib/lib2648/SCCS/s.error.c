h20104
s 00011/00000/00000
d D 4.1 83/03/09 16:22:39 ralph 1 0
c date and time created 83/03/09 16:22:39 by ralph
e
u
U
t
T
I 1
/*	%M%	%I%	%E%	*/
/*
 * error: default handling of errors.
 */

error(msg)
char *msg;
{
	message(msg);
	/* Maybe it would be nice to longjmp somewhere here */
}
E 1
