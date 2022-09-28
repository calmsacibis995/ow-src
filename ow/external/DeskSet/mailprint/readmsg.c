#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)readmsg.c 3.1 92/04/03 Copyr 1987 Sun Micro";
#endif
#endif

#include <stdio.h>

#define MAXBUFLEN 16384
#define TRUE 1

struct Msg_Store {
    struct Msg_Store *next;
    char buf[MAXBUFLEN];
};

/*
 * readmmsg() reads in a message from mfile into a character array and returns
 * a pointer to the array.  It also passes back a pointer to the size of the
 * array through the size parameter.  The function allocates space for the
 * msgbuf array.
 */

char *
readmmsg(fp, size)
FILE *fp;
int *size;		/* total size of the message */
{
    struct Msg_Store *mstore, *msghead, *prev_mstore;
    int n, index, charstogo, malloc_size;
    int lefttoread, offset;
    char *msgbuf;

    *size = 0;
    prev_mstore = mstore = (struct Msg_Store *) 
		ck_malloc(sizeof(struct Msg_Store));
    memset(mstore, 0, sizeof(struct Msg_Store));
    msghead = mstore;
    while ((n = fread(mstore->buf, 1, MAXBUFLEN, fp)) > 0) {
	*size = *size + n;
	if (n < MAXBUFLEN) {
		lefttoread = MAXBUFLEN - n;
		offset = n;
                while ((lefttoread > 0) &&
                       (n = fread(mstore->buf + offset, 1,
                        lefttoread, fp)) > 0) {

			*size = *size + n;
			lefttoread -= n;
			offset += n;
		}
	}
	mstore->next = (struct Msg_Store *) 
				    ck_malloc(sizeof(struct Msg_Store));
	prev_mstore = mstore;
	mstore = mstore->next;
	memset(mstore, 0, sizeof(struct Msg_Store));

    }
    /* We always allocate one too many mstores,
     * but don't worry, we'll free them all later
     */

    msgbuf = (char *) ck_malloc(*size);
    charstogo = *size;
    for (index = 0, mstore = msghead; charstogo > 0; 
	 charstogo -= MAXBUFLEN) {
	if (charstogo < MAXBUFLEN) {
	    memcpy(&msgbuf[index], mstore->buf, charstogo);
	} else {
	    memcpy(&msgbuf[index], mstore->buf, MAXBUFLEN);
	}
	mstore = mstore->next;
	index += MAXBUFLEN;
    }

#ifdef DEBUG
    printf(gettext("The message is:\n"));
    fwrite(msgbuf, *size, 1, stdout);
#endif
    for (mstore = msghead; mstore != NULL; 
	 mstore = mstore->next) {
    	ck_free(mstore);
    }
    return(msgbuf);
}

