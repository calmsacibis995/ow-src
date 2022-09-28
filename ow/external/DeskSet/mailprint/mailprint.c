#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)mailprint.c 3.1 92/04/03 Copyr 1987 Sun Micro";
#endif
#endif

/* 
 * Mailprint - program to print out mail messages 
 * without printing the attachments.
 * It outputs always to stdout.
 */
/*
 * Syntax: mailprint [-b] [input file]
 * 		-b = want page break
 * assume last argument is file name
 */


#include <stdio.h>
#include "../maillib/obj.h"

#define TRUE 	1
#define FALSE 	0
#define MT_CE_TEXT_TYPE             "text"

static int want_page_break = FALSE;


/*
 * Get the first non-deleted message.
 */
struct msg *
FIRST_NDMSG (folder)
struct folder_obj *folder;
{                
        struct msg *p_msg;
 
        if (folder == NULL) {
                return (NULL);
        }
        else {
                p_msg = folder->fo_first_msg;
                while ((p_msg != NULL) && p_msg->mo_deleted)
                        p_msg = p_msg->mo_next;
                return (p_msg);
        }
}

/*
 * Get the next non-deleted message.
 */
struct msg *
NEXT_NDMSG(m)
struct msg *m;
{
        if (m == NULL)
                return (NULL);
        while ((m = m->mo_next) != NULL) {
                if (!m->mo_deleted)
                        break;
        }
        return (m);
}

/*
 * writeout messages to fp
 */
static int
writeout_msgs(folder, fp)

	struct folder_obj	*folder;
	FILE		*fp;

{
	struct msg	*m;
	int		write_error = 0;
	int		n;

	n = 0;

	for( m = FIRST_NDMSG(folder); m != NULL; m = NEXT_NDMSG( m ) ) {

		/* Write out message */
		if (n != 0)
		{
			/* Subsequent message.  Write page break */
			if (want_page_break)
			    fwrite("\f\n", 1, 2, fp);
		}
		if (write_msg(m, fp) < 0) {
			write_error = TRUE;
			goto EXIT;
		};

		/* Keep track of number of messages printed */
		n++;
	}
	
EXIT:
	if (write_error) {
		fprintf(stderr,"%s\n", gettext("Error in printing message"));
		return(-1);
	} else {
		return(0);
	}
}


/*
 * Get next attachment skipping delete ones
 */
struct attach *
get_next_attach(at)

        struct attach   *at;
{
        do {
                at = attach_methods.at_next(at);
        } while (at != NULL && attach_methods.at_get(at, ATTACH_DELETED));

        return(at);
}


is_text_attachment(at)

        struct attach   *at;

{
        /*
         * Check if the attachment is of a type which should be
         * displayed in the view or compose textsw
         */
        return ((int) attach_methods.at_get(at, ATTACH_IS_TEXT));
}


/*
 * Write a message to fp.  If a message has attachments then we only 
 * write out the first attachment if it is text.  We replace all other
 * attachments with a line telling the user that the message contains
 * attachments.
 */
write_msg(m, fp)

	struct msg	*m;
	FILE		*fp;

{
	struct attach	*at, *get_next_attach();
	int		rcode = 0;
	char		buf[80];
	int		n_attachments = 0;
	int		msg_fwrite();

	if ((at = attach_methods.at_first(m)) != NULL) {
		/* Message has attachments */

		/* If first attachment is text, print it */
		if (is_text_attachment(at)) {
			attach_methods.at_decode(at);
		}

		/* Count the number of attachments */
		while ((at = get_next_attach(at)) != NULL) {
			n_attachments++;
			/*
			 * mark things as deleted to force strip out of
			 * content-type later in mm_write_attach
			 */
			attach_methods.at_set(at, ATTACH_DELETED, TRUE);
		}
	
		/* use FULL_HDRS since we dont know about ignore */	
		msg_methods.mm_write_attach(m, MSG_DONT_TST|MSG_FULL_HDRS, fp);
		if (n_attachments > 0) {
			sprintf(buf,
	"\n\n\t<This message contains %d attachments (not printed)>\n\n\n",
				n_attachments);
			
			if ((int)fwrite(buf, strlen(buf), 1, fp) < 1)
			{
				return(-1);
			}
		}
	} else {
		if ( msg_methods.mm_write_msg(m, MSG_DONT_TST|MSG_FULL_HDRS, fp) != 0)
			{
			return(-1);
			}
	}

	return(0);
}

msg_fwrite(buffer, len, fp)

	char	*buffer;
	int	len;
	FILE	*fp;
{
	if ((int)fwrite(buffer, len, 1, fp) < 1)
		return(-1);
	else
		return(0);
}



main(argc, argv)
int argc;
char *argv[];
{
    struct folder_obj *folder;
    char *msgbuf;	    /* buffer to hold message */
    char *msgbufend;	    /* pointer to end of message buffer */
    int size;		    /* size of message */
    FILE *input_file = stdin; /* get message from standard input */
    FILE *output_file = stdout; /* get message from standard input */
    int error;
    int i;

    /*
     * Syntax: mailprint [-b] [input file]
     * 		-b = want page break
     * assume last argument is file name
     */
    for (i = 1; i < argc; i++) {
    	if (strcmp("-b", argv[i]) == 0)
		want_page_break = TRUE;
    	else if (strcmp("-?", argv[i]) == 0) {
		printf("Usage: mailprint [-b] [input file]\n");
		exit(1);
	}
	else if (i == (argc -1)) {
	   	input_file = fopen(argv[i], "r");
		if (!input_file) {
		    fprintf(stderr, "%s%s\n", gettext("Cannot open "), argv[1]);
		    exit(1);
    		} 
	}
    	else {
		printf("Usage: mailprint [-b] [input file]\n");
		exit(1);
	}
    }

    msgbuf = (char *) readmmsg(input_file, &size);
    msgbufend = &msgbuf[size];

    folder = folder_methods.fm_init(msgbuf, msgbufend);
    error = writeout_msgs(folder, output_file);
    if (error != 0) {
	fprintf(stderr,"%s\n", gettext("Error in printing message"));
	exit(1);
    }
    
    if (output_file != stdout)
    	fclose(output_file);

    folder_methods.fm_free( folder);
    exit(0);
}

