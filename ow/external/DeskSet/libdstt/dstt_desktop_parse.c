/*
 *
 * dstt_desktop_parse.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_desktop_parse.c 1.15 93/01/05 Copyr 1990 Sun Micro";
#endif

#include <desktop/tt_c.h>
#include "dstt.h"
#include "dstt_vtype.h"
#include "ds_verbose_malloc.h"

status_t
dstt_get_status_param(Tt_message m, status_t (*cb)(Tt_message,
char **, char **, char **, char **, char *))
{
	char		*v[4];
	status_t	stat;
	
	stat = (*cb)(m, &v[0], &v[1], &v[2], &v[3],
		dstt_get_sopt(m, 0, VTYPE_MESSAGEID));
	tt_message_arg_val_set(m, 0, v[0]); if(v[0]) FREE(v[0]);
	tt_message_arg_val_set(m, 1, v[1]);
	tt_message_arg_val_set(m, 2, v[2]);
	tt_message_arg_val_set(m, 3, v[3]);

	return(stat);
}

status_t
dstt_set_environment_param(Tt_message m, status_t (*cb)(Tt_message,
char **, char **))
{
	int		count, i = 0;
	char		**var;
	char		**val;
	status_t	stat;

	count = tt_message_args_count(m);
	if(count & 1)
	{
		return(FAIL);
	}
	i = count/2 + 1;
	var = (char **)MALLOC(i*sizeof(char *));
	val = (char **)MALLOC(i*sizeof(char *));
	for(i = 0; i < count/2; i++)
	{
		var[i] = tt_message_arg_val(m, i*2);
		val[i] = tt_message_arg_val(m, i*2+1);
	}
	var[i] = 0;
	val[i] = 0;
	stat = (*cb)(m, var, val);
	FREE(var);
	FREE(val);
	return(stat);
}

status_t
dstt_get_environment_param(Tt_message m, status_t (*cb)(Tt_message,
char **, char **))
{
	int		count, i = 0;
	char		**var;
	char		**val;
	status_t	stat;

	count = tt_message_args_count(m);
	if(count & 1)
	{
		return(FAIL);
	}
	i = count/2 + 1;
	var = (char **)MALLOC(i*sizeof(char *));
	val = (char **)MALLOC(i*sizeof(char *));
	for(i = 0; i < count/2; i++)
	{
		var[i] = tt_message_arg_val(m, i*2);
		val[i] = (char *)0;
	}
	var[i] = (char *)0;
	val[i] = (char *)0;
	stat = (*cb)(m, var, val);
	for(i = 0; i < count/2; i++)
	{
		tt_message_arg_val_set(m, i*2+1, val[i]);
	}
	i = 0;
	while(var[i])
	{
		if(val[i]) FREE(val[i]);
		i++;
	}
	FREE(var);
	FREE(val);
	return(stat);
}

status_t 
dstt_set_geometry_param(Tt_message m, status_t (*cb)(Tt_message,
int *, int *, int *, int *, char *, char *))
{
	int	i;
	char	*type;
	int	w, *wa = 0, wi;
	int	h, *ha = 0, hi;
	int	x, *xa = 0, xi;
	int	y, *ya = 0, yi;
	char	*msg, mi;
	char	*buff, bi;
	int	count = tt_message_args_count(m);
	status_t	stat;
	Tt_state	state;

	for(i = 0; i < count; i++)
	{
		type = tt_message_arg_type(m, i);
		switch(*type)
		{
		case	'w':
			wi = i;
			tt_message_arg_ival(m, wi, &w);
			wa = &w;
			break;
		case	'h':
			hi = i;
			tt_message_arg_ival(m, hi, &h);
			ha = &h;
			break;
		case	'x':
			xi = i;
			tt_message_arg_ival(m, xi, &x);
			xa = &x;
			break;
		case	'y':
			yi = i;
			tt_message_arg_ival(m, yi, &y);
			ya = &y;
			break;
		case	'm':
			mi = i;
			msg = tt_message_arg_val(m, mi);
			break;
		case	'b':
			bi = i;
			buff = tt_message_arg_val(m, bi);
			break;
		}
	}
	stat = (*cb)(m, wa, ha, xa, ya, msg, buff);
	if(wa) tt_message_arg_ival_set(m, wi, *wa);
	if(ha) tt_message_arg_ival_set(m, hi, *ha);
	if(xa) tt_message_arg_ival_set(m, xi, *xa);
	if(ya) tt_message_arg_ival_set(m, yi, *ya);
	return(stat);
}

status_t
dstt_get_geometry_param(Tt_message m, status_t (*cb)(Tt_message,
int *, int *, int *, int *, char *, char *))
{
	int	i;
	char	*type;
	int	w, *wa = 0, wi;
	int	h, *ha = 0, hi;
	int	x, *xa = 0, xi;
	int	y, *ya = 0, yi;
	char	*msg, mi;
	char	*buff, bi;
	int	count = tt_message_args_count(m);
	status_t	stat;

	for(i = 0; i < count; i++)
	{
		type = tt_message_arg_type(m, i);
		switch(*type)
		{
		case	'w':
			wi = i;
			wa = &w;
			break;
		case	'h':
			hi = i;
			ha = &h;
			break;
		case	'x':
			xi = i;
			xa = &x;
			break;
		case	'y':
			yi = i;
			ya = &y;
			break;
		case	'm':
			mi = i;
			msg = tt_message_arg_val(m, mi);
			break;
		case	'b':
			bi = i;
			buff = tt_message_arg_val(m, bi);
			break;
		}
	}
	stat = (*cb)(m, wa, ha, xa, ya, msg, buff);
	if(wa) tt_message_arg_ival_set(m, wi, *wa);
	if(ha) tt_message_arg_ival_set(m, hi, *ha);
	if(xa) tt_message_arg_ival_set(m, xi, *xa);
	if(ya) tt_message_arg_ival_set(m, yi, *ya);
	return(stat);
}

status_t
dstt_set_iconified_param(Tt_message m, status_t (*cb)(Tt_message,
dstt_bool_t *, char *, char *))
{
	dstt_bool_t	bool;
	char		*msg;
	char		*buff;
	status_t	stat;

	tt_message_arg_ival(m, 0, (int *)(&bool));
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
	stat = (*cb)(m, &bool, msg, buff);
	tt_message_arg_ival_set(m, 0, (int)(bool));
	return(stat);
}

status_t
dstt_get_iconified_param(Tt_message m, status_t (*cb)(Tt_message,
dstt_bool_t *, char *, char *))
{
	dstt_bool_t	bool;
	char		*msg;
	char		*buff;
	status_t	stat;

	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
	stat = (*cb)(m, &bool, msg, buff);
	tt_message_arg_ival_set(m, 0, (int)(bool));
	return(stat);
}

status_t
dstt_set_locale_param(Tt_message m, status_t (*cb)(Tt_message,
char **, char **))
{
	int		count, i = 0;
	char		**var;
	char		**val;
	status_t	stat;

	count = tt_message_args_count(m);
	if(count & 1)
	{
		return(FAIL);
	}
	i = count/2 + 1;
	var = (char **)MALLOC(i*sizeof(char *));
	val = (char **)MALLOC(i*sizeof(char *));
	for(i = 0; i < count/2; i++)
	{
		var[i] = tt_message_arg_val(m, i*2);
		val[i] = tt_message_arg_val(m, i*2+1);
	}
	var[i] = 0;
	val[i] = 0;
	stat = (*cb)(m, var, val);
	FREE(var);
	FREE(val);
	return(stat);
}

status_t
dstt_get_locale_param(Tt_message m, status_t (*cb)(Tt_message,
char **, char **))
{
	int		count, i = 0;
	char		**var;
	char		**val;
	status_t	stat;

	count = tt_message_args_count(m);
	if(count & 1)
	{
		return(FAIL);
	}
	i = count/2 + 1;
	var = (char **)MALLOC(i*sizeof(char *));
	memset(var, '\0', i*sizeof(char *));
	val = (char **)MALLOC(i*sizeof(char *));
	memset(val, '\0', i*sizeof(char *));
	for(i = 0; i < count/2; i++)
	{
		var[i] = tt_message_arg_val(m, i*2);
		val[i] = (char *)0;
	}
	stat = (*cb)(m, var, val);
	for(i = 0; i < count/2; i++)
	{
		tt_message_arg_val_set(m, i*2+1, val[i]);
		if(val[i]) FREE(val[i]);
	}
	FREE(var);
	FREE(val);
	return(stat);
}

status_t
dstt_set_mapped_param(Tt_message m, status_t (*cb)(Tt_message,
dstt_bool_t *, char *, char *))
{
	dstt_bool_t	bool;
	char		*msg;
	char		*buff;
	status_t	stat;

	tt_message_arg_ival(m, 0, (int *)(&bool));
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
	stat = (*cb)(m, &bool, msg, buff);
	tt_message_arg_ival_set(m, 0, (int)(bool));
	return(stat);
}

status_t
dstt_get_mapped_param(Tt_message m, status_t (*cb)(Tt_message,
dstt_bool_t *, char *, char *))
{
	dstt_bool_t	bool;
	char		*msg;
	char		*buff;
	status_t	stat;

	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
	stat = (*cb)(m, &bool, msg, buff);
	tt_message_arg_ival_set(m, 0, (int)(bool));
	return(stat);
}

status_t
dstt_set_xinfo_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *, int *, char **, char **, char *))
{
	status_t	stat;
	char	*dsp, *vis, *msg;
	int	dep;
	int	count = 0;
	int	i = 0;
	int	n = 0;
	char	**list = 0;
	char	**values = 0;

	count = tt_message_args_count(m);
	dsp = tt_message_arg_val(m, 0);
	vis = tt_message_arg_val(m, 1);
	tt_message_arg_ival(m, 2, &dep);
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);

	if(msg) count--;
	n = (count-3)%2;
	
	list = (char **)MALLOC((n+1)*sizeof(char *));
	memset(list, '\0', (n+1)*sizeof(char *));
	values = (char **)MALLOC((n+1)*sizeof(char *));
	memset(values, '\0', (n+1)*sizeof(char *));
	
	for(i = 0; i < n; i++)
	{
		list[i] = tt_message_arg_val(m, 3+i*2);
		values[i] = tt_message_arg_val(m, 4+i*2);
	}
	list[i] = 0;
	values[i] = 0;

	stat = (*cb)(m, dsp, vis, &dep, list, values, msg);

	tt_message_arg_val_set(m, 0, dsp);
	tt_message_arg_val_set(m, 0, vis);
	tt_message_arg_ival_set(m, 0, dep);
	for(i = 0; i < n; i++)
	{
		tt_message_arg_val_set(m, 3+i*2, list[i]);
		tt_message_arg_val_set(m, 4+i*2, values[i]);
	}
	return(stat);
}

status_t
dstt_get_xinfo_param(Tt_message m, status_t (*cb)(Tt_message,
char **, char **, int *, char **, char **, char *))
{
	status_t	stat;
	char	*dsp, *vis, *msg;
	int	dep;
	int	count = 0;
	int	i = 0;
	int	n = 0;
	char	**list = 0;
	char	**values = 0;

	count = tt_message_args_count(m);
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);

	if(msg) count--;
	n = (count-3)%2;
	
	list = (char **)MALLOC((n+1)*sizeof(char *));
	memset(list, '\0', (n+1)*sizeof(char *));
	values = (char **)MALLOC((n+1)*sizeof(char *));
	memset(values, '\0', (n+1)*sizeof(char *));
	
	for(i = 0; i < n; i++)
	{
		list[i] = tt_message_arg_val(m, 3+i*2);
		values[i] = 0;
	}
	list[i] = 0;
	values[i] = 0;

	stat = (*cb)(m, &dsp, &vis, &dep, list, values, msg);

	tt_message_arg_val_set(m, 0, dsp); if(dsp) FREE(dsp);
	tt_message_arg_val_set(m, 1, vis); if(vis) FREE(vis);
	tt_message_arg_ival_set(m, 2, dep); 
	for(i = 0; i < n; i++)
	{
		tt_message_arg_val_set(m, 4+i*2, values[i]);
		if(values[i]) FREE(values[i]);
	}
	FREE(list);
	FREE(values);
	return(stat);
}

status_t
dstt_raise_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *))
{
	char		*msg;
	char		*buff;
	status_t	stat;

	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
	stat = (*cb)(m, msg, buff);
	return(stat);
}

status_t
dstt_lower_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char *))
{
	char		*msg;
	char		*buff;
	status_t	stat;

	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buff = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
	
	stat = (*cb)(m, msg, buff);
	return(stat);
}

status_t
dstt_do_command_param(Tt_message m, status_t (*cb)(Tt_message,
char *, char **, char *))
{
	char		*msg;
	char		*cmd;
	char		*results;
	status_t	stat;

	cmd = tt_message_arg_val(m, 0);
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	
	stat = (*cb)(m, cmd, &results, msg);
	tt_message_arg_val_set(m, 1, results);
	if(results) FREE(results);
	return(stat);
}

status_t
dstt_quit_param(Tt_message m, status_t (*cb)(Tt_message,
dstt_bool_t, dstt_bool_t, char *))
{
	dstt_bool_t	silent;
	dstt_bool_t	force;
	char		*msg;
	status_t	stat;

	tt_message_arg_ival(m, 0, (int *)(&silent));
	tt_message_arg_ival(m, 1, (int *)(&force));
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	
	stat = (*cb)(m, silent, force, msg);
	return(stat);
}

status_t
dstt_signal_param(Tt_message m, status_t (*cb)(Tt_message,
int))
{
	int		asig;
	char		*msg;
	status_t	stat;

	tt_message_arg_ival(m, 0, &asig);
	
	stat = (*cb)(m, asig);
	return(stat);
}

status_t
dstt_set_situation_param(Tt_message m, status_t (*cb)(Tt_message,
char *))
{
	char		*apath;
	status_t	stat;

	apath = tt_message_arg_val(m, 0);
	
	stat = (*cb)(m, apath);
	return(stat);
}

status_t
dstt_get_situation_param(Tt_message m, status_t (*cb)(Tt_message,
char **))
{
	char		*apath;
	status_t	stat;

	stat = (*cb)(m, &apath);
	tt_message_arg_val_set(m, 0, apath);
	if(apath) FREE(apath);
	return(stat);
}

status_t
dstt_deposit_param(Tt_message m, status_t (*cb)(Tt_message, char *,
Data_t, void *, int, char *, char *))
{
	status_t	results = REJECT;
	char		*file;
	char		*media = 0;
	void		*data = 0;
	int		size;
	char		*msg;
	char		*buffid;
	char		*xsel;
	
	media = tt_message_arg_type(m, 0);
	msg = dstt_get_sopt(m, 0, VTYPE_MESSAGEID);
	buffid = dstt_get_sopt(m, 0, VTYPE_BUFFERID);
#ifndef	PRIOR_493
	if((xsel = (char *)tt_message_context_val(m, VTYPE_X_SELECTION)) && 
			*xsel)
	{
		results = (*cb)(m, media, x_selection, xsel, strlen(xsel),
				buffid, msg);

	}
	else
#endif	/* NOTE! if NOT PRIOR_493 this is all one big if! */
	if(file = tt_message_file(m))
	{
		if(strchr(file, ':'))
		{
			dstt_set_status(m, dstt_status_file_not_avail);
			results = FAIL;
		}
		else
		{
			size = strlen(file);
			results = (*cb)(m, media,
					path, file, size, buffid, msg);
		}
	}
	else
	{
		tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);
		results = (*cb)(m, media, contents, data, size, buffid, msg);
	}

	return(results);
}

