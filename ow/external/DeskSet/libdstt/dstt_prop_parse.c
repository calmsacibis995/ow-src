/*
 *
 * dstt_prop_parse.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_prop_parse.c 1.3 93/01/04 Copyr 1990 Sun Micro";
#endif

#include <desktop/tt_c.h>
#include "dstt.h"
#include "dstt_vtype.h"
#include "ds_verbose_malloc.h"

status_t
dstt_prop_edit_param(Tt_message m, status_t (*cb)(Tt_message,
			char *, void *, int,
			void *, int,
			void *, int,
			char *, char *))
{
	status_t	results = REJECT;
	char		*prop = 0;
	void		*data = 0;
	int		size = 0;

	void		*icon1 = 0;
	int		isize1 = 0;
	void		*icon2 = 0;
	int		isize2 = 0;

	char		*msg = 0;
	char		*title = 0;
	
	prop = tt_message_arg_type(m, 0);
	tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);

	tt_message_arg_bval(m, 1, (unsigned char **)&icon1, &isize1);
	tt_message_arg_bval(m, 2, (unsigned char **)&icon2, &isize2);

	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	title = dstt_get_sopt(m, 0, VTYPE_TITLE);

	results = (*cb)(m, prop, data, size, icon1, isize1, icon2, isize2,
			msg, title);

	return(results);
}

