/*
 *
 * dstt_xview_desktop.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_xview_desktop.c 1.26 93/06/17 Copyr 1990 Sun Micro";
#endif

#include <xview/notify.h>
#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include <stropts.h>
#include <poll.h>
#include <locale.h>
#include "dstt_config.h"
#include "ds_verbose_malloc.h"

extern status_t dstt_get_status_def(...);
extern status_t dstt_set_environment_def(...);
extern status_t dstt_get_environment_def(...);
extern status_t dstt_set_geometry_def(...);
extern status_t dstt_get_geometry_def(...);
extern status_t dstt_set_iconified_def(...);
extern status_t dstt_get_iconified_def(...);
extern status_t dstt_set_mapped_def(...);
extern status_t	dstt_get_mapped_def(...);
extern status_t dstt_set_xinfo_def(...);
extern status_t	dstt_get_xinfo_def(...);
extern status_t dstt_set_locale_def(...);
extern status_t	dstt_get_locale_def(...);
extern status_t dstt_raise_def(...);
extern status_t dstt_lower_def(...);
extern status_t dstt_do_command_def(...);
extern status_t	dstt_signal_def(...);
extern status_t	dstt_set_situation_def(...);
extern status_t	dstt_get_situation_def(...);

extern status_t dstt_get_status_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_environment_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_environment_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_geometry_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_geometry_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_iconified_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_iconified_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_mapped_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_mapped_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_xinfo_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_xinfo_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_locale_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_locale_param(Tt_message, status_t (*)(...));
extern status_t dstt_raise_param(Tt_message, status_t (*)(...));
extern status_t dstt_lower_param(Tt_message, status_t (*)(...));
extern status_t dstt_do_command_param(Tt_message, status_t (*)(...));
extern status_t dstt_quit_param(Tt_message, status_t (*)(...));
extern status_t dstt_signal_param(Tt_message, status_t (*)(...));
extern status_t dstt_set_situation_param(Tt_message, status_t (*)(...));
extern status_t dstt_get_situation_param(Tt_message, status_t (*)(...));
extern status_t dstt_deposit_param(Tt_message, status_t (*)(...));

static Msg_t	msg_list[] =
{
{GET_STATUS,		dstt_get_status_param,		dstt_get_status_def				,1},
{SET_ENVIRONMENT,	dstt_set_environment_param,	dstt_set_environment_def	,1},
{GET_ENVIRONMENT,	dstt_get_environment_param,	dstt_get_environment_def	,1},
{SET_GEOMETRY,		dstt_set_geometry_param,	dstt_set_geometry_def		,1},
{GET_GEOMETRY,		dstt_get_geometry_param,	dstt_get_geometry_def		,1},
{SET_ICONIFIED,		dstt_set_iconified_param,	dstt_set_iconified_def		,1},
{GET_ICONIFIED,		dstt_get_iconified_param,	dstt_get_iconified_def		,1},
{SET_MAPPED,		dstt_set_mapped_param,		dstt_set_mapped_def		,1},
{GET_MAPPED,		dstt_get_mapped_param,		dstt_get_mapped_def		,1},
{SET_XINFO,		dstt_set_xinfo_param,		dstt_set_xinfo_def		,1},
{GET_XINFO,		dstt_get_xinfo_param,		dstt_get_xinfo_def		,1},
{SET_LOCALE,		dstt_set_locale_param,		dstt_set_locale_def		,1},
{GET_LOCALE,		dstt_get_locale_param,		dstt_get_locale_def		,1},
{RAISE,			dstt_raise_param,		dstt_raise_def			,1},
{LOWER,			dstt_lower_param,		dstt_lower_def			,1},
{DO_COMMAND,		dstt_do_command_param,		NULL				,0},
{QUIT, 			dstt_quit_param,		NULL			,0},
{SIGNAL, 		dstt_signal_param,		dstt_signal_def			,1},
{SET_SITUATION, 	dstt_set_situation_param,	dstt_set_situation_def		,1},
{GET_SITUATION, 	dstt_get_situation_param,	dstt_get_situation_def		,1},
{DEPOSIT,		dstt_deposit_param,		NULL				,0}
};

#define	MSG_LIST_SIZE	(sizeof(msg_list)/sizeof(Msg_t))

static	Tt_message	save_msg[] = {0, 0, 0, 0, 0};
static	Message_group_t	*this_group = 0;
void (*dstt_vers_info)(char**, char**,char**);
static	int	ddvl = 0;
static	char	*argname[] = {"-display", "-visual", "-depth", "-lc_basiclocale"};
static	char	*newargs[40];
static	int	argcount = 0;

static	char	*program_name = 0;
int	dstt_ttfd = -1;

Xv_opaque
dstt_main_win()
{
	return((Xv_opaque)((this_group->sets)[1])->key);
}

char *
progname()
{
	return(program_name);
}

Message_set_t *
dstt_xview_find(void *key, int create, Message_group_t *group)
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
		msg->key = key;
		return(msg);
	}
	return((group->sets)[i]);
}

void *
dstt_xview_desktop_get_key(Tt_message m)
{
	return((*(this_group->sets)[1]).key);
}

void
dstt_xview_rm(void *key)
{
}

Tt_pattern
dstt_xview_desktop_pattern(void *key)
{
	Tt_pattern	pat = tt_pattern_create();

	tt_pattern_scope_add(pat, TT_SESSION);
	tt_pattern_session_add(pat, tt_default_session());
	tt_pattern_category_set(pat, TT_OBSERVE);
	return(pat);
}

static int
xinfo(Tt_message m, void *args, int ret_status,
		char *display, char *visual, int depth,
		char **nm, char **val, char *msgid)
{
	char	**new = (char **)args;
	char	buff[20];

	/*
	 * lc_basiclocale	LC_CTYPE
	 * lc_inputlang		LC_CTYPE
	 * lc_displaylang	LC_MESSAGES
	 * lc_timeformat	LC_TIME
	 * lc_numeric		LC_NUMERIC
	 */

	switch(tt_message_state(m))
	{
	case	TT_HANDLED:
		if(display)
		{
			new[argcount++] = "-display";
			new[argcount++] = (char *)DS_STRDUP(display);
		}
		if(visual)
		{
			new[argcount++] = "-visual";
			new[argcount++] = (char *)DS_STRDUP(visual);
		}
		if(depth > 0)
		{
			new[argcount++] = "-depth";
			sprintf(buff, "%d", depth);
			new[argcount++] = (char *)DS_STRDUP(buff);
		}
		break;
	default:
		DP printf("%d - tt_message_state = %d\n", getpid(), tt_message_state(m));
	}
	ddvl |= 1;
	return(0);
}


static int
locale_cb(Tt_message m, void *args, int ret_status,
		char **cat, char **locale)
{
	char	**new = (char **)args;
	int	i = 0;

	for(i = 0; cat[i]; i++)
	{
		if(locale[i])
		{
			if(strcmp(cat[i], "LC_CTYPE") == 0)
			{
				new[argcount++] = "-lc_basiclocale";
				new[argcount++] = (char *)DS_STRDUP(locale[i]);
				new[argcount++] = "-lc_inputlang";
				new[argcount++] = (char *)DS_STRDUP(locale[i]);
			}
			else if(strcmp(cat[i], "LC_TIME") == 0)
			{
				new[argcount++] = "-lc_timeformat";
				new[argcount++] = (char *)DS_STRDUP(locale[i]);
			}
			else if(strcmp(cat[i], "LC_NUMERIC") == 0)
			{
				new[argcount++] = "-lc_numeric";
				new[argcount++] = (char *)DS_STRDUP(locale[i]);
			}
			else if(strcmp(cat[i], "LC_MESSAGES") == 0)
			{
				new[argcount++] = "-lc_displaylang";
				new[argcount++] = (char *)DS_STRDUP(locale[i]);
			}
		}
	}
	ddvl |= 2;
	return(0);
}

Tt_status
dstt_check_startup(void (*vers_info)(char**, char**,char**),
int *argc, char ***argv)
{
	char	*loc[5];
	Tt_message	m;
	struct	pollfd	myfds;
	char	**tmp, **str = (*argv);
	int	i=0, n, j = 0;
	char	buff[10];
	Tt_status	status;
	time_t		start_time;
	time_t		timeout = 5;	/* a 5 sec time out value */

	status = dstt_start_tt();
	if(status != TT_OK)
	{
		return(status);
	}

	program_name = *argv[0];

	dstt_vers_info = vers_info;
	save_msg[0] = tt_message_receive();
	dstt_prnt_msg(save_msg[0], "in dstt_check_startup after tt_message_receive");
 	if(TT_WRN_START_MESSAGE == tt_message_status(save_msg[0]))
	{
#ifndef	PRIOR_493
		tt_message_accept(save_msg[0]);
#endif
		dstt_get_xinfo(xinfo, newargs,
				tt_message_sender(save_msg[0]),
				NULL, NULL);
		loc[0] = "LC_CTYPE";
		loc[1] = "LC_TIME";
		loc[2] = "LC_NUMERIC";
		loc[3] = "LC_MESSAGES";
		loc[4] = NULL;
		dstt_get_locale(locale_cb, newargs,
				tt_message_sender(save_msg[0]),
				loc);
		/* poll for messages */
		myfds.fd = tt_fd();
		myfds.events = POLLIN;
		start_time = time(0);
		while(ddvl != 3 && (time(0) - start_time) < timeout)
		{
			poll(&myfds, 1, timeout*1000);
			m = dstt_msg_receive(0);
			if(m)
			{
				save_msg[i] = m;
			}
		}
		
		n = *argc+argcount;
		tmp = (char **)malloc((n+1)*sizeof(char *));
		for(i = 0 ; i < *argc; i++)
		{
			tmp[i] = str[i];
		}
		for(j = 0; i < n; i++,j++)
		{
			tmp[i] = newargs[j];
		}
		tmp[i] = 0;
		*argc = n;
		*argv = tmp;
	}
	dstt_restore_tt();
	return(TT_OK);
}

Notify_value
dstt_xview_receive(Notify_client client, int fd)
{
	dstt_start_tt();
	dstt_msg_receive(0);
	dstt_restore_tt();
	return(NOTIFY_DONE);
}

int
dstt_xview_start_notifier()
{
	int		i = 0, j = 0;
	Tt_message	m;
	Tt_status	rc = TT_OK;

	dstt_oldtt_callback(NULL, NULL);
	dstt_oldtt_register(NULL, NULL);

	dstt_start_tt();
	dstt_ttfd = tt_fd();
	rc = tt_int_error(dstt_ttfd);
	if(rc != TT_OK)
	{
		return(rc);
	}

	notify_set_input_func((Notify_client)(&dstt_ttfd), 
		dstt_xview_receive, dstt_ttfd);
	while(save_msg[i])
	{
		m = dstt_msg_receive(save_msg[i]);
		if(m)
		{
			save_msg[j] = m;
			j++;
		}
		save_msg[i] = (Tt_message)0;
		i++;
	}
	dstt_restore_tt();
}

int
dstt_xview_desktop_callback(Xv_opaque frame, ...)
{
	va_list ap ;

	dstt_start_tt();
	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_xview_find,
			dstt_xview_desktop_get_key,
			dstt_xview_rm,
			dstt_xview_desktop_pattern);
	}
	va_start(ap, frame);
	dstt_callback_register(this_group, (void *)frame, &ap);
	dstt_restore_tt();
}

int
dstt_xview_desktop_register(Xv_opaque frame, ...)
{
	va_list ap ;
	int	stat = 0;

	dstt_start_tt();
	if(this_group == 0)
	{
		this_group = dstt_config_group(msg_list,
			MSG_LIST_SIZE,
			dstt_xview_find,
			dstt_xview_desktop_get_key,
			dstt_xview_rm,
			dstt_xview_desktop_pattern);
	}
	va_start(ap, frame);
	stat = dstt_handle_register(this_group, (void *)frame, &ap);
	dstt_restore_tt();
	return(stat);
}
