#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)txt_again.c 20.46 95/05/17";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

/*
 * AGAIN action recorder and interpreter for text subwindows.
 */
#include <xview/pkg.h>
#include <xview/attrol.h>
#include <xview_private/primal.h>
#include <xview_private/txt_impl.h>
#include <xview_private/ev_impl.h>
#ifdef SVR4
#include <stdlib.h>
#include <string.h>
#endif /* SVR4 */

#define     STR_INC_SIZE	1024   /* string increment size */

Pkg_private Es_index textsw_do_input();
Pkg_private Es_index textsw_do_pending_delete();

extern void textsw_SetSelection();

string_t        null_string = {0, 0, 0};

#define	TEXT_DELIMITER	"\\"
char           *text_delimiter = TEXT_DELIMITER;

wchar_t            text_delimiter_wc[2] = {'\\','\0'};

typedef enum {
    CARET_TOKEN,
    DELETE_TOKEN,
    EDIT_TOKEN,
    EXTRAS_TOKEN,
    FIELD_TOKEN,
    FILTER_TOKEN,
    FIND_TOKEN,
    INSERT_TOKEN,
    MENU_TOKEN,
    NULL_TOKEN
}               Token;

char           *cmd_tokens[] = {
    "CARET", "DELETE", "EDIT", "EXTRAS", "FIELD", "FILTER", "FIND", "INSERT", "MENU", NULL
};

typedef enum {
    FORWARD_DIR,
    BACKWARD_DIR,
    NO_DIR,
    EMPTY_DIR
}               Direction;

char           *direction_tokens[] = {
    "FORWARD", "BACKWARD", TEXT_DELIMITER, "", NULL
};
char           *edit_tokens[] = {
    "CHAR", "WORD", "LINE", NULL
};

#define	CHAR_TOKEN	0
#define WORD_TOKEN	1
#define LINE_TOKEN	2

char           *text_tokens[] = {
    "PIECES", "TRASHBIN", TEXT_DELIMITER, NULL
};

#define	PIECES_TOKEN	0
#define TRASH_TOKEN	1
#define DELIMITER_TOKEN	2

#define		    MB_STR_BUFFER_LEN		100
static char	    mb_str[MB_STR_BUFFER_LEN];

static int
txtsw_string_length(tString)
    string_t *tString;
{
    if (multibyte)
        return ((wchar_t *)(tString->free) - (wchar_t *)(tString->base));
    else
        return ((char *)(tString->free) - (char *)(tString->base));
}

Pkg_private int
match_in_table(to_match, table)	/* Modified from ucb/lpr/lpc.c */
    register char  *to_match;
    register char **table;
{
    register char  *p, *q;
    int             found, index, nmatches, longest;

    longest = nmatches = 0;
    found = index = -1;
    for (p = *table; p; p = *(++table)) {
	index++;
	for (q = to_match; *q == *p++; q++)
	    if (*q == 0)	/* exact match? */
		return (index);
	if (!*q) {		/* the to_match was a prefix */
	    if (q - to_match > longest) {
		longest = q - to_match;
		nmatches = 1;
		found = index;
	    } else if (q - to_match == longest)
		nmatches++;
	}
    }
    if (nmatches > 1)
	return (-1);
    return (found);
}

static	int
textsw_string_append(ptr_to_string, buffer, buffer_length)
    string_t       *ptr_to_string;
    void           *buffer;
    int             buffer_length;
/* Returns FALSE iff it needed to malloc and the malloc failed */
{
    if (textsw_string_min_free(ptr_to_string, buffer_length) != TRUE)
	return (FALSE);
    if (multibyte) {
        XV_BCOPY(buffer, ptr_to_string->free, buffer_length*sizeof(wchar_t));
        ptr_to_string->free = (wchar_t *)ptr_to_string->free + buffer_length;
        *(wchar_t *)(ptr_to_string->free) = '\0';
    } else {
        XV_BCOPY(buffer, ptr_to_string->free, buffer_length);
        ptr_to_string->free = (char *)ptr_to_string->free + buffer_length;
        *(char *)(ptr_to_string->free) = '\0';
    }
    return (TRUE);
}

int             textsw_again_debug;	/* = 0 for -A-R */

static int
textsw_string_min_free(ptr_to_string, min_free_desired)
    register string_t *ptr_to_string;
    int             min_free_desired;
/* Returns FALSE iff it needed to malloc and the malloc failed */
{
    int             used = txtsw_string_length(ptr_to_string);    
    register int    desired_max = ((((used + min_free_desired) / STR_INC_SIZE) 
                                             + 1) * STR_INC_SIZE);
    if (!multibyte) {
        if (ptr_to_string->max_length <
	    ((char *)ptr_to_string->free - (char *)ptr_to_string->base)) {
	    while (!textsw_again_debug) {
	    }
        }
    } else {
        if (ptr_to_string->max_length <
	    ((wchar_t *)ptr_to_string->free - (wchar_t *)ptr_to_string->base)) {
	    while (!textsw_again_debug) {
	    }
        }
    }
    if (ptr_to_string->max_length < desired_max) {
     /* 
      * string will be incremented by STR_INC_SIZE if necessary.  
      * This prevent memory fragmentation from small block malloc and free.
      */
	void  *old_string = ptr_to_string->base;

        if (multibyte)
	    if (old_string == NULL)
	        ptr_to_string->base = xv_malloc(
		    ((unsigned)desired_max + 1) * sizeof(wchar_t));
	    else			 
	        ptr_to_string->base = realloc(old_string,
		     (unsigned) (desired_max + 1) * sizeof(wchar_t));
        else
	    if (old_string == NULL)
	        ptr_to_string->base = xv_malloc((unsigned)desired_max + 1);
	    else			 
	        ptr_to_string->base = realloc(old_string,
		     (unsigned) (desired_max + 1));
				 
	if (ptr_to_string->base == NULL) {
	    ptr_to_string->base = old_string;
	    return (FALSE);
	}

	ptr_to_string->max_length = desired_max;

        if (multibyte)
	    if (old_string == NULL) {
	        ptr_to_string->free = ptr_to_string->base;
	        *(wchar_t *)(ptr_to_string->free) = '\0';
	    } else
	        ptr_to_string->free = (wchar_t *)ptr_to_string->base + used;
        else
           if (old_string == NULL) {
	        ptr_to_string->free = ptr_to_string->base;
	        *(char *)(ptr_to_string->free) = '\0';
	    } else
	        ptr_to_string->free = (char *)ptr_to_string->base + used;
    } 
    return (TRUE);
}

/*
 * Recording routines
 */
/*
 * SIDE_EFFECT: ptr_to_string->free is modified by this routine.
 */

static void
#ifdef ANSI_FUNC_PROTO
textsw_printf(register string_t *ptr_to_string, register char  *fmt, ...)
#else
textsw_printf(ptr_to_string, fmt, va_alist)
    register string_t *ptr_to_string;
    register char  *fmt;
va_dcl
#endif
{
    int      result;
    va_list  args;
    int      num_char;

    mb_str[0] = NULL;
    VA_START(args, fmt);
    (void) vsprintf(mb_str, fmt, args);
    result = strlen(mb_str);
    va_end(args);

    if (multibyte) {
        num_char = mbstowcs((wchar_t *)ptr_to_string->free, mb_str,
	    (ptr_to_string->max_length - txtsw_string_length(ptr_to_string)));
        ptr_to_string->free = (wchar_t *)ptr_to_string->free + num_char;
    } else {
        /* the result+1 ensures the null byte is copied */
        (void) strncpy(ptr_to_string->free, mb_str, result+1);
        ptr_to_string->free = (char *)ptr_to_string->free + result;
    }
}

static	void
textsw_record_buf(again, buffer, buffer_length)
    register string_t *again;
    void           *buffer;
    int             buffer_length;
{
    (void) textsw_printf(again, "%s %6d %s\n",
			 text_delimiter, buffer_length, text_delimiter);
    (void) textsw_string_append(again, buffer, buffer_length);
    (void) textsw_printf(again, "\n%s\n", text_delimiter);
}

Pkg_private void
textsw_record_caret_motion(textsw, direction, loc_x)
    Textsw_folio    textsw;
    unsigned        direction;
    int             loc_x;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                    (textsw->state & TXTSW_NO_AGAIN_RECORDING))
        return;

    textsw->again_insert_length = 0;
    if (textsw_string_min_free(again, 15) != TRUE)
	return;			/* Cannot guarantee enough space */

    (void) textsw_printf(again, "%s %x %d\n", cmd_tokens[ord(CARET_TOKEN)],
			 direction, loc_x);
}

Pkg_private void
textsw_record_delete(textsw)
    Textsw_folio    textsw;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                    (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    textsw->again_insert_length = 0;
    if (textsw_string_min_free(again, 10) != TRUE)
	return;			/* Cannot guarantee enough space */
    (void) textsw_printf(again, "%s\n", cmd_tokens[ord(DELETE_TOKEN)]);
}

Pkg_private void
textsw_record_edit(textsw, unit, direction)
    Textsw_folio    textsw;
    unsigned        unit, direction;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                        (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    textsw->again_insert_length = 0;
    if (textsw_string_min_free(again, 25) != TRUE)
	return;			/* Cannot guarantee enough space */
    (void) textsw_printf(again, "%s %s %s\n", cmd_tokens[ord(EDIT_TOKEN)],
			 edit_tokens[(unit == EV_EDIT_CHAR) ? CHAR_TOKEN
				     : (unit == EV_EDIT_WORD) ? WORD_TOKEN
				     : LINE_TOKEN],
			 direction_tokens[(direction == 0)
					  ? ord(FORWARD_DIR)
					  : ord(BACKWARD_DIR)]);
}

Pkg_private void
textsw_record_extras(folio, cmd_line)
    Textsw_folio    folio;
    void           *cmd_line;
{
    register string_t *again = &folio->again[0];
    int             cmd_len;
 
    if (multibyte)
       cmd_len = (cmd_line ? wslen(cmd_line) : 0);
    else
       cmd_len = (cmd_line ? strlen(cmd_line) : 0);

    if ((folio->func_state & TXTSW_FUNC_AGAIN) ||
                        (folio->state & TXTSW_NO_AGAIN_RECORDING))
	return;

    folio->again_insert_length = 0;

    if (textsw_string_min_free(again, cmd_len + 30) != TRUE)
	return;			/* Cannot guarantee enough space */

    (void) textsw_printf(again, "%s ", cmd_tokens[ord(EXTRAS_TOKEN)]);

    textsw_record_buf(again, cmd_line, cmd_len);
}

Pkg_private void
textsw_record_find(textsw, pattern, pattern_length, direction)
    Textsw_folio    textsw;
    void           *pattern;
    int             pattern_length, direction;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                        (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    if (textsw->state & (TXTSW_AGAIN_HAS_FIND | TXTSW_AGAIN_HAS_MATCH)) {
	(void) textsw_checkpoint_again(
				       VIEW_REP_TO_ABS(textsw->first_view));
    } else {
	textsw->again_insert_length = 0;
    }
    if (textsw_string_min_free(again, pattern_length + 30) != TRUE)
	return;			/* Cannot guarantee enough space */
    (void) textsw_printf(again, "%s %s ", cmd_tokens[ord(FIND_TOKEN)],
			 direction_tokens[(direction == 0)
					  ? ord(FORWARD_DIR)
					  : ord(BACKWARD_DIR)]);
    textsw_record_buf(again, pattern, pattern_length);
    textsw->state |= TXTSW_AGAIN_HAS_FIND;
}

Pkg_private void
textsw_record_filter(textsw, event)
    Textsw_folio    textsw;
    Event          *event;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                        (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    textsw->again_insert_length = 0;
    if (textsw_string_min_free(again, 50) != TRUE)
	return;			/* Cannot guarantee enough space */
    (void) textsw_printf(again, "%s %x %x %x ",
			 cmd_tokens[ord(FILTER_TOKEN)],
			 event_action(event),
			 event->ie_flags, event->ie_shiftmask);
    if (!multibyte)
        textsw_record_buf(again, textsw->to_insert,
              (char *)textsw->to_insert_next_free - (char *)textsw->to_insert);
    else
        textsw_record_buf(again, textsw->to_insert,
         (wchar_t *)textsw->to_insert_next_free - (wchar_t *)textsw->to_insert);
}

static int
my_wstoi (num_string)
    wchar_t	*num_string;
{
    int		i = 0;
    int		result = 0;
    
    while (((num_string + i) != NULL) &&
           (*(num_string + i) == L' '))
           i++;
           
    while (((num_string + i) != NULL) &&
           (*(num_string + i) >= L'0')  &&
           (*(num_string + i) <= L'9')) {
           result = (result * 10) + (*(num_string + i) - L'0');
           i++;
    }
    return(result);     
}

Pkg_private void
textsw_record_input(textsw, buffer, buffer_length)
    Textsw_folio    textsw;
    void           *buffer;
    long int        buffer_length;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    if (textsw_string_min_free(again, buffer_length + 25) != TRUE)
	return;			/* Cannot guarantee enough space */
    /* Above guarantees enough space */
    if (textsw->again_insert_length == 0) {
	(void) textsw_printf(again, "%s ", cmd_tokens[ord(INSERT_TOKEN)]);
        if (multibyte)
	    textsw->again_insert_length =      
	        txtsw_string_length(again) + wslen(text_delimiter_wc) + 1;
        else
	    textsw->again_insert_length =      
	       txtsw_string_length(again) + strlen(text_delimiter) + 1;
	textsw_record_buf(again, buffer, buffer_length);
    } else {
	/*
	 * Following is a disgusting efficiency hack to compress a sequence
	 * of INSERTs.
	 */
	void           *insert_length; 
        wchar_t          new_length_buf[7];
	int             i, old_length;

        if (multibyte) {
	    insert_length = (wchar_t *)again->base +
	                         textsw->again_insert_length;
	    old_length = my_wstoi(insert_length);
        } else {
	    insert_length = (char *)again->base +
	                           textsw->again_insert_length;
	    old_length = atoi(insert_length);
        }
	ASSUME(old_length > 0);
        if (multibyte) {
	    (void)wsprintf(new_length_buf, "%6d",old_length + buffer_length);
	    for (i = 0; i < 6; i++) {
	        ((wchar_t *)insert_length)[i] = new_length_buf[i];
            }
	    again->free = (wchar_t *)(again->free) - 
                                      (wslen(text_delimiter_wc) + 2);
        } else {
	    (void)sprintf((char *)new_length_buf, 
                                  "%6d",old_length + buffer_length);
	    for (i = 0; i < 6; i++) {
	        ((char *)insert_length)[i] = ((char *)new_length_buf)[i];
            }
	    again->free = (char *)(again->free) - (strlen(text_delimiter) + 2);
	}
	(void) textsw_string_append(again, buffer, buffer_length);
	(void) textsw_printf(again, "\n%s\n", text_delimiter);
    }
}

Pkg_private void
textsw_record_match(textsw, flag, start_marker)
    Textsw_folio    textsw;
    unsigned        flag;
    void           *start_marker;
{
    register string_t *again = &textsw->again[0];
    char	   *start_marker_mb;

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    if (textsw->state & TXTSW_AGAIN_HAS_MATCH) {
	(void) textsw_checkpoint_again(
				       VIEW_REP_TO_ABS(textsw->first_view));
    } else {
	textsw->again_insert_length = 0;
    }
    if (textsw_string_min_free(again, 15) != TRUE)
	return;			/* Cannot guarantee enough space */

    if (multibyte) {
         start_marker_mb = _xv_wcstombsdup(start_marker);
        (void) textsw_printf(again, "%s %x %s\n", 
                  cmd_tokens[ord(FIELD_TOKEN)], flag, start_marker_mb);
        free(start_marker_mb);
    } else
        (void) textsw_printf(again, "%s %x %s\n", 
                   cmd_tokens[ord(FIELD_TOKEN)], flag, start_marker);
    textsw->state |= TXTSW_AGAIN_HAS_MATCH;
}


Pkg_private void
textsw_record_piece_insert(textsw, pieces)
    Textsw_folio    textsw;
    Es_handle       pieces;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                        (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    textsw->again_insert_length = 0;
    if (textsw_string_min_free(again, 25) != TRUE)
	return;			/* Cannot guarantee enough space */
    (void) textsw_printf(again, "%s %s %d\n",
			 cmd_tokens[ord(INSERT_TOKEN)],
			 text_tokens[PIECES_TOKEN], pieces);
}

Pkg_private void
textsw_record_trash_insert(textsw)
    Textsw_folio    textsw;
{
    register string_t *again = &textsw->again[0];

    if ((textsw->func_state & TXTSW_FUNC_AGAIN) ||
                (textsw->state & TXTSW_NO_AGAIN_RECORDING))
	return;
    textsw->again_insert_length = 0;
    if (textsw_string_min_free(again, 20) != TRUE)
	return;			/* Cannot guarantee enough space */
    (void) textsw_printf(again, "%s %s\n",
			 cmd_tokens[ord(INSERT_TOKEN)],
			 text_tokens[TRASH_TOKEN]);
}

/*
 * Replaying routines
 */

/*
 * Following is stolen from sscanf(str, fmt, args) SIDE_EFFECT:
 * ptr_to_string->base is modified by this routine.
 */
/* VARARGS2 */

static int
#ifdef ANSI_FUNC_PROTO
textsw_scanf(register string_t *ptr_to_string, register char  *fmt, ...)
#else
textsw_scanf(ptr_to_string, fmt, va_alist)
    register string_t *ptr_to_string;
    register char  *fmt;
va_dcl
#endif
{
    FILE            _strbuf;
    int             result;
    va_list         args;

    if (multibyte) {
    /*
     *  OW_I18N:  The string within ptr_to_string is wchar_t,
     *  but the string in va_alist should be multibytes
     */

        int      num_char;
        wchar_t  dummy[MB_STR_BUFFER_LEN];

        mb_str[0] = NULL;
        (void) wcstombs(mb_str, ptr_to_string->base, MB_STR_BUFFER_LEN);

#ifdef sun
        _strbuf._base = (unsigned char *) mb_str;
#else
        _strbuf._base = (char *) mb_str;
#endif
#ifdef SVR4
        _strbuf._flag = _IOREAD | _IOWRT;
        _strbuf._base = (unsigned char *) mb_str;
#else /* SVR4 */
        _strbuf._flag = _IOREAD | _IOSTRG;
#endif /* SVR4 */
        _strbuf._ptr = _strbuf._base;
#ifndef SVR4
        _strbuf._bufsiz = _strbuf._cnt = strlen(mb_str);
#else /* SVR4 */
        _strbuf._cnt = strlen(mb_str);
        _strbuf._file = _NFILE;
#endif /* SVR4 */
        VA_START(args, fmt);
        result = _doscan(&_strbuf, fmt, args);
        va_end(args);
        mb_str[(_strbuf._ptr - _strbuf._base)] = NULL;
        num_char = mbstowcs(dummy, mb_str, MB_STR_BUFFER_LEN);
        ptr_to_string->base = (wchar_t *)ptr_to_string->base + num_char;
    } else {
#ifdef SVR4
        _strbuf._flag = _IOREAD | _IOWRT;
        _strbuf._base = (unsigned char *)ptr_to_string->base;
        _strbuf._cnt = strlen((char *)ptr_to_string);
        _strbuf._file = _NFILE;
#else /* SVR4 */
        _strbuf._flag = _IOREAD | _IOSTRG; 
#ifdef sun
        _strbuf._base = (unsigned char *) ptr_to_string->base;
#else /* sun */
        _strbuf._base = (char *) ptr_to_string->base;
#endif /* sun */
        _strbuf._bufsiz = _strbuf._cnt = txtsw_string_length(ptr_to_string);
#endif /* SVR4 */
        _strbuf._ptr = _strbuf._base;
        VA_START(args, fmt);
        result = _doscan(&_strbuf, fmt, args);
        va_end(args);
        ptr_to_string->base = (char *) _strbuf._ptr;
    }
    return (result);
}

static int
textsw_next_is_delimiter(again)
    string_t       *again;
{
    char            token[2];
    int             count;

    count = textsw_scanf(again, "%1s", token);
    if AN_ERROR
	(count != 1 || token[0] != text_delimiter[0]) {
	return (FALSE);
    } else
	return (TRUE);
}

static          Es_handle
textsw_pieces_for_replay(again)
    register string_t *again;
{
#define CHECK_ERROR(test)	if AN_ERROR(test) goto Again_Error;
    int             count;
    Es_handle       pieces = (Es_handle) NULL;
    int  test;

    count = textsw_scanf(again, "%d", &pieces);
    CHECK_ERROR(count != 1 || pieces == (Es_handle) NULL);
    if (multibyte) {
        test = *((wchar_t *)again->base) != (wchar_t)'\n';
        again->base = (wchar_t *)again->base + 1;
    } else {
        test = *((char *)again->base) != '\n';
        again->base = (char *)again->base + 1;
    }
    CHECK_ERROR(test);
Again_Error:
    return (pieces);
#undef CHECK_ERROR
}

static int
textsw_text_for_replay(again, ptr_to_buffer)
    register string_t *again;
    register void **ptr_to_buffer;
{
#define CHECK_ERROR(test)	if AN_ERROR(test) goto Again_Error;
    int             count, length = -1;
    int  test;

    count = textsw_scanf(again, "%d", &length);
    CHECK_ERROR(count != 1 || length < 0);
    CHECK_ERROR(textsw_next_is_delimiter(again) == 0);
    if (multibyte) {
        test = *((wchar_t *)again->base) != (wchar_t)'\n';
        again->base = (wchar_t *)again->base + 1;
        CHECK_ERROR(test);
        if (length) {
	    *ptr_to_buffer = xv_malloc(((unsigned) length + 1) *
		(sizeof(**((wchar_t **)ptr_to_buffer)))*sizeof(wchar_t));
	    (void) wsncpy(*ptr_to_buffer,
		       again->base, length);
	    again->base = (wchar_t *)again->base + length;
        } else {
	    *(wchar_t *)ptr_to_buffer = NULL;
        }
        test = *((wchar_t *)again->base) != (wchar_t)'\n';
        again->base = (wchar_t *)again->base + 1;
        CHECK_ERROR(test);
        test = *((wchar_t *)again->base) != text_delimiter_wc[0];
        again->base = (wchar_t *)again->base + 1;
        CHECK_ERROR(test);
        test = *((wchar_t *)again->base) != (wchar_t)'\n';
        again->base = (wchar_t *)again->base + 1;
        CHECK_ERROR(test);
    } else {
        test = *((char *)again->base) != '\n';
        again->base = (char *)again->base + 1;
        CHECK_ERROR(test);
        if (length) {
	    *ptr_to_buffer = xv_malloc(((unsigned) length + 1) *
		(sizeof(**((char **)ptr_to_buffer))));
	    (void) strncpy(*ptr_to_buffer,
		       again->base, length);
	    again->base = (char *)again->base + length;
        } else {
	    *(char *)ptr_to_buffer = NULL;
        }
        test = *((char *)again->base) != '\n';
        again->base = (char *)again->base + 1;
        CHECK_ERROR(test);
        test = *((char *)again->base) != text_delimiter[0];
        again->base = (char *)again->base + 1;
        CHECK_ERROR(test);
        test = *((char *)again->base) != '\n';
        again->base = (char *)again->base + 1;
        CHECK_ERROR(test);
    }
Again_Error:
    return (length);
#undef CHECK_ERROR
}

static caddr_t
token_index(string, token)
    void           *string;
    register void  *token;
{

    if (string == NULL || token == NULL)
	goto NoMatch;

    if (multibyte) {
        register wchar_t *result = (wchar_t *)string, *r, *t;
        register wchar_t *ttoken = (wchar_t *)token;
        for (; *result; result++) {
	    if (*result == *ttoken) {
	        r = result + 1;
	        for (t = ttoken + 1; *t; r++, t++) { 
		    if (*r != *t) {
		        if (*r == 0)
			    goto NoMatch;
		        else
			    break;
		    }
	        }
	        if (*t == 0)
		    return ((caddr_t)result);
	    }
        }
    } else {
        register char *result = (char *)string, *r, *t;
        register char *ttoken = (char *)token;

        for (; *result; result++) {
	    if (*result == *ttoken) {
	        r = result + 1;
	        for (t = ttoken + 1; *t; r++, t++) {
		    if (*r != *t) {
		        if (*r == 0)
			    goto NoMatch;
		        else
			    break;
		    }
	        }
	        if (*t == 0)
		    return ((caddr_t)result);
	    }
        }
    }
NoMatch:
    return (NULL);
}

/* ARGSUSED */
Pkg_private void
textsw_free_again(textsw, again)
    Textsw_folio    textsw;	/* Currently unused */
    register string_t *again;
{
    void           *saved_base = again->base;
    Es_handle       pieces;
    wchar_t         text_tokens_wc[20];

    if (TXTSW_STRING_IS_NULL(again))
	return;
    ASSERT(allock());
    if (multibyte) {
        (void) mbstowcs(text_tokens_wc, text_tokens[PIECES_TOKEN], 20);
        while ((wchar_t *)(again->base = (wchar_t *)token_index(
		       again->base, text_tokens_wc)) != NULL) {
	    again->base = (wchar_t *)again->base + wslen(text_tokens_wc);
	    if (pieces = textsw_pieces_for_replay(again))
	        es_destroy(pieces);
        }
    } else {
        while ((again->base = (char *)token_index(
		       again->base, text_tokens[PIECES_TOKEN]))
	   != NULL) {
	    again->base = (char *)again->base + 
                                  strlen(text_tokens[PIECES_TOKEN]);
	    if (pieces = textsw_pieces_for_replay(again))
	        es_destroy(pieces);
        }
    }
    free(saved_base);
    ASSERT(allock());
    *again = null_string;
}

Pkg_private int
textsw_get_recorded_x(view)
    register Textsw_view_handle view;
{
#define CHECK_ERROR(test)	if AN_ERROR(test) goto Again_Error;
    register Textsw_folio folio = FOLIO_FOR_VIEW(view);
    register string_t *again;
    void           *buffer = NULL, *saved_base;
    char            token_buf[20];
    register char  *token = token_buf;
    register int    count;
    Textsw_Caret_Direction direction;
    int             loc_x, result = -1;
    int             found_it_already = FALSE;

    if (!TXTSW_DO_AGAIN(folio))
	return (result);
    again = &folio->again[0];
    if (TXTSW_STRING_IS_NULL(again)) {
	return (result);
    }
    saved_base = again->base;
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
    FOREVER {
	count = textsw_scanf(again, "%19s", token);
	if (count == -1)
	    break;
	ASSERT(count == 1);
	count = match_in_table(token, cmd_tokens);
	CHECK_ERROR(count < 0);
	if (buffer != NULL) {
	    free(buffer);
	    buffer = NULL;
	}
	if ((Token) count == CARET_TOKEN) {
	    count = textsw_scanf(again, "%x %d", &direction, &loc_x);
	    CHECK_ERROR(count != 2);

	    if ((direction == TXTSW_NEXT_LINE) ||
		(direction == TXTSW_PREVIOUS_LINE)) {
		if (!found_it_already) {
		    result = loc_x;
		    found_it_already = TRUE;
		}
	    } else {
		if (found_it_already) {
		    /*
		     * Any other direction following TXTSW_NEXT_LINE or
		     * TXTSW_PREVIOUS_LINE will clear the loc_x value
		     */
		    result = -1;
		    found_it_already = FALSE;
		}
	    }
	} else if (found_it_already) {
	    /*
	     * Any other event following TXTSW_NEXT_LINE or
	     * TXTSW_PREVIOUS_LINE will clear the loc_x value
	     */
	    result = -1;
	    found_it_already = FALSE;
	}
    }
Again_Error:
    if (buffer != NULL)
	free(buffer);
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
    ev_update_chain_display(folio->views);
    again->base = saved_base;
    ASSERT(allock());
#undef CHECK_ERROR
    return (result);
}

Pkg_private	void
textsw_do_again(view, x, y)
    register Textsw_view_handle view;
    int             x, y;
{
#define CHECK_ERROR(test)	if AN_ERROR(test) goto Again_Error;
    Pkg_private int ev_get_selection();
    Pkg_private void textsw_move_caret();
    register Textsw_folio textsw = FOLIO_FOR_VIEW(view);
    register string_t *again;
    void           *buffer = NULL, *saved_base;
    char            token_buf[20];
    register char  *token = token_buf;
    register int    buffer_length, count, test;
    Es_index        first, last_plus_one;

    if (!TXTSW_DO_AGAIN(textsw))
	return;
    again = &textsw->again[0];
    if (TXTSW_STRING_IS_NULL(again)) {
	again = &textsw->again[1];
	if (TXTSW_STRING_IS_NULL(again))
	    return;
    }
    saved_base = again->base;
    (void) ev_get_selection(textsw->views, &first, &last_plus_one,
			    EV_SEL_PRIMARY);
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, TRUE, 0);
    FOREVER {
	count = textsw_scanf(again, "%19s", token);
	if (count == -1)
	    break;
	ASSERT(count == 1);
	count = match_in_table(token, cmd_tokens);
	CHECK_ERROR(count < 0);
	if (buffer != NULL) {
	    free(buffer);
	    buffer = NULL;
	}
	switch ((Token) count) {
	  case CARET_TOKEN:{
		unsigned        direction;
		int             unused;	/* This is an x location */

		count = textsw_scanf(again, "%x %d", &direction, &unused);
		CHECK_ERROR(count != 2);
		(void) textsw_move_caret(view, 
                          (Textsw_Caret_Direction) direction);
		break;
	    }
	  case DELETE_TOKEN:
            if (multibyte) {
                test = *((wchar_t *)again->base) != (wchar_t)'\n';
                again->base = (wchar_t *)again->base + 1;
             } else {
                test = *((char *)again->base) != '\n';
                again->base = (char *)again->base + 1;
             }
            CHECK_ERROR(test);
	    /*
	     * Do the delete and update the selection.
	     */
	    (void) textsw_delete_span(view, first, last_plus_one,
				      TXTSW_DS_ADJUST | TXTSW_DS_SHELVE);
	    first = last_plus_one;
	    break;
	  case EDIT_TOKEN:{
		unsigned        unit, direction;
		/*
		 * Determine the type of edit.
		 */
		count = textsw_scanf(again, "%19s", token);
		CHECK_ERROR(count != 1);
		count = match_in_table(token, edit_tokens);
		switch (count) {
		  case CHAR_TOKEN:
		    unit = EV_EDIT_CHAR;
		    break;
		  case WORD_TOKEN:
		    unit = EV_EDIT_WORD;
		    break;
		  case LINE_TOKEN:
		    unit = EV_EDIT_LINE;
		    break;
		  default:	/* includes case -1 */
		    LINT_IGNORE(CHECK_ERROR(TRUE));
		}
		/*
		 * Determine the direction of edit.
		 */
		count = textsw_scanf(again, "%19s", token);
		CHECK_ERROR(count != 1);
		count = match_in_table(token, direction_tokens);
		switch ((Direction) count) {
		  case FORWARD_DIR:
		    direction = 0;
		    break;
		  case BACKWARD_DIR:
		    direction = EV_EDIT_BACK;
		    break;
		  default:	/* includes case -1 */
		    LINT_IGNORE(CHECK_ERROR(TRUE));
		}
                if (multibyte) {
                    test = *((wchar_t *)again->base) != (wchar_t)'\n';
                    again->base = (wchar_t *)again->base + 1;
                 } else {
                    test = *((char *)again->base) != '\n';
                    again->base = (char *)again->base + 1;
                 }
                CHECK_ERROR(test);
		/*
		 * Process the selection.
		 */
		if (first < last_plus_one) {
		    (void) textsw_SetSelection(
						VIEW_REP_TO_ABS(view),
				  ES_INFINITY, ES_INFINITY, EV_SEL_PRIMARY);
		    first = last_plus_one = ES_INFINITY;
		}
		/*
		 * Finally, do the actual edit.
		 */
		(void) textsw_do_edit(view, unit, direction, 0);
		break;
	    }

	  case EXTRAS_TOKEN:{
		Pkg_private char **textsw_string_to_argv();
		char          **filter_argv;

		CHECK_ERROR(textsw_next_is_delimiter(again) == 0);
		buffer_length = textsw_text_for_replay(again, &buffer);
		CHECK_ERROR(buffer_length <= 0);

                if (multibyte)
		    ((wchar_t *)buffer)[buffer_length] = NULL;
                else
		    ((char *)buffer)[buffer_length] = NULL;
		filter_argv = textsw_string_to_argv(buffer);
		(void) textsw_call_filter(view, filter_argv);

		break;
	    }

	  case FIELD_TOKEN:{
		unsigned        field_flag;
		int             matched;
		char            start_marker_mb[8];                    
                wchar_t         start_marker[4];

		count = textsw_scanf(again, "%x", &field_flag);
		CHECK_ERROR(count != 1);

                if (multibyte) {
		    count = textsw_scanf(again, "%3s", start_marker_mb);
		    (void) mbstowcs(start_marker, start_marker_mb, 3);
                } else 
		    count = textsw_scanf(again, "%3s", (char *)start_marker);

		CHECK_ERROR(count != 1);

		textsw_flush_caches(view, TFC_STD);

		if ((field_flag == TEXTSW_FIELD_ENCLOSE) ||
		    (field_flag == TEXTSW_DELIMITER_ENCLOSE)) {
		    int             first, last_plus_one;

		    first = last_plus_one = EV_GET_INSERT(textsw->views);
                    if (multibyte) 
		        matched = textsw_match_field_and_normalize(view, 
                                    &first, &last_plus_one, start_marker,
                                    wslen (start_marker), field_flag, TRUE);
                    else
		        matched = textsw_match_field_and_normalize(view, 
                                    &first, &last_plus_one, start_marker,
                               strlen((char *)start_marker), field_flag, TRUE);
		} else {
		    matched = textsw_match_selection_and_normalize(view,
                                                   start_marker, field_flag);
		}
		CHECK_ERROR(!matched);
		(void) ev_get_selection(textsw->views, &first, &last_plus_one,
					EV_SEL_PRIMARY);
		break;
	    }
	  case FILTER_TOKEN:{
		Event           event;
		int             dummy;

		count = textsw_scanf(again, "%x", &dummy);
		CHECK_ERROR(count != 1);
		event_set_action(&event, dummy);
		count = textsw_scanf(again, "%x", &dummy);
		CHECK_ERROR(count != 1);
		event.ie_flags = dummy & (~IE_NEGEVENT);
		count = textsw_scanf(again, "%x", &dummy);
		CHECK_ERROR(count != 1);
		event.ie_shiftmask = dummy;
		event.ie_locx = x;
		event.ie_locy = y;
		event.ie_time = textsw->last_point;
		/* Something reasonable */
		CHECK_ERROR(textsw_next_is_delimiter(again) == 0);
		/*
		 * Enter the filtering state
		 */
		(void) textsw_do_filter(view, &event);
		buffer_length = textsw_text_for_replay(again, &buffer);
		if (buffer_length > 0) {
                    if (multibyte) {
		        XV_BCOPY(buffer, textsw->to_insert, 
                                  buffer_length * sizeof(wchar_t));
		        textsw->to_insert_next_free =
			    (wchar_t *)textsw->to_insert + buffer_length;
                    } else {
		        XV_BCOPY(buffer, textsw->to_insert, 
                                  buffer_length);
		        textsw->to_insert_next_free =
			    (char *)textsw->to_insert + buffer_length;
                    }
		}
		/*
		 * Actually invoke the filter
		 */
		event.ie_flags |= IE_NEGEVENT;
		(void) textsw_do_filter(view, &event);
		/*
		 * Find out what the selection looks like now
		 */
		(void) ev_get_selection(textsw->views, &first,
					&last_plus_one, EV_SEL_PRIMARY);
		break;
	    }
	  case FIND_TOKEN:{
		unsigned        flags = EV_FIND_DEFAULT;
		Es_index        insert = EV_GET_INSERT(textsw->views);

		/*
		 * Determine the direction of the search.
		 */
		count = textsw_scanf(again, "%19s", token);
		CHECK_ERROR(count != 1);
		count = match_in_table(token, direction_tokens);
		CHECK_ERROR(count < 0);
		switch ((Direction) count) {
		  case BACKWARD_DIR:
		    CHECK_ERROR(textsw_next_is_delimiter(again) == 0);
		    flags = EV_FIND_BACKWARD;
		    if (first >= last_plus_one) {
			first = insert;
		    }
		    break;
		  case FORWARD_DIR:
		  case EMPTY_DIR:
		    CHECK_ERROR(textsw_next_is_delimiter(again) == 0);
		    if (first >= last_plus_one)
			first = insert;
		    else
			first = last_plus_one;

		    break;
		  default:
		    break;
		}
		/*
		 * Get the pattern for the search, then do it.
		 */
		buffer_length = textsw_text_for_replay(again, &buffer);
		CHECK_ERROR(buffer_length <= 0);
		textsw_find_pattern_and_normalize(
					 view, x, y, &first, &last_plus_one,
				      buffer, (u_int) buffer_length, flags);
		CHECK_ERROR(first == ES_CANNOT_SET);
		/*
		 * Find out what the selection looks like now
		 */
		(void) ev_get_selection(textsw->views, &first,
					&last_plus_one, EV_SEL_PRIMARY);
		break;
	    }
	  case INSERT_TOKEN:{
		(void) textsw_do_pending_delete(view, EV_SEL_PRIMARY,
						TFC_INSERT | TFC_SEL);
		first = last_plus_one;
		/*
		 * Determine the type of text to be inserted.
		 */
		count = textsw_scanf(again, "%19s", token);
		CHECK_ERROR(count != 1);
		count = match_in_table(token, text_tokens);
		switch (count) {
		  case DELIMITER_TOKEN:
		    buffer_length = textsw_text_for_replay(again, &buffer);
		    CHECK_ERROR(buffer_length <= 0);
		    (void) textsw_do_input(view, buffer, (long) buffer_length,
					   TXTSW_UPDATE_SCROLLBAR_IF_NEEDED);
		    break;
		  case PIECES_TOKEN:{
			Es_handle       pieces;
			Es_index        pos = EV_GET_INSERT(textsw->views);
			pieces = textsw_pieces_for_replay(again);
			(void) textsw_insert_pieces(view, pos, pieces);
			break;
		    }
		  case TRASH_TOKEN:{
			Es_index        pos = EV_GET_INSERT(textsw->views);
			(void) textsw_insert_pieces(view, pos, textsw->trash);
			break;
		    }
		  default:
		    LINT_IGNORE(CHECK_ERROR(TRUE));	/* includes case -1 */
		}
		break;
	    }
	  default:
	    LINT_IGNORE(CHECK_ERROR(TRUE));
	}
    }
Again_Error:
    if (buffer != NULL)
	free(buffer);
    ev_set(view->e_view, EV_CHAIN_DELAY_UPDATE, FALSE, 0);
    ev_update_chain_display(textsw->views);
    again->base = saved_base;
    ASSERT(allock());
}

#undef	STR_INC_SIZE    
#undef CHECK_ERROR
