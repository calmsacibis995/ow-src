/*
 *
 * dstt_paste.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_paste.c 1.5 93/01/19 Copyr 1990 Sun Micro";
#endif

#include <desktop/tt_c.h>
#include "dstt_vtype.h"
#include "dstt.h"
#include "ds_verbose_malloc.h"

#ifdef  DEBUG
#define DP      if(1) 
#else
#define DP      if(0)
#endif

#define	REQ_ARG	3

static Tt_callback_action
dstt_paste_results(Tt_message m, Tt_pattern p)
{
	int			err = 0;
	Paste_CB		*cb;
	void			*key = 0;
	char			*file;
	char			*media = 0;
	void			*data = 0;
	int			size;
	bufftype_t		bufftype;
	char			*ID;
	int			offset;
	char			*locator;
	char			*xsel;
	int			i = 0;
	int			mark = tt_mark();

	if(cb = (Paste_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);

		bufftype = dstt_get_bufftype(m, 0);
		ID = tt_message_arg_val(m, 0);

		media = tt_message_arg_type(m, 1);
		tt_message_arg_ival(m, 2, &offset);
		locator = dstt_get_sopt(m, REQ_ARG, VTYPE_LOCATOR);
#ifndef	PRIOR_493
		if((xsel = (char *)tt_message_context_val(m,
				VTYPE_X_SELECTION)) && 
				*xsel)
		{
			(*cb)(m, key, bufftype, ID, media, x_selection,
					xsel, strlen(xsel),
					offset, locator);
		}
		else
#endif	/* NOTE! if NOT PRIOR_493 this is all one big if! */
		if(file = tt_message_file(m))
		{
			if(strchr(file, ':'))
			{
				dstt_set_status(m, dstt_status_file_not_avail);
			}
			else
			{
				size = strlen(file);
				(*cb)(m, key, bufftype, ID, media, path, file, size,
					offset, locator);
			}
		}
		else
		{
			tt_message_arg_bval(m, 1,
					(unsigned char **)&data, &size);
			(*cb)(m, key, bufftype, ID, media, contents, data, size,
					offset, locator);
		}
	}
	tt_release(mark);
	tt_message_destroy(m);
	return(TT_CALLBACK_PROCESSED);
}

static Tt_callback_action
dstt_paste_handler(Tt_message m, Tt_pattern p)
{
	status_t		results = REJECT;
	Paste_Handle		*cb;
	void			*key = 0;
	char			*file;
	char			*media = 0;
	void			*data = 0;
	int			size;
	bufftype_t		bufftype;
	char			*ID;
	int			offset;
	char			*locator;
	char			*xsel;
	int			mark = tt_mark();

	if(cb = (Paste_Handle *)tt_pattern_user(p, 0))
	{
		key = tt_pattern_user(p, 1);

		ID = tt_message_arg_val(m, 0);
		bufftype = dstt_get_bufftype(m, 0);
		media = tt_message_arg_type(m, 1);
		tt_message_arg_ival(m, 2, &offset);
		locator = dstt_get_sopt(m, REQ_ARG, VTYPE_LOCATOR);

#ifndef	PRIOR_493
		if((xsel = (char *)tt_message_context_val(m,
				VTYPE_X_SELECTION)) && 
				*xsel)
		{
			results = (*cb)(m, key, bufftype, ID, media,
					x_selection, xsel, strlen(xsel),
					offset, locator);
	
		}
		else
#endif	/* NOTE! if NOT PRIOR_493 this is all one big if! */
		if(file = tt_message_file(m))
		{
			if(strchr(file, ':'))
			{
				dstt_set_status(m, dstt_status_file_not_avail);
				results = FAIL;
			}
			else
			{
				size = strlen(file);
				results = (*cb)(m, key, bufftype, ID, media,
						path, file, size,
						offset, locator);
			}
		}
		else
		{
			tt_message_arg_bval(m, 1,
					(unsigned char **)&data, &size);
			results = (*cb)(m, key, bufftype, ID, media,
					contents, data, size,
					offset, locator);
		}
	}
	switch(results)
	{
	case	OK:
		tt_message_reply(m);
		tt_release(mark);
		break;
	case	REJECT:
		tt_message_reject(m);
		tt_release(mark);
		tt_message_destroy(m);
		break;
	case	FAIL:
		tt_message_fail(m);
		tt_message_destroy(m);
		tt_release(mark);
		break;
	case	HOLD:
		tt_release(mark);
		break;
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_paste(
	Paste_CB	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	int		offset,
	char		*locator)
{
	Tt_message	m = 0;
	Tt_mode		mode;
		
	dstt_start_tt();
	m = tt_message_create();
	tt_message_address_set(m, TT_PROCEDURE);
	tt_message_class_set(m, TT_REQUEST);
	tt_message_scope_set(m, TT_SESSION);
	tt_message_session_set(m,  tt_default_session());
	tt_message_disposition_set(m, TT_DISCARD);

	tt_message_op_set(m, PASTE);
	mode = TT_IN;
	switch(bufftype)
	{
	case	buffer:
		tt_message_arg_add(m, TT_IN, VTYPE_BUFFERID, ID);
		break;
	case	view:
		tt_message_arg_add(m, TT_IN, VTYPE_VIEWID, ID);
		break;
	default:
		tt_message_destroy(m);
		return(FALSE);
	}

	switch(type)
	{
	case    contents:
		tt_message_barg_add(m, mode, media, data, size);
		break;
	case    path:
		tt_message_barg_add(m, mode, media, NULL,  NULL);
		tt_message_file_set(m, (char *)data);
#ifndef PRIOR_493
		break;
	case    x_selection:
		tt_message_barg_add(m, mode,
				media, NULL, NULL);
		tt_message_context_set(m,
				VTYPE_X_SELECTION, (char *)data);
#endif
        }
	tt_message_iarg_add(m, TT_IN, VTYPE_VECTOR, offset);
	if(locator) tt_message_arg_add(m, TT_IN,
			VTYPE_LOCATOR, locator);

	tt_message_callback_add(m, dstt_paste_results);
	tt_message_user_set(m, 0, (void *)cb);
	tt_message_user_set(m, 1, key);

	tt_message_send(m);
	dstt_restore_tt();
	return(TRUE);
}

void
dstt_handle_paste(
	Paste_Handle	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	int		offset)
{
	Tt_pattern	p;

	if(cb)
	{
		DP printf("handle PASTE\n");
		p = tt_pattern_create();
	
		tt_pattern_scope_add(p, TT_SESSION);
		tt_pattern_session_add(p, tt_default_session());
		tt_pattern_category_set(p, TT_HANDLE);
		tt_pattern_op_add(p, PASTE);
	
		switch(bufftype)
		{
		case	buffer:
			if(ID)tt_pattern_arg_add(p, TT_IN, VTYPE_BUFFERID, ID);
			break;
		case	view:
			if(ID)tt_pattern_arg_add(p, TT_IN, VTYPE_VIEWID, ID);
		}

		if(media)tt_pattern_arg_add(p, TT_IN, media, NULL);
	
		tt_pattern_callback_add(p, dstt_paste_handler);
		tt_pattern_user_set(p, 0, (void *)cb);
		tt_pattern_user_set(p, 1, key);
	
		tt_pattern_register(p);

		dstt_pat_add(PASTE, NULL, dstt_paste_handler, p);
	}
	else
	{
		DP printf("UNhandle PASTE\n");
		dstt_pat_destroy(PASTE, NULL);
	}
}
