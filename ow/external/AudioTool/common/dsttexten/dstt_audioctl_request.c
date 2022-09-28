/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)dstt_audioctl_request.c	1.3	93/02/22 SMI"

#include <desktop/tt_c.h>
#include "dstt_vtype.h"
#include "dstt.h"
#include "dstt_audio.h"

#define	VTYPE_DEVNAME		"string"


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
	return (tt_msg);
}

static void
close_msg(Tt_message tt_msg)
{
	tt_message_send(tt_msg);
	dstt_restore_tt();
}

static Tt_callback_action
audioctl_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Audioctl_CB 	*cb;
	void		*key = 0;
	int		ret_status = tt_message_status(m);
	Tt_state	state = tt_message_state(m);

	char		*prop;
	void		*data;
	int		size;


	char		*msg;
	char		*title;
	char		*devname;

	if(cb = (Audioctl_CB *)tt_message_user(m, 0)) {
		key = tt_message_user(m, 1);
		prop = tt_message_arg_type(m, 0);
		tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);
	
		devname = tt_message_arg_val(m, 1);

		msg = dstt_get_sopt(m, 2, VTYPE_MESSAGEID);
		title = dstt_get_sopt(m, 2, VTYPE_TITLE);
		
		err = (*cb)(m, key, ret_status,
			    prop, data, size,
			    devname,
			    msg, title);
	}
	return (TT_CALLBACK_PROCESSED);
}

int
dstt_audioctl(Audioctl_CB *cb, void *key,
	      char *prop, void *data, int size,
	      char *devname,
	      char *msgid, char *title)
{
	Tt_message	tt_msg = 0;
	int		null = 0;
		
	tt_msg = bld_msg();
	tt_message_op_set(tt_msg, AUDIO_CONTROL);
	tt_message_barg_add(tt_msg, TT_INOUT, prop, data, size);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_DEVNAME, devname);

	if (msgid)
		tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if (title)
		tt_message_arg_add(tt_msg, TT_IN, VTYPE_TITLE, title);
	tt_message_callback_add(tt_msg, audioctl_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}
