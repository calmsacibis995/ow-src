/*
 *
 * dstt_general.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_general.c 1.32 95/11/01 Copyr 1990 Sun Micro";
#endif

#include <string.h>
#include <desktop/tt_c.h>
#include <stdarg.h>
#include "dstt_config.h"
#include "ds_verbose_malloc.h"

#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

#define	EXTRA	5
void	dstt_prnt_msg(Tt_message, char *);
extern	Tt_message	dstt_match_pattern(Tt_message m);
static	Message_group_t	**msg_groups = 0;
static	int		msg_group_count = 0;
static	char		*old_procid = 0;
static	char		*procid = 0;

extern	Notify_value dstt_xview_receive(Notify_client, int);
extern	int	dstt_ttfd;

typedef	status_t cb_t(...);
char *
dstt_get_sopt(Tt_message m, int i, char *name)
{
	int	count = tt_message_args_count(m);
	char	*type = 0;
	char	*contents = 0;

	for(; !contents && i < count; i++)
	{
		type = tt_message_arg_type(m, i);
		if(type && strcmp(name, type) == 0)
		{
			contents = tt_message_arg_val(m, i);
		}
	}
	return(contents);
}

int
dstt_get_iopt(Tt_message m, int i, char *name)
{
	int	count = tt_message_args_count(m);
	char	*type = 0;
	int	contents;

	for(; i < count; i++)
	{
		type = tt_message_arg_type(m, i);
		if(type && strcmp(name, type) == 0)
		{
			tt_message_arg_ival(m, i, &contents);
		}
	}
	return(contents);
}


Message_set_t *
dstt_config_set(Msg_t *list, int count, Message_group_t *group)
{
	Message_set_t	*new_set;
	Message_set_t	**sets;
	Message_set_t	*grp;
	Message_set_t	**tmp;
	int		i;

	new_set = (Message_set_t *)MALLOC(sizeof(Message_set_t));
	memset(new_set, '\0', sizeof(Message_set_t));
	new_set->key = 0;
	new_set->pat = 0;
	new_set->count = count;
	new_set->list = (Msg_t *)MALLOC(count*sizeof(Msg_t));
	memcpy(new_set->list, list, count*sizeof(Msg_t));

	sets = group->sets;
	for(i = 0; sets && i < group->count; i++)
	{
		if(sets[i] == (Message_set_t *)0)
		{
			break;
		}
	}
	if(i == group->count)
	{
		group->count += EXTRA;
		tmp = (Message_set_t **)
			MALLOC((group->count+EXTRA)*sizeof(Message_set_t));
		memset(tmp, '\0', (group->count+EXTRA)*sizeof(Message_set_t));
		memcpy(tmp, group->sets, i*sizeof(Message_set_t));
		if(group->sets)FREE(group->sets);
		group->sets = tmp;
	}
	group->sets[i] = new_set;
	return(new_set);
}

Message_group_t	*
dstt_config_group(Msg_t *list, int count,
	Message_set_t *(*find)(void *, int, Message_group_t *),
	void *(get_key)(Tt_message),
	void (*rm)(void *),
	Tt_pattern (*pattern)(void *key))
{
	Message_group_t	*new_group;
	Message_group_t	**tmp;
	int		i;

	new_group = (Message_group_t *)MALLOC(sizeof(Message_group_t));
	memset(new_group, '\0', sizeof(Message_group_t));

	for(i = 0; i < msg_group_count; i++)
	{
		if(msg_groups[i] == (Message_group_t *)0)
		{
			break;
		}
	}
	if(i == msg_group_count)
	{
		msg_group_count += EXTRA;
		tmp = (Message_group_t **)
			MALLOC((msg_group_count)*sizeof(Message_group_t));
		memset(tmp, '\0', (msg_group_count)*sizeof(Message_group_t));
		memcpy(tmp, msg_groups, i*sizeof(Message_set_t));
		if(msg_groups)FREE(msg_groups);
		msg_groups = tmp;
	}
	msg_groups[i] = new_group;



	new_group->find = find;
	new_group->rm = rm;
	new_group->pattern = pattern;
	new_group->get_key = get_key;
	new_group->count = 0;
	new_group->sets = 0;

	new_group->sets[0] = dstt_config_set(list, count, new_group);
	return(new_group);
}

int
dstt_callback_register(Message_group_t *group, void *key, va_list *varptr)
{
#if defined(__ppc)
	va_list		ap;
#else
	va_list		ap = *varptr;
#endif /* __ppc */
	int		i = 0;
	char		*type;
	Message_set_t	*msgs;
#if defined(__ppc)
	va_copy( ap, *varptr );
#endif /* __ppc */

 	if((msgs = (*group->find)(key, TRUE, group)) == NULL)
	{
		return(FALSE);
	}
	while((type = va_arg(ap, char *)) != NULL)
	{
		for(i = 0; i < msgs->count; i++)
		{
			if(strcmp(msgs->list[i].name, type) == 0)
			{
				msgs->list[i].cb = (cb_t *)va_arg(ap, void *);
				break;
			}
		}
		if(i >= msgs->count)
		{
			return(FALSE);
		}
	}
        va_end(ap) ;
	return(TRUE);
}


int
dstt_handle_register(Message_group_t *group, void *key, va_list *varptr)
{
#if defined(__ppc)
	va_list		ap;
#else
	va_list		ap = *varptr;
#endif /* __ppc */
	int		i = 0;
	char		*type;
	Message_set_t	*msgs;
	int		status = TRUE;
#if defined(__ppc)
	va_copy( ap, *varptr );
#endif /* __ppc */

	if((msgs = (*group->find)(key, FALSE, group)) == NULL)
	{
		status = FALSE;
	}
	while(status && ((type = va_arg(ap, char *)) != NULL))
	{
		for(i = 0; i < msgs->count; i++)
		{
			if(strcmp(msgs->list[i].name, type) == 0)
			{
				msgs->list[i].handle = va_arg(ap, int);
				break;
			}
		}
		if(i >= msgs->count)
		{
			status = FALSE;
		}
	}
        va_end(ap) ;

	if(status && msgs->pat)
	{
		DP printf("destroy pattern\n");
		tt_pattern_destroy(msgs->pat);
		msgs->pat = 0;
	}
	else if(status)
	{
		dstt_start_tt();
	}


	for(i = 0; status && i < msgs->count; i++)
	{
		if(msgs->list[i].handle && msgs->list[i].cb)
		{
			if(!msgs->pat && tt_ptr_error(msgs->pat =
					 (*group->pattern)(key)) != TT_OK)
			{
				msgs->pat = 0;
				status = FALSE; 
			}
			if(status && (tt_pattern_op_add(msgs->pat,
					msgs->list[i].name) != TT_OK))
			{
				DP printf("destroy pattern\n");
				tt_pattern_destroy(msgs->pat);
				msgs->pat = 0;
				status = FALSE; 
			}
		}
	}

	if(status)
	{
		tt_pattern_register(msgs->pat);
	}
	return(status);
}

Tt_message
dstt_tt_message_receive()
{
	return(dstt_msg_receive(NULL));
}

Tt_message
dstt_msg_receive(Tt_message save_msg)
{
	Tt_message	m = 0;
	char		*op = 0;
	int		i, g;
	status_t	rc = REJECT;
	Tt_status	ttrc = TT_OK;
	Message_set_t	*msgs;
	void		*key;
	Message_group_t	*group;
	int		found = 0;

	if(save_msg == 0)
	{
		m = tt_message_receive();
		DP dstt_prnt_msg(m, "after tt_message_receive");
		if(m == 0)
		{
			return(m);
		}
		ttrc = tt_ptr_error(m);
		if(ttrc == TT_ERR_NOMP)
		{
#ifdef	USE_DEBUGGER
			tt_default_session_set(tt_X_session(getenv("DISPLAY")));
#endif	USE_DEBUGGER
			/* xview specific but . . . */
			tt_close();
			DP printf("1 dstt_ttfd = %d\n", dstt_ttfd);
			notify_set_input_func((Notify_client)(&dstt_ttfd), 
				NOTIFY_FUNC_NULL, dstt_ttfd);
			procid = tt_open();
			ttrc = tt_ptr_error(procid);
			if(ttrc != TT_OK)
			{
				return(m);
			}
			dstt_ttfd = tt_fd();
			ttrc = tt_int_error(dstt_ttfd);
			if(ttrc != TT_OK)
			{
				printf("ttrc = %X\n", ttrc);
				return(m);
			}
			DP printf("2 dstt_ttfd = %d\n", dstt_ttfd);
			notify_set_input_func((Notify_client)(&dstt_ttfd), 
				dstt_xview_receive, dstt_ttfd);

			DP printf("3 dstt_ttfd = %d\n", dstt_ttfd);
			return(NULL);
		}
		else if(ttrc != TT_OK)
		{
			return(m);
		}
	}
	else
	{
		m = save_msg;
		save_msg = 0;
		DP dstt_prnt_msg(m, "saved msg");
	}


	if(msg_groups == 0)
	{
		return(m);
	}

	op = tt_message_op(m);
	if((m = dstt_match_pattern(m)) == 0)
	{
		return(m);
	}
	for(g = 0; !found && msg_groups[g]; g++)
	{
		group = msg_groups[g];
		msgs = (group->sets)[0];
		for(i = 0; !found && i < msgs->count; i++)
		{
			if(!strcmp(op, msgs->list[i].name))
			{
				found++;
				break;
			}
		}
	}
	if(!found)
	{
		DP printf("%d - message(Not Found)\n", getpid());
		return(m);
	}

	key = (*group->get_key)(m);
	msgs = (*group->find)(key, FALSE, group);
	for(i = 0; i < msgs->count; i++)
	{
		if(!strcmp(op, msgs->list[i].name))
		{
			if(msgs->list[i].handle && msgs->list[i].cb)
			{
				rc = (*msgs->list[i].param)(m, 
						msgs->list[i].cb);
			}
			else
			{
				rc = REJECT;
			}
			break;
		}
	}
	switch(rc)
	{
	case	OK:
		DP printf("%d - Reply to message(0x%X)\n", getpid(), m);
		tt_message_reply(m);
		tt_message_destroy(m);
		break;
	case	FAIL:
		DP printf("%d - Fail message(0x%X)\n", getpid(), m);
		tt_message_fail(m);
		tt_message_destroy(m);
		break;
	case	REJECT:
		DP printf("%d - Reject message(0x%X)\n", getpid(), m);
		tt_message_reject(m);
		tt_message_destroy(m);
		break;
	case	HOLD:
		DP printf("%d - Hold message(0x%X)\n", getpid(), m);
		break;
	case	OLD:
		DP printf("%d - Old message(0x%X)\n", getpid(), m);
		return(m);
	}
	return(save_msg);
}

void
dstt_message_return(Tt_message m, void *contents, int size)
{
	if(contents) tt_message_arg_bval_set(m, 0, contents, size);
	tt_message_reply(m);
	tt_message_destroy(m);
	DP printf("%d - Reply to message(0x%X)\n", getpid(), m);
}

void
dstt_message_fail(Tt_message m, dstt_status_t status)
{
	dstt_set_status(m, status);
	tt_message_fail(m);
	tt_message_destroy(m);
}

char	*
dstt_messageid(void)
{
	static	int	count = 0;
	int	numc, i;
	char	*buf;

	for(i = 1, numc = 10; numc-1 < count; i++, numc *= 10);
	buf = (char *)MALLOC(strlen(procid)+i+2);
	sprintf(buf, "%s#%d\0", procid, count);
	count++;
	return(buf);
}

int
dstt_start_tt(void)
{
	Tt_status	status;

	if(old_procid)
	{
		return(TT_OK);
	}
	old_procid = tt_default_procid();
	
	
	if(procid == 0)
	{
#ifdef	USE_DEBUGGER
		tt_default_session_set(tt_X_session(getenv("DISPLAY")));
#endif	USE_DEBUGGER
		procid = tt_open();
		status = tt_ptr_error(procid);
	}
	else
	{
		tt_default_procid_set(procid);
	}
	return(status);
}

dstt_restore_tt()
{
	if(tt_ptr_error(old_procid) == TT_OK)
	{
		tt_default_procid_set(old_procid);
		old_procid = 0;
	}
}

#ifndef	DEBUG
void dstt_set_app(char *ap){}
void dstt_prnt_msg(Tt_message m, char *label){}
char *dstt_prnt_state(Tt_message m){}
void dstt_prnt_grp(void){}
#else
static	char	*application = 0;

char *
dstt_prnt_state(Tt_message m)
{
	switch(tt_message_state(m))
	{
	case	TT_CREATED:
		return("TT_CREATED");
	case	TT_SENT:
		return("TT_SENT");
	case	TT_HANDLED:
		return("TT_HANDLED");
	case	TT_FAILED:
		return("TT_FAILED");
	case	TT_QUEUED:
		return("TT_QUEUED");
	case	TT_STARTED:
		return("TT_STARTED");
	case	TT_REJECTED:
		return("TT_REJECTED");
	case	TT_STATE_LAST:
		return("TT_STATE_LAST");
	}
}

void
dstt_set_app(char *app)
{
	if(app)
	{
		application = strdup(app);
	}
}

void
dstt_prnt_msg(Tt_message m, char *label)
{
	static char *ints[] = {"int\0", "integer\0", "boolean\0", "width\0",
			"height\0", "xOffset\0", "yOffset\0", NULL};
	static char *ttstate[] =  {"TT_CREATED", "TT_SENT", "TT_HANDLED",
				   "TT_FAILED", "TT_QUEUED", "TT_STARTED",
				   "TT_REJECTED"};
	static char *ttmode[] = {"TT_MODE_UNDEFINED", "TT_IN", "TT_OUT",
				 "TT_INOUT"};
	static char *ttscope[] ={"TT_SCOPE_NONE", "TT_SESSION", "TT_FILE", 
				 "TT_BOTH", "TT_FILE_IN_SESSION"};
	static char *ttclass[] ={"TT_CLASS_UNDEFINED", "TT_NOTICE", 
				 "TT_REQUEST", "TT_CLASS_LAST"};

	int	n, i, j;
	char	*op, *val, *type, *file;
	int	isint = 0;
	char	*str;

	if(m != (Tt_message )0)
	{
		op = tt_message_op(m);
		printf("---------- %s/%d (0x%X) %s ---------------\n", 
				application?application:"???",
				getpid(),
				m, label);

		printf("| op = %s\n", op?op:"(null)");
		if(tt_message_state(m) == TT_FAILED)
		{
			if(dstt_test_status(m) == dstt_status_not_valid)
			{
				printf("| state = TT_FAILED status = %d\n",
						tt_message_status(m));
			}
			else
			{
				printf("| state = TT_FAILED %s\n",
					dstt_set_status(0, 
						dstt_test_status(m)));
			}
		}
		else
		{
			printf("| state = %s\n", ttstate[tt_message_state(m)]);
		}
		printf("| scope = %s\n", ttscope[tt_message_scope(m)]);
		printf("| class = %s\n", ttscope[tt_message_class(m)]);

		printf("| default procid = %s\n", tt_default_procid());

		file = tt_message_file(m);
		printf("| file = %s\n", file?file:"(null)");
		str = tt_message_handler(m);
		printf("| handler = %s\n", str?str:"(null)");
		str = tt_message_sender(m);
		printf("| sender = %s\n", str?str:"(null)");
		str = tt_message_context_val(m, "Sun_Deskset_X_Selection");
		printf("| x_selection = %s\n", str?str:"(null)");
		n = tt_message_args_count(m);
		printf("| nargs = %d\n", n);
		for(i = 0; i < n; i++)
		{
			type = tt_message_arg_type(m, i);
			printf("| arg[%d] = %s\t", i, 
				ttmode[tt_message_arg_mode(m, i)]);
			printf("%s\t", type);
			isint = 0;
			for(j = 0; ints[j]; j++)
			{
				if(strncmp(ints[j], type, strlen(ints[j])) == 0)
				{
					isint = 1;
					break;
				}
			}
			if(isint)
			{
				tt_message_arg_ival(m, i, &j);
				printf("# %d\n", j);
			}
			else
			{
				val = tt_message_arg_val(m, i);
				printf("\"%.50s\"\n",
					val?val:"(null)");
			}
		}
		printf("----------------------------------------------\n");
	}
	else
	{
		printf("%d - Empty message\n", getpid());
	}
}
void dstt_prnt_grp(void)
{
	int		i,j,k;
	Message_group_t	*group;
	Message_set_t	*set;
	Msg_t		*msg;

	for(i=0; group = msg_groups[i]; i++)
	{
		printf("msg_groups[%d]\n", i);
		for(j = 1; set = group->sets[j]; j++)
		{
			printf("\tset[%d]\n", j);
			for(k = 0; k < set->count; k++)
			{
				printf("\t\top = %s [%s]\n", 
					set->list[k].name,
					set->list[k].handle?"TRUE":"FALSE");
			}
		}
	}
}

#endif
