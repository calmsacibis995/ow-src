/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)dstt_audioctl.c	1.2	96/02/20 SMI"

#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include "dstt_config.h"

#include "dstt_audio.h"

extern status_t dstt_audioctl_param(Tt_message, status_t (*)(...));

static Msg_t	msg_list[] =
{
{AUDIO_CONTROL,		dstt_audioctl_param,	NULL	,0}
};

#define	MSG_LIST_SIZE	(sizeof(msg_list)/sizeof(Msg_t))

static	Tt_message	save_msg = 0;
static	Message_group_t	*this_group = 0;

void *
dstt_audioctl_get_key(Tt_message m)
{
	void	*key;

	key = (void *)tt_message_arg_type(m, 0);
	return(key);
}

Message_set_t *
dstt_audioctl_find(void *key, int create, Message_group_t *group)
{
	int		i = 1;
	Message_set_t	*msg;
	
	while((group->sets)[i] != (Message_set_t *)NULL)
	{
		if(strcmp((char *)key, (char *)((*(group->sets)[i]).key)) == 0)
		{
			break;
		}
		i++;
	}
	if(create && (group->sets)[i] == (Message_set_t *)NULL)
	{
		msg = dstt_config_set(msg_list, MSG_LIST_SIZE, group);
		msg->key = strdup((char*)key);
		return(msg);
	}
	return((group->sets)[i]);
}

void
dstt_audioctl_rm(void *key)
{
}

Tt_pattern
dstt_audioctl_pattern(void *v)
{
	Tt_pattern	pat = tt_pattern_create();

	tt_pattern_scope_add(pat, TT_SESSION);
	tt_pattern_session_add(pat, tt_default_session());
	tt_pattern_category_set(pat, TT_HANDLE);
	return(pat);
}

int
dstt_audioctl_callback(char *key, ...)
{
	va_list ap ;

	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_audioctl_find,
			dstt_audioctl_get_key,
			dstt_audioctl_rm,
			dstt_audioctl_pattern);
	}
	va_start(ap, frame);
	dstt_callback_register(this_group, key, &ap);
}

int
dstt_audioctl_register(char *key, ...)
{
	va_list ap ;

	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_audioctl_find,
			dstt_audioctl_get_key,
			dstt_audioctl_rm,
			dstt_audioctl_pattern);
	}
	va_start(ap, frame);
	return(dstt_handle_register(this_group, key, &ap));
}

