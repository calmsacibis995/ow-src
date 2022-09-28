#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_filter.c 20.51 97/05/21";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Filter invocation routines for textsw package.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#ifdef SVR4
#include <dirent.h>
#else
#include <sys/dir.h>
#endif /* SVR4 */
#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#include <xview/notify.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>

#ifdef sparc
#ifdef SVR4
#include <unistd.h>
#else
#include <vfork.h>
#endif /* SVR4 */
#endif
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <xview_private/ultrix_cpt.h>

#define FAIL		-1
#define	INPUT		0
#define	OUTPUT		1
#define	STDIN		0
#define	STDOUT		1
#define	STDERR		2
#define	PIPSIZ		4096	/* See uipc_pipe.c */
#define	SLOP_SIZE	16

#define	REPLY_ERROR	-1
#define	REPLY_OKAY	 0
#define	REPLY_SEND	 1

/* performance: global cache of getdtablesize() */
extern int      dtablesize_cache;

#ifdef SVR4
#define GETDTABLESIZE() \
(dtablesize_cache?dtablesize_cache:(dtablesize_cache=(int)sysconf(_SC_OPEN_MAX)))
#else
#define GETDTABLESIZE() \
(dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))
#endif /* SVR4 */


extern void textsw_SetSelection();

Xv_public char    *xv_getlogindir();
Pkg_private Es_index textsw_do_input();
static short unsigned type_for_filter_rec();
static int      event_code_for_filter_rec();

/*
 * WARNING: this is a hack to force the variable to be in memory. this var
 * gets changed somehow when it is in register in the context of a vfork();
 */
static int      execvp_failed;

Pkg_private Ev_mark_object textsw_add_mark_internal();
Xv_public Notify_value notify_default_wait3();

static int
interpret_filter_reply(view, buffer, buffer_length, delta)
    Textsw_view_handle view;
    void           *buffer;
    int             buffer_length;
    Es_index       *delta;
{
    *delta = textsw_do_input(view, buffer, (long) buffer_length,
			     TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
    if AN_ERROR
	(*delta != buffer_length)
	    return (REPLY_ERROR);
    return (REPLY_OKAY);
}

/*
 * Ignores SIGPIPE so that write(2) to nonexistent or dead filter does not
 * cause parent (containing textsw) to terminate.
 */
/* ARGSUSED */
static          Notify_value
textsw_sigpipe_func(textsw, sig, mode)
    Textsw_folio    textsw;	/* Currently unused */
    int             sig;	/* Currently unused */
    Notify_signal_mode mode;	/* Currently unused */
{
    return (NOTIFY_DONE);
}

static int
textsw_filter_selection(folio, fill_in)
    register Textsw_folio folio;
    register Textsw_selection_handle fill_in;
/*
 * Note: this routine guarantees to return a valid (but possibly empty)
 * selection, thus callers need not error check the return value.
 */
{
    extern wchar_t	_xv_null_string_wc[];
    if (multibyte)
        textsw_init_selection_object(folio, fill_in, NULL_STRING, 0, FALSE);
    else
        textsw_init_selection_object(folio, fill_in, "", 0, FALSE);

    fill_in->type = textsw_func_selection_internal(
					 folio, fill_in, EV_SEL_PRIMARY, 0);
    if (TFS_IS_ERROR(fill_in->type) || (fill_in->type & TFS_IS_OTHER)) {
	fill_in->last_plus_one = fill_in->first = ES_INFINITY;
	fill_in->type = EV_SEL_PRIMARY | TFS_IS_SELF;
    }
    if (fill_in->first < fill_in->last_plus_one) {
	textsw_SetSelection(VIEW_REP_TO_ABS(folio->first_view),
			     ES_INFINITY, ES_INFINITY,
			     (unsigned) fill_in->type);
    } else {
	fill_in->type &= ~EV_SEL_PENDING_DELETE;
    }
    return (fill_in->type);
}

Pkg_private int
textsw_call_filter(view, filter_argv)
    register Textsw_view_handle view;
    char           *filter_argv[];
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    int             filter_input, filter_output, result = 0;
    Es_index        save_length;
    Ev_mark_object  save_lpo_id;
    Ev_mark_object  save_insert = NULL;
    int             pid;
    Textsw_selection_object selection;
    Notify_func     old_sigpipe = (Notify_func) 0;

    (void) textsw_filter_selection(folio, &selection);
    if (selection.type & EV_SEL_PENDING_DELETE) {
	save_length = selection.last_plus_one - selection.first;
	save_lpo_id =
	    textsw_add_mark_internal(folio, selection.last_plus_one,
				     TEXTSW_MARK_DEFAULTS);
    }
    pid = start_filter(filter_argv, &filter_input, &filter_output);
    if (pid == FAIL) {
	result = 1;
	goto NoFilterReturn;
    }
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
    if ((int) ev_get(view->e_view, EV_CHAIN_LOWER_CONTEXT)
	!= EV_NO_CONTEXT)
	save_insert = textsw_add_mark_internal(folio,
					       EV_GET_INSERT(folio->views),
					       TEXTSW_MARK_MOVE_AT_INSERT);
    old_sigpipe =
	notify_set_signal_func((Notify_client) folio, textsw_sigpipe_func,
			       SIGPIPE, NOTIFY_ASYNC);
    (void) notify_set_wait3_func((Notify_client) folio,
				 notify_default_wait3, pid);
    /* Note that there is no meaningful old_wait3_func */
    switch (talk_to_filter(view, filter_input, filter_output,
			   selection.first, selection.last_plus_one,
			   interpret_filter_reply)) {
      case 0:
	break;
      case 1:
	goto ErrorReturn;
    }

NormalReturn:
    (void) close(filter_output);
    if ((result == 0) && (selection.type & EV_SEL_PENDING_DELETE)) {
	Es_index        saved_lpo;

	saved_lpo = textsw_find_mark_internal(folio, save_lpo_id);
	if AN_ERROR
	    (saved_lpo == ES_INFINITY) {
	} else {
	    /* delete replaces clipboard only if resource
	       text.DeleteReplacesClipboard is True.  Default is False. */
	    if (folio->state & TXTSW_DELETE_REPLACES_CLIPBOARD) {
		(void) textsw_delete_span(view, saved_lpo - save_length,
					  saved_lpo,
					  TXTSW_DS_ADJUST | TXTSW_DS_SHELVE);
	    } else {
		(void) textsw_delete_span(view, saved_lpo - save_length,
					  saved_lpo,
					  TXTSW_DS_ADJUST);
	    }
	}
    }
    if (old_sigpipe)
	(void) notify_set_signal_func((Notify_client) folio, old_sigpipe,
				      SIGPIPE, NOTIFY_ASYNC);

NoFilterReturn:
    if (selection.type & EV_SEL_PENDING_DELETE)
	textsw_remove_mark_internal(folio, save_lpo_id);
    /* Complete the deferred display update and autoscroll. */
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
    ev_update_chain_display(folio->views);
    TEXTSW_DO_INSERT_MAKES_VISIBLE(view);
    if (save_insert) {
	ev_scroll_if_old_insert_visible(folio->views,
			      textsw_find_mark_internal(folio, save_insert),
	/*
	 * Assign delta to minimum to ensure correct behavior, may not be
	 * optimal.
	 */
					1);
	textsw_remove_mark_internal(folio, save_insert);
    }
    return (result);

ErrorReturn:
    result = 2;
    goto NormalReturn;
}

static int
talk_to_filter(view, filter_input, filter_output, first, last_plus_one,
	       interpret_reply)
    register Textsw_view_handle view;
    int             filter_input, filter_output;
    register Es_index first, last_plus_one;
    int             (*interpret_reply) ();
{
#define	NR_FIRST		0
#define	NR_LAST_PLUS_ONE	1
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    int             max_fds = GETDTABLESIZE(), nr_valid = 0, status, result = 0;
    unsigned long   buffer_and_slop[(PIPSIZ + SLOP_SIZE) /
				                    sizeof(unsigned long)];
    unsigned char  *buffer;
    struct timeval  tv;
    Es_index        insert, next_request[2];

		    /* Number of unconverted bytes from last read */
    int		    unconverted_mb_num = 0;

		    /* Unconverted bytes from last read */
    char	    unconverted_mb_bytes[MB_LEN_MAX];

		    /* buffer for converting mb read from filter to wchar_t */
    wchar_t	    wc_buf[sizeof(buffer_and_slop) - SLOP_SIZE + 1];

    char	   *mb_buf = 0;
    int		    written_num, to_write_mb = 0;
    Es_index	    next;
    int             wc_len;


    /*
     * buffer_and_slop is declared unsigned long in order to meet the most
     * restrictive of the alignment restrictions.
     */
    buffer = (unsigned char *) buffer_and_slop + SLOP_SIZE;
    /*
     * Copy data into the filter.  As filter makes the filtered data
     * available, copy it to output_file. Stdio is not used to read from the
     * filter because it is necessary to make sure that the read does not
     * block.  In addition, for both reading and writing of the filter, the
     * stdio automatic buffering is only a hinderance.
     */
    for (;;) {
	register int    nfds;
	fd_set          exceptfds, readfds, writefds;
	long int        written[3];

	FD_ZERO(&exceptfds);
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);

	if(filter_output >= 0)
	  FD_SET(filter_output, &readfds);

	if(first < last_plus_one)
	{
	  if(filter_input >= 0)
	    FD_SET(filter_input, &writefds);
	}
	else
	{
	  /*
	   * For a "dumb" filter, we must close the input so that it does
	   * not hang waiting for more of the selection from us.
	   */
	  if(interpret_reply == interpret_filter_reply)
	  {
	    (void) close(filter_input);
	    filter_input = -1;
	  }
	}

	/*
	 * Filter only has tv.tv_sec seconds to reply, or we assume it is
	 * permanently hung. BUG ALERT! This should be customizable by user
	 * and/or client.
	 */
	tv.tv_sec = 15;
	tv.tv_usec = 0;
	nfds = select(max_fds, &readfds, &writefds, &exceptfds, &tv);
	switch (nfds) {
	  case -1:
	    if (errno == EINTR)
		continue;	/* Probably itimer expired */
	    goto ErrorReturn;
	  case 0:
	    goto ErrorReturn;	/* Assume filter has hung */
	  default:
	    break;
	}

	if((filter_output >= 0) && (FD_ISSET(filter_output, &readfds))) {
	    register int    bytes_read;
            if (multibyte) {    
              wc_len = 0;

	    /*
	     * As there is a set buffer size for reading from filter_output,
	     * the last some bytes may be the portion of a multibyte character.
	     * If so, the unconverted bytes are saved in unconverted_mb_bytes[]
	     * and unconverted_mb_num is set to the number of bytes saved.
	     * This unconverted bytes are tried to convert again with another
	     * read.
	     */

	    /*
	     * If there is unconverted data left over, copy it into the read 
	     * buffer.
	     */
	        if (unconverted_mb_num)
		    XV_BCOPY(unconverted_mb_bytes, buffer, unconverted_mb_num);

	    /* Read data from the filter into the buffer after the leftovers. */
	        bytes_read = read(filter_output,
			      (char *) buffer + unconverted_mb_num,
			      sizeof(buffer_and_slop) - SLOP_SIZE -
			      unconverted_mb_num);

	        if (bytes_read) {
		    bytes_read += unconverted_mb_num;
		    unconverted_mb_num = bytes_read;
		    wc_len = textsw_error_skip_mbstowcs(wc_buf, buffer,
					&unconverted_mb_num, MB_CUR_MAX);

		    if (unconverted_mb_num) {
		        ASSUME(unconverted_mb_num < MB_CUR_MAX);
		        XV_BCOPY(buffer + bytes_read - unconverted_mb_num,
			     unconverted_mb_bytes, unconverted_mb_num);
		    }
	        }    
	        else if (unconverted_mb_num) {
		    wc_len = textsw_error_skip_mbstowcs(wc_buf, buffer,
						  &unconverted_mb_num, 0);
	        }
            } else
	        wc_len = bytes_read = read(filter_output, (char *) buffer,
			      sizeof(buffer_and_slop) - SLOP_SIZE);

	    switch (wc_len) {
	      case 0:
		if (first >= last_plus_one)
		    goto NormalReturn;
		break;
	      case -1:
		if (errno == EINTR)
		    break;	/* Probably itimer expired */
		goto ErrorReturn;
	      default:
		insert = EV_GET_INSERT(folio->views);
                if (multibyte)
		    status = interpret_reply(view,wc_buf,
                                             wc_len,&written[0]);
                else
		    status =interpret_reply(view,buffer,
                                             bytes_read,&written[0]);
		switch (status) {
		  case REPLY_ERROR:
		    goto ErrorReturn;
		  case REPLY_OKAY:
		    break;
		  case REPLY_SEND:
		    if AN_ERROR
			(nr_valid != 0) {
		    } else {
			nr_valid = 1;
			next_request[NR_FIRST] = written[1];
			next_request[NR_LAST_PLUS_ONE] = written[2];
		    }
		    break;
		}
		if (insert <= first) {
		    first += written[0];
		    last_plus_one += written[0];
		}
		break;
	    }
	}

	if((filter_input >= 0) && (FD_ISSET(filter_input, &writefds)))
        if (multibyte)
	{
	    int             to_write;	/* number of characters */

	    if (to_write_mb) {   /* All bytes in mb_buf isn't written yet. */
		written[0] = write(filter_input, mb_buf + written_num,
				   to_write_mb);
		goto Write_check;
	    }
	    (void) es_set_position(folio->views->esh, first);
	    to_write = sizeof(buffer_and_slop) - SLOP_SIZE;
	    if (to_write > last_plus_one - first)
		to_write = last_plus_one - first;

	    next = es_read(folio->views->esh, to_write, wc_buf, &to_write);
	    wc_buf[to_write] = NULL;
	    if (READ_AT_EOF(first, next, to_write)) {
		first = last_plus_one;
	    } else {
		mb_buf = wcstombsdup(wc_buf);
		to_write_mb = strlen(mb_buf);
		written[0] = write(filter_input, mb_buf, to_write_mb);
    Write_check:
		if (written[0] == -1) {
		    if (errno == EMSGSIZE) {
			/* Go around and wait for filter to run */
			written_num = 0;
		    } else {
			xv_free(mb_buf);
			goto ErrorReturn;
		    }
		} else {
		    if (written[0] < to_write_mb) {
			written_num = written[0];
			to_write_mb -= written[0];
	    		/* Retry to write the rest bytes in mb_buf. */
		    }
		    else {
			xv_free(mb_buf);
			to_write_mb = 0;
		        first = next;
		    }
		}
	    }
	}
        else
	{
	    int             to_write;
	    Es_index        next;

	    (void) es_set_position(folio->views->esh, first);
	    to_write = sizeof(buffer_and_slop) - SLOP_SIZE;
	    if (to_write > last_plus_one - first)
		to_write = last_plus_one - first;

	    next = es_read(folio->views->esh, to_write, buffer, &to_write);
	    if (READ_AT_EOF(first, next, to_write)) {
		first = last_plus_one;
	    } else {
		ASSUME(to_write <= PIPSIZ);
		written[0] = write(filter_input, (char *) buffer, to_write);
		if (written[0] == -1) {
		    if (errno == EMSGSIZE) {
			/* Go around and wait for filter to run */
		    } else
			goto ErrorReturn;
		} else {
		    if (written[0] < to_write)
			next = (first + written[0]);

		    first = next;
		}
	    }
	}
	if (first >= last_plus_one) {
	    if (nr_valid != 0) {
		first = next_request[NR_FIRST];
		last_plus_one = next_request[NR_LAST_PLUS_ONE];
		nr_valid = 0;
	    }
	}
    }

NormalReturn:
    if (filter_input != -1)
	(void) close(filter_input);
    return (result);

ErrorReturn:
    result = 1;
    goto NormalReturn;
#undef	NR_FIRST
#undef	NR_LAST_PLUS_ONE
}

static int
smarter_interpret_filter_reply(view, buffer, buffer_length, delta)
    Textsw_view_handle view;
    register void  *buffer;
    int             buffer_length;
    Es_index       *delta;
{
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    void           *save_buffer = buffer;
    register void  *buffer_last_plus_one;
    int            *int_buffer;
    int             result = REPLY_OKAY;
    Es_index        temp;

    /* First deal with old stuff that is left over */
    if (folio->owed_by_filter < 0) {
        if (multibyte) {
	    XV_BCOPY(buffer, (wchar_t *)buffer - SLOP_SIZE -
                    folio->owed_by_filter, buffer_length*sizeof(wchar_t));
	    buffer_length -= folio->owed_by_filter;
	    buffer = (wchar_t *)buffer - SLOP_SIZE;
        } else {
	    XV_BCOPY(buffer,(char *)buffer-SLOP_SIZE-folio->owed_by_filter,
	          buffer_length);
	    buffer_length -= folio->owed_by_filter;
	    buffer = (char *)buffer - SLOP_SIZE;
        }
	folio->owed_by_filter = 0;
    }

    if (multibyte)
        buffer_last_plus_one = (wchar_t *)buffer + buffer_length;
    else 
        buffer_last_plus_one = (char *)buffer + buffer_length;

    /* Process the requests from the filter */
    while (buffer < buffer_last_plus_one) {
	if (folio->owed_by_filter > 0) {
	    int             to_insert = folio->owed_by_filter;
            if (!multibyte) {
	        if (to_insert > (char *)buffer_last_plus_one - 
                                             (char *)buffer)
		    to_insert = (char *)buffer_last_plus_one - (char *)buffer;
	        *delta = textsw_do_input(view, buffer, (long) to_insert,
				     TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
	        if AN_ERROR(*delta != to_insert)
		    return (REPLY_ERROR);
	        folio->owed_by_filter -= to_insert;
	        buffer = (char *)buffer + 
                  ((to_insert + sizeof(int) - 1) / sizeof(int)) * sizeof(int);
            } else {
	        if (to_insert > (wchar_t *)buffer_last_plus_one - 
                                             (wchar_t *)buffer)
		    to_insert = (wchar_t *)buffer_last_plus_one - 
                                           (wchar_t *)buffer;
	        *delta = textsw_do_input(view, buffer, (long) to_insert,
				     TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
	        if AN_ERROR(*delta != to_insert)
		    return (REPLY_ERROR);
	        folio->owed_by_filter -= to_insert;
	        buffer = (wchar_t *)buffer + 
                  ((to_insert + sizeof(int) - 1) / sizeof(int)) * sizeof(int);
            }
	    continue;
	}
	int_buffer = (int *) buffer;
        if (!multibyte) {
	    if (((char *)buffer_last_plus_one - 
                   (char *)buffer < 2 * sizeof(int)) ||
	           ((int_buffer[0]) !=TEXTSW_FILTER_MAGIC))
	        goto Deal_With_Slop;
        } else {
	    if (((wchar_t *)buffer_last_plus_one - 
                   (wchar_t *)buffer < 2 * sizeof(int)) ||
	           ((int_buffer[0]) !=TEXTSW_FILTER_MAGIC))
	        goto Deal_With_Slop;
        }
	switch ((Textsw_filter_command) (int_buffer[1])) {
	  case TEXTSW_FILTER_DELETE_RANGE:
            if (!multibyte) {
	        if ((char *)buffer_last_plus_one - 
                         (char *)buffer < 4 * sizeof(int))
		    goto Deal_With_Slop;
	        *delta = textsw_delete_span(view, (long) int_buffer[2],
					(long) int_buffer[3],
					TXTSW_DS_ADJUST | TXTSW_DS_SHELVE);
	        buffer = (char *)buffer + (4 * sizeof(int));
            } else {
	        if ((wchar_t *)buffer_last_plus_one - 
                         (wchar_t *)buffer < 4 * sizeof(int))
		    goto Deal_With_Slop;
	        *delta = textsw_delete_span(view, (long) int_buffer[2],
					(long) int_buffer[3],
					TXTSW_DS_ADJUST | TXTSW_DS_SHELVE);
	        buffer = (wchar_t *)buffer + (4 * sizeof(int));
            }
	    break;
	  case TEXTSW_FILTER_INSERT:
            if (!multibyte) {
	        if ((char *)buffer_last_plus_one - 
                             (char *)buffer < 3 * sizeof(int))
		    goto Deal_With_Slop;
	        folio->owed_by_filter = int_buffer[2];
	        buffer = (char *)buffer + (3 * sizeof(int));
            } else {
	        if ((wchar_t *)buffer_last_plus_one - 
                             (wchar_t *)buffer < 3 * sizeof(int))
		    goto Deal_With_Slop;
	        folio->owed_by_filter = int_buffer[2];
	        buffer = (wchar_t *)buffer + (3 * sizeof(int));
            }
	    break;
	  case TEXTSW_FILTER_SEND_RANGE:
            if (!multibyte) {
		if ((char *)buffer_last_plus_one - (char *)buffer < 
                                                4 * sizeof(int))
		    goto Deal_With_Slop;
		delta[1] = int_buffer[2];
		delta[2] = int_buffer[3];
		result = REPLY_SEND;
	        buffer = (char *)buffer + (4 * sizeof(int));
            } else {
		if ((wchar_t *)buffer_last_plus_one - (wchar_t *)buffer < 
                                                4 * sizeof(int))
		    goto Deal_With_Slop;
		delta[1] = int_buffer[2];
		delta[2] = int_buffer[3];
		result = REPLY_SEND;
	        buffer = (wchar_t *)buffer + (4 * sizeof(int));
            }
            break;
	  case TEXTSW_FILTER_SET_INSERTION:
            if (!multibyte) {
	        if ((char *)buffer_last_plus_one - 
                        (char *)buffer < 3 * sizeof(int))
		    goto Deal_With_Slop;
	        EV_SET_INSERT(folio->views, (Es_index) int_buffer[2], temp);
	        buffer = (char *)buffer + (3 * sizeof(int));
            } else {
	        if ((wchar_t *)buffer_last_plus_one - 
                        (wchar_t *)buffer < 3 * sizeof(int))
		    goto Deal_With_Slop;
	        EV_SET_INSERT(folio->views, (Es_index) int_buffer[2], temp);
	        buffer = (wchar_t *)buffer + (3 * sizeof(int));
            }
	    break;
	  case TEXTSW_FILTER_SET_SELECTION:
            if (!multibyte) {
	        if ((char *)buffer_last_plus_one - 
                      (char *)buffer < 4 * sizeof(int))
		    goto Deal_With_Slop;
	        textsw_SetSelection(VIEW_REP_TO_ABS(folio->first_view),
				 (Es_index) int_buffer[2],
				 (Es_index) int_buffer[3],
				 EV_SEL_PRIMARY);
	        buffer = (char *)buffer + (4 * sizeof(int));
            } else {
	        if ((wchar_t *)buffer_last_plus_one - 
                      (wchar_t *)buffer < 4 * sizeof(int))
		    goto Deal_With_Slop;
	        textsw_SetSelection(VIEW_REP_TO_ABS(folio->first_view),
				 (Es_index) int_buffer[2],
				 (Es_index) int_buffer[3],
				 EV_SEL_PRIMARY);
	        buffer = (wchar_t *)buffer + (4 * sizeof(int));
            }
	    break;
	  default:
	    return (REPLY_ERROR);
	}
    }
    return (result);

Deal_With_Slop:

    if (!multibyte) {
        ASSUME((char *)buffer_last_plus_one - (char *)buffer < SLOP_SIZE);
        if ((char *)buffer >= (char *)save_buffer)
	    XV_BCOPY(buffer, (char *)buffer - SLOP_SIZE, 
                    (char *)buffer_last_plus_one - (char *)buffer);
        folio->owed_by_filter = (char *)buffer - (char *)buffer_last_plus_one;
    } else {
        ASSUME((wchar_t *)buffer_last_plus_one - (wchar_t *)buffer < SLOP_SIZE);
        if ((wchar_t *)buffer >= (wchar_t *)save_buffer)
	    XV_BCOPY(buffer, (wchar_t *)buffer - SLOP_SIZE, 
                       ((wchar_t *)buffer_last_plus_one - 
                               (wchar_t *)buffer)*sizeof(wchar_t));
        folio->owed_by_filter = 
            (wchar_t *)buffer - (wchar_t *)buffer_last_plus_one;
    }
    return (result);
}

Pkg_private int
textsw_call_smart_filter(view, event, filter_argv)
    register Textsw_view_handle view;
    Event          *event;
    char           *filter_argv[];
{
    /*
     * A smart filter is expected to do more than just filter the current
     * selection.  As a result, it is passed more information; in particular:
     * the entire input event that resulted in the filter invocation the
     * position of the insertion point, and the first and last_plus_one
     * positions of the line containing the insertion point, the first and
     * last_plus_one positions of the selection, the line containing the
     * insertion point, the selection
     */
#define	FILTER_STARTED		0
#define	HEADER_WRITTEN		1
#define	INSERT_LINE_WRITTEN	2
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    int             filter_input, filter_output, max_fds = GETDTABLESIZE(), result = 0;
    Textsw_filter_attribute filter_attribute;
    char            buffer[PIPSIZ];   
    struct timeval  tv;
    Es_index        insert, save_length;
    Es_index        insert_line_first, insert_line_last_plus_one;
    unsigned        state = FILTER_STARTED;
    Ev_mark_object  save_lpo_id;
    int             pid;
    Textsw_selection_object selection;
    Notify_func     old_sigpipe = (Notify_func) 0;

    insert = EV_GET_INSERT(folio->views);
    ev_span(folio->views, insert, &insert_line_first,
	    &insert_line_last_plus_one, EI_SPAN_LINE);
    if (insert_line_first == ES_CANNOT_SET) {
	insert_line_first = insert_line_last_plus_one = insert;
    }
    (void) textsw_filter_selection(folio, &selection);
    if (selection.type & EV_SEL_PENDING_DELETE) {
	save_length = selection.last_plus_one - selection.first;
	save_lpo_id =
	    textsw_add_mark_internal(folio, selection.last_plus_one,
				     TEXTSW_MARK_DEFAULTS);
    }
    pid = start_filter(filter_argv, &filter_input, &filter_output);
    if (pid == FAIL) {
	result = 1;
	goto NoFilterReturn;
    }
    old_sigpipe =
	notify_set_signal_func((Notify_client) folio, textsw_sigpipe_func,
			       SIGPIPE, NOTIFY_ASYNC);
    (void) notify_set_wait3_func((Notify_client) folio,
				 notify_default_wait3, pid);
    /* Note that there is no meaningful old_wait3_func */
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
    /*
     * Give the filter the initial information
     */
    for (;;) {
	register int    nfds;
	fd_set          exceptfds, readfds, writefds;
	int             written;
	register struct timeval *tv_to_use;

	FD_ZERO(&exceptfds);
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_SET(filter_input, &writefds);
	tv_to_use = (timerisset(&tv)) ? &tv : 0;
	nfds = select(max_fds, &readfds, &writefds, &exceptfds, tv_to_use);
	switch (nfds) {
	  case -1:
	    if (errno == EINTR)
		continue;	/* Probably itimer expired */
	    goto ErrorReturn;
	  case 0:		/* Fall through */
	  default:
	    timerclear(&tv);
	    break;
	}
	/* if (writefds & (1 << filter_input)) { */
	if (FD_ISSET(filter_input, &writefds)) {
	    int             to_write;
	    Es_index        next;

	    if (state == FILTER_STARTED) {
		filter_attribute = TEXTSW_FATTR_INPUT_EVENT;
		/*  BIG BUG: Need to write in wchar	OW_I18N */
		if ((write(filter_input, (char *) &filter_attribute,
			   sizeof(filter_attribute)) == -1) ||
		    (write(filter_input, (char *) event,
			   sizeof(*event)) == -1))
		    goto Write_Failed;
		if ((char *)folio->to_insert_next_free > 
                        (char *)folio->to_insert) {
		    filter_attribute = TEXTSW_FATTR_INPUT;
		    to_write = (char *)folio->to_insert_next_free -
			(char *)folio->to_insert;
		    if ((write(filter_input, (char *) &filter_attribute,
			       sizeof(filter_attribute)) == -1) ||
			(write(filter_input, (char *) &to_write,
			       sizeof(to_write)) == -1) ||
			(write(filter_input, folio->to_insert,
			       to_write) == -1))
			goto Write_Failed;
		}
		filter_attribute = TEXTSW_FATTR_INSERTION_POINTS;
		if ((write(filter_input, (char *) &filter_attribute,
			   sizeof(filter_attribute)) == -1) ||
		    (write(filter_input, (char *) &insert,
			   sizeof(insert)) == -1) ||
		    (write(filter_input, (char *) &insert_line_first,
			   sizeof(insert_line_first)) == -1) ||
		    (write(filter_input,
			   (char *) &insert_line_last_plus_one,
			   sizeof(insert_line_last_plus_one)) == -1))
		    goto Write_Failed;
		filter_attribute = TEXTSW_FATTR_SELECTION_ENDPOINTS;
		if ((write(filter_input, (char *) &filter_attribute,
			   sizeof(filter_attribute)) == -1) ||
		    (write(filter_input, (char *) &selection.first,
			   sizeof(selection.first)) == -1) ||
		    (write(filter_input, (char *) &selection.last_plus_one,
			   sizeof(selection.last_plus_one)) == -1))
		    goto Write_Failed;
		if (insert_line_first < insert_line_last_plus_one) {
		    filter_attribute = TEXTSW_FATTR_INSERTION_LINE;
		    if (write(filter_input, (char *) &filter_attribute,
			      sizeof(filter_attribute)) == -1)
			goto Write_Failed;
		}
		state = HEADER_WRITTEN;
	    }
	    if (state == HEADER_WRITTEN) {
		if (insert_line_last_plus_one <= insert_line_first)
		    goto Done_Initial_Data;
		(void) es_set_position(folio->views->esh,
				       insert_line_first);
		to_write = sizeof(buffer);
		if (to_write > insert_line_last_plus_one - insert_line_first)
		    to_write = insert_line_last_plus_one - insert_line_first;
		next = es_read(folio->views->esh, to_write, buffer,
			       &to_write);
		if (READ_AT_EOF(insert_line_first, next, to_write)) {
		    goto Done_Initial_Data;
		} else {
		    ASSUME(to_write <= PIPSIZ);
		    written = write(filter_input, (char *) buffer,
				    to_write);
		    if (written == -1)
			goto Write_Failed;
		    insert_line_first = next;
		}
	    }
	}
	continue;
Write_Failed:
	if (errno == EMSGSIZE) {
	    /* Give filter a chance to run */
	    tv.tv_sec = 0;
	    tv.tv_usec = 50000;
	} else
	    goto ErrorReturn;
    }
Done_Initial_Data:
    state = INSERT_LINE_WRITTEN;
    switch (talk_to_filter(view, filter_input, filter_output,
			   ES_INFINITY, ES_INFINITY,
			   smarter_interpret_filter_reply)) {
      case 0:
	break;
      case 1:
	goto ErrorReturn;
    }

NormalReturn:
    (void) close(filter_output);
    if ((result == 0) && (selection.type & EV_SEL_PENDING_DELETE)) {
	Es_index        saved_lpo;

	saved_lpo = textsw_find_mark_internal(folio, save_lpo_id);
	if AN_ERROR
	    (saved_lpo == ES_INFINITY) {
	} else {
	    Bool defaults_get_boolean();
	    /* delete replaces clipboard only if resource
	       text.DeleteReplacesClipboard is True.  Default is False. */
	    if (folio->state & TXTSW_DELETE_REPLACES_CLIPBOARD) {
		(void) textsw_delete_span(view, saved_lpo - save_length,
					  saved_lpo,
					  TXTSW_DS_ADJUST | TXTSW_DS_SHELVE);
	    } else {
		(void) textsw_delete_span(view, saved_lpo - save_length,
					  saved_lpo,
					  TXTSW_DS_ADJUST);
	    }
	}
    }
    if (old_sigpipe)
	(void) notify_set_signal_func((Notify_client) folio, old_sigpipe,
				      SIGPIPE, NOTIFY_ASYNC);

NoFilterReturn:
    if (selection.type & EV_SEL_PENDING_DELETE)
	textsw_remove_mark_internal(folio, save_lpo_id);
    folio->owed_by_filter = 0;
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
    ev_update_chain_display(folio->views);
    return (result);

ErrorReturn:
    result = 3;
    goto NormalReturn;
}

Pkg_private void
textsw_close_nonstd_fds_on_exec()
{
    int             fd, max_fds = GETDTABLESIZE();

    for (fd = STDERR + 1; fd < max_fds; fd++)
#ifdef SVR4
	(void) xv_fcntl(fd, F_SETFD, 1);
#else
	(void) fcntl(fd, F_SETFD, 1);
#endif
}

static int
start_filter(filter_argv, filter_input, filter_output)
    char           *filter_argv[];
    int            *filter_input, *filter_output;
{
    int             to_filter[2], from_filter[2], pid;

    errno = 0;
    if ((pipe(to_filter) != 0) || (pipe(from_filter) != 0))
	return (FAIL);
    pid = vfork();
    if (pid == 0) {		/* child */
	/*
	 * Fiddle with the file descriptors, since the exec'd filter expects
	 * to read the data from stdin, and to write to stdout.
	 */
	if ((dup2(to_filter[INPUT], STDIN) == -1) ||
	    (dup2(from_filter[OUTPUT], STDOUT) == -1))
	    _exit(6);
	textsw_close_nonstd_fds_on_exec();
	(void) execvp(filter_argv[0], filter_argv);
	/*
	 * Since we used vfork, the child is sharing the parent's address
	 * space.  The advantage is that we can let the parent know that the
	 * execvp failed simply by changing the (shared) execvp_failed.  The
	 * disadvantage is that we have to be very careful about how we
	 * terminate the (now unwanted) child.
	 */
	execvp_failed = TRUE;
	_exit(7);
    }
    /* parent */
    if (execvp_failed || (pid < 0)) {
	execvp_failed = FALSE;
	return (FAIL);
    }
    if ((close(to_filter[INPUT]) == -1) ||
	(close(from_filter[OUTPUT]) == -1))
	return (FAIL);
    /*
     * The pipes to the filter must be non-blocking so that caller can avoid
     * deadly embrace while pushing the data through the filter.
     */

#ifdef SVR4
    if (xv_fcntl(to_filter[OUTPUT], F_SETFL, FNDELAY) == -1)
#else
    if (fcntl(to_filter[OUTPUT], F_SETFL, FNDELAY) == -1)
#endif
	return (FAIL);

#ifdef SVR4
    if (xv_fcntl(from_filter[INPUT], F_SETFL, FNDELAY) == -1)
#else
    if (fcntl(from_filter[INPUT], F_SETFL, FNDELAY) == -1)
#endif
	return (FAIL);

    *filter_input = to_filter[OUTPUT];
    *filter_output = from_filter[INPUT];
    return (pid);
}

/*
 * Parser of the .textswrc file at startup time
 */

#include <xview_private/filter.h>
#undef FOREVER
#include <xview_private/io_stream.h>

extern STREAM  *xv_file_input_stream();
extern STREAM  *xv_filter_comments_stream();

Pkg_private int
textsw_parse_rc(textsw)
    Textsw_folio    textsw;
{
    char           *base_name = ".textswrc", file_name[MAXNAMLEN], *login_dir;
    STREAM         *rc_stream = NULL;
    STREAM         *rc_wo_comments_stream = NULL;
    Key_map_handle  current_key;
    Key_map_handle *previous_key;
    int             i;
    short unsigned  type;
    short           event_code;
    int             result = 0;
    struct filter_rec **filters = NULL;

    textsw->key_maps = NULL;
    if ((login_dir = xv_getlogindir()) == NULL)
	return (1);
    (void) snprintf(file_name, MAXNAMLEN, "%s/%s", login_dir, base_name);
    if ((rc_stream = xv_file_input_stream(file_name, (FILE *) 0)) == NULL) {
	result = 2;
	goto Done;
    }
    if ((rc_wo_comments_stream = xv_filter_comments_stream(rc_stream))
	== NULL) {
	result = 3;
	goto Done;
    }
    if ((filters = xv_parse_filter_table(rc_wo_comments_stream, base_name)) == NULL) {
	result = 4;
	goto Done;
    }
    previous_key = &textsw->key_maps;
    for (i = 0; filters[i]; i++) {
	if ((event_code = event_code_for_filter_rec(filters[i])) == -1) {
	    continue;
	}
	if ((type = type_for_filter_rec(filters[i])) == TXTSW_KEY_NULL) {
	    continue;
	}
	*previous_key = current_key = NEW(Key_map_object);
	current_key->next = NULL;
	current_key->event_code = event_code;
	current_key->type = type;
	current_key->shifts = 0;
	current_key->maps_to = (caddr_t) filters[i]->call;
	filters[i]->call = NULL;/* Transfer storage ownership. */
	previous_key = &(current_key->next);
    }
Done:
    if (rc_stream != NULL)
	stream_close(rc_stream);
    if (rc_wo_comments_stream != NULL)
	stream_close(rc_wo_comments_stream);
    if (filters != NULL)
	xv_free_filter_table(filters);
    return (result);
}

static char    *type_groups[] = {
    "FILTER", "SMART_FILTER", "MACRO", NULL
};

static short unsigned
type_for_filter_rec(rec)
    struct filter_rec *rec;
{
    int             count;

    count = match_in_table(rec->class, type_groups);
    switch (count) {
      case -1:
	return (TXTSW_KEY_NULL);
      case TXTSW_KEY_FILTER:
      case TXTSW_KEY_SMART_FILTER:
      case TXTSW_KEY_MACRO:
	return (count);
      default:
	LINT_IGNORE(ASSUME(0));
	return (TXTSW_KEY_NULL);
    }
}

static char    *key_groups[] = {
    "KEY_LEFT", "KEY_TOP", "KEY_RIGHT", "KEY_BOTTOM",
    "L", "T", "F", "R", "B", NULL
};

static int
event_code_for_filter_rec(rec)
    struct filter_rec *rec;
{
    int             count;

    count = match_in_table(rec->key_name, key_groups);
    switch (count) {
      case -1:
	return (-1);
      case 0:
      case 4:
	return ((rec->key_num < 0 || rec->key_num > 15)
		? -1
		: KEY_LEFT(rec->key_num));
      case 1:
      case 5:
      case 6:
	return ((rec->key_num < 0 || rec->key_num > 15)
		? -1
		: KEY_TOP(rec->key_num));
      case 2:
      case 7:
	return ((rec->key_num < 0 || rec->key_num > 15)
		? -1
		: KEY_RIGHT(rec->key_num));
      case 3:
      case 8:
	return ((rec->key_num < 0 || rec->key_num > 1)
		? -1
		: KEY_BOTTOMLEFT + rec->key_num);
      default:
	LINT_IGNORE(ASSUME(0));
	return (-1);
    }
}
