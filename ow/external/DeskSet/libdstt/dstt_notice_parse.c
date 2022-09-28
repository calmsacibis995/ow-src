/*
 *
 * dstt_notice_parse.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_notice_parse.c 1.7 93/01/04 Copyr 1990 Sun Micro";
#endif

#include <desktop/tt_c.h>
#include "dstt.h"
#include "dstt_vtype.h"
#include "ds_verbose_malloc.h"

/* notices are always OK */

status_t
dstt_modified_param(Tt_message m, status_t (*cb)(Tt_message, char *, char *))
{
	status_t	results = REJECT;
	char		*type;
	char		*entity;
	
	type = tt_message_arg_type(m, 0);
	if(strcmp(type, "File") == 0)
	{
		entity = tt_message_file(m);
	}
	else
	{
		entity = tt_message_arg_val(m, 0);
	}
	(*cb)(m, type, entity);
	return(OK);
}

status_t
dstt_saved_param(Tt_message m, status_t (*cb)(Tt_message, char *, char *))
{
	status_t	results = REJECT;
	char		*type;
	char		*entity;
	
	type = tt_message_arg_type(m, 0);
	if(strcmp(type, "File") == 0)
	{
		entity = tt_message_file(m);
	}
	else
	{
		entity = tt_message_arg_val(m, 0);
	}
	(*cb)(m, type, entity);
	return(OK);
}


status_t
dstt_status_param(Tt_message m, status_t (*cb)(Tt_message, char *, char *, char *, char *, char *, char *))
{
	status_t	results = REJECT;
	char		*status;
	char		*vendor;
	char		*toolName;
	char		*toolVersion;
	char		*messageID;
	char		*domain;
	
	status = tt_message_arg_val(m, 0);
	vendor = tt_message_arg_val(m, 1);
	toolName = tt_message_arg_val(m, 2);
	toolVersion = tt_message_arg_val(m, 3);
	messageID = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	domain = dstt_get_sopt(m, 0, VTYPE_DOMAIN);
	(*cb)(m, status, vendor, toolName, toolVersion,
			messageID, domain);
	return(OK);
}

status_t
dstt_created_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *, char **))
{
	status_t	results = REJECT;
	char		*type;
	char		*path;
	char		**files;
	int		count = tt_message_args_count(m);
	int		i;
	
	type = tt_message_arg_type(m, 0);
	path = tt_message_file(m);
	files = (char **)malloc((count+1)*sizeof(char *));
	for(i = 0; i < count; i++)
	{
		files[i] = tt_message_arg_val(m, i);
	}
	files[i] = 0;
	(*cb)(m, type, path, files);
	return(OK);
}

status_t
dstt_deleted_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *, char **))
{
	status_t	results = REJECT;
	char		*type;
	char		*path;
	char		**files;
	int		count = tt_message_args_count(m);
	int		i;

	type = tt_message_arg_type(m, 0);
	path = tt_message_file(m);
	files = (char **)malloc((count+1)*sizeof(char *));
	for(i = 0; i < count; i++)
	{
		files[i] = tt_message_arg_val(m, i);
	}
	files[i] = 0;
	(*cb)(m, type, path, files);
	return(OK);
}

status_t
dstt_started_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *, char *))
{
	status_t	results = REJECT;
	char		*vendor;
	char		*toolName;
	char		*toolVersion;
	
	vendor = tt_message_arg_val(m, 0);
	toolName = tt_message_arg_val(m, 1);
	toolVersion = tt_message_arg_val(m, 2);
	(*cb)(m, vendor, toolName, toolVersion);
	return(OK);
}

status_t
dstt_stopped_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *, char *))
{
	status_t	results = REJECT;
	char		*vendor;
	char		*toolName;
	char		*toolVersion;
	
	vendor = tt_message_arg_val(m, 0);
	toolName = tt_message_arg_val(m, 1);
	toolVersion = tt_message_arg_val(m, 2);
	(*cb)(m, vendor, toolName, toolVersion);
	return(OK);
}

