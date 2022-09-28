#ifndef lint
static 	char sccsid[] = "@(#)textedit_dstt.c 1.23 96/06/19 Copyr 1990 Sun Micro";
#endif

#ifdef	MA_DEBUG
#define	DP(block)	if(1) { printf("%s:%d ", __FILE__, __LINE__); block }
#else
#define	DP(block)	if(0) { block }
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * New Tool Talk interface routines for textedit.
 *
 */

#include <xview/xview.h>
#include <xview/textsw.h>
#include <xview/dragdrop.h>
#include <xview/notice.h>
#include <desktop/tt_c.h>
#include <dstt.h>
#include <ds_verbose_malloc.h>
#include <locale.h>

static	Xv_opaque		textsw = 0;
static	Xv_opaque		frame = 0;
static	Xv_opaque		save_item = 0;
extern	Xv_Server		My_server;
extern	Selection_requestor	Sel;
extern	int			save_thru_load();
extern	int			edited;

typedef	void			RD(Menu, Menu_item);

static	RD	*return_default;
static	int	dont_break_link = FALSE;
extern Frame   application_frame;      /* Base frame of application */

typedef	enum
{
	display,
	edit
}type_t;

struct	putback
{
	Tt_message	message;
	type_t		type;
	Data_t		data;
	char		*media;
	char		*buffid;
	char		*msgid;
} putback;

static void
clear_putback()
{
	putback.message = 0;
	putback.type = None;
	if(putback.media)
	{
		FREE(putback.media);
		putback.media = 0;
	}
	if(putback.buffid)
	{
		FREE(putback.buffid);
		putback.buffid = 0;
	}
	if(putback.msgid)
	{
		FREE(putback.msgid);
		putback.msgid = 0;
	}
}

void
textedit_dstt_breaklink()
{
	if(dont_break_link) return;

	DP (printf("called textedit_dstt_breaklink();\n"););
	if(putback.message)
	{
		dstt_set_status(putback.message, dstt_status_quit_rec);
		dstt_message_fail(putback.message);
		clear_putback();
	}
}

static int
retrieve_selection(char	*sel_name)
{
	Atom	transient_selection;

	transient_selection = (Atom)xv_get(My_server, SERVER_ATOM, sel_name);

	if (transient_selection == NULL)
		return(FALSE);

	/*
	 * Let the dragNdrop code handle the data transfer
	 */
	(void)xv_set(Sel, SEL_RANK, transient_selection, 0);
	dont_break_link = TRUE;
        DP (printf("dont_break_link = TRUE;\n"););
	load_from_dragdrop();
	dont_break_link = FALSE;
        DP (printf("dont_break_link = FALSE;\n"););

	return(TRUE);
}

static Notify_value
quit_after_sleep()
{
	DP (printf("quit_after_sleep();\n");dstt_stopped(););
	exit(0);
}

static Notify_value
ma_timeout_quit(client)
Xv_opaque	client;
{
	struct	itimerval	timeout;
	struct	itimerval	otimeout;

	DP (printf("ma_timeout_quit(client);\n"););

	timeout.it_value.tv_sec = 180;
	timeout.it_value.tv_usec = 0;
	timeout.it_interval.tv_sec = 0;
	timeout.it_interval.tv_usec = 0;

	dstt_editor_register("ISO_Latin_1",
		DISPLAY,		TRUE,
		EDIT,			TRUE,
		NULL);

	if (xv_get(frame, FRAME_CMD_PIN_STATE) != FRAME_CMD_PIN_IN) {

		xv_set(frame, XV_SHOW, FALSE, NULL);

		(void) notify_set_itimer_func(client,
				quit_after_sleep,
				ITIMER_REAL,
				&timeout,
				&otimeout);
	} else {
        /*
        ** Bug 1207619
        ** Can't quit if the user has pinned the window.
        ** So instead enroll this function as the FRAME_DONE_PROC.
        ** When the user un-pins the window it will be called again.
        */

        xv_set(frame, FRAME_DONE_PROC, ma_timeout_quit, NULL);
	}
	return(NOTIFY_DONE);
}

status_t
textedit_dstt_quit(Tt_message m, int silent, int force, char *msgID)
{
	int	quit = TRUE;

	DP (printf("textedit_dstt_quit(silent = %s force = %s);\n",
			silent?"TRUE":"FALSE",
			force?"TRUE":"FALSE"););

	if(!force && !silent)
	{
		quit = ma_done_proc(frame);
	}

	if(!force && !quit)
	{
		return(OK);
	}
	dstt_set_status(putback.message, dstt_status_quit_rec);
	tt_message_fail(putback.message);
	clear_putback();
	ma_timeout_quit(frame);
	return(OK);
}

status_t
textedit_dstt_set_iconified(Tt_message m, dstt_bool_t *iconify, char *msgID, char *buff)
{
        xv_set(dstt_main_win(), XV_SHOW, !(*iconify), NULL);
        *iconify = !xv_get(dstt_main_win(), XV_SHOW);
	return(OK);
}

status_t
textedit_dstt_get_iconified(Tt_message m, dstt_bool_t *iconify, char *msgID, char *buff)
{
        *iconify = !xv_get(dstt_main_win(), XV_SHOW);
	return(OK);
}

succesfull_putback(Tt_message m, void *key, int status, char *media,
	Data_t type, void *data, int size, char *buffid, char *msgid)
{
	int	confirm;
	Event	event;

	DP (printf("succesfull_putback called\n"););
	switch(tt_message_state(m))
	{
	case	TT_HANDLED:
		xv_set(frame, FRAME_LEFT_FOOTER,
			gettext("Save back succeeded"), NULL);
		if((int)key)
		{
			tt_message_reply(putback.message);
			clear_putback();
			ma_timeout_quit(frame);
			textsw_reset(textsw, 0, 0);
		}
		xv_set(textsw,
			TEXTSW_INSERTION_POINT, 0,
			TEXTSW_FIRST,   0,
#if 0
			TEXTSW_FILE_CONTENTS, 0,
#endif
			NULL);
		break;
	case	TT_FAILED:
	case	TT_REJECTED:
		xv_set(frame, FRAME_LEFT_FOOTER,
			gettext("Save back failed"), NULL);
		confirm = notice_prompt(frame, &event,
			NOTICE_MESSAGE_STRINGS,
			gettext("Unable to save data."),
			0,
			NOTICE_BUTTON, gettext("Continue"), 1,
			NULL);
		break;
	default:
		DP (printf("deposit state %d\n", tt_message_state(m)););
	}
	if(!key)
	{
		xv_set(frame, XV_SHOW, TRUE, NULL);
	}
}

void
save_data(Menu menu, Menu_item menu_item)
{
	int	size;
	char	*buf;
	Event	event;

	DP (printf("save_data()\n"););

	switch(putback.data)
	{
	case	contents:
		size = xv_get(textsw, TEXTSW_LENGTH)+1;
		buf = MALLOC(size+1);
		xv_get(textsw, TEXTSW_CONTENTS, 0, buf, size);
		DP (printf("dstt_deposit message %d, '%40.40s', 0x%X\n",
			size, buf, putback.message););
		dstt_deposit(succesfull_putback, 
			tt_message_sender(putback.message),
			FALSE, putback.media, putback.data,
			buf, size, putback.buffid, putback.msgid);
		FREE(buf);
		(void)xv_set(menu, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
		break;
	case	x_selection:
		DP (printf("save_thru_load called"););
		if(xv_get(frame, FRAME_CMD_PIN_STATE) != FRAME_CMD_PIN_IN)
		{
			tt_message_reply(putback.message);
			ma_timeout_quit(frame);
			clear_putback();
		}
		break;
	case	path:
		if (return_default == NULL)
			break;
		(*return_default)(menu, menu_item);
		break;
	default:
		DP (printf("OOPs somethngs wrong!!\n"););
	}
}

int
ma_done_proc(Xv_opaque base_frame)
{
	char		*buf;
	int		size;
	int		confirm;
	Event		event;
	Xv_opaque	menu;

	DP (printf("ma_done_proc();\n"););

	if(putback.message)
	{
		if (xv_get(textsw, TEXTSW_MODIFIED))
		{
			confirm = notice_prompt(frame, &event,
				NOTICE_MESSAGE_STRINGS,
				gettext("The current data has been edited.\nDo you wish to save your changes?"),
				0,
				NOTICE_BUTTON, gettext("Save changes"), 1,
				NOTICE_BUTTON, gettext("Discard changes"), 2,
				NOTICE_BUTTON, gettext("Cancel"), 3,
				NULL);
			if(confirm == 2)
			{
				textsw_reset_2(textsw, 0, 0, TRUE, TRUE);
			}
			else if(confirm == 3)
			{
				return(FALSE);
			}
		}
		switch(putback.data)
		{
		case	contents:
			DP (printf("\tdoing contents\n"););
			size = xv_get(textsw, TEXTSW_LENGTH)+1;
			buf = MALLOC(size+1);
			xv_get(textsw, TEXTSW_CONTENTS, 0, buf, size);
			if(putback.type == edit)
			{
				DP (printf("return msg %d, '%40.40s', 0x%X\n",
					size, buf, putback.message););
				dstt_message_return(putback.message, buf, size);
			}
			else
			{
				DP (printf("\t   display\n"););
				DP (printf("deposit msg %d, '%40.40s', 0x%X\n",
					size, buf, putback.message););
				if(xv_get(textsw, TEXTSW_MODIFIED))
				{
					dstt_deposit(succesfull_putback,
					    tt_message_sender(putback.message),
					    TRUE, putback.media, putback.data,
					    buf, size, putback.buffid, 
					    putback.msgid);
				}
				else
				{
					tt_message_reply(putback.message);
				}
			}
			FREE(buf);
			break;
		case	x_selection:
			DP (printf("save_thru_load for 0x%X\n", 
					putback.message););
			if(save_item)
			{
				menu = xv_get(save_item, MENU_PARENT);
				save_thru_load(menu, save_item);
			}
			tt_message_reply(putback.message);
			break;
		case	path:
			if(save_item)
			{
				menu = xv_get(save_item, MENU_PARENT);
				(*return_default)(menu, save_item);
			}
			tt_message_reply(putback.message);
			break;
		default:
			DP (printf("OOPs somethngs wrong!!\n"););
		}
	}
	if(confirm == 3)
	{
		return(FALSE);
	}
	else
	{
		ma_timeout_quit(base_frame);
		clear_putback();
		return(TRUE);
	}
}

status_t
textedit_dstt_display(Tt_message m, char * media, Data_t type, void *data, int size, char *msg, char *title)
{
	char		*host, *file;
	int		len;
	char		*status_msg;
	status_t	rc = HOLD;

	DP (printf("textedit_dstt_display();\n"););

	if(xv_get(frame, FRAME_CMD_PIN_STATE) == FRAME_CMD_PIN_IN)
	{
		return(REJECT);
	}

	xv_set(frame, XV_SHOW, TRUE, NULL);
	turnoff_timer();

	if(putback.message)
	{
		dstt_message_return(putback.message, NULL, NULL);
	}
	clear_putback();
	putback.message = m;
	putback.type = display;
	putback.media = DS_STRDUP(media);
	putback.buffid = 0;
	putback.msgid = msg?DS_STRDUP(msg):msg;
	putback.data = type;

	textsw_reset(textsw, 0, 0);
	switch(type)
	{
	case	contents:
		DP (printf("MA contents %40.40s...\n", data););
		if(data)
		{
			xv_set(textsw, TEXTSW_CONTENTS, (char *)data, NULL);
			edited = 0;
			DP (printf("edited = 0;\n"););
			set_tool_label(0, 0);
			xv_set(textsw,
				TEXTSW_INSERTION_POINT, 0,
				TEXTSW_FIRST,   0,
#if 0
				TEXTSW_FILE_CONTENTS, 0,
#endif
				NULL);
		}
		else
		{
			textsw_reset(textsw, 0, 0);
		}
		if (save_item)
		{
			xv_set(save_item,
				MENU_NOTIFY_PROC, save_data,
				NULL);
		}
		break;
	case	path:
		DP (printf("MA path %s\n", data););
		xv_set(textsw, TEXTSW_FILE, (char *)data, NULL);
		if (save_item)
		{
			xv_set(save_item,
				MENU_NOTIFY_PROC, return_default,
				NULL);
		}
		break;
	case	x_selection:
		DP (printf("MA x_selection %s\n", data););
		if(retrieve_selection(data))
		{
			rc = HOLD;
			if (save_item)
			{
				DP (printf("save_item = %X\n", save_item););
				xv_set(save_item,
					MENU_NOTIFY_PROC, save_data,
					NULL);
			}
		}
		else
		{
			dstt_set_status(m, dstt_status_data_not_avail);
			rc = FAIL;
		}
	}

	if(rc == HOLD)
	{
		status_msg = (char *)dstt_set_status(0, dstt_status_req_rec);
		dstt_status(tt_message_sender(m),
			    status_msg,
			    msg,
			    setlocale(LC_CTYPE, NULL));
	}

	textsw_normalize_view(textsw, 0);
	return(rc);
}

status_t
textedit_dstt_edit(Tt_message m, char * media, Data_t type, void *data, int size, char *msg, char *title)
{
	char	*host, *file;
	int	len;
	char		*status_msg;
	status_t	rc = HOLD;

	DP (printf("textedit_dstt_edit();\n"););

	xv_set(frame, XV_SHOW, TRUE, NULL);
	turnoff_timer();
	dstt_editor_register(media,
			DISPLAY,		FALSE,
			EDIT,			FALSE,
			NULL);
	clear_putback;
	putback.message = m;
	putback.type = display;
	putback.media = DS_STRDUP(media);
	putback.buffid = 0;
	putback.msgid = msg?DS_STRDUP(msg):msg;
	putback.data = type;

	status_msg = (char *)dstt_set_status(0, dstt_status_req_rec);
	dstt_status(tt_message_sender(m),
		    status_msg,
		    msg,
		    setlocale(LC_CTYPE, NULL));

	if (save_item)
	{
		xv_set(save_item, MENU_NOTIFY_PROC, save_data, 0);
	}
	
	textsw_reset(textsw, 0, 0);
	switch(type)
	{
	case	contents:
		DP (printf("MA contents %40.40s\n", data););
		if(data)
		{
			xv_set(textsw, TEXTSW_CONTENTS, (char *)data, NULL);
			xv_set(textsw,
				TEXTSW_INSERTION_POINT, 0,
				TEXTSW_FIRST,   0,
#if 0
				TEXTSW_FILE_CONTENTS, 0,
#endif
				NULL);
			edited = 0;
		}
		else
		{
			textsw_reset(textsw, 0, 0);
		}
		if (save_item)
		{
			xv_set(save_item,
				MENU_NOTIFY_PROC, save_data,
				NULL);
		}
		break;
	case	path:
		DP (printf("MA path %s\n", data););
		xv_set(textsw, TEXTSW_FILE, (char *)data, NULL);
		if (save_item)
		{
			xv_set(save_item,
				MENU_NOTIFY_PROC, return_default,
				NULL);
		}
		break;
	case	x_selection:
		DP (printf("MA x_selection %s\n", data););
		if(retrieve_selection(data))
		{
			rc = HOLD;
			if (save_item)
			{
				xv_set(save_item,
					MENU_NOTIFY_PROC, save_data,
					NULL);
			}
		}
		else
		{
			rc = FAIL;
		}
	}
	textsw_normalize_view(textsw, 0);
	return(rc);
}

textedit_dstt_start(Xv_opaque base_frame, Xv_opaque tsw)
{
	Xv_opaque	file_submenu;

	clear_putback();

	application_frame = frame = base_frame;
	textsw = tsw;

	/* This is a workaround for an xview
	 * problem where the window does not stay up afer a menu item is
	 * selected from the panel/button/menus. (they work fine if you use
	 * the same menus from the textsw).
	 */
	workaround_init(textsw);

	file_submenu = xv_get(textsw, TEXTSW_SUBMENU_FILE);
	save_item = xv_get(file_submenu, MENU_NTH_ITEM, 2);
	if (save_item)
	{
		return_default = (RD *)xv_get(save_item, MENU_NOTIFY_PROC);
	}

	xv_set(frame, FRAME_DONE_PROC, ma_done_proc, NULL);
	dstt_xview_desktop_callback(frame,
		QUIT,		textedit_dstt_quit,
		SET_ICONIFIED,	textedit_dstt_set_iconified,
		GET_ICONIFIED,	textedit_dstt_get_iconified,
		NULL);
	dstt_xview_desktop_register(frame,
		QUIT,		TRUE,
		NULL);

	dstt_editor_callback("ISO_Latin_1",
		DISPLAY,	textedit_dstt_display,
		EDIT,		textedit_dstt_edit,
		NULL);

	dstt_editor_register("ISO_Latin_1",
			DISPLAY,		TRUE,
			EDIT,			TRUE,
			NULL);

	dstt_xview_start_notifier();

	DP (printf("textedit_dstt_start();\n");dstt_started(););
}

