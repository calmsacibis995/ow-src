#ident "@(#)attach_list.c 3.12 93/05/03 Copyr 1990 Sun Micro"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifdef	MA_DEBUG
#define	DP	if(1)
#else
#define	DP	if(0)
#endif

#include <stdio.h>
#include <sys/stat.h>

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/scrollbar.h>

#include "../maillib/obj.h"
#include "attach.h"
#include "string.h"
#include "header.h"
#include "tool.h"
#include "dstt.h"
#include "mail_dstt.h"

extern	list_t	*find_messageid(char *);
extern	list_t	*find_messageal(Attach_list *, list_t *);

/* Attachment list free list */
static Attach_node *free_list;

static Attach_node *mt_first_attach_node(Attach_list *, int skip_deleted);
static Attach_node *mt_next_attach_node(Attach_node *, int skip_deleted);

Attach_list *
mt_create_attach_list(
	Frame	frame,
	Frame	error_frame,
	struct msg	*msg,
	Frame	headerframe
)
{
	Attach_list	*al;

	if ((al = (Attach_list *)calloc(1, sizeof(Attach_list))) == NULL)
		return(NULL);
	else {
		al->al_frame = frame;
		al->al_errorframe = error_frame;
		al->al_msg = msg;
		al->al_headerframe = headerframe;
		return(al);
	}
}



void
mt_set_attach_msg(
	Attach_list	*al,
	struct msg	*msg
)
{
	al->al_msg = msg;
	return;
}



static void
mt_free_attach_node(
	Attach_node	*node
)
{
	/*
	 * Place free'd node onto free list
	 */
	node->an_next = free_list;
	free_list = node;
}



Attach_node *
mt_get_attach_node(
	struct attach *at
)
{
	Attach_node	*an;

	/*
	 * Get entry off of free list, if freelist is empty we
	 * must malloc a new one.
	 */
	if (free_list == NULL) {
		an = (Attach_node *)malloc(sizeof(Attach_node));
	} else {
		an = free_list;
		free_list = an->an_next;
	}

	/* Null out struct */
	if (an != NULL) {
		(void)memset((char *)an, 0, sizeof(Attach_node));
	}

	/* make the at point to the an */
	an->an_at = at;
	attach_methods.at_set(at, ATTACH_CLIENT_DATA, an);

	return(an);
}



void
mt_init_attach_list(
	Attach_list	*list,
	Frame		frame,
	Canvas		canvas,
	Panel		panel
)
{
	Xv_Window	pw;
	struct header_data *hd;
	struct attach	*at;

	hd = mt_get_header_data(list->al_headerframe);

	/*
	 * Initialize the attachment list struct
	 */

	pw = canvas_paint_window(canvas);
	list->al_canvas = canvas;
	list->al_drawable = (Drawable)xv_get(pw, XV_XID);
	list->al_display = (Display *)xv_get(pw, XV_DISPLAY);
	list->al_w = MT_ATTACH_ICON_W;
	list->al_h = MT_ATTACH_ICON_H;
	list->al_font = hd->hd_textfont;
	list->al_fontset = hd->hd_textfontset;
	list->al_fontid = hd->hd_fontid;
	list->al_gc = hd->hd_attachgc;
	list->al_cleargc = hd->hd_cleargc;
	list->al_x_gap = MT_ATTACH_MIN_X_GAP;
	list->al_y_gap = 2 * (int)xv_get(list->al_font,FONT_DEFAULT_CHAR_HEIGHT);
	list->al_panel = panel;
	list->al_frame = frame;
	list->al_delete_cnt = 0;

	list->al_msg_item = NULL;
	if (panel) {
		list->al_msg_item = mt_get_attach_msg_item(panel);
	}

	return;
}



void
mt_destroy_attach_nodes(
	Attach_list	*al
)
{
	Attach_node	*an;

	if (al->al_canvas == NULL)
		return;

	/*
	 * Destroy all nodes in the attachment list. 
	 */
	for (an = mt_first_attach_node(al, FALSE); an != NULL;
					an = mt_next_attach_node(an, FALSE)) {
		attach_methods.at_set(an->an_at, ATTACH_CLIENT_DATA, NULL);
		mt_destroy_attach_node(an);
	}

	/* Destroy all nodes in the pending list */
	mt_destroy_pending_attachments(al);

	return;
}



void
mt_destroy_attach_list(
	Attach_list	*al
)
{
	if (al == NULL)
		return;

	/* Free the header data for recursive message or multipart */
	if (al->al_hd) {
		ck_free(al->al_hd);
		al->al_hd = NULL;
	}

	/* Free all data contained in this list */
	mt_destroy_attach_nodes(al);

	/* Finally, free the actual list */
	free(al);
}



void
mt_destroy_attach_node(
	Attach_node	*an
)
{
	list_t		*list;

	extern Quit_CB	ma_quit_cb;

	/*
	 * If there is a TT application displaying this
	 * data, take it down
	 */

	if (an->an_msgID)
	{
		/* Send a quit message to an invoked app.
		 * dstt_quit(Quit_CB *cb,	Who to call when done
		 * 		void *key,	Client Data
		 *		char *toolid,	ToolID
		 *		int silent,	TRUE = Don't talk to user
		 *		int force,	TRUE = Just Do It!
		 *		char *msg)	msgID
		 */
		list = find_messageid(an->an_msgID);
		if (list->toolID)
		{
			DP printf("mt_destroy_attach_node: %x toolID: %s\n", list, list->toolID);
			dstt_quit(NULL, (void *)list,
				list->toolID, TRUE, TRUE,
				an->an_msgID);
		}
		list->delete = TRUE;
		DP printf("mt_destroy_attach_node: %x msgID: %s\n", list, an->an_msgID);
	}

	/* 
	 * Free malloc'd data in the node struct and place it on free list
	 */

	if (an->an_handler_id) {
		mt_send_tt_quit(NULL, an->an_handler_id);
		free(an->an_handler_id);
		an->an_handler_id = NULL;
	}

	if (an->an_open_tt) {
		free(an->an_open_tt);
		an->an_open_tt = NULL;
	}

	if (an->an_application) {
		free(an->an_application);
		an->an_application = NULL;
	}

	if (an->an_msg) {
		msg_methods.mm_free_msg(an->an_msg);
		an->an_msg = NULL;
		ck_zfree(an->an_buffer);
		an->an_buffer = NULL;
	}

	/* Place node on free list */
	mt_free_attach_node(an);
}




Attach_node *
mt_find_attach_node(
	Attach_list	*al,
	short		x,
	short		y
)
{
	Attach_node	*an;

	/*
	 * Find the attachment node under x,y
	 */
	an = NULL;
	for (an = mt_first_attach_node(al, TRUE); an != NULL;
					an = mt_next_attach_node(an, TRUE)) {

		if (mt_within(x, y, an->an_x, an->an_y, al->al_w, al->al_h))
			break;
	}

	return(an);
}


void
mt_iconify_attach_node(
	Attach_list	*al,
	int		ICONIFY
)
{
	list_t		*list = NULL;


	/*
	 * Find all the attachment nodes and send them dstt iconify message
	 */

	while (list = find_messageal(al, list))
	{
		if (list->node->an_msgID)
		{
			dstt_set_iconified(NULL, (void *)list,
						list->toolID, ICONIFY,
						list->node->an_msgID, NULL);
			if (!ICONIFY)
			{
				dstt_raise(NULL, (void *)list,
						list->toolID,
						list->node->an_msgID, NULL);
			}
		}
	}

	return;
}




static Attach_node *
mt_first_attach_node(
	Attach_list	*al,		/* List to get node from */
	int		skip_deleted	/* TRUE to skip deleted nodes */
)
{
	struct attach	*at;
	Attach_node	*an = NULL;

	at = attach_methods.at_first(al->al_msg);

	while (at) {
		an = (Attach_node *)attach_methods.at_get(at,
			ATTACH_CLIENT_DATA);

		/* If there is an attach node and the attachment is either
		 * not deleted or we are not skipping deleted nodes then
		 * return this node.
		 */
		if (an && (!attach_methods.at_get(at, ATTACH_DELETED) ||
			   !skip_deleted))
		{
			break;
		} else {
			an = NULL;
		}

		at = attach_methods.at_next(at);
	}

	return(an);
}



static Attach_node *
mt_next_attach_node(
	Attach_node	*an,
	int		skip_deleted	/* TRUE to skip deleted nodes */
)
{
	struct attach	*at;

	at = an->an_at;
	an = NULL;
	while (at = attach_methods.at_next(at)) {
		an = (Attach_node *)attach_methods.at_get(at,
							ATTACH_CLIENT_DATA);
		if (an && (!attach_methods.at_get(at, ATTACH_DELETED) ||
			   !skip_deleted)) {

			break;
		} else {
			an = NULL;
		}
	}

	return(an);
}



/*
 * Set the amount of whitespace between icons as well as the attachment 
 * number
 */
int
mt_set_attach_display_xgap(
	Attach_list	*al
)
{
	Attach_node	*an;
	int		widest_label = 0;
	int		new_gap;
	int		n = 1;

	/*
	 * Set the amount of whitespace between icons.  We must take into
	 * account how wide the widest label is.  Also update the attachment
	 * numbers and update the panel message
	 * Return TRUE if the xgap changes.
	 */
	for (an = mt_first_attach_node(al, TRUE); an != NULL;
					an = mt_next_attach_node(an, TRUE)) {
		an->an_number = n++;
		if (an->an_label_width > widest_label) {
			widest_label = an->an_label_width;
		}
	}

	new_gap = widest_label - al->al_w + 5;
	if (new_gap < MT_ATTACH_MIN_X_GAP)
		new_gap = MT_ATTACH_MIN_X_GAP;

	if (al->al_x_gap == new_gap)
		return(FALSE);

	al->al_x_gap = new_gap;

	return(TRUE);
}


long
mt_set_attach_display_xys(
	Attach_list	*al,
	long		width
)
{
	Attach_node	*an;
	long		x;
	long		y;
	long		x_delta;
	long		y_delta;

	/*
	 * Compute new xy locations for attachment list icons.  This is 
	 * usually called after the x_gap changes.
	 * Return the height required to display all icons.
	 */
	x = al->al_x_gap;
	y = al->al_y_gap / 4;
	x_delta = al->al_x_gap + al->al_w;
	y_delta = al->al_y_gap + al->al_h;
	for (an = mt_first_attach_node(al, TRUE); an != NULL;
					an = mt_next_attach_node(an, TRUE)) {
		an->an_x = x;
		an->an_y = y;
		x += x_delta;
		if (x + x_delta > width) {
			y += y_delta;
			x = al->al_x_gap;
		}
	}

	return(y + y_delta);
}




/*
 * Traverse through attachments.  This skips over deleted attachments
 */
void
mt_traverse_attach_list(al, call_back, arg)

	Attach_list	*al;
	Function	call_back;

{
	Attach_node	*an;
	struct attach	*at;

	/* 
	 * Loop through the list calling call_back() for each node
	 * We need "next" since the call_back proc may delete the node
	 * p points to.  The "arg" is optional.
	 * Stop if the call_back proc returns FALSE;
	 */
	at = attach_methods.at_first(al->al_msg);
	while (at) {

		an = (Attach_node *)
			attach_methods.at_get(at, ATTACH_CLIENT_DATA);

		/* save next ptr before current attachment is destroyed */
		at = attach_methods.at_next(at);

		/* Skip deleted attachments */
		if (an == NULL || an->an_ndelete)
			continue;

		if (an && !(*call_back)(al, an, arg)) {
			return;
		}
	}

	return;
}



/* Find the node with the largest ndelete value.  This
 * will be the most recent node deleted.  Returns NULL if
 * no nodes have been delete.
 */
Attach_node *
mt_find_last_deleted(
	Attach_list	*al
)
{
	Attach_node	*an, *delete_an = NULL;

	/* Loop through attachment list */
	for (an = mt_first_attach_node(al, FALSE); an != NULL;
					an = mt_next_attach_node(an, FALSE)) {

		/* Skip nodes which are not delete */
		if (an == NULL || an->an_ndelete == 0)
			continue;

		/* Found a delete node.  Check it. */
		if (delete_an == NULL || an->an_ndelete > delete_an->an_ndelete)
			delete_an = an;
	}

	return(delete_an);
}




Attach_node *
mt_get_next_selected_node(
	Attach_list	*al,
	Attach_node	*an
)
{
	struct attach	*at;

	/* 
	 * Find the next selected node after "node".   If "node" is NULL
	 * start with the first node.
	 */
	if (an == NULL) {
		an = mt_first_attach_node(al, TRUE);
	} else {
		an = mt_next_attach_node(an, TRUE);
	}

	while (an) {
		if (an && an->an_selected) {
			return(an);
		}

		an = mt_next_attach_node(an, TRUE);
	}

	return (NULL);
}



/*
 * Add up the number of bytes of the attachments in a list.  If selected_only
 * is TRUE, then only sum up the selected attachments
 */
long
mt_attach_list_size(
	Attach_list	*al,
	int		selected_only
)
{
	Attach_node	*an;
	int		total;

	total = 0;
	for (an = mt_first_attach_node(al, TRUE); an != NULL;
					an = mt_next_attach_node(an, TRUE)) {

		if (selected_only && !an->an_selected)
			continue;

		total += (int)attach_methods.at_get(an->an_at,
							ATTACH_CONTENT_LEN);
	}
	return(total);
}



/*
 * Return number of attachments in list.
 */
int
mt_nattachments(
	Attach_list	*al,
	int		selected_only	/* TRUE to only count selected ones */
)
{
	Attach_node	*an;
	int		total;

	total = 0;
	for (an = mt_first_attach_node(al, TRUE); an != NULL;
					an = mt_next_attach_node(an, TRUE)) {
		if (selected_only && !an->an_selected)
			continue;

		total++;
	}

	return(total);
}

/*
 * Routines to manage the pending attachment list.  This list is used
 * to hold "attachments in progress", ie the user has selected something
 * off of the Attach menu which creates an empty attachment, puts it on
 * the pending list, and starts the application.  When (and if) we get
 * data from the application the attachment becomes real and is moved to
 * the real attachment list.
 */

/* Add a node to the attachment list */
void
mt_add_pending_attachment(
	Attach_list	*al,
	Attach_node	*an
)
{
	an->an_next = al->al_pending_list;
	al->al_pending_list = an;

	return;
}

/*
 * Deletes the specified attachment node from the pending list.  Returns
 * a pointer to the node if it is found, else NULL.  
 * 
 * This routine only removes the node from the list.  It does not destroy
 * the node.
 */
Attach_node *
mt_delete_pending_attachment(
	Attach_list	*al,
	Attach_node	*an
)

{
	Attach_node	*p;

	/* The pending list will typically contain just 1 node.  Check
	 * for this case first.
	 */
	if (al->al_pending_list == an) {
		al->al_pending_list = an->an_next;
		an->an_next = NULL;
		return an;
	}

	/* Traverse list to find node */
	for (p = al->al_pending_list; p != NULL; p = p->an_next) {
		if (p->an_next == an) {
			p->an_next = an->an_next;
			an->an_next = NULL;
			return an;
		}
	}

	return NULL;
}

void
mt_destroy_pending_attachments(
	Attach_list	*al
)

{
	list_t		*list = NULL;

	extern Quit_CB	ma_quit_cb;

	/*
	 * If there is a TT application having pending data (not a real
	 * attachment yet), take it down.
	 */

	while (list = find_messageal(al, list))
	{
		if ((list->node->an_pending) && (!list->delete))
		{
			attach_methods.at_destroy(list->node->an_at);
			mt_destroy_attach_node(list->node);
			
		}
	}
 	return;
	
}

#ifdef DEBUG
mt_dump_pending_attachments(
	char		*s,
	Attach_list	*al
)
{
	Attach_node	*p, *tmp_p;
	int		i = 0;

	if (s != NULL)
		puts(s);
	for (p = al->al_pending_list; p != NULL; p = p->an_next ) {
		i++;
		printf("Node %d: %s\n", i, mt_attach_name(p->an_at));
	}
	puts("\n");
}	
#endif
