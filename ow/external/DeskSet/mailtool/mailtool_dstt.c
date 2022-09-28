#ifndef lint
static  char sccsid[] = "@(#)mailtool_dstt.c 1.17 94/05/04 Copyright 1992 Sun Micro";
#endif

#ifdef	MA_DEBUG
#define	DP	if(1) 
#else
#define	DP	if(0)
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * New Tool Talk interface routines for mailtool.
 *
 */

#include <xview/xview.h>
#include <xview/textsw.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <desktop/tt_c.h>
#include <dstt.h>
#include <ds_verbose_malloc.h>
#include <locale.h>


#include "tool.h"
#include "tool_support.h"
#include "attach.h"
#include "../maillib/folder.h"
#include "../maillib/msg.h"
#include "../maillib/attach.h"
#include "mail_dstt.h"


#define MT_MSG_MEDIA	"RFC_822_Message"


extern	list_t	*dsll_init();
extern	list_t	*dsll_add_b_cp();
extern	list_t	*dsll_del();
extern	list_t	*dsll_next_e();

extern	list_t	*find_messageid(char *);
extern	void	delete_messageid(char *);

static	list_t	*que = 0;


static	Xv_opaque frame = 0;

extern struct reply_panel_data	*mt_get_compose_frame(struct header_data *);
static char *allocate_bufferid(void);
#ifdef NEVER
static struct reply_panel_data 	*bufferid_to_compose_window(
						struct header_data *, char *);
#endif
static int copy_file_into_memory(char *, void **, int *, char **);

extern Open_Handle mailtool_open;
extern Close_Handle mailtool_close;
extern Paste_Handle mailtool_paste;

status_t
mailtool_deposit(Tt_message m, char *media, Data_t type, void *data,
	int size, char *buffid, char *msgID)
{
	list_t		*list = find_messageid(msgID);
	Attach_list	*al;
	Attach_node	*node;
	int		rcode;
	char		*message;
	char		*p;

	if(list == 0)
	{
		return(FAIL);
	}
	if(strcmp(media, list->node->an_tt_media) != 0)
	{
		return(FAIL);
	}
	switch(type)
	{
	case	path:

		DP printf ("mailtool_deposit: path deposit\n");
		rcode = attach_methods.at_set(list->node->an_at,
					ATTACH_DATA_FILE, data);
		if (rcode < 0)
		{
			DP printf ("mailtool_deposit: Path Failed Attach\n");

			/* STRING_EXTRACTION -
			 *
			 * The message gets displayed when the data
			 * cannot be attached.
			 */
			message = gettext("Cannot update attachment...");
			mt_frame_msg(list->al->al_errorframe,
					FALSE, message);
		}
		if (list->node->an_pending) /* pending update */
		{
			DP printf ("mailtool_deposit: Pending Update\n");
			mt_add_attachment(list->al,
					list->node, list->al->al_msg);
			list->node->an_pending = FALSE;
		}
		break;

	case	contents:

		DP printf ("mailtool_deposit: contents deposit\n");
		/* Copy data and add it to the attachment */
		if ((p = (char *)ck_zmalloc(size)) != NULL)
		{
			memcpy(p, data,size);
			attach_methods.at_set(list->node->an_at,
					ATTACH_MMAP_BODY, (char *)p);
			attach_methods.at_set(list->node->an_at,
					ATTACH_CONTENT_LEN, size);
		}
		else
		{
			DP printf ("mailtool_deposit: Contents Failed Attach\n");

			/* STRING_EXTRACTION -
			 *
			 * The message gets displayed when the data
			 * cannot be attached.
			 */
			message = gettext("Cannot update attachment - Insufficient memory.");
			mt_frame_msg(list->al->al_errorframe,
					FALSE, message);
		}

		if (list->node->an_pending) /* pending update */
		{
			DP printf ("mailtool_deposit: Pending Update\n");
			mt_add_attachment(list->al,
					list->node, list->al->al_msg);
			list->node->an_pending = FALSE;
		}
		break;

	case	x_selection:

		DP printf ("mailtool_deposit: X_Selection Response.\n");
		break;

	default:
		DP printf("mailtool_deposit: Default - TT state returned %x\n",
				tt_message_state(m));
	}
	return(OK);
}

int
mailtool_open(
	Tt_message	m,
	void		*key,
	char		*media,
	Data_t		type,
	void 		*data,
	int 		size,
	bufftype_t	bufftype,
	char		**ID,
	dstt_bool_t	readonly,
	dstt_bool_t	mapped,
	int		sharelevel,
	char		*locator)
{
	struct header_data	*hd;
	struct reply_panel_data	*rpd;

	DP printf("mailtool_open(%d, %d, %s, %d, %d, %d, %d, %d, %d, %d, %d, %s)\n",
                           m, key, media ? media : "(nil)", type, data, size,
			   bufftype, ID, readonly, mapped, sharelevel,
			   locator ? locator : "(nil)");

	/*
	 * Open a mailtool buffer (compose window) to edit an email message.
	 * "data"  should contain the header of the email message.
	 */
	 
	hd = (struct header_data *)key;

	/* Make sure we don't evaporate while processing this request */
	mt_stop_self_destruct();

	/* Get a compose window */
	rpd = mt_get_compose_frame(hd);

	switch(type) {
	case	x_selection:
		/* Don't handle yet */
		dstt_set_status(m, dstt_status_not_valid);
		return FAIL;
		break;

	case	path:
		if (mt_load_template(data, rpd) < 0) {
			dstt_set_status(m, dstt_status_file_not_avail);
			return FAIL;
		}
		break;

	case	contents:
	default:
		mt_load_template_memory(data, size, rpd);
		break;
	}

	*ID = allocate_bufferid();
	rpd->rpd_bufferid = *ID;

	dstt_handle_paste(mailtool_paste, rpd, buffer, *ID, NULL, contents, 0);
	dstt_handle_close(mailtool_close, rpd, buffer, *ID);

	if (mapped) {
		mt_display_reply(rpd, NULL);
	}

	DP printf ("mailtool_open: Returning OK (%d)\n", OK);

	return(OK);
}

int
mailtool_close(
	Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	int		inquisitive,
	int		force)
{
	struct reply_panel_data	*rpd;

	DP printf ("mailtool_close(%d, %d, %d, %s, %d, %d)\n",
                            m, key, bufftype, ID ? ID : "(nil)",
			    inquisitive, force);

	/*
	 * Close a mailtool buffer (compose window)
	 */
	 
	rpd = (struct reply_panel_data *)key;

	if (rpd != NULL && strcmp(ID, rpd->rpd_bufferid) == 0) {
		free(rpd->rpd_bufferid);
		rpd->rpd_bufferid = NULL;

		/* De-register the handlers for this buffer */
		dstt_handle_paste(NULL, NULL, buffer, ID, NULL,
			  contents, 0);

		dstt_handle_close(NULL, NULL, buffer, ID);

		if (force) {
			if (!inquisitive) {
				/* Disable confirmation */
				mt_destroying = TRUE;
			}
			xv_destroy(rpd->frame);
			mt_destroying = FALSE;
		} else {
			if (inquisitive) {
				mt_display_reply(rpd, NULL);
			} else {
				/* Should just submit mail.  Not implemented */
				;
			}
		}
		DP printf ("mailtool_close: Returning OK\n");
		return OK;
	} else {
		dstt_set_status(m, dstt_status_invalid_message);
		return FAIL;
	}

}

int
mailtool_paste(
	Tt_message	m,
	void		*key,
	bufftype_t	bufftype,
	char		*ID,
	char		*media,
	Data_t		type,
	void 		*data,
	int 		size,
	int		offset,
	char		*locator)
{
	struct reply_panel_data	*rpd;
	Attach_node	*an;
	Attach_list	*al;
	char			*cetype;
	int	attach_size = size;
	void	*attach_data = data;
	char	*label = "";
	char	*emsg;
	int	rcode;

	DP printf ("mailtool_paste(%d, %d, %d, %s, %s, %d, %d, %d, %d, %s)\n",
                            m, key, bufftype,
			    ID ? ID : "(nil)", media ? media : "(nil)",
			    type, data, size,
		            offset, locator ? locator : "(nil)");
	/*
	 * Paste an attachment to the end of a mail message. I.e. add
	 * the data as an attachment.
	 */
	 
	rpd = (struct reply_panel_data *)key;

	if (rpd == NULL || strcmp(ID, rpd->rpd_bufferid) != 0) {
		dstt_set_status(m, dstt_status_not_valid);
		return FAIL;
	}

	al = rpd->rpd_al;

	switch(type) {
	case	x_selection:
		/* Don't handle yet */
		dstt_set_status(m, dstt_status_not_valid);
		return FAIL;
		break;

	case	path:
		/* Copy file into memory */
		if ((rcode = copy_file_into_memory(data, &attach_data,
					  &attach_size, &label)) < 0) {
			if (rcode == -1) {
				dstt_set_status(m, dstt_status_file_not_avail);
				mt_frame_msg(al->al_errorframe, FALSE,
				gettext("Could not add file as an attachment"));
			} else {
				dstt_set_status(m, dstt_status_data_not_avail);
				mt_frame_msg(al->al_errorframe, FALSE,
			gettext("Could not add attachment: Out of memory"));
			}
			return FAIL;
		}

		break;
	case	contents:
		/* Copy data */
		if ((attach_data = (char *)ck_zmalloc(size)) != NULL) {
			memcpy(attach_data, data, size);
			attach_size = size;
		} else {
			dstt_set_status(m, dstt_status_data_not_avail);
			mt_frame_msg(al->al_errorframe, FALSE,
			gettext("Could not add attachment: Out of memory"));
			return FAIL;
		}
		break;
	default:
		dstt_set_status(m, dstt_status_not_valid);
		return FAIL;
		break;
	}

	cetype = mt_get_data_type(label, attach_data, attach_size);
	if (!mt_attach_list_visible(al)) {
		mt_show_attach_list(al, TRUE);
		mt_layout_compose_window(rpd);
	}

	add_attachment_memory(al, al->al_msg, attach_data, attach_size,
				      cetype, label, FALSE);

	DP printf ("mailtool_paste: Returning OK\n");

	return(OK);
}


status_t
mailtool_status(Tt_message m, char *status, char *vendor, char *toolName,
	char *toolVersion, char *msgID, char *domain)
{
	list_t	*list = find_messageid(msgID);

	if (list && list->toolID == NULL)
	{
		DP printf("mailtool_status: TT status returned %x\n",
				tt_message_sender(m));
		list->toolID = tt_message_sender(m);
	}
	return(OK);
}


mailtool_dstt_start(Xv_opaque base_frame)
{
	struct header_data	*hd;

	frame = base_frame;

	DP printf("mailtool_dstt_start();\n");

	hd = mt_get_header_data(frame);

	dstt_xview_desktop_callback(frame,
		DEPOSIT,	mailtool_deposit,
		NULL);
	dstt_xview_desktop_register(frame,
		DEPOSIT,	TRUE,
		NULL);

	dstt_notice_callback(NULL,
		STATUS,		mailtool_status,
		NULL);
	dstt_notice_register(NULL,
		STATUS,		TRUE,
		NULL);

	/* dstt_handle_open ignores the fourth parameter */
	dstt_handle_open(mailtool_open, hd, MT_MSG_MEDIA, contents,
			 buffer, false);

	dstt_xview_start_notifier();
}

int
ma_edit_cb(Tt_message m, void *arg_list, int status, char *media,
		Data_t type, void *data, int size, char *msgID,
		char *title)
{
	list_t	*list = find_messageid(msgID);
	char		*type_string;
	char		*message;
	char		*p;
	int		rcode;
	
	switch (tt_message_state (m))
	{
	case TT_HANDLED:	/* Data returned */
		DP printf ("ma_edit_cb: TT_HANDLED\n");
		switch(type)
		{
		case	contents:

			/* Copy data and add it to the attachment */
			if ((p = (char *)ck_zmalloc(size)) != NULL)
			{
				memcpy(p, data,size);
				attach_methods.at_set(list->node->an_at,
						ATTACH_MMAP_BODY, (char *)p);
				attach_methods.at_set(list->node->an_at,
						ATTACH_CONTENT_LEN, size);
			}
			else
			{
				DP printf ("ma_edit_cb: Contents Failed Attach\n");
	
				/* STRING_EXTRACTION -
				 *
				 * The message gets displayed when the data
				 * cannot be attached.
				 */
				message = gettext("Cannot update attachment - Insufficient memory.");
				mt_frame_msg(list->al->al_errorframe,
						FALSE, message);
			}

			if (list->node->an_pending) /* pending update */
			{
				mt_add_attachment(list->al,
						list->node, list->al->al_msg);
				list->node->an_pending = FALSE;
			}

			break;

		case	path:
			break;
		case	x_selection:
			break;
		}
		list->node->an_msgID = NULL;
		delete_messageid (msgID);
		break;

	case TT_FAILED:					/* Invalid data fmt */
		DP printf ("ma_edit_cb: TT_FAILED Status %x\n",
				tt_message_status(m));
		DP printf ("ma_edit_cb: DSTT TEST Status %x\n",
				dstt_test_status(m));
		switch (dstt_test_status(m))
		{
		case dstt_status_process_died:
		case dstt_status_unmodified:		/* Abort work */
			DP printf ("ma_edit_cb: DSTT Unmodified \n");
			list->node->an_msgID = NULL;
			delete_messageid (msgID);
			break;

		case dstt_status_user_request_cancel:	/* User wants out */
			DP printf ("ma_edit_cb: DSTT User Quit Req.\n");
			list->node->an_msgID = NULL;
			delete_messageid(msgID);
			break;

		case dstt_status_service_not_found:
		case dstt_status_not_valid:		/* Invalid frm dstt   */
		case dstt_status_invalid_message:	/* Unknown rcvr msgID */
		case dstt_status_data_not_avail:	/* can't do data type */
		default:
			if ((type == x_selection) && (!list->delete))
			{
				DP printf ("ma_edit_cb: No x_selection\n");
				list->sel = NULL;
				dstt_edit(ma_edit_cb, arg_list,
					list->node->an_tt_media, contents,
					attach_methods.at_get(list->node->an_at,
							ATTACH_BODY),
					(int)attach_methods.at_get(list->node->an_at,
							ATTACH_CONTENT_LEN),
					msgID,
					(char *)attach_methods.at_get(list->node->an_at,
							ATTACH_DATA_NAME));
			}
			else if ((type == contents) && (!list->delete))
			{
				DP printf ("ma_edit_cb: No contents\n");
				list->node->an_msgID = NULL;
				mt_tt1_invoke_application(list->al, list->node,
							list->use_tooltalk);
				delete_messageid (msgID);
			}
			else if (list->delete)
			{
				DP printf ("ma_edit_cb: Node Deleted\n");
				delete_messageid (msgID);
			}
		}
		break;
	case TT_REJECTED:	/* No service found */
		DP printf ("ma_edit_cb: TT_REJECTED \n");
		list->node->an_msgID = NULL;
		delete_messageid (msgID);
		break;
	case	TT_STARTED:
		DP printf ("ma_edit_cb: TT_STARTED \n");
		break;
	default:
		DP printf("ma_edit_cb: Default - TT state returned %x\n",
				tt_message_state(m));
	}
}

int
ma_display_cb(Tt_message m, void *arg_list, int status, char *media,
		Data_t type, void *data, int size, char *msgID,
		char *title)
{
	list_t	*list = find_messageid(msgID);
	char		*type_string;
	char		*message;
	int		rcode;
	
	switch (tt_message_state (m))
	{
	case TT_HANDLED:				/* Data returned */
		DP printf ("ma_display_cb: TT_HANDLED\n");
		list->node->an_msgID = NULL;
		delete_messageid (msgID);
		break;
	case TT_FAILED:					/* Invalid data fmt */
		DP printf ("ma_display_cb: TT_FAILED Status %x\n",
				tt_message_status(m));
		DP printf ("ma_display_cb: DSTT TEST Status %x\n",
				dstt_test_status(m));
		switch (dstt_test_status(m))
		{
		case dstt_status_process_died:
		case dstt_status_unmodified:		/* Abort work */
			DP printf ("ma_display_cb: DSTT Unmodified \n");
			list->node->an_msgID = NULL;
			delete_messageid (msgID);
			break;

		case dstt_status_user_request_cancel:	/* User wants out */
			DP printf ("ma_display_cb: DSTT User Quit Req.\n");
			list->node->an_msgID = NULL;
			delete_messageid(msgID);
			break;

		case dstt_status_service_not_found:
		case dstt_status_not_valid:		/* Invalid frm dstt   */
		case dstt_status_invalid_message:	/* Unknown rcvr msgID */
		case dstt_status_data_not_avail:	/* can't do data type */
		default:
			if ((type == x_selection) && (!list->delete))
			{
				DP printf ("ma_display_cb: No x_selection\n");
				list->sel = NULL;
				dstt_display(ma_display_cb, arg_list,
					list->node->an_tt_media, contents,
					attach_methods.at_get(list->node->an_at,
							ATTACH_BODY),
					(int)attach_methods.at_get(list->node->an_at,
							ATTACH_CONTENT_LEN),
					msgID,
					(char *)attach_methods.at_get(list->node->an_at,
							ATTACH_DATA_NAME));
			}
			else if ((type == contents) && (!list->delete))
			{
				DP printf ("ma_display_cb: No contents\n");
				list->node->an_msgID = NULL;
				mt_tt1_invoke_application(list->al, list->node,
							list->use_tooltalk);
				delete_messageid (msgID);
			}
			else if (list->delete)
			{
				DP printf ("ma_display_cb: Node Deleted\n");
				delete_messageid (msgID);
			}
		}
		break;
	case TT_REJECTED:	/* No service found */
		DP printf ("ma_display_cb: TT_REJECTED \n");
		list->node->an_msgID = NULL;
		delete_messageid (msgID);
		break;
	case TT_STARTED:
		DP printf ("ma_display_cb: TT_STARTED \n");
		break;
	default:
		DP printf("ma_display_cb: Default - TT state returned %x\n",
				tt_message_state(m));
	}
}

int
ma_quit_cb(Tt_message m, void *arg_list, int status, int silent,
		int force, char *msgID)
{
	list_t	*list = find_messageid(msgID);

	switch (tt_message_state (m))
	{
	case TT_HANDLED:	/* Data returned */
		DP printf ("ma_quit_cb: TT_HANDLED\n");
		list->node->an_msgID = NULL;
		delete_messageid (msgID);
		break;

	case TT_FAILED:					/* Invalid data fmt */
		DP printf ("ma_quit_cb: TT_FAILED Status %x\n",
				tt_message_status(m));
		DP printf ("ma_quit_cb: DSTT TEST Status %x\n",
				dstt_test_status(m));
		switch (dstt_test_status(m))
		{
		case dstt_status_process_died:
		case dstt_status_service_not_found:
		case dstt_status_unmodified:		/* Abort work */
			DP printf ("ma_quit_cb: DSTT Unmodified \n");
			list->node->an_msgID = NULL;
			delete_messageid (msgID);
			break;

		case dstt_status_user_request_cancel:	/* User wants out */
			DP printf ("ma_quit_cb: DSTT User Quit Req.\n");
			list->node->an_msgID = NULL;
			delete_messageid(msgID);
			break;

		case dstt_status_not_valid:		/* Invalid frm dstt   */
		case dstt_status_invalid_message:	/* Unknown rcvr msgID */
		case dstt_status_data_not_avail:	/* can't do data type */
		default:
			DP printf ("ma_quit_cb: DSTT Default \n");
			list->node->an_msgID = NULL;
			delete_messageid(msgID);
			break;
		}
		break;
	case TT_REJECTED:	/* No service found */
		DP printf ("ma_quit_cb: TT_REJECTED \n");
		list->node->an_msgID = NULL;
		delete_messageid (msgID);
		break;
	case	TT_STARTED:
		DP printf ("ma_quit_cb: TT_STARTED \n");
		break;
	default:
		DP printf("ma_quit_cb: Default - TT state returned %x\n",
				tt_message_state(m));
	}
}

char	*
new_messageid(Attach_list *al, Attach_node *node, int use_tooltalk)
{
	int	i;
	list_t	tmp;
	
	if(!que)
	{
		DP printf ("new_messageid: Initialize\n");
		que = dsll_init();
	}
	tmp.msg = (char *)dstt_messageid();
	tmp.al = al;
	tmp.node = node;
	tmp.sel = NULL;
	tmp.use_tooltalk = use_tooltalk;
	tmp.toolID = NULL;
	tmp.delete = FALSE;
	DP printf ("new_messageid  node:  %x msgID: %s al: %x\n", node, tmp.msg, al);
	dsll_add_b_cp(que, sizeof(tmp), &tmp);

	return(tmp.msg);
}

void
delete_messageid(char *msg)
{
	list_t	*item = que;

	while((item = dsll_next_e(item)) != NULL)
	{
		if(item->msg && strcmp(item->msg, msg) == 0)
		{
			DP printf ("delete_messageid que: %x msgID: %s\n", item, msg);
			free(item->msg);
			dsll_del(item);
			return;
		}
	}
	DP printf ("delete_messageid que: %x msgID: %s\n", NULL, msg);
	return;
}

list_t *
find_messageid(char *msg)
{
	list_t	*item = que;

	while((item = dsll_next_e(item)) != NULL)
	{
		if(item->msg && strcmp(item->msg, msg) == 0)
		{
			DP printf ("find_messageid item:  %x msgID: %s\n", item, msg);
			return(item);
		}
	}
	DP printf ("find_messageid item:  %x msgID: %s\n", NULL, msg);
	return(NULL);
}

list_t *
find_messageal(Attach_list *al, list_t *start)
{
	list_t	*item;

	if (!start)
		item = que;
	else
		item = start;

	while((item = dsll_next_e(item)) != NULL)
	{
		if(al == item->al)
		{
			DP printf ("find_messageal item:  %x msgID: %s al: %x node: %x\n", item, item->msg, al, item->node);
			return(item);
		}
	}
	DP printf ("find_messageal item:  %x al: %x\n", NULL, al);
	return(NULL);
}


static char *
allocate_bufferid()
{
	static int	id = 100; /* Just in case we want to reserve id's */
	char		*id_str;

	if ((id_str = (char *)malloc(21)) != NULL)
		sprintf(id_str, "%d", id++);
	return id_str;
}

#ifdef NEVER
/* Currently not used */
static struct reply_panel_data *
bufferid_to_compose_window(struct header_data	*hd, char *bufferid)

{
	struct	reply_panel_data	*rpd;

	/*
	 * Search through all compose windows until we find the one
	 * with a matching bufferid
	 */
	for (rpd = hd->hd_rpd_list; rpd != NULL; rpd = rpd->next_ptr) {
		if (rpd->rpd_bufferid != NULL &&
		    strcmp(rpd->rpd_bufferid, bufferid) == 0)
			return rpd;
	}

	return NULL;
}
#endif

/*
 * Copy a file into a chunk of memory
 *
 *	Returns:	-1 if we can't open the file
 *			-2 if we can't allocate the memory to copy the file
 *
 *	ZZZ [dipol] This routine should probably be used by
 *		    mt_fcreate_attachment() someday.
 */
static int
copy_file_into_memory(
	char	*path,	/* IN: Path to the file to copy */
	void	**data,	/* OUT: Pointer to allocated buffer (free w/ck_zfree) */
	int	*size,	/* OUT: Size of buffer */
	char	**label  /* OUT: Pointer to last component of path */
)
{
	char	*file_buf, *mt_get_data_type();

	/* mmap in file */
	if ((file_buf = (char *)ck_mmap(path, size)) == NULL) {
		return -1;
	}

	/* Allocate memory to hold contents of file */
	if ((*data = (char *)ck_zmalloc(*size)) == NULL) {
		return -2;
	}

	/* Copy data into allocated buffer */
	memcpy(*data, file_buf, *size);

	/* Unmap file */
	ck_unmap(path, *size);

	/* Use the last component of the path as the label */
	if ((*label = strrchr(path, '/')) == NULL) {
		*label = path;
	} else {
		(*label)++;
	}

	return *size;
}

