#include <desktop/tt_c.h>
#include <locale.h>

#ifndef	DEBUG
#define	DP	if(0)
#else
#define	DP	if(1)
#endif

#define	TT_DESKTOP_ENOENT	1538 
#define	TT_DESKTOP_EINVAL	1558 
#define	TT_DESKTOP_EXITING	1697 
#define	TT_DESKTOP_CANCELED	1698 
#define	TT_DESKTOP_UNMODIFIED	1699 

#define	TT_MEDIA_ERR_SIZE	1700 
#define	TT_MEDIA_ERR_FORMAT	1701 
#define	TT_MEDIA_NO_CONTENTS	1702 

struct	startup
{
	char	*new[40];
	int	argcount;
	int	ddvl;
};

static	char	*argname[] = {"-display", "-visual", "-depth", "-lc_basiclocale"};

extern	void	show_frame();
extern	void	hide_frame();
extern	void	quit();

#define	MSG_DISPLAY	1
#define	MSG_EDIT	2

struct	cur_msg_state
{
	Tt_message	tt_msg;
	int		type;
	char		*msgid;
} cur_msg_state = { NULL, NULL};

void close_out_old_msg(struct cur_msg_state *);

/* Tooltalk call back to handle the Get_XInfo request */
Tt_callback_action
xinfo_cb(Tt_message m, Tt_pattern p)
{
	char	*display;		/* Xdisplay */
	char	*visual;		/* Screen visual */
	int	depth;			/* Screen depth */
	int	count;			/* Number of args in this msg */
	char	buff[10];		/* tmp format buffer */
	struct	startup	*newargs;	/* new argv data structure */

	/* check the state of the returned message */
	switch(tt_message_state(m))
	{
	case	TT_HANDLED:	/* everything came back ok */

		/* get the structure to save the argv data to */
		newargs = tt_message_user(m, 0);

		/* create the display argument */
		display = tt_message_arg_val(m, 0);
		if(display)
		{
			newargs->new[newargs->argcount++] = "-display";
			newargs->new[newargs->argcount++] = (char *)strdup(display);
		}

		/* create the visual argument */
		visual = tt_message_arg_val(m, 1);
		if(visual)
		{
			newargs->new[newargs->argcount++] = "-visual";
			newargs->new[newargs->argcount++] = (char *)strdup(visual);
		}

		/* create the depth argument */
		tt_message_arg_ival(m, 2, &depth);
		if(depth > 0)
		{
			newargs->new[newargs->argcount++] = "-depth";
			sprintf(buff, "%d", depth);
			newargs->new[newargs->argcount++] = (char *)strdup(buff);
		}
		break;
	default:
		/* just in case something goes wrong */
		DP printf("xinfo_cb: tt_message_state = %d\n", tt_message_state(m));
	}

	/* set the flage so we know we got this one */
	newargs->ddvl |= 1;

	/* clean up the message and return */
	tt_message_destroy(m);
	return(TT_CALLBACK_PROCESSED);
}


/* Tooltalk call back to handle the Get_Locale request */
Tt_callback_action
locale_cb(Tt_message m, Tt_pattern p)
{
	int	i;			/* counter */
	int	count = 0;		/* number of locale values */
	char	*cat;			/* category */
	char	*locale;		/* locale */
	struct	startup	*newargs;	/* new argv data structure */

	/* check the state of the returned message */
	switch(tt_message_state(m))
	{
	case	TT_HANDLED:	/* everything came back ok */

		/* get the structure to save the argv data to */
		newargs = tt_message_user(m, 0);

		/* get the number of values */
		count = tt_message_args_count(m);

		/* for each set of category/locale */
		for(i = 0; i < count/2; i++)
		{
			/* get the category and locale info */
			cat = tt_message_arg_val(m, i);
			locale = tt_message_arg_val(m, i+1);

			/* if the locale has been set */
			if(locale)
			{
				/* and if the category is one we are 
				 * interested in add the arguments
				 */
				if(strcmp(cat, "LC_CTYPE") == 0)
				{
					newargs->new[newargs->argcount++] = "-lc_basiclocale";
					newargs->new[newargs->argcount++] = (char *)strdup(locale);
					newargs->new[newargs->argcount++] = "-lc_inputlang";
					newargs->new[newargs->argcount++] = (char *)strdup(locale);
				}
				else if(strcmp(cat, "LC_TIME") == 0)
				{
					newargs->new[newargs->argcount++] = "-lc_timeformat";
					newargs->new[newargs->argcount++] = (char *)strdup(locale);
				}
				else if(strcmp(cat, "LC_NUMERIC") == 0)
				{
					newargs->new[newargs->argcount++] = "-lc_numeric";
					newargs->new[newargs->argcount++] = (char *)strdup(locale);
				}
				else if(strcmp(cat, "LC_MESSAGES") == 0)
				{
					newargs->new[newargs->argcount++] = "-lc_displaylang";
					newargs->new[newargs->argcount++] = (char *)strdup(locale);
				}
			}
		}
		/* set the flage so we know we got this one */
		newargs->ddvl |= 2;
		break;
	default:
		DP printf("locale_cb: tt_message_state = %d\n", tt_message_state(m));
	}
	/* clean up the message and return */
	tt_message_destroy(m);
	return(TT_CALLBACK_PROCESSED);
}

/* when the tooltalk file discriptor becomes active */
handle_tt_message()
{
	Tt_message	m;

	/* receive the message */
	m = tt_message_receive();
	if(m)
	{
		/* this means none of our callbacks got called so it
		 * is not one of ours so throw it back 
		 */
		tt_message_reject(m);
	}
}

/* when the message is one from the desktop pattern */
Tt_callback_action
handle_desktop(Tt_message m, Tt_pattern p)
{
	int	mapped;	/* the map operater */
	char	*op;	/* the type of message */

	/* get the message name */
	op = tt_message_op(m);

	if(strcmp(op, "Set_Mapped") == 0)	/* if it is a mapped message */
	{
		/* get the map operator */
		tt_message_arg_ival(m, 0, &mapped);

		/* set it to mapped or not based on the operator */
		if(mapped == 0)
		{
			hide_frame();
		}
		else
		{
			show_frame();
		}
	}
	else if(strcmp(op, "Quit") == 0)	/* if it is the quit message */
	{
		/* since this is a simple demo just quit */
		quit();
	}
	return(TT_CALLBACK_PROCESSED);
}

/* if we get a Display message */
Tt_callback_action
handle_display(Tt_message m, Tt_pattern p)
{
	char		*file;	/* not used for this simple app */
	char		*media;	/* media (for this it SHOULD be ISO_Latin_1 */
	char		*type;	/* vtype of message arg */
	char		*data;	/* contents of message */
	int		size;	/* size of the data */
	char		*msgid;	/* request's ID */
	char		*title;	/* request's title */
	int		count;	/* argument count */
	int		i;	/* tmp counter */
	Tt_message	tt_msg;	/* out going messages */

	/* get the media type (paranoids should check it */
	media = tt_message_arg_type(m, 0);

	/* get the count to see if we have the optional msgID/title */
	count = tt_message_args_count(m);
	DP printf("count = %d\n", count);
	for(i = 1; i < count; i++)
	{
		/* get the msg type */
		type = tt_message_arg_type(m, i);
		DP printf("type = '%s'\n", type);
		if(strcmp(type, "messageID") == 0) /* its optional msgID */
		{
			/* save it */
			msgid = tt_message_arg_val(m, i);
			DP printf("msgid = '%s'\n", msgid);
		}
		else if(strcmp(type, "title") == 0) /* itst optional title */
		{
			/* save it */
			title = tt_message_arg_val(m, i);
		}
	}
	if(file = tt_message_file(m)) /* its a file type */
	{
		DP printf("Displaying a file\n");
		/* this type of message is not handled for simplicty sake */
		tt_message_reject(m);
		tt_message_destroy(m);
		return(TT_CALLBACK_PROCESSED);
	}
	else	/* its a contents type */
	{
		/* get the data */
		tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);
		if(data == NULL || *data == '\0')
		{
			DP printf("data is NULL so fail this message\n");
			/* its not good so fail it */
			tt_message_status_set(m, TT_MEDIA_NO_CONTENTS);
			tt_message_fail(m);
			return(TT_CALLBACK_PROCESSED);
		}
		else
		{
			/* if we have an out standing msg handle it */
			close_out_old_msg(&cur_msg_state);
			cur_msg_state.tt_msg = m;
			DP printf("Displaying data\n");

			/* display the new data */
			display_data(data);
		}
	}

	/* save the current message state */
	cur_msg_state.type = MSG_DISPLAY;
	cur_msg_state.msgid = msgid;

	/* send back pt-pt a status msg to say all is well */
	tt_msg = tt_message_create();
	tt_message_address_set(tt_msg, TT_HANDLER);
	tt_message_handler_set(tt_msg, tt_message_sender(m));
	tt_message_op_set(tt_msg, "Status");
	tt_message_class_set(tt_msg, TT_NOTICE);
	tt_message_scope_set(tt_msg, TT_SESSION);
	tt_message_session_set(tt_msg,  tt_default_session());
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_arg_add(tt_msg, TT_IN, "string", "Request Received");
	tt_message_arg_add(tt_msg, TT_IN, "string", "ACME Vendor");
	tt_message_arg_add(tt_msg, TT_IN, "string", "Sample Editor");
	tt_message_arg_add(tt_msg, TT_IN, "string", "0.1");
	tt_message_arg_add(tt_msg, TT_IN, "messageID", cur_msg_state.msgid);
	tt_message_arg_add(tt_msg, TT_IN, "domain", setlocale(LC_CTYPE, NULL));
	tt_message_scope_set(tt_msg, TT_SESSION);
	tt_message_send(tt_msg);

	return(TT_CALLBACK_PROCESSED);
}

/* if we get a Edit message */
Tt_callback_action
handle_edit(Tt_message m, Tt_pattern p)
{

	char		*file;	/* not used for this simple app */
	char		*media;	/* media (for this it SHOULD be ISO_Latin_1 */
	char		*type;	/* vtype of message arg */
	char		*data;	/* contents of message */
	int		size;	/* size of the data */
	char		*msgid;	/* request's ID */
	char		*title;	/* request's title */
	int		count;	/* argument count */
	int		i;	/* tmp counter */
	Tt_message	tt_msg;	/* out going messages */

/*
 *	REJECT MESSAGES 
 */
	/* get the media type (paranoids should check it */
	media = tt_message_arg_type(m, 0);

	/* get the count to see if we have the optional msgID/title */
	count = tt_message_args_count(m);
	DP printf("count = %d\n", count);
	for(i = 1; i < count; i++)
	{
		/* get the msg type */
		type = tt_message_arg_type(m, i);
		DP printf("type = '%s'\n", type);
		if(strcmp(type, "messageID") == 0)
		{
			/* save it */
			msgid = tt_message_arg_val(m, i);
			DP printf("msgid = '%s'\n", msgid);
		}
		else if(strcmp(type, "title") == 0)
		{
			/* save it */
			title = tt_message_arg_val(m, i);
		}
	}
	if(file = tt_message_file(m)) /* its a file type */
	{
		/* this type of message is not handled for simplicty sake */
		tt_message_reject(m);
		tt_message_destroy(m);
		return(TT_CALLBACK_PROCESSED);
	}
	else	/* its a contents type */
	{
		/* get the data */
		tt_message_arg_bval(m, 0, (unsigned char **)&data, &size);
		close_out_old_msg(&cur_msg_state);
		if(data == NULL || *data == '\0')
		{
			/* its compose time */
			clear_data();
		}
		else
		{
			cur_msg_state.tt_msg = m;
			DP printf("Displaying data\n");
			/* display the new data */
			display_data(data);
		}
	}
	/* save the current message state */
	cur_msg_state.type = MSG_EDIT;
	cur_msg_state.msgid = msgid;

	/* send back pt-pt a status msg to say all is well */
	tt_msg = tt_message_create();
	tt_message_address_set(tt_msg, TT_HANDLER);
	tt_message_handler_set(tt_msg, tt_message_sender(m));
	tt_message_op_set(tt_msg, "Status");
	tt_message_class_set(tt_msg, TT_NOTICE);
	tt_message_scope_set(tt_msg, TT_SESSION);
	tt_message_session_set(tt_msg,  tt_default_session());
	tt_message_disposition_set(tt_msg, TT_DISCARD);
	tt_message_arg_add(tt_msg, TT_IN, "string", "Request Received");
	tt_message_arg_add(tt_msg, TT_IN, "string", "ACME Vendor");
	tt_message_arg_add(tt_msg, TT_IN, "string", "Sample Editor");
	tt_message_arg_add(tt_msg, TT_IN, "string", "0.1");
	tt_message_arg_add(tt_msg, TT_IN, "messageID", cur_msg_state.msgid);
	tt_message_arg_add(tt_msg, TT_IN, "domain", setlocale(LC_CTYPE, NULL));
	tt_message_scope_set(tt_msg, TT_SESSION);
	tt_message_send(tt_msg);

	return(TT_CALLBACK_PROCESSED);
}

void
close_out_old_msg(struct cur_msg_state *old_msg)
{
	if(old_msg->tt_msg == NULL)
	{
		return;
	}
	if(strcmp(tt_message_op(old_msg->tt_msg), "Display") == NULL)
	{
		tt_message_reply(old_msg->tt_msg);
		tt_message_destroy(old_msg->tt_msg);
	}
	else
	{
		/* more work here for save/old data etc. */
		tt_message_reply(old_msg->tt_msg);
		tt_message_destroy(old_msg->tt_msg);
	}
	old_msg->tt_msg = NULL;
}

/* when we want to quit we need to make sure the
 * current message gets handled
 */
void
quit_tt()
{
	char	*data;	/* current data */
	int	size;	/* current size */

	/* if no current message we are done */
	if(cur_msg_state.tt_msg == 0)
	{
		return;
	}
	if(cur_msg_state.type == MSG_DISPLAY) /* we're handling a Display msg */
	{
		/* just reply to it */
		tt_message_reply(cur_msg_state.tt_msg);
		tt_message_destroy(cur_msg_state.tt_msg);
	}
	else if(cur_msg_state.type == MSG_EDIT)/* handling an Edit msg */
	{
		/* get the correct data */
		if(data_is_modified())
		{
			restore_data();
		}
		get_data(&data, &size);

		/* use that data to reply to the message */
		tt_message_arg_val_set(cur_msg_state.tt_msg, 0, data);
		tt_message_reply(cur_msg_state.tt_msg);
		tt_message_destroy(cur_msg_state.tt_msg);
	}
	cur_msg_state.tt_msg = 0;
}

/* called when a deposit has completed */
Tt_callback_action
save_cb(Tt_message m, Tt_pattern p)
{
	switch(tt_message_state(m))
	{
	case	TT_HANDLED:
		/* show a successfull save was done */
		break;
	case	TT_FAILED:
		/* error state */
		break;
	default:
		DP printf("some thing else\n");
	}
}

/* called when you need to save the data back to the
 * calling process. (This only needs to be done if you're
 * handling a Display msg or you want to save an intermediate
 * step in Edit)
 */
save_tt()
{
	Tt_message      tt_msg = 0;
	int             null = 0;
	Tt_status       rc;
	char		*toolid;
	char		*data;
	int		size;

	/* if we really have a tooltalk msg to save */
	if(cur_msg_state.tt_msg != NULL)
	{
		/* create and send a Deposit message */
		tt_msg = tt_message_create();
		tt_message_address_set(tt_msg, TT_PROCEDURE);
		tt_message_class_set(tt_msg, TT_REQUEST);
		tt_message_scope_set(tt_msg, TT_SESSION);
		tt_message_session_set(tt_msg,  tt_default_session());
		tt_message_disposition_set(tt_msg, TT_DISCARD);
	
		tt_message_address_set(tt_msg, TT_HANDLER);
	
		toolid = tt_message_sender(cur_msg_state.tt_msg);
		tt_message_handler_set(tt_msg, toolid);
		tt_message_op_set(tt_msg, "Deposit");
		get_data(&data, &size);
		tt_message_arg_add(tt_msg, TT_IN, "ISO_Latin_1", data);
	
		tt_message_arg_add(tt_msg, TT_IN,
				"messageID", cur_msg_state.msgid);
		tt_message_callback_add(tt_msg, save_cb);
		tt_message_send(tt_msg);
	}
}
