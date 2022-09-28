/*
 *
 * dstt_prop_request.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_prop_request.c 1.5 93/01/04 Copyr 1990 Sun Micro";
#endif

#include <desktop/tt_c.h>
#include "dstt_vtype.h"
#include "dstt.h"
#include "ds_verbose_malloc.h"

static Tt_message
bld_msg()
{
	Tt_message	tt_msg = 0;

	dstt_start_tt();
	tt_msg = tt_message_create();
	tt_message_address_set(tt_msg, TT_PROCEDURE);
	tt_message_class_set(tt_msg, TT_REQUEST);
	tt_message_scope_set(tt_msg, TT_SESSION);
	tt_message_session_set(tt_msg,  tt_default_session());
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	return(tt_msg);
}

static void
close_msg(Tt_message tt_msg)
{
	tt_message_send(tt_msg);
	dstt_restore_tt();
}

static Tt_callback_action
prop_edit_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Prop_Edit_CB 	*cb;
	void		*key = 0;
	int		ret_status = tt_message_status(m);
	Tt_state	state = tt_message_state(m);

	char		*prop;
	void		*data;
	int		size;

	void		*icon1 = 0;
	int		isize1 = 0;
	void		*icon2 = 0;
	int		isize2 = 0;

	char		*msg;
	char		*title;

	if(cb = (Prop_Edit_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		prop = tt_message_arg_type(m, 0);
		tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);
		tt_message_arg_bval(m, 1, (unsigned char **)&icon1, &isize1);
		tt_message_arg_bval(m, 2, (unsigned char **)&icon2, &isize2);
	
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		title = dstt_get_sopt(m, 0, VTYPE_TITLE);
		err = (*cb)(m, key, ret_status,
			prop, data, size,
			icon1, isize1,
			icon2, isize2,
			msg, title);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_prop_edit(Prop_Edit_CB *cb, void *key,
	char *prop, void *data, int size,
	void *icon1, int isize1,
	void *icon2, int isize2,
	char *msgid, char *title)
{
	Tt_message	tt_msg = 0;
	int		null = 0;
		
	tt_msg = bld_msg();
	tt_message_op_set(tt_msg, PROP_EDIT);
	tt_message_barg_add(tt_msg, TT_INOUT, prop, data, size);

	tt_message_barg_add(tt_msg, TT_IN, "byte", icon1,  isize1);
	tt_message_barg_add(tt_msg, TT_IN, "byte", icon2,  isize2);

	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(title) tt_message_arg_add(tt_msg, TT_IN, VTYPE_TITLE, title);
	tt_message_callback_add(tt_msg, prop_edit_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

