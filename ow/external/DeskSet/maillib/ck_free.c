/* @(#)ck_free.c	3.1 - 92/04/03 */

/* Free.c -- corresponds to Malloc */


void
ck_free( ptr )
void *ptr;
{
	if( ptr ) free( ptr );
}
