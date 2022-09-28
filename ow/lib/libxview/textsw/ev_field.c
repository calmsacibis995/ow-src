#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)ev_field.c 20.15 93/07/26";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * Procedure for doing field matching.
 */

#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>

#define BUF_SIZE        1024

static int getBufSize()
{
    if (!multibyte)
        return BUF_SIZE;
    else
        return (BUF_SIZE/sizeof(wchar_t));
}

Pkg_private     Es_index
ev_match_field_in_esh(esh, symbol1, symbol1_len, symbol2, symbol2_len, start_pattern, direction)
    Es_handle       esh;
    void           *symbol1, *symbol2;	/* starting and ending symbol */
    int             symbol1_len, symbol2_len;
    Es_index        start_pattern;	/* start search position */
    unsigned        direction;	/* search direction */
{

    int             match_done = 0;
    Es_index        pos, new_pos;
    int             buf_size;
    char            buf[BUF_SIZE];
    int             read;
    Es_index        result_pos = ES_CANNOT_SET;
    int             count = 0;
    int             buf_index, i, j, k, l;

    buf_size = getBufSize();
    pos = new_pos = ((direction & EV_FIND_BACKWARD) ? --start_pattern :
                         start_pattern);

    if (direction & EV_FIND_BACKWARD) {
	struct es_buf_object esbuf;
	esbuf.esh = esh;
	esbuf.buf = buf;
	esbuf.sizeof_buf = getBufSize();
	esbuf.first = ES_INFINITY;

	while (!match_done) {
	    if (pos < 0)
		goto Return;
	    if (es_make_buf_include_index(&esbuf,
					  pos, getBufSize() - 1)) {
		Es_index        length = es_get_length(esh);

		if (esbuf.first == ES_CANNOT_SET)
		    goto Return;

		if (pos < length)
		    goto Return;

	    } else {
		new_pos = esbuf.first;
		read = esbuf.last_plus_one - new_pos;
		buf_index = pos - new_pos;
		if (read <= 0)
		    goto Return;
		else {

                    if (!multibyte) {

		        FOREVER {
			    if (((char *)symbol1)[symbol1_len - 1] == 
                                                     buf[buf_index]) {
			        k = symbol1_len - 2;
			        l = buf_index - 1;
			        while ((k >= 0) && 
                                     (((char *)symbol1)[k] == buf[l])) {
				    k--;
				    l--;
			        }
			        if (k < 0) {
				    count++;
				    buf_index = l;
			        } else
				    buf_index--;
			    } else 
                                if (((char *)symbol2)[symbol2_len - 1] ==
                                                      buf[buf_index]) {
			        i = symbol2_len - 2;
			        j = buf_index - 1;
			        while ((i >= 0) && 
                                        (((char *)symbol2)[i] == buf[j])) {
				    i--;
			    	    j--;
			        }
			        if (i < 0) {
				    if (--count == 0) {
				        result_pos = new_pos + j + 1;
				        match_done = 1;
				    } else
				        buf_index = j;
			        } else
				    buf_index--;
			    } else
			        buf_index--;

			    if (match_done || (buf_index < 0)) {
			        pos = new_pos - 1;
			        break;
			    }
                        }

		    } else { 
                
		        FOREVER {
			    if (((wchar_t *)symbol1)[symbol1_len - 1] == 
                                                 ((wchar_t *)buf)[buf_index]) {
			        k = symbol1_len - 2;
			        l = buf_index - 1;
			        while ((k >= 0) && 
                                (((wchar_t *)symbol1)[k] == ((wchar_t *)buf)[l])) {
				    k--;
				    l--;
			        }
			        if (k < 0) {
				    count++;
				    buf_index = l;
			        } else
				    buf_index--;
			    } else 
                                if (((wchar_t *)symbol2)[symbol2_len - 1] ==
                                               ((wchar_t *)buf)[buf_index]) {
			            i = symbol2_len - 2;
			            j = buf_index - 1;
			            while ((i >= 0) && (((wchar_t *)symbol2)[i]
                                                 == ((wchar_t *)buf)[j])) {
				        i--;
			    	        j--;
			            }
			            if (i < 0) {
				        if (--count == 0) {
				            result_pos = new_pos + j + 1;
				            match_done = 1;
				        } else
				            buf_index = j;
			            } else
				       buf_index--;
			        } else
			            buf_index--;

			        if (match_done || (buf_index < 0)) {
			            pos = new_pos - 1;
			            break;
			        }
                        }
                    }
		}
	    }
	}

    } else {

	while (!match_done) {
	    pos = new_pos;
	    es_set_position(esh, pos);
	    new_pos = es_read(esh, buf_size, buf, &read);
	    buf_index = 0;
	    if (READ_AT_EOF(pos, new_pos, read))
		goto Return;

	    if (read > 0) {

                if (!multibyte) {

		    FOREVER {
		        if (((char *)symbol1)[0] == buf[buf_index]) {
			    k = 1;
			    l = buf_index + 1;
			    while ((k < symbol1_len) &&
                                      (((char *)symbol1)[k] == buf[l])) {
			        k++;
			        l++;
			    }
			    if (k == symbol1_len) {
			        count++;
			        buf_index = l;
			    } else
			        buf_index++;
		        } else if (((char *)symbol2)[0] == buf[buf_index]) {
			    i = 1;
			    j = buf_index + 1;
			    while ((i < symbol2_len) && 
                                (((char *)symbol2)[i] == buf[j])) {
			        i++;
			        j++;
			    }
			    if (i == symbol2_len) {
			        if (--count == 0) {
				    result_pos = pos + j;
				    match_done = 1;
			        } else
				    buf_index = j;
			    } else
			        buf_index++;
		        } else
			    buf_index++;
		    
		        if (match_done || (buf_index == read))
		            break;
                    }
          
                } else {

		    FOREVER {
		        if (((wchar_t *)symbol1)[0] == 
                              ((wchar_t *)buf)[buf_index]) {
			    k = 1;
			    l = buf_index + 1;
			    while ((k < symbol1_len) && 
                                (((wchar_t *)symbol1)[k] == ((wchar_t *)buf)[l])) {
			        k++;
			        l++;
			    }
			    if (k == symbol1_len) {
			        count++;
			        buf_index = l;
			    } else {
			        buf_index++;
			    }
		        } else if (((wchar_t *)symbol2)[0] == 
                                   ((wchar_t *)buf)[buf_index]) {
			    i = 1;
			    j = buf_index + 1;
			    while ((i < symbol2_len) && 
                               (((wchar_t*)symbol2)[i] == ((wchar_t *)buf)[j])) {
			        i++;
			        j++;
			    }
			    if (i == symbol2_len) {
			        if (--count == 0) {
				    result_pos = pos + j;
				    match_done = 1;
			        } else
				    buf_index = j;
			    } else
			        buf_index++;
		        } else
			    buf_index++;
		
		        if (match_done || (buf_index == read))
			    break;
		    } 
                }
	    }
	}
    }
Return:
    return (result_pos);
}

Pkg_private     Es_index
ev_find_enclose_end_marker(esh, symbol1, symbol1_len, symbol2, symbol2_len, start_pos)
    Es_handle       esh;
    void           *symbol1, *symbol2;	/* starting and ending symbol */
    int             symbol1_len, symbol2_len;
    Es_index        start_pos;
{
    int             done = FALSE;
    char            buf[BUF_SIZE];
    int             char_read = 0;
    Es_index        result_pos = ES_CANNOT_SET;
    Es_index        new_pos, pos = start_pos;


    while (!done && (pos >= 0) && (pos != ES_CANNOT_SET)) {
	es_set_position(esh, pos);
	new_pos = es_read(esh, getBufSize(), buf, &char_read);

	if (READ_AT_EOF(pos, new_pos, char_read) || (char_read <= 0)) {
	    done = TRUE;
	} else {
	    void           *buf_ptr = buf;
	    int             i;
	    int             keep_looping = TRUE;


            if (!multibyte) {
	        for (i = 0; (keep_looping); char_read--, i++) {
		    if (strncmp(buf_ptr, symbol1, symbol1_len) == 0) {
		        done = TRUE;
		        keep_looping = FALSE;
		        result_pos = pos + i + symbol1_len;
		    } else if (strncmp(buf_ptr, symbol2, symbol2_len) == 0) {
		        keep_looping = FALSE;
		        pos = ev_match_field_in_esh(esh, symbol2, symbol2_len, 
                                                 symbol1, symbol1_len,
						pos + i, EV_FIND_DEFAULT);
		    } else {
		        if (char_read > 0)
			    buf_ptr = (char *)buf_ptr +1;
		        else {
			    pos = new_pos;
			    break;
		        }
		    }
	        } 
            } else {
	        for (i = 0; (keep_looping); char_read--, i++) {
		    if (wsncmp(buf_ptr, symbol1, symbol1_len) == 0) {
		        done = TRUE;
		        keep_looping = FALSE;
		        result_pos = pos + i + symbol1_len;
		    } else if (wsncmp(buf_ptr, symbol2, symbol2_len) == 0) {
		        keep_looping = FALSE;
		        pos = ev_match_field_in_esh(esh, symbol2, symbol2_len, 
                                                symbol1, symbol1_len,
						pos + i, EV_FIND_DEFAULT);
		    } else {
		        if (char_read > 0)
			    buf_ptr = (wchar_t *)buf_ptr +1;
		        else {
			    pos = new_pos;
			    break;
		        }
		    }
	        }
            }
	}   
    }

    return (result_pos);
}

#undef BUF_SIZE
