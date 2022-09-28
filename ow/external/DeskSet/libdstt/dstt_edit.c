/*
 *
 * dstt_edit.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_edit.c 1.5 93/01/19 Copyr 1990 Sun Micro";
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

static Tt_callback_action
dstt_edit_cb(Tt_message m, Tt_pattern p)
{
	int			err = 0;
	Edit_CB			*cb;
	void			*key = 0;
	char			*file;
	char			*media = 0;
	void			*data = 0;
	int			size;
	char			*msg;
	char			*title;
	char			*xsel;
	int			i = 0;
	int			mark = tt_mark();

	DP dstt_prnt_msg(m, "dstt_edit_cb");
	if(cb = (Edit_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		media = tt_message_arg_type(m, 0);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		title = dstt_get_sopt(m, 0, VTYPE_TITLE);

#ifndef	PRIOR_493
		if((xsel = (char *)tt_message_context_val(m,
				VTYPE_X_SELECTION)) && 
				*xsel)
		{
			(*cb)(m, key, 0, media, x_selection, xsel, 
					strlen(xsel), msg, title);
	
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
				(*cb)(m, key, 0, media,
						path, file, size, msg, title);
			}
		}
		else
		{
			tt_message_arg_bval(m, 0,
					(unsigned char **)&data, &size);
			(*cb)(m, key, 0, media, contents,
					data, size, msg, title);
		}
	}
	tt_release(mark);
	tt_message_destroy(m);
	return(TT_CALLBACK_PROCESSED);
}

static Tt_callback_action
dstt_edit_handler(Tt_message m, Tt_pattern p)
{
	status_t		results = REJECT;
	Edit_Handle		*cb;
	void			*key = 0;
	char			*file;
	char			*media = 0;
	void			*data = 0;
	int			size;
	char			*msg;
	char			*title;
	char			*xsel;
	int			mark = tt_mark();

	DP dstt_prnt_msg(m, "dstt_edit_handler");
	if(cb = (Edit_Handle *)tt_pattern_user(p, 0))
	{
		key = tt_pattern_user(p, 1);

		media = tt_message_arg_type(m, 0);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		title = dstt_get_sopt(m, 0, VTYPE_TITLE);

#ifndef	PRIOR_493
		if((xsel = (char *)tt_message_context_val(m,
				VTYPE_X_SELECTION)) && 
				*xsel)
		{
			results = (*cb)(m, media, x_selection, xsel, 
					strlen(xsel), msg, title);
	
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
				results = (*cb)(m, media,
						path, file, size, msg, title);
			}
		}
		else
		{
			tt_message_arg_bval(m, 0,
					(unsigned char **)&data, &size);
			results = (*cb)(m, media, contents,
					data, size, msg, title);
		}
		
	}
	switch(results)
	{
	case	OK: 
		DP printf("%d - Reply to 0x%X\n", getpid(), m);
		tt_message_reply(m);
		tt_release(mark);
		break;
	case	REJECT:
		DP printf("%d - Reject to 0x%X\n", getpid(), m);
		tt_message_reject(m);
		tt_release(mark);
		tt_message_destroy(m);
		break;
	case	FAIL:
		DP printf("%d - Fail to 0x%X\n", getpid(), m);
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
dstt_edit(Edit_CB *cb, void *key,
char *media, Data_t type, void *data, int size, char *msgID, char * title)
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

	tt_message_op_set(m, EDIT);
	if(data)
	{
		mode = TT_INOUT;
	}
	else
	{
		mode = TT_OUT;
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
	if(msgID) tt_message_arg_add(m, TT_IN, VTYPE_MESSAGEID, msgID);
	if(title) tt_message_arg_add(m, TT_IN, VTYPE_TITLE, title);
	tt_message_callback_add(m, dstt_edit_cb);
	tt_message_user_set(m, 0, (void *)cb);
	tt_message_user_set(m, 1, key);

	tt_message_send(m);
	DP dstt_prnt_msg(m, "dstt_edit");
	dstt_restore_tt();
}

void
dstt_handle_edit(Edit_Handle *cb, void *key, char *media, Data_t type,
char *msg, char *title)
{
	Tt_pattern	p1,p2;

	if(cb)
	{
		DP printf("%d - handle EDIT %s\n", getpid(), media);
		p1 = tt_pattern_create();
	
		tt_pattern_scope_add(p1, TT_SESSION);
		tt_pattern_session_add(p1, tt_default_session());
		tt_pattern_category_set(p1, TT_HANDLE);
		tt_pattern_op_add(p1, EDIT);
	
		tt_pattern_arg_add(p1, TT_OUT, media, NULL);
	
		tt_pattern_callback_add(p1, dstt_edit_handler);
		tt_pattern_user_set(p1, 0, (void *)cb);
		tt_pattern_user_set(p1, 1, key);
	
		tt_pattern_register(p1);

		p2 = tt_pattern_create();
	
		tt_pattern_scope_add(p2, TT_SESSION);
		tt_pattern_session_add(p2, tt_default_session());
		tt_pattern_category_set(p2, TT_HANDLE);
		tt_pattern_op_add(p2, EDIT);
	
		tt_pattern_arg_add(p2, TT_INOUT, media, NULL);
	
		tt_pattern_callback_add(p2, dstt_edit_handler);
		tt_pattern_user_set(p2, 0, (void *)cb);
		tt_pattern_user_set(p2, 1, key);
		tt_pattern_user_set(p2, 2, p1);
	
		tt_pattern_register(p2);

		dstt_pat_add(EDIT, media, dstt_edit_handler, p2);
	}
	else
	{
		DP printf("%d - UNhandle EDIT %s\n", getpid(), media);
		dstt_pat_destroy(EDIT, media);
	}
}
