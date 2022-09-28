/* @(#)buttons.c       3.8 - 93/04/27 */


/* button.c -- define button operations */



#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <memory.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/textsw.h>

#include "tool.h"
#include "tool_support.h"
#include "glob.h"
#include "header.h"
#include "buttons.h"
#include "mle.h"
#include "../maillib/ck_strings.h"

#define DEBUG_FLAG mt_debugging
extern DEBUG_FLAG;
#include "debug.h"

extern Frame	mt_frame;


struct button {
	struct button * b_next;

	char *		b_name;		/* internal name */
	void *		b_menu;		/* submenu ptr */
	void 		(*b_notify)();	/* notify proceedure */
	char *		b_helpid;	/* help text */
	void *		b_client_data;
};


struct binding {
	struct binding *b_next;

	int		b_row;
	int		b_col;
	char *		b_label;
	char *		b_name;

	ButtonCallback	b_callback;
	void *		b_callarg;

	struct button *	b_button;
	Panel_item	b_item;
	int		b_realwidth;
	int		b_labelwidth;
	int		b_errorgiven : 1;
	int		b_userdefined : 1;
	int		b_x;		/* x and y pos for callbacks */
	int		b_y;
};


#define STREQ(a,b)	(!strcmp((a),(b)))

static struct button *
alloc_button(
	char *name
)
{
	struct button *b;

	b = ck_malloc(sizeof *b);

	memset((char *)b, '\0', sizeof *b);
	b->b_name = ck_strdup(name);

	return(b);
}


static void
free_button(
	struct button *b
)
{
	ck_free(b->b_name);
	ck_free(b);
}


static struct binding *
alloc_binding(
	char *label,
	char *name
)
{
	struct binding *b;

	b = ck_malloc(sizeof *b);

	memset((char *)b, '\0', sizeof *b);
	b->b_label = ck_strdup(label);
	b->b_name = ck_strdup(name);

	return(b);
}


static void
free_binding(
	struct binding *b
)
{
	ck_free(b->b_label);
	ck_free(b->b_name);
	ck_free(b);
}


/*
 * given a name, return a button structure (or null if the name wasn't found 
 */
static struct button *
find_button_on_list(
	char *name,
	struct button *list
)
{
	while (list) {
		if (STREQ(list->b_name, name)) {
			return(list);
		}
		list = list->b_next;
	}
	return (NULL);
}



static struct button *
find_button(
	struct header_data *hd,
	char *name
)
{
	struct button *b;

	b = find_button_on_list(name, hd->hd_sys_button_list);

	return (b);
}


/*
 * this is a public function
 *
 * define_sys_button defines a "system" button to the code.  A system
 * button is one the user can't delete, and is one of the "fundamental"
 * button types.  If we ever allow user-built menu buttons then
 * this will be a more important concept.
 *
 * This routine is called with:
 *
 * name - the "public" name of this button.  These must be unique
 *	within the system.
 *
 * notify - if this button does not have a menu, then it needs a notify
 *	routine.  This routine is called when the button is selected.
 *
 *	*IMPORTANT NOTE* the notify proceedure is called with a null
 *	Menu argument and a special Menu_item.  This menu item is
 *	set up to have the specified client data, but no other values
 *	are preserved.  You must not check for anything but client data
 *	in the notify proceedure.
 *
 * menu - the menu to be displayed for this button
 *
 * helpid - the help string for this button
 *
 * client_data - passed along to the notify proceedure
 *
 *
 * return value: 1 for success, 0 for failure.
 */
int
define_sys_button(
	struct header_data *hd,
	char *name,
	void (*notify)(),
	void *menu,
	char *helpid,
	void *client_data
)
{
	struct button *b;

	if (find_button(hd, name)) {
		/* an error -- no duplicate names */
		return (0);
	}

	b = alloc_button(name);

	b->b_menu = menu;
	b->b_notify = notify;
	b->b_helpid = helpid;
	b->b_client_data = client_data;

	b->b_next = hd->hd_sys_button_list;
	hd->hd_sys_button_list = b;

	return(1);
}



static struct binding *
find_binding(
	struct header_data *hd,
	int row,
	int col
)
{
	struct binding *b;

	for (b = hd->hd_binding_list; b; b = b->b_next) {
		if (b->b_row == row && b->b_col == col) break;
	}
	return (b);
}



/*
 * this is a public function.  it returns the label of the button
 * at position (row, col), or zero if there is no button there
 */
/*
 * It gets called by props only. Since props tie to the first hd,
 * we can simply use mt_frame here.
 */
int
get_button_at_position(
	int row,
	int col,
	char **label,
	char **name
)
{
	struct binding *b;
	struct header_data *hd;

	hd = mt_get_header_data(mt_frame);
	b = find_binding(hd, row, col);

	if (! b)  return (0);

	if (label) {
		*label = b->b_label;
	}

	if (name) {
		if (b->b_button) {
			*name = b->b_button->b_name;
		}
	}

	return (1);

}




static void
delete_binding(
	struct header_data *hd,
	struct binding *b
)
{
	struct binding *list;

	/* first, delete it from the list */
	if (hd->hd_binding_list == b) {
		/* special case the head of the list */
		hd->hd_binding_list = b->b_next;

		/* check to see if this is the last thing on the list */
		if (hd->hd_last_binding == b) 
			hd->hd_last_binding = NULL;
	} else {
		for (list = hd->hd_binding_list; list->b_next; list = list->b_next) {
			if (list->b_next == b) {
				list->b_next = b->b_next;
				break;
			}
		}

		/* see if we just deleted the last binding */
		if (! b->b_next) {
			hd->hd_last_binding = list;
		}
	}

	/* now get rid of the button */
	free_binding(b);
}




static void
add_binding(
	struct header_data *hd,
	struct binding *b
)
{
	if (hd->hd_last_binding) {
		hd->hd_last_binding->b_next = b;
		hd->hd_last_binding = b;
	} else {
		/* first element on the list */
		hd->hd_binding_list = hd->hd_last_binding = b;
	}
}





/*
 * our notify routine, used to send things on to menu_items
 */
static int
notify_proc(
Panel_item item,
Event *ie
)
{
	struct button *b;
	static Menu bogus_menu;
	static Menu_item bogus_item;
	struct header_data *hd;

	hd = mt_get_header_data(event_window(ie));
	b = (struct button *) xv_get(item, PANEL_CLIENT_DATA);

	/* we have to do this in order to get a menu_item for the notify
	 * proc.  implicit assumption: the notify proc only uses menu_item
	 * (not menu), and only looks at the client_data in the item.
	 */
	/*
	 * Instead of passing NULL to notify's first arg,
	 * we create a bogus menu to pass in header_data struct 
	 */

	if (b->b_notify) {
		bogus_menu = mt_create_bogus_menu(hd);
		bogus_item = mt_create_bogus_menu_item(hd);

		xv_set(bogus_item, MENU_CLIENT_DATA, b->b_client_data, 0);
		(*b->b_notify)(bogus_menu, bogus_item, ie);
	}

	return (XV_OK);
}



/*
 * Just like bind_button, but with a user flag.  The user
 * flag allows us to delete existing bindings
 */
/*
 * Since this can be called from .mailrc, we cannot
 * pass in hd; that's ok since we have to bind
 * for all hd frames anyway.
 */
void
bind_button_user(
        struct header_data *hd,
	int row,
	int col,
	char *label,
	char *name,
	int user
)
{
	struct binding *bind;
	Panel_item item;

	MP(("bind_button_user(%d, %d, %s, %s, %d)\n",
		row, col, label, name, user));

	if (row < 0 || col < 0) {
		return;
	}

	bind = find_binding(hd, row, col);
	if (bind) {
		if (! user) {
			/* we don't let system buttons override user ones */
			return;
		}

		if (STREQ(label, bind->b_label) && STREQ(name, bind->b_name)) {
			/* this is really the same button... */
			return;
		}

		/* there is already a button at this position */
		DP(("bind_button_user: button %s already here\n",
			bind->b_label));

		if (bind->b_item) {
			xv_set(bind->b_item,
				PANEL_ITEM_MENU, NULL,
				0);
			
			/* Don't make this destroy_safe because it won't
			 * refresh when we apply props and the buttons
			 * are updated
			 */
			xv_destroy(bind->b_item);
		}
		delete_binding(hd, bind);
	}

	/* allocate a new binding structure */
	bind = alloc_binding(label, name);

	bind->b_row = row;
	bind->b_col = col;

	add_binding(hd, bind);

	/* make the user buttons show up; system buttons explicitly
	 * call the commit routine on their own...
	 */
	if (user) {
		/* 
		 * This routine (bind_button_user) gets called
		 * from bind_button (which sets user to 0),
		 * and from user_button (which makes commit_buttons
		 * return immediately (hd_sys_button_list = 0)
		 * After a props_apply, though, commit_buttons
		 * do get called to put in new button and
		 * to resize.
		 */
		commit_buttons(hd);

		if (MT_FRAME(hd)) {
			window_fit_height(MT_FRAME(hd));
		}
	}
}



/*
 * the public bind-a-button routine
 *
 * This procedure actually arranges to put a button up on the screen.
 * The buttons are arranged in rows and columns; the columns are lined up
 * vertically.  If there is already a button existing at this row/column,
 * then it is deleted.
 *
 * row - the row to use for this button.
 *
 * col - the column to use for this button.
 *
 * label - the displayed text for the button (PANEL_LABEL_STRING)
 *
 * name - the internal button name to map this label to.
 *
 * user - a user definition.  User defines can override system defs, but
 *	not visa versa.
 */
/*
 * FYI: From here, commit_buttons
 * never gets called, since user = 0
 */
void
bind_button(
	struct header_data *hd,
	int row,
	int col,
	char *label,
	char *name
)
{
	bind_button_user(hd, row, col, label, name, 0);
}



/*
 * the 2nd half of binding, that we can only do if the button
 * structure binding already exists.
 * ALL buttons are created here, some of its attributes
 * (like PANEL_ITEM_MENU) are sometimes set to NULL
 * (if it does not hold a menu).
 */
static void
bind_button2(
	struct header_data *hd,
	struct binding *bind
)
{
	struct button *button;

	button = find_button(hd, bind->b_name);

	if (! button) {
		return;
	}

	bind->b_button = button;

	/* get the panel item to display */
	bind->b_item = xv_create(hd->hd_cmdpanel, PANEL_BUTTON,
		PANEL_LABEL_STRING, bind->b_label,
		XV_SHOW, FALSE,
		0);

	xv_set(bind->b_item,
		PANEL_ITEM_MENU, button->b_menu,
		XV_HELP_DATA, button->b_helpid,
		PANEL_CLIENT_DATA, button,
		0);

	bind->b_realwidth = xv_get(bind->b_item, XV_WIDTH);
	bind->b_labelwidth = xv_get(bind->b_item, PANEL_LABEL_WIDTH);

	if (button->b_notify) {
		xv_set(bind->b_item, PANEL_NOTIFY_PROC, notify_proc, 0);
	}

}


/*
 * this is a public function
 *
 * commit_buttons make the current bindings public and updates
 * the panel.  This routine actually figures out where everything
 * should go.  Nothing is displayed until this routine is called.
 *
 * This is used when at least one of the bindings changed, then
 * we apply the change(s) to all the hd's.
 */
void
commit_buttons(
	struct header_data *hd
)
{
	int max_row;
	int max_col;
	int row_gap;
	int col;
	struct binding *b;
	int border;
	int width;
	int max_width;
	int spacing;
   
	/* 
	 * don't bother if we haven't finished initializing 
	 */
   	
	if (! hd->hd_sys_button_list) 
		return;

	/* find the maximum row number, and bind any user defined buttons */
	max_row = max_col = -1;
	for (b = hd->hd_binding_list; b ; b = b->b_next) {
		max_col = MAX(max_col, b->b_col);
		max_row = MAX(max_row, b->b_row);

		if (! b->b_button && ! b->b_callback) {
			bind_button2(hd, b);
			if (! b->b_button && ! b->b_errorgiven) {
				/* STRING_EXTRACTION -
				 *
				 * This message is printed out when the
				 * user has a "button" command in mailrc
				 * that does not map to a defined button
				 *
				 * The first %s is the name of the mailtool
				 * program; the second is the name that wasn't
				 * found.
				 */
				fprintf(stderr,
					gettext(
				    "%s: There is no button by the name %s\n"),
					mt_cmdname, b->b_name);

				b->b_errorgiven = 1;
			}
		}
	}

	/* if there are no buttons then just return */
	if (max_row < 0) return;

	border = xv_col(hd->hd_cmdpanel, 0);
	row_gap = xv_get(hd->hd_cmdpanel, WIN_ROW_GAP);

	MP(("xv_row(panel, 0) = %d\n", xv_row(hd->hd_cmdpanel, 0)));
	MP(("xv_row(panel, 1) = %d\n", xv_row(hd->hd_cmdpanel, 1)));
	MP(("row_gap = %d\n", row_gap));


	/* walk our way in, column by column */
	for (col = 0; col <= max_col; col++) {
		max_width = 0;

		/* adjust the buttons in this row, and figure out how
		 * wide they are
		 */
		for (b = hd->hd_binding_list; b; b = b->b_next) {
			if (b->b_col != col) continue;

			/* run through and get the width */
			max_width = MAX(b->b_realwidth, max_width);
		}

		for (b = hd->hd_binding_list; b; b = b->b_next) {
			if (b->b_col != col) continue;

			if (b->b_callback) {
				b->b_x = border;
				b->b_y = xv_row(hd->hd_cmdpanel, b->b_row);
				continue;
			} else if (! b->b_button) {
				continue;
			}

			/* now adjust the width */
			spacing = max_width - b->b_realwidth;

			xv_set(b->b_item,
				XV_Y, xv_row(hd->hd_cmdpanel, b->b_row),
				XV_X, border,
				PANEL_LABEL_WIDTH, b->b_labelwidth + spacing,
				XV_SHOW, FALSE,
				0);
		}

		border += max_width + row_gap;
	}

	/* now go back and set stuff to show */
	for (b = hd->hd_binding_list; b; b = b->b_next) {
		if (b->b_callback) {
			(*b->b_callback)(b->b_callarg, b->b_x, b->b_y);
		} else if (b->b_button) {
			xv_set(b->b_item, XV_SHOW, TRUE, 0);
		}
	}

	/* now resize the panel */
	window_fit_height(hd->hd_cmdpanel);

	DP(("old panel height %d\n", xv_get(hd->hd_cmdpanel, XV_HEIGHT)));

#if 1
	/* hack -- avoid the extra whitespace at the bottom */
	/* ZZZ: what's the right spacing?  how much of a border should
	 * we have at the bottom?  right now, we have almost none.
	 * should we leave xv_row(panel,0) (i.e. 4 pixels)?
	 */
	border = xv_row(hd->hd_cmdpanel, max_row +1);
	xv_set(hd->hd_cmdpanel, WIN_HEIGHT, border, 0);

	DP(("new panel height %d\n", xv_get(hd->hd_cmdpanel, XV_HEIGHT)));
#endif

}



/*
 * the cmdline interface to defining buttons
 *
 * by arrangements with lex.c, we know that argv has the right number of args
 */
int
user_button(
	char **argv
)
{
	int row;
	int col;
	char *label;
	char *name;
	struct header_data *hd;

	row = atoi(argv[0]);
	col = atoi(argv[1]);
	label = argv[2];
	name = argv[3];

	/* STRING_EXTRACTION -
	 *
	 * we give this error if the user tries to redefine a button
	 * on the first row.  %s is the program name, and %s is the button
	 * label.
	 */
	if (row <= 0) {
		fprintf(stderr, "%s: can't define button %s on the first row\n",
			mt_cmdname, label);
		return (0);
	}

	/* only do half the binding because we read the database before
	 * we have defined any buttons
	 *
	 * we'll patch this up at commit_bindings time
	 */
	/*
	 * This call eventually calls commit_buttons, but
	 * at startup time that
	 * returns immediately since hd_sys_button_list is
	 * still NULL.
	 */
	/*
	 * Do it for all hd frames
	 */
	for (hd = mt_header_data_list; hd != NULL; hd = hd->hd_next) {
		bind_button_user(hd, row, col, label, name, 1);
	}

	return (0);
}



/* this is a public function
 *
 * This routine is really a hack: I needed a way to set the position
 * of the "File Menu" string; this procedure is it.  This basically
 * defines a "virtual button" at the given position, and calls the
 * callback proceedure whenever we would have set the button's postion.
 *
 * The callback proceedure is called with the specified callarg, and the
 * x and y position for the cell.
 */
/*
 * Just a way to use bind_button2 later to call reposition callback
 * which sets the x,y for the textfield file_fillin
 */
void
set_button_callback(
	struct header_data *hd,
	int row,
	int col,
	ButtonCallback callback,
	void *callarg
)
{
	struct binding *b;

	b = alloc_binding("", "");

	b->b_callback = callback;
	b->b_callarg = callarg;
	b->b_row = row;
	b->b_col = col;

	add_binding(hd, b);
}


