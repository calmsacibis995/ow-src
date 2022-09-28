/*
 *
 * dstt_notice_request.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_notice_request.c 1.7 92/11/30 Copyr 1990 Sun Micro";
#endif

#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include "dstt_vtype.h"
#include "ds_verbose_malloc.h"

extern	void	*dstt_main_win(void);
extern	void (*dstt_vers_info)(char**, char**,char**);

static Tt_message
bld_msg()
{
	Tt_message	tt_msg;

	dstt_start_tt();
	tt_msg = tt_message_create();
	tt_message_address_set(tt_msg, TT_PROCEDURE);
	tt_message_class_set(tt_msg, TT_NOTICE);
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
	tt_message_destroy(tt_msg);	
	dstt_restore_tt();
}


int
dstt_created(char *type, char *path, ...)
{
	Tt_message	tt_msg;
	va_list ap ;
	char	*file;

	tt_msg = bld_msg();

	tt_message_op_set(tt_msg, "Created");
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_file_set(tt_msg, path);

	va_start(ap, path);

	while((file = va_arg(ap, char *)) != NULL)
	{
		tt_message_arg_add(tt_msg, TT_IN, type, file);
	}
	close_msg(tt_msg);
}

int
dstt_deleted(char *type, char *path, ...)
{
	Tt_message	tt_msg;
	va_list ap ;
	char	*file;

	tt_msg = bld_msg();

	tt_message_op_set(tt_msg, "Deleted");
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_file_set(tt_msg, path);

	va_start(ap, path);

	while((file = va_arg(ap, char *)) != NULL)
	{
		tt_message_arg_add(tt_msg, TT_IN, type, file);
	}
	close_msg(tt_msg);
}

int
dstt_opened(char *type, char *id)
{
	dstt_start_tt();
printf("Not done yet\n");
	dstt_restore_tt();
}

int
dstt_closed(char *type, char *id)
{
	dstt_start_tt();
printf("Not done yet\n");
	dstt_restore_tt();
}

int
dstt_modified(char *type, char *id)
{
	Tt_message	tt_msg;

	tt_msg = bld_msg();
	tt_message_op_set(tt_msg, "Modified");
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_arg_add(tt_msg, TT_IN, type, NULL);
	tt_message_file_set(tt_msg, id);
	close_msg(tt_msg);
}

int
dstt_reverted(char *type, char *id)
{
	dstt_start_tt();
	dstt_restore_tt();
}

int
dstt_moved(char *type, char *oldid, char *newid)
{
	dstt_start_tt();
printf("Not done yet\n");
	dstt_restore_tt();
}

int
dstt_saved(char *type, char *id)
{
	Tt_message	tt_msg;

	tt_msg = bld_msg();
	tt_message_op_set(tt_msg, "Saved");
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_arg_add(tt_msg, TT_IN, type, NULL);
	tt_message_file_set(tt_msg, id);
	close_msg(tt_msg);
}

int
dstt_started(void)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Started");
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	close_msg(tt_msg);
}

int
dstt_stopped(void)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Stopped");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	close_msg(tt_msg);
}

int
dstt_iconified(char *msgid, char *buffid)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Iconified");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	close_msg(tt_msg);
}

int
dstt_deiconified(char *msgid, char *buffid)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "DeIconified");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	close_msg(tt_msg);
}

int
dstt_mapped(char *msgid, char *buffid)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Mapped");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	close_msg(tt_msg);
}

int
dstt_unmapped(char *msgid, char *buffid)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "UnMapped");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	close_msg(tt_msg);
}

int
dstt_raised(char *msgid, char *buffid)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Raised");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	close_msg(tt_msg);
}

int
dstt_lowered(char *msgid, char *buffid)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Lowered");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(buffid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_BUFFERID, buffid);
	close_msg(tt_msg);
}

int
dstt_status(char *toolid, char *status, char *msgid, char *domain)
{
	Tt_message	tt_msg;
	char		*vend;
	char		*name;
	char		*ver;

	tt_msg = bld_msg();
	if(toolid)
	{
		tt_message_address_set(tt_msg, TT_HANDLER);
		tt_message_handler_set(tt_msg, toolid);
	}
	(*dstt_vers_info)(&vend, &name, &ver);
	tt_message_op_set(tt_msg, "Status");
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_STATUS, status);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_VENDOR, vend);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLNAME, name);
	tt_message_arg_add(tt_msg, TT_IN, VTYPE_TOOLVERSION, ver);
	if(msgid) tt_message_arg_add(tt_msg, TT_IN, VTYPE_MESSAGEID, msgid);
	if(domain) tt_message_arg_add(tt_msg, TT_IN, VTYPE_DOMAIN, domain);
	close_msg(tt_msg);
}

