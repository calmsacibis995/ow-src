/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)dstt_audioctl_parse.c	1.3	93/02/22 SMI"

#include <desktop/tt_c.h>
#include "dstt.h"
#include "dstt_vtype.h"

status_t
dstt_audioctl_param(Tt_message m, 
		    status_t (*cb)(Tt_message,
				   char *, void *, int,
				   char *,
				   char *, char *))
{
	status_t	results = REJECT;
	char		*prop = 0;
	void		*data = 0;
	int		size = 0;

	char		*msg = 0;
	char		*title = 0;
	char		*devname = 0;
	
	prop = tt_message_arg_type(m, 0);
	tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);

	devname = tt_message_arg_val(m, 1);
	    
	msg = dstt_get_sopt(m, 2, VTYPE_MESSAGEID);
	title = dstt_get_sopt(m, 2, VTYPE_TITLE);

	results = (*cb)(m, prop, data, size, devname, msg, title);

	return (results);
}

