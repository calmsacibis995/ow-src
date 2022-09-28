/*
 *
 * dstt_job.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_job.c 1.3 93/01/19 Copyr 1990 Sun Micro";
#endif

#include <string.h>
#include <desktop/tt_c.h>
#include "ds_verbose_malloc.h"

#ifdef  DEBUG
#define DP      if(1) 
#else
#define DP      if(0)
#endif


typedef struct list
{
	char			*op;
	char			*media;
	Tt_message_callback	cb;
	Tt_pattern		p;
}list_t;

list_t		*dsll_add_b(list_t *data, unsigned int size);
list_t		*dsll_init();
list_t		*dsll_next_e(list_t *data);
list_t		*dsll_del(list_t *data);

static	list_t	*list = 0;
static	int	count = 0;

static void
print_struct(list_t *item)
{
	printf("op = '%s' media='%s' cb=0x%x, pat=0x%X\n",
		item->op,
		item->media,
		item->cb,
		item->p);
}

static list_t *
find_match(char *name, char *media)
{
	list_t	*item;

	if(list == NULL)
	{
		list = dsll_init();
		return(NULL);
	}

	item = list;

	while((item = dsll_next_e(item)) != NULL)
	{
		if(strcmp(item->op, name) == 0)
		{
			if(item->media)
			{
				if(strcmp(item->media, media) == 0)
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
	return(item);
}

static void
del_item(list_t *item)
{
	Tt_pattern	p;

	if(item)
	{
		p = item->p;
		while(item->p)
		{
			p = tt_pattern_user(item->p, 2);
			tt_pattern_destroy(item->p);
			item->p = p;
		}
		if(item->op)free(item->op);
		if(item->media)free(item->media);
		free(item);
		dsll_del(item);
	}
}

void
dstt_pat_destroy(char *name, char *media)
{
	list_t	*item;

	DP printf("%d - destroying pattern for %s/%s\n",
			getpid(), name, media);

	item = find_match(name, media);

	del_item(item);
}

dstt_pat_add(char *name, char *media, Tt_message_callback cb, Tt_pattern p)
{
	list_t	*item;

	DP printf("%d - adding pattern for %s/%s\n",
				getpid(), name, media);
	item = find_match(name, media);

	if(item)
	{
		del_item(item);
	}
	item = dsll_add_b(list, sizeof(list_t));
	item->op = name?strdup(name):0;
	item->media = media?strdup(media):0;
	item->cb = cb;
	item->p = p;
}

Tt_message
dstt_match_pattern(Tt_message m)
{
	list_t	*item;
	char	*media;
	char	*name;

	media = tt_message_arg_type(m, 0);
	name = tt_message_op(m);
	item = find_match(name, media);

	DP printf("%d - finding pattern for %s/%s\n",
				getpid(), name, media);
	if(item)
	{
		(*item->cb)(m, item->p);
		return(0);
	}
	return(m);
}
