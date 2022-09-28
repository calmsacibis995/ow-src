/*
 *
 * dstt_notices.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_notices.c 1.11 92/12/16 Copyr 1990 Sun Micro";
#endif

#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include "dstt_config.h"
#include "ds_verbose_malloc.h"

extern status_t dstt_started_param(Tt_message, status_t (*)(...));
extern status_t dstt_stopped_param(Tt_message, status_t (*)(...));
extern status_t dstt_modified_param(Tt_message, status_t (*)(...));
extern status_t dstt_saved_param(Tt_message, status_t (*)(...));
extern status_t dstt_status_param(Tt_message, status_t (*)(...));
extern status_t dstt_created_param(Tt_message, status_t (*)(...));
extern status_t dstt_deleted_param(Tt_message, status_t (*)(...));

static Msg_t	msg_list[] =
{
{STARTED,	dstt_started_param,	NULL	,0},
{STOPPED,	dstt_stopped_param,	NULL	,0},
{CREATED,	dstt_created_param,	NULL	,0},
{DELETED,	dstt_deleted_param,	NULL	,0},
{STATUS,	dstt_status_param,	NULL	,0},
{MODIFIED,	dstt_modified_param,	NULL	,0},
{SAVED,		dstt_saved_param,	NULL	,0}
};

#define	MSG_LIST_SIZE	(sizeof(msg_list)/sizeof(Msg_t))

static	Tt_message	save_msg = 0;
static	Message_group_t	*this_group = 0;

void *
dstt_notice_get_key(Tt_message m)
{
	void	*key;

	return((void *)1);
}

Message_set_t *
dstt_notice_find(void *key, int create, Message_group_t *group)
{
	int		i = (int)key;
	Message_set_t	*msg;

	if(i == 0)
	{
		i++;
	}
	if(create && (group->sets)[i] == (Message_set_t *)NULL)
	{
		msg = dstt_config_set(msg_list, MSG_LIST_SIZE, group);
		msg->key = key;
		return(msg);
	}
	return((group->sets)[i]);
}

Tt_pattern
dstt_notice_pattern(void *key)
{
	Tt_pattern	pat = tt_pattern_create();

	tt_pattern_scope_add(pat, TT_SESSION);
	tt_pattern_session_add(pat, tt_default_session());
	tt_pattern_category_set(pat, TT_OBSERVE);
	return(pat);
}

void
dstt_notice_rm(void *key)
{
}

int
dstt_notice_callback(char *item, ...)
{
	va_list ap ;

	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_notice_find,
			dstt_notice_get_key,
			dstt_notice_rm,
			dstt_notice_pattern);
	}
	va_start(ap, item);
	dstt_callback_register(this_group, NULL, &ap);
}

int
dstt_notice_register(char *item, ...)
{
	va_list ap ;

	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_notice_find,
			dstt_notice_get_key,
			dstt_notice_rm,
			dstt_notice_pattern);
	}
	va_start(ap, item);
	return(dstt_handle_register(this_group, NULL, &ap));
}

