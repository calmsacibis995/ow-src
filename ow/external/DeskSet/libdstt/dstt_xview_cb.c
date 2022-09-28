/*
 *
 * dstt_xview_cb.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_xview_cb.c 1.10 92/10/29 Copyr 1990 Sun Micro";
#endif

#include <xview/xview.h>
#include <desktop/tt_c.h>
#include "dstt.h"
#include <string.h>
#include <locale.h>
#include <unistd.h>
#include "ds_verbose_malloc.h"

#ifdef	DEBUG
#define	DP	if(1) 
#else
#define	DP	if(0)
#endif

char	*locales[] =
{
	"LC_CTYPE",
	"LC_NUMERIC",
	"LC_TIME",
	"LC_COLLATE",
	"LC_MONETARY",
	"LC_MESSAGES",
	"LC_ALL"
};

char	*visuals[] =
{
	"StaticGray",
	"GrayScale",
	"StaticColor",
	"PseudoColor",
	"TrueColor",
	"DirectColor"
};

void (*dstt_vers_info)(char**, char**,char**);

status_t
dstt_get_status_def(Tt_message m, char **status, char **vendor, char **tool,
	char **version, char *msgid)
{
	(*dstt_vers_info)(vendor, tool, version);
	*status = DS_STRDUP("Running");
	return(OK);
}


status_t
dstt_set_environment_def(Tt_message m, char **variable, char **value)
{
	char	*buf;
	int	i = 0;

	while(variable[i])
	{
		buf = MALLOC(strlen(variable[i])+strlen(value[i])+2);
		sprintf(buf, "%s=%s", variable[i], value[i]);
		putenv(buf);
		i++;
	}
	return(OK);
}

status_t
dstt_get_environment_def(Tt_message m, char **variable, char **value)
{
	int	i = 0;
	char	*val;

	while(variable[i])
	{
		if(val = getenv(variable[i]))
		{
			value[i] = DS_STRDUP(val);
		}
		i++;
	}
	return(OK);
}

status_t 
dstt_set_geometry_def(Tt_message m, int *width, int *height, int *xOffset, int *yOffset, char *msg, char *buff)
{
	Rect	rect;

	frame_get_rect(dstt_main_win(), &rect);
	if(xOffset) rect.r_left = *xOffset;
	if(yOffset) rect.r_top = *yOffset;
	if(width) rect.r_width = *width;
	if(height) rect.r_height = *height;
	frame_set_rect(dstt_main_win(), &rect);

	frame_get_rect(dstt_main_win(), &rect);
	if(xOffset) *xOffset = rect.r_left;
	if(yOffset) *yOffset = rect.r_top;
	if(width) *width = rect.r_width;
	if(height) *height = rect.r_height;

	return(OK);
}

status_t
dstt_get_geometry_def(Tt_message m, int *width, int *height, int *xOffset, int *yOffset, char *msg, char *buff)
{
	Rect	rect;

	frame_get_rect(dstt_main_win(), &rect);
	if(xOffset) *xOffset = rect.r_left;
	if(yOffset) *yOffset = rect.r_top;
	if(width) *width = rect.r_width;
	if(height) *height = rect.r_height;

	return(OK);
}

status_t
dstt_set_iconified_def(Tt_message m, dstt_bool_t *iconified, char *msg, char *buffid)
{
	xv_set(dstt_main_win(), FRAME_CLOSED, *iconified, NULL);
	*iconified = xv_get(dstt_main_win(), FRAME_CLOSED);
	return(OK);
}

status_t
dstt_get_iconified_def(Tt_message m, dstt_bool_t *iconified, char *msg, char *buffid)
{
	*iconified = xv_get(dstt_main_win(), FRAME_CLOSED);
	return(OK);
}

status_t
dstt_set_locale_def(Tt_message m, char **variable, char **value)
{
	int	i = 0;
	int	j;

	while(variable[i])
	{
		j = 0;
		while(locales[j] && strcmp(locales[j], variable[i]));
		if(locales[j])
		{
			setlocale(j, value[i]);
		}
		i++;
	}
	return(OK);
}

status_t
dstt_get_locale_def(Tt_message m, char **variable, char **value)
{
	int	i = 0;
	int	j;

	while(variable[i])
	{
		j = 0;
		while(locales[j] && strcmp(locales[j], variable[i]))j++;
		if(locales[j])
		{
			value[i] = DS_STRDUP(setlocale(j, NULL));
		}
		i++;
	}
	return(OK);
}

status_t
dstt_set_mapped_def(Tt_message m, dstt_bool_t *mapped, char *msg, char *buff)
{
	xv_set(dstt_main_win(), XV_SHOW, *mapped, NULL);
	*mapped = xv_get(dstt_main_win(), XV_SHOW);
	return(OK);	
}

status_t
dstt_get_mapped_def(Tt_message m, dstt_bool_t *mapped, char *msg, char *buff)
{
	*mapped = xv_get(dstt_main_win(), XV_SHOW);
	return(OK);	
}

status_t
dstt_set_xinfo_def(Tt_message m,
char *display, char *visual, int *depth, char **name, char **value, char *msg)
{
	return(FAIL);
}

status_t
dstt_get_xinfo_def(Tt_message m,
char **display, char **visual, int *depth, char **name, char **value, char *msg)
{
	int	vis;
	Display	*xdisp;


	xdisp = (Display *)xv_get(dstt_main_win(), XV_DISPLAY);
	*display = DS_STRDUP(DisplayString(xdisp));

	vis = xv_get(dstt_main_win(), XV_VISUAL_CLASS);
	*visual = DS_STRDUP(visuals[vis]);

	*depth = xv_get(dstt_main_win(), XV_DEPTH);

	return(OK);
}

status_t
dstt_lower_def(int which)
{
	return(FAIL);
}

status_t
dstt_raise_def(int which)
{
	if(which)
	{
		xv_set(dstt_main_win(), WIN_FRONT, NULL);
	}
	return(OK);
}

status_t
command_do_def(Tt_message m, char *command, char **output, char *msg)
{
	return(FAIL);	
}

status_t
dstt_quit_def(Tt_message m, dstt_bool_t silent, dstt_bool_t force, char *msg)
{
	xv_destroy_safe(dstt_main_win());
	return(OK);	
}

status_t
dstt_signal_def(Tt_message m, int sig)
{
	status_t	stat = FAIL;

	if(kill(getpid(), sig) == 0)
	{
		stat = OK;
	}
	return(stat);
}

status_t
dstt_set_situation_def(Tt_message m, char *situation)
{
	status_t	stat = FAIL;

	if(chdir(situation) == 0)
	{
		stat = OK;
	}
	return(stat);
}

status_t
dstt_get_situation_def(Tt_message m, char **situation)
{
	status_t	stat = FAIL;

	if((*situation = getcwd(NULL, 1024)) != 0)
	{
		stat = OK;
	}
	return(stat);
}

