/*
 *
 * dstt_close.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_close.c 1.5 93/01/19 Copyr 1990 Sun Micro";
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
dstt_close_results(Tt_message m, Tt_pattern p)
{
	Close_CB		*cb;
	void			*key = 0;
	bufftype_t		bufftype;
	char			*ID;
	int			inquisitive;
	int			force;
	int			mark = tt_mark();

	if(cb = (Close_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);

		bufftype = dstt_get_bufftype(m, 0);
		ID = tt_message_arg_val(m, 0);

		tt_message_arg_ival(m, 1, &inquisitive);
		tt_message_arg_ival(m, 2, &force);
		(*cb)(m, key, bufftype, ID, inquisitive, force);
	}
	tt_release(mark);
	tt_message_destroy(m);
	return(TT_CALLBACK_PROCESSED);
}

static Tt_callback_action
dstt_close_handler(Tt_message m, Tt_pattern p)
{
	status_t		results = REJECT;
	Close_Handle		*cb;
	void			*key = 0;
	bufftype_t		bufftype;
	char			*ID;
	int			inquisitive;
	int			force;
	int			mark = tt_mark();

	if(cb = (Close_Handle *)tt_pattern_user(p, 0))
	{
		key = tt_pattern_user(p, 1);

		ID = tt_message_arg_val(m, 0);
		bufftype = dstt_get_bufftype(m, 0);
		tt_message_arg_ival(m, 1, &inquisitive);
		tt_message_arg_ival(m, 2, &force);
		results = (*cb)(m, key, bufftype, ID, inquisitive, force);
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
dstt_close(
	Close_CB	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	int		inquisitive,
	int		force)
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

	tt_message_op_set(m, CLOSE);
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

	tt_message_iarg_add(m, TT_IN, VTYPE_BOOL, inquisitive);
	tt_message_iarg_add(m, TT_IN, VTYPE_BOOL, force);

	tt_message_callback_add(m, dstt_close_results);
	tt_message_user_set(m, 0, (void *)cb);
	tt_message_user_set(m, 1, key);

	tt_message_send(m);
	dstt_restore_tt();
	return(TRUE);
}

void
dstt_handle_close(
	Close_Handle	*cb,
	void		*key,
	bufftype_t	bufftype,
	char		*ID)
{
	Tt_pattern	p;

	if(cb)
	{
		DP printf("handle CLOSE\n");
		p = tt_pattern_create();
	
		tt_pattern_scope_add(p, TT_SESSION);
		tt_pattern_session_add(p, tt_default_session());
		tt_pattern_category_set(p, TT_HANDLE);
		tt_pattern_op_add(p, CLOSE);
	
		switch(bufftype)
		{
		case	buffer:
			tt_pattern_arg_add(p, TT_IN, VTYPE_BUFFERID, NULL);
			break;
		case	view:
			tt_pattern_arg_add(p, TT_IN, VTYPE_VIEWID, NULL);
		}

		tt_pattern_callback_add(p, dstt_close_handler);
		tt_pattern_user_set(p, 0, (void *)cb);
		tt_pattern_user_set(p, 1, key);
	
		tt_pattern_register(p);

		dstt_pat_add(CLOSE, NULL, dstt_close_handler, p);
	}
	else
	{
		DP printf("UNhandle CLOSE\n");
		dstt_pat_destroy(CLOSE, NULL);
	}
}
