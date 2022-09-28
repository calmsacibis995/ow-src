/*
 *
 * dstt_oldtt.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_oldtt.c 1.3 92/12/12 Copyr 1990 Sun Micro";
#endif

#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include "dstt_config.h"
#include "ds_verbose_malloc.h"

static	status_t
dstt_passthrough(Tt_message m, status_t (*cb)(...))
{
	return(OLD);
}

static	status_t
dummy(...)
{
	return(OLD);
}

static Msg_t	msg_list[] =
{
{"launch",		dstt_passthrough,	dummy,	1},
{"status",		dstt_passthrough,	dummy,	1},
{"dispatch_data",	dstt_passthrough,	dummy,	1},
{"move",		dstt_passthrough,	dummy,	1},
{"quit",		dstt_passthrough,	dummy,	1},
{"hide",		dstt_passthrough,	dummy,	1},
{"expose",		dstt_passthrough,	dummy,	1},
{"retrieve_data",	dstt_passthrough,	dummy,	1},
{"departing",		dstt_passthrough,	dummy,	1},
{"compose2",		dstt_passthrough,	dummy,	1}
};

#define	MSG_LIST_SIZE	(sizeof(msg_list)/sizeof(Msg_t))

static	Tt_message	save_msg = 0;
static	Message_group_t	*this_group = 0;

static	void *
dstt_oldtt_get_key(Tt_message m)
{
	return(NULL);
}

static	Message_set_t *
dstt_oldtt_find(void *key, int create, Message_group_t *group)
{
	int		i = 1;
	Message_set_t	*msg;
	
	while((group->sets)[i] != (Message_set_t *)NULL)
	{
		if(key == ((*(group->sets)[i]).key))
		{
			break;
		}
		i++;
	}
	if(create && (group->sets)[i] == (Message_set_t *)NULL)
	{
		msg = dstt_config_set(msg_list, MSG_LIST_SIZE, group);
		msg->key = 0;
		return(msg);
	}
	return((group->sets)[i]);
}

static	void
dstt_oldtt_rm(void *key)
{
}

static	Tt_pattern
dstt_oldtt_pattern(void *key)
{
	Tt_pattern	pat = tt_pattern_create();

	return(pat);
}

int
dstt_oldtt_callback(char *media, ...)
{
	va_list ap ;

	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_oldtt_find,
			dstt_oldtt_get_key,
			dstt_oldtt_rm,
			dstt_oldtt_pattern);
	}
	va_start(ap, frame);
	return(dstt_callback_register(this_group, NULL, &ap));
}

int
dstt_oldtt_register(char *media, ...)
{
	va_list ap ;

	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_oldtt_find,
			dstt_oldtt_get_key,
			dstt_oldtt_rm,
			dstt_oldtt_pattern);
	}
	va_start(ap, frame);
	return(dstt_handle_register(this_group, NULL, &ap));
}

