/*
 *
 * dstt_desktop_request.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_desktop_request.c 1.12 93/01/19 Copyr 1990 Sun Micro";
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


/* request calbacks allway return TT_CALLBACK_PROCESSED */
static Tt_message
bld_msg()
{
	Tt_message	tt_msg = 0;

	dstt_start_tt();
	tt_msg = tt_message_create();
	tt_message_address_set(tt_msg, TT_HANDLER);
	tt_message_class_set(tt_msg, TT_REQUEST);
	tt_message_scope_set(tt_msg, TT_SESSION);
	tt_message_session_set(tt_msg,  tt_default_session());
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_scope_set(tt_msg, TT_SESSION);
	return(tt_msg);
}

static void
close_msg(Tt_message tt_msg)
{
	tt_message_send(tt_msg);
	dstt_restore_tt();
}


static Tt_callback_action
get_status(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	int	ret_status = tt_message_status(m);
	Get_Status_CB	*cb;
	void	*key = 0;
	char	*arg[5];
	int	count = 0;
	int	i = 0;

	if(cb = (Get_Status_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		count = tt_message_args_count(m);
		for(i = 0; i < count; i++)
		{
			arg[i] = tt_message_arg_val(m, i);
		}
			
		err = (*cb)(m, key, ret_status, arg[0], arg[1],
				arg[2], arg[3], arg[4]);
	}
	return(TT_CALLBACK_PROCESSED);
}


int
dstt_get_status(Get_Status_CB *cb,
void *key, char *toolid, char *msgID)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();

	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_STATUS);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_VENDOR, NULL);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_TOOLNAME, NULL);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_TOOLVERSION, NULL);
	if(msgID) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgID);
	tt_message_callback_add(tt_msg, get_status);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
environment_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Environment_CB	*cb;
	void	*key = 0;
	int	ret_status = tt_message_status(m);
	char	*var = 0;
	char	*val = 0;

	DP printf("%d - called environment_cb (0x%X - %s)\n",
			getpid(), m, dstt_prnt_state(m));
	if(cb = (Environment_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		var = tt_message_arg_val(m, 0);
		val = tt_message_arg_val(m, 1);
	
		err = (*cb)(m, key, ret_status, var, val);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_set_environment(Environment_CB *cb,
void *key, char *toolid, char **var, char **val)
{
	Tt_message	tt_msg = 0;
	int		i = 0;
		
	tt_msg = bld_msg();

	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_ENVIRONMENT);
	while(var[i])
	{
		tt_message_arg_add(tt_msg, TT_IN, VTYPE_ENV_VAR, var[i]);
		tt_message_arg_add(tt_msg, TT_IN, VTYPE_ENV_VALUE, val[i]);
		i++;
	}
	tt_message_callback_add(tt_msg, environment_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}


int
dstt_get_environment(Environment_CB *cb,
void *key, char *toolid, char **var)
{
	Tt_message	tt_msg = 0;
	int		i = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_ENVIRONMENT);
	while(var[i])
	{
		tt_message_arg_add(tt_msg, TT_IN, VTYPE_ENV_VAR, var[i]);
		tt_message_arg_add(tt_msg, TT_OUT, VTYPE_ENV_VALUE, NULL);
		i++;
	}
	tt_message_callback_add(tt_msg, environment_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
geometry_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Geometry_CB	*cb;
	void	*key = 0;
	int	ret_status = tt_message_status(m);
	int	w, h, x, y;

	DP printf("%d - called geometry_cb (0x%X - %s)\n",
			getpid(), m, dstt_prnt_state(m));
	if(cb = (Geometry_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		tt_message_arg_ival(m, 0, &w);
		tt_message_arg_ival(m, 1, &h);
		tt_message_arg_ival(m, 2, &x);
		tt_message_arg_ival(m, 3, &y);
	
		err = (*cb)(m, key, ret_status, w, h, x, y);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_set_geometry(Geometry_CB *cb,
void *key, char *toolid, int w, int h, int x, int y, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();

	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_GEOMETRY);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_WIDTH, w);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_HEIGHT, h);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_XOFFSET, x);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_YOFFSET, y);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, geometry_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}


int
dstt_get_geometry(Geometry_CB *cb,
void *key, char *toolid, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_GEOMETRY);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_WIDTH, 0);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_HEIGHT, 0);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_XOFFSET, 0);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_YOFFSET, 0);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, geometry_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	DP dstt_prnt_msg(tt_msg, "Get_Geometry request");
	close_msg(tt_msg);
}

static Tt_callback_action
iconified_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Iconified_CB	*cb;
	void	*key = 0;
	int	val = 0;
	char	*msg = 0;
	char	*buff = 0;
	int	ret_status = tt_message_status(m);

	if(cb = (Iconified_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		msg = tt_message_arg_val(m, 1);
		buff = tt_message_arg_val(m, 1);
	
		err = (*cb)(m, key, ret_status, val, msg, buff);
	}
	return(TT_CALLBACK_PROCESSED);
}


int
dstt_set_iconified(Iconified_CB *cb,
void *key, char *toolid, int iconified, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_ICONIFIED);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_BOOL, iconified);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, iconified_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

int
dstt_get_iconified(Iconified_CB *cb,
void *key, char *toolid, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_ICONIFIED);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_BOOL, NULL);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, iconified_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
locale(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Locale_CB	*cb;
	void	*key = 0;
	int	ret_status = tt_message_status(m);
	Tt_state	state = tt_message_state(m);
	int	count;
	char	**var = 0;
	char	**val = 0;
	int	i;

	DP printf("%d - called locale (0x%X - %s)\n",
			getpid(), m, dstt_prnt_state(m));
	if(cb = (Locale_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
	
		count = tt_message_args_count(m);
		var = (char **)MALLOC((count/2+1)*sizeof(char *));
		val = (char **)MALLOC((count/2+1)*sizeof(char *));
		for(i = 0; i < count; i++)
		{
			if(i & 1)
			{
				val[i/2] = tt_message_arg_val(m, i);
			}
			else
			{
				var[i/2] = tt_message_arg_val(m, i);
			}
		}
		var[i/2] = NULL;
		val[i/2] = NULL;
	
		err = (*cb)(m, key, ret_status, var, val);
		FREE(var);
		FREE(val);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_set_locale(Locale_CB *cb,
void *key, char *toolid, char *var, char *val)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();

	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_LOCALE);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_CATEGORY, var);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_LOCALE, val);
	tt_message_callback_add(tt_msg, locale);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

int
dstt_get_locale(Locale_CB *cb,
void *key, char *toolid, char **var)
{
	Tt_message	tt_msg = 0;
	int		i = 0;
	
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_LOCALE);
	while(var && var[i])
	{
		tt_message_arg_add(tt_msg, TT_IN, VTYPE_CATEGORY, var[i]);
		tt_message_arg_add(tt_msg, TT_OUT, VTYPE_LOCALE, NULL);
		i++;
	}
	tt_message_callback_add(tt_msg, locale);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	DP dstt_prnt_msg(tt_msg, "GET_LOCALE request");
	close_msg(tt_msg);
}

static Tt_callback_action
mapped(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Mapped_CB	*cb;
	void	*key = 0;
	int	val = 0;
	char	*msg = 0;
	char	*buff = 0;
	int	ret_status = tt_message_status(m);

	if(cb = (Mapped_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		tt_message_arg_ival(m, 0, &val);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
		err = (*cb)(m, key, ret_status, val, msg, buff);
	}
	return(TT_CALLBACK_PROCESSED);
}


int
dstt_set_mapped(Mapped_CB *cb,
void *key, char *toolid, int val, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_MAPPED);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_BOOL, val);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, mapped);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}


int
dstt_get_mapped(Mapped_CB *cb,
void *key, char *toolid, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_MAPPED);
	tt_message_iarg_add(tt_msg, TT_INOUT, VTYPE_BOOL, NULL);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, mapped);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
xinfo_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	XInfo_CB	*cb;
	void	*key = 0;
	int	ret_status = tt_message_status(m);
	Tt_state	state = tt_message_state(m);
	char	*display, *visual, *msgid;
	int	depth;
	int	count = 0;
	int	i = 0;
	int	n = 0;
	char	**list = 0;
	char	**values = 0;

	DP printf("%d - called xinfo_cb (0x%X - %s)\n",
			getpid(), m, dstt_prnt_state(m));
	if(cb = (XInfo_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		count = tt_message_args_count(m);
	
		display = tt_message_arg_val(m, 0);
		visual = tt_message_arg_val(m, 1);
		tt_message_arg_ival(m, 2, &depth);
	
		n = (count-3)%2+1;
		
		list = (char **)MALLOC(n*sizeof(char *));
		memset(list, '\0', n*sizeof(char *));
		values = (char **)MALLOC(n*sizeof(char *));
		memset(values, '\0', n*sizeof(char *));
		for(i = 0; i < n; i++)
		{
			list[i] = tt_message_arg_val(m, 3+i*2);
			values[i] = tt_message_arg_val(m, 4+i*2);
		}
		list[i] = 0;
		values[i] = 0;
		msgid = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		err = (*cb)(m, key, ret_status,
			display, visual, depth, list, values, msgid);
	}
	return(TT_CALLBACK_PROCESSED);
}


int
dstt_set_xinfo(XInfo_CB *cb, void *key, char *toolid,
char *display, char *visual, int depth, char **list, char **value, char *msgID)
{
	Tt_message	tt_msg = 0;
	char		*name;
	char		*val;
	
	tt_msg = bld_msg();

	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_XINFO);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_DISPLAY, display);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_VISUAL, visual);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_DEPTH, depth);
	name = *list;
	val = *value;
	while(name && val)
	{
		tt_message_arg_add(tt_msg, TT_OUT, VTYPE_RESOURCENAME, name);
		tt_message_arg_add(tt_msg, TT_OUT, VTYPE_RESOURCEVAL, val);
		name++;
		val++;
	}
	if(msgID) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgID);
	tt_message_callback_add(tt_msg, xinfo_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

int
dstt_get_xinfo(XInfo_CB *cb, void *key, char *toolid,
char **list, char *msgID)
{
	Tt_message	tt_msg = 0;
	char		*name;
		
	tt_msg = bld_msg();

	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_XINFO);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_DISPLAY, NULL);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_VISUAL, NULL);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_DEPTH, NULL);
	if(list)
	{
		name = *list;
	}
	else
	{
		name = NULL;
	}
	while(name)
	{
		tt_message_arg_add(tt_msg, TT_OUT, VTYPE_RESOURCENAME, name);
		tt_message_arg_add(tt_msg, TT_OUT, VTYPE_RESOURCEVAL, NULL);
		name++;
	}
	if(msgID) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgID);
	tt_message_callback_add(tt_msg, xinfo_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	DP dstt_prnt_msg(tt_msg, "GET_XINFO request");
	close_msg(tt_msg);
}

static Tt_callback_action
raise_lower_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Raise_Lower_CB	*cb;
	void	*key = 0;
	int	val = 0;
	char	*msg = 0;
	char	*buff = 0;
	int	ret_status = tt_message_status(m);

	if(cb = (Raise_Lower_CB*)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
		err = (*cb)(m, key, ret_status, msg, buff);
	}
	return(TT_CALLBACK_PROCESSED);
}


int
dstt_raise(Raise_Lower_CB *cb,
void *key, char *toolid, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, RAISE);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, raise_lower_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

int
dstt_lower(Raise_Lower_CB *cb,
void *key, char *toolid, char *msg, char *buff)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, LOWER);
	if(msg)tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	if(buff)tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buff);
	tt_message_callback_add(tt_msg, raise_lower_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
do_command_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Do_Command_CB	*cb;
	void	*key = 0;
	int	ret_status = tt_message_status(m);
	char	*var = 0;
	char	*val = 0;
	char	*msg = 0;

	if(cb = (Do_Command_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		var = tt_message_arg_val(m, 0);
		val = tt_message_arg_val(m, 1);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	
		err = (*cb)(m, key, ret_status, var, val, msg);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_do_command(Do_Command_CB *cb,
void *key, char *toolid, char *var, char *msg)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, DO_COMMAND);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_COMMAND, var);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_RESULTS, NULL);
	if(msg) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	tt_message_callback_add(tt_msg, do_command_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
quit_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Quit_CB		*cb;
	void	*key = 0;
	int	ret_status = tt_message_status(m);
	int	silent = 0;
	int	force = 0;
	char	*msg = 0;

	if(cb = (Quit_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		tt_message_arg_ival(m, 0, &silent);
		tt_message_arg_ival(m, 1, &force);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		err = (*cb)(m, key, ret_status, silent, force, msg);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_quit(Quit_CB *cb,
void *key, char *toolid, int silent, int force, char *msg)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, QUIT);
	tt_message_iarg_add(tt_msg, TT_IN, VTYPE_SILENT, silent);
	tt_message_iarg_add(tt_msg, TT_OUT, VTYPE_FORCE, force);
	if(msg) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msg);
	tt_message_callback_add(tt_msg, quit_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
signal_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Signal_CB	*cb;
	char	*toolid = 0;
	int	ret_status = tt_message_status(m);
	void	*key = 0;
	int	sig;

	if(cb = (Signal_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		toolid = tt_message_handler(m);
		tt_message_arg_ival(m, 1, &sig);
	
		err = (*cb)(m, key, ret_status, sig);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_signal(Signal_CB *cb, void *key, char *toolid, int mysig)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SIGNAL);
	tt_message_iarg_add(tt_msg, TT_IN, VTYPE_SIGNAL, mysig);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_FORCE, NULL);
	tt_message_callback_add(tt_msg, signal_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
situation_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Situation_CB	*cb;
	char	*toolid = 0;
	int	ret_status = tt_message_status(m);
	void	*key = 0;
	char	*pathname;

	if(cb = (Situation_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		toolid = tt_message_handler(m);
		pathname = tt_message_arg_val(m, 1);
	
		err = (*cb)(m, key, ret_status, pathname);
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_set_situation(Situation_CB *cb, void *key, char *toolid, char *pathname)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, SET_SITUATION);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_SITUATION, pathname);
	tt_message_callback_add(tt_msg, situation_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

int
dstt_get_situation(Situation_CB *cb, void *key, char *toolid)
{
	Tt_message	tt_msg = 0;
		
	tt_msg = bld_msg();
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, GET_SITUATION);
	tt_message_arg_add(tt_msg, TT_OUT, VTYPE_SITUATION, NULL);
	tt_message_callback_add(tt_msg, situation_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

static Tt_callback_action
deposit_cb(Tt_message m, Tt_pattern p)
{
	int		err = 0;
	Tt_state	state = tt_message_state(m);
	Display_CB	*cb;
	void		*key = 0;
	int		ret_status = tt_message_status(m);
	char		*media;
	unsigned char	*data;
	char		*afile;
	int		size;
	char		*msg;
	char		*buffid;
	char		*xsel;

	DP printf("%d - calling deposit_cb\n", getpid());
	if(cb = (Deposit_CB *)tt_message_user(m, 0))
	{
		key = tt_message_user(m, 1);
		media = tt_message_arg_type(m, 0);
		msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
		buffid = dstt_get_sopt(m, 0, VTYPE_TITLE);
#ifndef PRIOR_493
		if((xsel = tt_message_context_val(m, VTYPE_X_SELECTION)) && *xsel)
		{
			err = (*cb)(m, key, ret_status, media,
					x_selection, xsel, strlen(xsel),
					buffid, msg);
		}
		else
#endif	/* NOTE! if PRIOR_493 not defined then this is ONE if statement */
		if(afile = tt_message_file(m))
		{
			afile = tt_message_file(m);
			size = strlen(afile);
			err = (*cb)(m, key, ret_status, media,
					path, afile, size, buffid, msg);
		}
		else
		{
			tt_message_arg_bval(m, 0, &data, &size);
			err = (*cb)(m, key, ret_status, media,
					contents, data, size, buffid, msg);
		}
	}
	return(TT_CALLBACK_PROCESSED);
}

int
dstt_deposit(Deposit_CB *cb, char *toolid,
void *key, char *media, Data_t type, void *val, int size, char *buffid, char *msgid)
{
	Tt_message	tt_msg = 0;
	int		null = 0;
	Tt_status	rc;
	
	tt_msg = bld_msg(type);
	tt_message_address_set(tt_msg, TT_HANDLER);
	tt_message_handler_set(tt_msg, toolid);
	tt_message_op_set(tt_msg, DEPOSIT);
	switch(type)
	{
	case	contents:
		tt_message_barg_add(tt_msg, TT_IN, media, val,  size);
		break;
	case	path:
		tt_message_barg_add(tt_msg, TT_IN, media, NULL,  NULL);
		tt_message_file_set(tt_msg, (char *)val);
#ifndef	PRIOR_493_1
		break;
	case	x_selection:
		tt_message_barg_add(tt_msg, TT_IN,
					media, NULL, NULL);
		rc = tt_message_context_set(tt_msg,
			VTYPE_X_SELECTION, (char *)val);
#endif
	}
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	tt_message_callback_add(tt_msg, deposit_cb);
	tt_message_user_set(tt_msg, 0, (void *)cb);
	tt_message_user_set(tt_msg, 1, key);
	close_msg(tt_msg);
}

