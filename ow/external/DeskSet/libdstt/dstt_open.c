/*
 *
 * dstt_open.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_open.c 1.5 93/01/19 Copyr 1990 Sun Micro";
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
bufftype_t	dstt_get_bufftype(Tt_message m, int n);

static Tt_callback_action
dstt_open_cb(Tt_message m, Tt_pattern p)
{
	int			err = 0;
	Open_CB			*cb;
	void			*key = 0;
	char			*file;
	char			*media = 0;
	void			*data = 0;
	int			size;
	bufftype_t		bufftype;
	char			*ID;
	dstt_bool_t		readonly;
	int			shareLevel;
	dstt_bool_t		mapped;
	char			*locator;
	char			*xsel;
	int			i = 0;
	int			mark = tt_mark();

	DP printf("%d - called dstt_open_cb\n", getpid());
	if(cb = (Open_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);

		media = tt_message_arg_type(m, 0);
		bufftype = dstt_get_bufftype(m, 1);
		ID = tt_message_arg_val(m, 1);
		tt_message_arg_ival(m, 2, (int *)&readonly);
		locator = dstt_get_sopt(m, REQ_ARG, VTYPE_LOCATOR);
		shareLevel = dstt_get_iopt(m, REQ_ARG, VTYPE_SHARELEVEL);
		mapped = dstt_get_iopt(m, REQ_ARG, VTYPE_MAPPED);
#ifndef	PRIOR_493
		if((xsel = (char *)tt_message_context_val(m,
				VTYPE_X_SELECTION)) && 
				*xsel)
		{
			(*cb)(m, key, media, x_selection, xsel, strlen(xsel),
					bufftype, ID, readonly,
					mapped, shareLevel, locator);
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
				(*cb)(m, key, media, path, file, size,
					bufftype, ID, readonly,
					mapped, shareLevel, locator);
			}
		}
		else
		{
			tt_message_arg_bval(m, 0,
					(unsigned char **)&data, &size);
			(*cb)(m, key, media, contents, data, size,
					bufftype, ID, readonly,
					mapped, shareLevel, locator);
		}
	}
	tt_release(mark);
	tt_message_destroy(m);
	return(TT_CALLBACK_PROCESSED);
}

static Tt_callback_action
dstt_open_handler(Tt_message m, Tt_pattern p)
{
	status_t		results = REJECT;
	Open_Handle		*cb;
	void			*key = 0;
	char			*file;
	char			*media = 0;
	void			*data = 0;
	int			size;
	bufftype_t		bufftype;
	char			*ID;
	dstt_bool_t		readonly;
	int			shareLevel;
	dstt_bool_t		mapped;
	char			*locator;
	char			*xsel;
	int			mark = tt_mark();

	DP dstt_prnt_msg(m, "dstt_open_handler");
	if(cb = (Open_Handle *)tt_pattern_user(p, 0))
	{
		key = tt_pattern_user(p, 1);

		media = tt_message_arg_type(m, 0);
		bufftype = dstt_get_bufftype(m, 1);
		ID = tt_message_arg_val(m, 1);
		readonly = tt_message_arg_ival(m, 1, (int *)&readonly);
		mapped = dstt_get_iopt(m, REQ_ARG, VTYPE_MAPPED);
		shareLevel = dstt_get_iopt(m, REQ_ARG, VTYPE_SHARELEVEL);
		locator = dstt_get_sopt(m, REQ_ARG, VTYPE_LOCATOR);

#ifndef	PRIOR_493
		if((xsel = (char *)tt_message_context_val(m,
				VTYPE_X_SELECTION)) && 
				*xsel)
		{
			results = (*cb)(m, key, media, x_selection, xsel, 
					strlen(xsel), bufftype, &ID, readonly,
					mapped, shareLevel, locator);
	
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
				results = (*cb)(m, key, media,
						path, file, size,
						bufftype, &ID, readonly,
						mapped, shareLevel, locator);
			}
		}
		else
		{
			tt_message_arg_bval(m, 0,
					(unsigned char **)&data, &size);
			results = (*cb)(m, key, media,
					contents, data, size,
					bufftype, &ID, readonly,
					mapped, shareLevel, locator);
		}
	}
	if(results == OK && ID)
	{
		tt_message_arg_val_set(m, 1, ID);
		free(ID);
	}
	else if(results == OK)
	{
		results = REJECT;
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
dstt_open(
	Open_CB		*cb,
	void		*key,
	char		*media,
	Data_t		type,
	void		*data,
	int		size,
	bufftype_t	bufftype,
	char		*ID,
	dstt_bool_t	readonly,
	int		shareLevel,
	dstt_bool_t	mapped,
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

	tt_message_op_set(m, OPEN);
	mode = TT_IN;
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
	switch(bufftype)
	{
	case	buffer:
		tt_message_arg_add(m, TT_OUT, VTYPE_BUFFERID, ID);
		break;
	case	view:
		tt_message_arg_add(m, TT_OUT, VTYPE_VIEWID, ID);
		break;
	default:
		tt_message_destroy(m);
		return(FALSE);
	}
	tt_message_iarg_add(m, TT_IN, VTYPE_MAPPED, (int)mapped);
	if(shareLevel) tt_message_iarg_add(m, TT_IN,
			VTYPE_SHARELEVEL, shareLevel);
	if(locator) tt_message_arg_add(m, TT_IN,
			VTYPE_LOCATOR, locator);

	tt_message_callback_add(m, dstt_open_cb);
	tt_message_user_set(m, 0, (void *)cb);
	tt_message_user_set(m, 1, key);

	tt_message_send(m);
	dstt_restore_tt();
	DP dstt_prnt_msg(m, "%d - send dstt_open", getpid());
	return(TRUE);
}

void
dstt_handle_open(
	Open_Handle	*cb,
	void		*key,
	char		*media,
	Data_t		type,
	bufftype_t	bufftype,
	dstt_bool_t	readonly)
{
	Tt_pattern	p;

	if(cb)
	{
		DP printf("%d - handle OPEN\n", getpid());
		p = tt_pattern_create();
	
		tt_pattern_scope_add(p, TT_SESSION);
		tt_pattern_session_add(p, tt_default_session());
		tt_pattern_category_set(p, TT_HANDLE);
		tt_pattern_op_add(p, OPEN);
	
		tt_pattern_arg_add(p, TT_IN, media, NULL);
		switch(bufftype)
		{
		case	buffer:
			tt_pattern_arg_add(p, TT_OUT, VTYPE_BUFFERID, NULL);
			break;
		case	view:
			tt_pattern_arg_add(p, TT_OUT, VTYPE_VIEWID, NULL);
		}
	
		tt_pattern_callback_add(p, dstt_open_handler);
		tt_pattern_user_set(p, 0, (void *)cb);
		tt_pattern_user_set(p, 1, key);
	
		tt_pattern_register(p);

		dstt_pat_add(OPEN, media, dstt_open_handler, p);
	}
	else
	{
		DP printf("%d - UNhandle OPEN\n", getpid());
		dstt_pat_destroy(OPEN, media);
	}
}
