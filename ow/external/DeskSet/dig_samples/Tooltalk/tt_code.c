#include <desktop/tt_c.h>
#include <poll.h>


extern	Tt_callback_action	xinfo_cb(Tt_message m, Tt_pattern p);
extern	Tt_callback_action	locale_cb(Tt_message m, Tt_pattern p);
extern	Tt_callback_action	handle_desktop(Tt_message m, Tt_pattern p);
extern	Tt_callback_action	handle_display(Tt_message m, Tt_pattern p);
extern	Tt_callback_action	handle_edit(Tt_message m, Tt_pattern p);

#ifndef	DEBUG
#define	DP	if(0)
#else
#define	DP	if(1)
#endif

Tt_message	save_msg;	/* startup message */

struct	startup
{
	char	*new[40];	/* pointers for new argv */
	int	argcount;	/* count for new argv pointers */
	int	ddvl;		/* or'ed flag to test for messages */
}newargs;

Tt_status
check_tt_startup(int *argc, char ***argv)
{
	struct	pollfd	myfds;		/* structure for poll command */
	Tt_message	m;		/* temporary message */
	Tt_status	status;		/* message status holder */
	char		*toolid;	/* procid of tool sending msg */
	int		i=0, n, j = 0;	/* counters */
	char		**tmp;		/* tmp location for argv array */	

	/* open Tooltalk session and check for errors */
	status = tt_ptr_error(tt_open());
	if(status != TT_OK)
	{
		return(status);
	}

	/* get and save the incomming message */
	save_msg = tt_message_receive();

	/* if Tooltalk started us up */
 	if(TT_WRN_START_MESSAGE == tt_message_status(save_msg))
	{
		/* argv initalization */
		newargs.argcount = 0;
		newargs.ddvl = 0;

		/* continue to deliver messages, I am just working on
		 * this one for a while.
		 *
		 * If you are using a TT library prior to Solaris 2.2 this line 
		 * is not available and you will not be able to handle multiple
		 * message untill you reply to the message that started you up.
		 */
		/* tt_message_accept(save_msg); */

		/* get the procid of the requesting application */
		toolid = tt_message_sender(save_msg),

		/* request the display, visual & depth */
		m = tt_message_create();
		tt_message_address_set(m, TT_HANDLER);
		tt_message_class_set(m, TT_REQUEST);
		tt_message_scope_set(m, TT_SESSION);
		tt_message_session_set(m,  tt_default_session());
		tt_message_op_set(m, "Get_XInfo");
		tt_message_handler_set(m, toolid);
		tt_message_disposition_set(m, TT_DISCARD);
		tt_message_arg_add(m, TT_OUT, "string", NULL);
		tt_message_arg_add(m, TT_OUT, "string", NULL);
		tt_message_arg_add(m, TT_OUT, "integer", "");
		tt_message_scope_set(m, TT_SESSION);
		tt_message_callback_add(m, xinfo_cb);
		tt_message_user_set(m, 0, &newargs);
		tt_message_send(m);

		/* request the locale info */
		m = tt_message_create();
		tt_message_address_set(m, TT_HANDLER);
		tt_message_class_set(m, TT_REQUEST);
		tt_message_scope_set(m, TT_SESSION);
		tt_message_session_set(m,  tt_default_session());
		tt_message_op_set(m, "Get_Locale");
		tt_message_handler_set(m, toolid);
		tt_message_disposition_set(m, TT_DISCARD);
		tt_message_arg_add(m, TT_OUT, "string", "LC_CTYPE");
		tt_message_arg_add(m, TT_OUT, "string", NULL);
		tt_message_arg_add(m, TT_OUT, "string", "LC_TIME");
		tt_message_arg_add(m, TT_OUT, "string", NULL);
		tt_message_arg_add(m, TT_OUT, "string", "LC_NUMERIC");
		tt_message_arg_add(m, TT_OUT, "string", NULL);
		tt_message_arg_add(m, TT_OUT, "string", "LC_MESSAGES");
		tt_message_arg_add(m, TT_OUT, "string", NULL);
		tt_message_scope_set(m, TT_SESSION);
		tt_message_callback_add(m, locale_cb);
		tt_message_user_set(m, 0, &newargs);
		tt_message_send(m);

		/* poll for messages */
		myfds.fd = tt_fd();
		myfds.events = POLLIN;
		while(newargs.ddvl != 3)
		{
			poll(&myfds, 1, -1);
			/* got one */
			m = tt_message_receive();
			if(m)
			{
				/* it's not one we are looking for */
				tt_message_reject(m);
			}
		}

		/* take all the new args that we got and create a 
		 * new argv/argc
		 */
		n = *argc+newargs.argcount;
		tmp = (char **)malloc((n+1)*sizeof(char *));
		for(i = 0 ; i < *argc; i++)
		{
			tmp[i] = *argv[i];
		}
		for(j = 0; i < n; i++,j++)
		{
			tmp[i] = newargs.new[j];
		}
		tmp[i] = 0;
		*argc = n;
		*argv = tmp;
	}
	return(TT_OK);
}

int
start_handling_messages()
{
	/* pattersn for desktop messages and display/edit messages */
	Tt_pattern	desktop_pat;
	Tt_pattern	display_pat;
	Tt_pattern	edit_pat;
	char		*op;		/* op of saved message */

	/* prepare to handle Desktop messages */
	desktop_pat = tt_pattern_create();
	tt_pattern_op_add(desktop_pat, "Set_Mapped");
	tt_pattern_op_add(desktop_pat, "Quit");
	tt_pattern_scope_add(desktop_pat, TT_SESSION);
	tt_pattern_session_add(desktop_pat, tt_default_session());
	tt_pattern_category_set(desktop_pat, TT_HANDLE);
	tt_pattern_callback_add(desktop_pat, handle_desktop);
	tt_pattern_register(desktop_pat);

	/* prepare to handle Display messages */
	display_pat = tt_pattern_create();
	tt_pattern_op_add(display_pat, "Display");
	tt_pattern_arg_add(display_pat, TT_IN, "ISO_Latin_1", NULL);
	tt_pattern_scope_add(display_pat, TT_SESSION);
	tt_pattern_session_add(display_pat, tt_default_session());
	tt_pattern_category_set(display_pat, TT_HANDLE);
	tt_pattern_class_add(display_pat, TT_REQUEST);
	tt_pattern_callback_add(display_pat, handle_display);
	tt_pattern_register(display_pat);

	/* prepare to handle Edit messages */
	edit_pat = tt_pattern_create();
	tt_pattern_op_add(edit_pat, "Edit");
	tt_pattern_arg_add(edit_pat, TT_OUT, "ISO_Latin_1", NULL);
	tt_pattern_arg_add(edit_pat, TT_INOUT, "ISO_LATIN_1", NULL);
	tt_pattern_scope_add(edit_pat, TT_SESSION);
	tt_pattern_session_add(edit_pat, tt_default_session());
	tt_pattern_category_set(edit_pat, TT_HANDLE);
	tt_pattern_class_add(edit_pat, TT_REQUEST);
	tt_pattern_callback_add(edit_pat, handle_edit);
	tt_pattern_register(edit_pat);

	/* if we have one we haven't handled because we were doing
	 * the window/application setup, handle it now
	 */
	if(save_msg != NULL)
	{
		/* find out its type */
		op = tt_message_op(save_msg);

		if(strcmp(op, "Display") == 0)	/* if Display message */
		{
			handle_display(save_msg, display_pat);
		}
		else if(strcmp(op, "Edit") == 0)/* if Edit message */
		{
			handle_edit(save_msg, edit_pat);;
		}
		else /* all others we don't want yet */
		{
			tt_message_reject(save_msg);
			tt_message_destroy(save_msg);
		}
	}

	/* return the file discriptor to watch for tooltalk activity on */
	return(tt_fd());
}

