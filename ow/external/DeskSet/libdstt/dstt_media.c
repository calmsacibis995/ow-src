/*
 *
 * dstt_media.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_media.c 1.5 95/05/22 Copyr 1990 Sun Micro";
#endif

#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include "ds_verbose_malloc.h"
#include "dstt.h"

#ifdef  DEBUG
#define DP      if(1) 
#else
#define DP      if(0)
#endif

typedef	int	(*func_t)(...);
typedef	struct	e_cb_list
{
	char		*media;
	func_t		cb;
}cb_list_t;

static	cb_list_t	*display_list = NULL;
static	cb_list_t	*edit_list = NULL;

cb_list_t		*dsll_add_b(cb_list_t *data, unsigned int size);
cb_list_t		*dsll_init();
cb_list_t		*dsll_next_e(cb_list_t *data);

void
cb_print(cb_list_t *item)
{
	printf("%s(0x%X)\n", item->media, item->cb);
}

int
dstt_editor_callback(char *media, ...)
{
	va_list ap ;
	char		*cmd = 0;
	cb_list_t	*item;
	cb_list_t	*list;

	if(edit_list == NULL)
	{
		edit_list = dsll_init();
	}
	if(display_list == NULL)
	{
		display_list = dsll_init();
	}
	va_start(ap, frame);
	while(cmd = va_arg(ap, char *))
	{
		if(strcmp(cmd, EDIT) == 0)
		{
			list = edit_list;
		}
		else if(strcmp(cmd, DISPLAY) == 0)
		{
			list = display_list;
		}
		item = list;
		while((item = dsll_next_e(item)) != NULL)
		{
			if(strcmp(item->media, media) == 0)
			{
				break;
			}
		}
		if(item == NULL)
		{
			item = dsll_add_b(list, sizeof(cb_list_t));
			item->media = strdup(media);
		}
		item->cb = va_arg(ap, func_t);
	}

	return(0);
}

int
dstt_editor_register(char *media, ...)
{
	va_list		ap ;
	char		*cmd = 0;
	Tt_pattern	pat;
	cb_list_t	*item;

	va_start(ap, media);
	while(cmd = va_arg(ap, char *)) {
		if(cmd != NULL && strcmp(cmd, EDIT) == 0) {
			item = edit_list;
			while((item = dsll_next_e(item)) != NULL)
				if(strcmp(item->media, media) == 0)
					break;

			if (item) {
				if (va_arg(ap, int))
					dstt_handle_edit((Edit_CB *)
						item->cb, NULL, media, NULL, NULL, NULL);
				else
					dstt_handle_edit(NULL, NULL,
						 media, NULL, NULL, NULL);
			}
#if !defined(__ppc)
			else va_arg(ap, int);
#else
			else {
				int tmpint;
				tmpint = va_arg(ap, int);
			}
#endif /* __ppc */
		}
		else if (cmd != NULL && strcmp(cmd, DISPLAY) == 0) {
			item = display_list;
			while((item = dsll_next_e(item)) != NULL)
				if(strcmp(item->media, media) == 0)
					break;

			if (item) {
				if (va_arg(ap, int))
					dstt_handle_display((Display_CB *)item->cb,
						 NULL,media, NULL, NULL, NULL);
				else
					dstt_handle_display(NULL, NULL, 						media, NULL, NULL, NULL);
			}
#if !defined(__ppc)
			else va_arg(ap, int);
#else
			else {
				int tmpint;
				tmpint = va_arg(ap, int);
			}
#endif /* __ppc */
		}
	}
	return(0);
}

