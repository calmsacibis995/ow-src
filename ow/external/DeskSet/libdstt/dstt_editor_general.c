/*
 *
 * dstt_editor_general.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_editor_general.c 1.2 93/01/04 Copyr 1990 Sun Micro";
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

bufftype_t
dstt_get_bufftype(Tt_message m, int n)
{
	char	*vtype;

	vtype = tt_message_arg_type(m, n);
	if(strcmp(vtype, VTYPE_BUFFERID) == 0)
	{
		return(buffer);
	}
	else if(strcmp(vtype, VTYPE_VIEWID) == 0)
	{
		return(view);
	}
}

