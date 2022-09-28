/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)format_xv.c	1.23	93/03/03 SMI"

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <string.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/xv_xrect.h>
#include <xview/notice.h>
#include <group.h>

#include "atool_i18n.h"
#include "ds_popup.h"		/* deskset popup routines */
#include "xv_defaults.h"	/* lives in ../config, for now */
#include "listmgr.h"
#include "undolist_c.h"
#include "format_panel_impl.h"
#include "format_ui.h"

Attr_attribute INSTANCE;		/* devguide global key ... */
Attr_attribute FORMATKEY;
Attr_attribute FORMAT_APPLY_KEY;	/* notify proc on apply or selection */
Attr_attribute FORMAT_CLIENT_KEY;	/* arg to apply proc */
Attr_attribute FORMAT_BANNER_KEY;

int format_selproc(ListMgr *lmp, int row);
int format_deselproc(ListMgr *lmp, int row);
int format_changeproc(ListMgr *lmp, caddr_t dp);
int format_insertproc(ListMgr *lmp, caddr_t dp);
int format_delproc(ListMgr *lmp, caddr_t dp);
int format_saveproc(ListMgr *lmp);

void format_menuitem_notify(Menu menu, Menu_item menu_item);
void create_format_notify(Menu menu, Menu_item menu_item);
void current_format_notify(Menu menu, Menu_item menu_item);
void format_shrink_panel(Panel_item item, Event *event);

/* Create a Format Panel */
ptr_t
FormatPanel_INIT(ptr_t owner, ptr_t dp)
{
	format_panel_objects		*ip;
	ListMgr				*lmp;
	struct format_panel_data	*fdp = (struct format_panel_data*)dp;
	int				i;
	int				j;
	int				k;
	int				max;
	int				top;
	Menu				m;
	Menu_item			mi;

	/* Init global keys, if necessary */
	if (INSTANCE == 0) {
		INSTANCE = xv_unique_key();
	}
	if (FORMATKEY == 0) {
	    	FORMATKEY = xv_unique_key();
	}
	if (FORMAT_APPLY_KEY == 0) {
	    	FORMAT_APPLY_KEY = xv_unique_key();
	}
	if (FORMAT_CLIENT_KEY == 0) {
	    	FORMAT_CLIENT_KEY = xv_unique_key();
	}
	if (FORMAT_BANNER_KEY == 0) {
	    	FORMAT_BANNER_KEY = xv_unique_key();
	}

	/* Initialize XView status panel */
	if ((ip = format_panel_objects_initialize(NULL, (Xv_opaque)owner)) ==
	    NULL) {
		return (NULL);
	}

	/* Initialize audio format menu item values */
	/* XXX - The menu should be set up more dynamically */
	m = (Menu) xv_get(ip->enc_button, PANEL_ITEM_MENU);
	mi = (Menu_item) xv_get(m, MENU_NTH_ITEM, 1);		/* u-law */
	xv_set(mi, XV_KEY_DATA, FORMAT_CLIENT_KEY, AUDIO_ENCODING_ULAW, NULL);
	mi = (Menu_item) xv_get(m, MENU_NTH_ITEM, 2);		/* A-law */
	xv_set(mi, XV_KEY_DATA, FORMAT_CLIENT_KEY, AUDIO_ENCODING_ALAW, NULL);
	mi = (Menu_item) xv_get(m, MENU_NTH_ITEM, 3);		/* linear */
	xv_set(mi, XV_KEY_DATA, FORMAT_CLIENT_KEY, AUDIO_ENCODING_LINEAR, NULL);

	m = (Menu) xv_get(ip->chan_button, PANEL_ITEM_MENU);
	mi = (Menu_item) xv_get(m, MENU_NTH_ITEM, 1);		/* mono */
	xv_set(mi, XV_KEY_DATA, FORMAT_CLIENT_KEY, 1, NULL);
	mi = (Menu_item) xv_get(m, MENU_NTH_ITEM, 2);		/* stereo */
	xv_set(mi, XV_KEY_DATA, FORMAT_CLIENT_KEY, 2, NULL);

	/* Save the address of local storage */
	xv_set(ip->panel, XV_KEY_DATA, FORMATKEY, dp, NULL);

	/* init the listmgr. the fmtlist is inited by the caller  */
	lmp = listmgr_init(ip->list, 
			   ip->label,
			   ip->list_controls,
			   (Xv_opaque)ip,
			   fdp->fmtlist,
			   format_selproc,
			   format_deselproc,
			   format_insertproc,
			   format_changeproc,
			   format_delproc,
			   format_saveproc,
			   FALSE); /* no auto-change */

	/* allow non-selection */
	xv_set(ip->list, PANEL_CHOOSE_NONE, TRUE, NULL);

	/* shrink the main panel width slightly */
	i = 8;
	xv_set(ip->controls, XV_WIDTH,
	    i + xv_get(ip->plus, XV_X) + xv_get(ip->plus, XV_WIDTH), NULL);

	/* move the minus button to reflect the plus button position */
	j =  xv_get(ip->plus, XV_Y);
	xv_set(ip->minus_group, XV_X, i, XV_Y, j, NULL);

	/* move the scrolling list group to reflect the encoding group */
	k = i + xv_get(ip->minus, XV_WIDTH) + 5;
	top = xv_get(ip->encoding, XV_Y);
	xv_set(ip->list_group, XV_X, k, XV_Y, top, NULL);

	/* extend the scrolling list to the edge of the name label field */
	max = xv_get(ip->label, XV_WIDTH) + xv_get(ip->label, XV_X);
	i = xv_get(ip->list, XV_WIDTH) + xv_get(ip->list, XV_X);
	xv_set(ip->list, PANEL_LIST_WIDTH,
	    xv_get(ip->list, PANEL_LIST_WIDTH) + (max - i), NULL);

	/* move the button group to the edge of the scrolling list */
	xv_set(ip->listbutton_group, XV_X, max + 10, XV_Y, top, NULL);

	/* set add/change/delete buttons to the same width */
	max = i = xv_get(ip->add, XV_WIDTH);
	if ((j = xv_get(ip->change, XV_WIDTH)) > max)
		max = j;
	if ((k = xv_get(ip->delete, XV_WIDTH)) > max)
		max = k;
	xv_set(ip->add, PANEL_LABEL_WIDTH,
	    xv_get(ip->add, PANEL_LABEL_WIDTH) + max - i, NULL);
	xv_set(ip->change, PANEL_LABEL_WIDTH,
	    xv_get(ip->change, PANEL_LABEL_WIDTH) + max - j, NULL);
	xv_set(ip->delete, PANEL_LABEL_WIDTH,
	    xv_get(ip->delete, PANEL_LABEL_WIDTH) + max - k, NULL);

	/* refit the extended panel */
	i = 8;
	xv_set(ip->list_controls, XV_WIDTH,
	    i + max + xv_get(ip->add, XV_X), NULL);

	/* fit the window and shrink the panel height a little bit */
	i = 8 + xv_get(ip->plus, XV_Y) + xv_get(ip->plus, XV_HEIGHT);
	xv_set(ip->panel, XV_HEIGHT, i + 2, NULL);
	xv_set(ip->list_controls, WIN_RIGHT_OF, ip->controls,
	    XV_HEIGHT, i, NULL);
	xv_set(ip->controls, XV_HEIGHT, i, NULL);

	fdp->panel_min_width = (int)xv_get(ip->controls, XV_WIDTH);
	fdp->frame_min_width = fdp->panel_min_width + 2;
	fdp->frame_max_width = fdp->frame_min_width +
	    (int)xv_get(ip->list_controls, XV_WIDTH);

	/* Now show just the basic panel and we're done */
	format_shrink_panel(ip->minus, (Event*) NULL);
	return ((ptr_t) ip->panel);
}

/* Set the Format Panel Label */
void
FormatPanel_SETLABEL(ptr_t cp, char *str)
{
	xv_set((Xv_opaque)cp, FRAME_LABEL, str, NULL);
}

/* set left footer label */
void
FormatPanel_SETLEFTFOOTER(ptr_t cp, char *str)
{
	xv_set((Xv_opaque)cp, FRAME_LEFT_FOOTER, str, NULL);
}

/* Pop up the File Panel */
void
FormatPanel_SHOW(ptr_t cp)
{
static int 				mapped = FALSE;
	format_panel_objects		*ip;
	struct format_panel_data	*dp;

	ip = (format_panel_objects *)
	    xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);
	dp = (struct format_panel_data *) 
	    xv_get(ip->panel, XV_KEY_DATA, FORMATKEY);
	
	/* position it */
	if (dp->owner && !mapped) {
		ds_position_popup((Xv_opaque)dp->owner, (Xv_opaque)dp->panel,
		    DS_POPUP_AOB);
		mapped = TRUE;
	}

	xv_set((Xv_opaque)cp, XV_SHOW, TRUE, NULL);
}

/* Set image string for Apply button */
void
FormatPanel_SETAPPLY(
	ptr_t			cp,
	char			*str)
{
	format_panel_objects	*ip;

	ip = (format_panel_objects *)
	    xv_get((Xv_opaque)cp, XV_KEY_DATA, INSTANCE);

	xv_set(ip->apply, PANEL_LABEL_STRING, str, NULL);
	xv_set(ip->basic_group, GROUP_LAYOUT, TRUE, NULL);
}

/* Dismiss the Format Panel */
void
FormatPanel_UNSHOW(ptr_t cp)
{
	xv_set((Xv_opaque)cp, FRAME_CMD_PUSHPIN_IN, FALSE, XV_SHOW, FALSE, 0);
}

int
load_format_panel(format_panel_objects *ip, struct format_list_entry *fle)
{
	do_load_format_from_header(ip, &fle->hdr);
	xv_set(ip->label, PANEL_VALUE, fle->label, NULL);

	if (fle->readonly) {
		xv_set(ip->change, PANEL_INACTIVE, TRUE, NULL);
		xv_set(ip->delete, PANEL_INACTIVE, TRUE, NULL);
	} else {
		xv_set(ip->change, PANEL_INACTIVE, FALSE, NULL);
		xv_set(ip->delete, PANEL_INACTIVE, FALSE, NULL);
	}
}

/*
 * search the list for a format that matches the header. if it's
 * there, select that and load. if not, load and unselect.
 */
int
load_format_from_header(ptr_t panel, Audio_hdr *hdrp)
{
	format_panel_objects		*ip;
	struct format_panel_data	*dp;
	static int 			mapped = FALSE;
	char *cp;

	ip = (format_panel_objects *) xv_get((Xv_opaque)panel,
					     XV_KEY_DATA, INSTANCE);
	dp = (struct format_panel_data *) 
	    xv_get(ip->panel, XV_KEY_DATA, FORMATKEY);

	if (cp = find_format_hdr(dp, hdrp)) {
		listmgr_select_item((Xv_opaque)panel, cp);
	} else {
		/* can't find in list, unselect ... */
		do_load_format_from_header(ip, hdrp);
		unselect_list_item(ip->list);
	}
}

int
do_load_format_from_header(
	format_panel_objects	*ip,
	Audio_hdr		*hdrp)
{
	char			*cp;

	cp = audio_print_rate(hdrp);
	xv_set(ip->rate, PANEL_VALUE, cp, NULL);
	free(cp);

	cp = audio_print_encoding(hdrp);
	xv_set(ip->encoding, PANEL_VALUE, cp,
	    XV_KEY_DATA, FORMAT_CLIENT_KEY, hdrp->encoding, NULL);
	free(cp);

	cp = audio_print_channels(hdrp);
	xv_set(ip->channels, PANEL_VALUE, cp,
	    XV_KEY_DATA, FORMAT_CLIENT_KEY, hdrp->channels, NULL);
	free(cp);
}

int
format_selproc(ListMgr *lmp, int row)
{
	format_panel_objects *ip = (format_panel_objects *) lmp->ip;
	struct format_list_entry *fle;
	Link *lp;

	if (!(lp = GetLink(lmp->list, row))) {
		return (XV_ERROR);
	}

	if (!(fle = (struct format_list_entry*)lp->dp)) {
		return (XV_ERROR);
	}

	xv_set(ip->delete, PANEL_INACTIVE, FALSE, NULL);
	xv_set(ip->change, PANEL_INACTIVE, FALSE, NULL);

	load_format_panel(ip, fle);

	return (XV_OK);
}

int
format_deselproc(ListMgr *lmp, int row)
{
	format_panel_objects *ip = (format_panel_objects *) lmp->ip;

	xv_set(ip->delete, PANEL_INACTIVE, TRUE, NULL);
	xv_set(ip->change, PANEL_INACTIVE, TRUE, NULL);
	return (XV_OK);
}

int
get_format(
	format_panel_objects	*ip,
	Audio_hdr		*hdrp)
{
	char			*cp;
	Audio_hdr		hdr;

	if (((cp = (char*)xv_get(ip->rate, PANEL_VALUE)) == NULL) ||
	    (audio_parse_rate(cp, &hdr) != AUDIO_SUCCESS)) {
		(void) notice_prompt(ip->panel, (Event *) 0,
		    NOTICE_MESSAGE_STRINGS,
		    MGET("Please enter a sample rate"),
		    0,
		    NOTICE_BUTTON_YES, MGET("Continue"),
		    NOTICE_LOCK_SCREEN, FALSE,
		    0);
		return (FALSE);
	}
	if (audio_parse_encoding(
	    (char*) xv_get(ip->encoding, PANEL_VALUE), &hdr) != AUDIO_SUCCESS) {
		/*
		 * XXX - if the parsing the encoding string fails,
		 * then revert to assumptions about the menu items.
		 * This needs to be replaced with something real.
		 */
		hdr.encoding = (unsigned)
		    xv_get(ip->encoding, XV_KEY_DATA, FORMAT_CLIENT_KEY);
		hdr.samples_per_unit = 1;
		if (hdr.encoding == AUDIO_ENCODING_LINEAR) {
			hdr.bytes_per_unit = 2;
		} else {
			hdr.bytes_per_unit = 1;
		}
	}
	if (audio_parse_channels(
	    (char*) xv_get(ip->channels, PANEL_VALUE), &hdr) != AUDIO_SUCCESS) {
		hdr.channels = (unsigned)
		    xv_get(ip->channels, XV_KEY_DATA, FORMAT_CLIENT_KEY);
	}

	memcpy((char*)hdrp, (char*)&hdr, sizeof (Audio_hdr));
	return (TRUE);
}
	
int
format_changeproc(
	ListMgr				*lmp,
	caddr_t				dp)
{
	format_panel_objects		*ip = (format_panel_objects *) lmp->ip;
	struct format_panel_data	*fdp;
	struct format_list_entry	*fle;
	struct format_list_entry	*tfle;
	Link				*lp;
	char				*label;
	char				*cp;
	Audio_hdr			hdr;
	int				i;

	fdp = (struct format_panel_data *)
		xv_get(ip->panel, XV_KEY_DATA, FORMATKEY);

	if (!(fle = (struct format_list_entry*)dp))
		return (-1);

	if (fle->readonly) {
		/* XXX - just ignore (don't change) if read-only entry? */
		return (0);
	}

	/* if label is inactive, nothing was selected to begin with */
	if (xv_get(ip->label, PANEL_INACTIVE)) {
		return (0);
	}

	/* get values from various panel items and store them */
	if (((label = (char *)xv_get(ip->label, PANEL_VALUE)) == NULL) ||
	    (*label == NULL)) {
		(void) notice_prompt(ip->panel, (Event *) 0,
		    NOTICE_MESSAGE_STRINGS,
		    MGET("Please enter a format name"),
		    0,
		    NOTICE_BUTTON_YES, MGET("Continue"),
		    NOTICE_LOCK_SCREEN, FALSE,
		    0);
		return (-1);
	}

	if (!get_format(ip, &hdr))
		return (-1);

	/* see if this new entry is a dup. if so, post msg and return error */
	for (i = 0, lp = lmp->list->head;
	    lp && (i < lmp->list->num);
	    lp=lp->next, i++) {
		tfle = (struct format_list_entry*)lp->dp;
		if (fle == tfle) {
			/* pointing at myself! next please... */
			continue;
		}
		/* duplicate name */
		if (strcmp(label, tfle->label) == 0) {
			(void) notice_prompt(ip->panel, (Event *) 0,
			    NOTICE_MESSAGE_STRINGS,
			    MGET("An entry with this name already exists."),
			    MGET("Please choose another name."),
			    0,
			    NOTICE_BUTTON_YES, MGET("Continue"),
			    NOTICE_LOCK_SCREEN, FALSE,
			    0);
			return (-1);
		}
		/* duplicate header */
		if (audio_cmp_hdr(&hdr, &(tfle->hdr)) == 0) {
			(void) notice_prompt(ip->panel, (Event *) 0,
			    NOTICE_MESSAGE_STRINGS, 
		MGET("An entry with these settings already exists."),
		MGET("Please select different settings, or delete this entry."),
		0,
			    NOTICE_BUTTON_YES, MGET("Continue"),
			    NOTICE_LOCK_SCREEN, FALSE,
			    0);
			return (-1);
		}
	}
	change_format_entry(fle, label, &hdr);
	return (1);
}

int
format_insertproc(ListMgr *lmp, caddr_t dp)
{
	format_panel_objects *ip = (format_panel_objects *) lmp->ip;
	struct format_list_entry *fle = (struct format_list_entry *)dp;

	if (fle->readonly) {
		/* XXX - just ignore if read-only entry? */
		return (XV_OK);
	}

	/* get values from various panel items and store them */
	if (!get_format(ip, &(fle->hdr))) {
		return (XV_ERROR);
	}
	return (XV_OK);
}

int
format_delproc(ListMgr *lmp, caddr_t dp)
{
	format_panel_objects *ip = (format_panel_objects *) lmp->ip;
	struct format_panel_data *fdp;

	fdp = (struct format_panel_data *)
		xv_get(ip->panel, XV_KEY_DATA, FORMATKEY);

	return (XV_OK);
}

Menu
do_create_format_menu(
	Menu		menu,			/* parent menu */
	ptr_t		dp,			/* format panel data ptr */
	int		(*apply_proc)(),	/* apply proc */
	int		(*valid_fmt_proc)(),	/* is this a valid format? */
	Audio_hdr	*curhdr,		/* current format */
	ptr_t		cdata,			/* client data */
	char		*banner)		/* banner for popup panel */
{
	Menu_item	mi;
	int		i;
	Link		*lp;
	char		*s;
	struct format_panel_data *fdp;
	format_panel_objects *ip;
	struct format_list_entry *fle;

	fdp = (struct format_panel_data *)dp;
	ip = (format_panel_objects *)
	    xv_get((Xv_opaque)fdp->panel, XV_KEY_DATA, INSTANCE);

	if ((menu == NULL) || ((i = (int) xv_get(menu, MENU_NITEMS)) < 1)) {
		/* didn't supply one - create one */
		if (menu == NULL)
			menu = xv_create(XV_NULL, MENU, NULL);

		/* first item is always "Current Format" */
		mi = xv_create(XV_NULL, MENUITEM,
			       MENU_STRING, MGET("Current Format"),
			       XV_KEY_DATA, FORMATKEY, (Xv_opaque)fdp,
			       XV_KEY_DATA, FORMAT_CLIENT_KEY, curhdr,
			       MENU_NOTIFY_PROC, current_format_notify,
			       NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
		xv_set(menu, MENU_DEFAULT_ITEM, mi, NULL);
	} else {
		/* 
		 * destroy all existing items - EXCEPT the first one
		 * which is the "Current format" item (and default).
		 */
		for (/* i = (int) xv_get(menu, MENU_NITEMS) */; i > 1; i--) {
			xv_set(menu, MENU_REMOVE, i, NULL);
			xv_destroy(xv_get(menu, MENU_NTH_ITEM, i));
		}
	}
	xv_set(menu,
	       XV_KEY_DATA, FORMATKEY, (Xv_opaque)fdp,
	       XV_KEY_DATA, FORMAT_CLIENT_KEY, cdata,
	       XV_KEY_DATA, FORMAT_APPLY_KEY, apply_proc,
	       XV_KEY_DATA, FORMAT_BANNER_KEY, banner,
	       NULL);

	/* don't do anything if not initialized yet */
	if (fdp->fmtlist) {
		for (i = 0, lp = fdp->fmtlist->head;
		     lp && (i < fdp->fmtlist->num);
		     lp=lp->next, i++) {
			if (fle = (struct format_list_entry*)lp->dp) {
				/* 
				 * XXX - define GREY_UNSUPPORTED if you want 
				 * unsupported formats greyed out
				 * instead of removed.
				 */
#ifndef GREY_UNSUPPORTED
				/* 
				 * check if this item is a valid format for
				 * this operation. if not, don't create an
				 * item for it.
				 */
				if (valid_fmt_proc &&
				    !(*valid_fmt_proc)(cdata, &(fle->hdr))) {
					continue;
				}
#endif
				mi = xv_create(XV_NULL, MENUITEM,
				    MENU_STRING, fle->label,
				    XV_KEY_DATA, FORMATKEY, (Xv_opaque)fdp,
				    MENU_NOTIFY_PROC, format_menuitem_notify,
				    NULL);

#ifdef GREY_UNSUPPORTED
				/* 
				 * check if this item is a valid format for
				 * this operation. if not, inactivate the
				 * menu item for it.
				 */
				if (valid_fmt_proc &&
				    !(*valid_fmt_proc)(cdata, &(fle->hdr))) {
					xv_set(mi, MENU_INACTIVE, TRUE, NULL);
				}
#endif
				xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
			}
		}

		/* now add the "New format..." item (first a NULL item). */
		mi = xv_create(XV_NULL, MENUITEM,
			       MENU_STRING, " ",
			       MENU_INACTIVE, TRUE,
			       NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);

		mi = xv_create(XV_NULL, MENUITEM,
			       MENU_STRING, MGET("New Format..."),
			       XV_KEY_DATA, FORMATKEY, (Xv_opaque)fdp,
			       MENU_NOTIFY_PROC, create_format_notify,
			       NULL);
		xv_set(menu, MENU_APPEND_ITEM, mi, NULL);
	}
	return (menu);
}

void
create_format_notify(
	Menu		menu,
	Menu_item	menu_item)
{
	struct format_panel_data *dp;
	ptr_t		 cdata;
	int		(*apply_proc)();
	char		*banner;

	cdata = (ptr_t) xv_get(menu, XV_KEY_DATA, FORMAT_CLIENT_KEY);
	dp = (struct format_panel_data *) xv_get(menu, XV_KEY_DATA, FORMATKEY);
	banner = (ptr_t) xv_get(menu, XV_KEY_DATA, FORMAT_BANNER_KEY);
 	apply_proc = (PFUNCI) xv_get(menu, XV_KEY_DATA, FORMAT_APPLY_KEY);
	FormatPanel_Show(dp, apply_proc, cdata, banner);
	return;
}

void
current_format_notify(
	Menu		menu,
	Menu_item	menu_item)
{
	int		(*apply_proc)();
	ptr_t		cdata;
	Audio_hdr	*curhdr;

 	cdata = (ptr_t) xv_get(menu, XV_KEY_DATA, FORMAT_CLIENT_KEY);
 	apply_proc = (PFUNCI) xv_get(menu, XV_KEY_DATA, FORMAT_APPLY_KEY);

	/* try to re-load the current format */
	curhdr = (Audio_hdr*) xv_get(menu_item, XV_KEY_DATA, FORMAT_CLIENT_KEY);
	if ((*apply_proc)(cdata, curhdr) == FALSE) {
		/* if this failed, leave the values on the panel alone */
		return;
	}
	return;
}

void
format_menuitem_notify(
	Menu		menu,
	Menu_item	menu_item)
{
	int		(*apply_proc)();
	ptr_t		cdata;
	Link		*link;
	char		*label;
	struct format_list_entry *fle;
	struct format_panel_data *dp;

	dp = (struct format_panel_data *)xv_get(menu, XV_KEY_DATA, FORMATKEY);

	cdata = (ptr_t)xv_get(menu, XV_KEY_DATA, FORMAT_CLIENT_KEY);
 	apply_proc = (PFUNCI)xv_get(menu, XV_KEY_DATA, FORMAT_APPLY_KEY);
	if (((label = (char*) xv_get(menu_item, MENU_STRING)) == NULL) ||
	    (*label == '\0')) {
		/* XXX - error msg? */
		return;
	}

	if ((link = FindLink(dp->fmtlist, label)) == NULL) {
		/* XXX - error msg? */
		return;
	}

	if ((fle = (struct format_list_entry *)link->dp) == NULL) {
		/* XXX - error msg? */
		return;
	}

	if ((*apply_proc)(cdata, &fle->hdr) == FALSE) {
		/* if this failed, leave values on the panel alone */
		return;
	} else {
		/* update local copy of header */
		memcpy((char*)&(dp->curhdr), (char*)&(fle->hdr),
		    sizeof (Audio_hdr));
	}
	return;
}
		
/*
 * Notify function for `apply'.
 */
void
format_apply(
	Panel_item			item,
	Event				*event)
{
	format_panel_objects		*ip;
	struct format_panel_data	*dp;
	Audio_hdr			hdr;
	int				status;

	ip = (format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	dp = (struct format_panel_data*)
	    xv_get(ip->panel, XV_KEY_DATA, FORMATKEY);

	/*
	 * need to do 2 things: if the list panel is up, apply
	 * changes and add to list. also, get values from panel
	 * and call user's apply proc.
	 */
	if (!get_format(ip, &hdr)) {
		return;
	}

	status = dp->apply_proc(dp->cdata, &hdr);

	if (status != TRUE) {
		/* if this failed, leave the values on the panel alone */
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
	} else {
		/* update local copy of header */
		memcpy((char*)&(dp->curhdr), (char*)&hdr, sizeof (Audio_hdr));
	}
}

/*
 * Notify function for `reset'.
 */
void
format_reset(Panel_item item, Event *event)
{
	struct format_panel_data *dp;
	format_panel_objects *ip =
		(format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	dp = (struct format_panel_data*) xv_get(ip->panel, 
						 XV_KEY_DATA, FORMATKEY);

	load_format_from_header(dp->panel, &dp->curhdr);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}


/*
 * Notify function for `plus'.
 */
void
format_grow_panel(Panel_item item, Event *event)
{
	struct format_panel_data *dp;
	int w;
	format_panel_objects *ip =
		(format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	dp = (struct format_panel_data*) xv_get(ip->panel, 
						 XV_KEY_DATA, FORMATKEY);

	/* set panel to proper size - and expose the scroll list panel */
	xv_set(ip->list_controls, XV_SHOW, TRUE, NULL);
	xv_set(ip->panel, XV_WIDTH, dp->frame_max_width, NULL);

	xv_set(ip->plus, PANEL_INACTIVE, TRUE, NULL);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

/*
 * Notify function for `minus'.
 */
void
format_shrink_panel(Panel_item item, Event *event)
{
	format_panel_objects *ip =
		(format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	struct format_panel_data *dp;
	int w;
	
	dp = (struct format_panel_data*) xv_get(ip->panel, 
						 XV_KEY_DATA, FORMATKEY);

	/* set panel to proper size - and hide the scroll list panel */
	xv_set(ip->list_controls, XV_SHOW, FALSE, NULL);
	xv_set(ip->panel, XV_WIDTH, dp->frame_min_width, NULL); 

	xv_set(ip->plus, PANEL_INACTIVE, FALSE, NULL);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

/* set sample rate text item from menu */
Menu_item
sample_rate_notify(
	Menu_item		item,
	Menu_generate		op)
{
	char			*cp;
	Menu			m;
	Audio_hdr		hdr;
	format_panel_objects	*ip;

	ip = (format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	if (op == MENU_NOTIFY) {
		if ((cp = (char*)xv_get(item, MENU_STRING)) && *cp) {
			/* Parse and re-print sample rate */
			(void) audio_parse_rate(cp, &hdr);
			cp = audio_print_rate(&hdr);
			xv_set(ip->rate, PANEL_VALUE, cp, NULL);
			free(cp);
		}
	}

	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}
	return (item);
}

/* set encoding text item from menu */
Menu_item
enc_menu_notify(
	Menu_item		item,
	Menu_generate		op)
{
	char			*cp;
	Menu			m;
	format_panel_objects	*ip;

	ip = (format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	if (op == MENU_NOTIFY) {
		if ((cp = (char*)xv_get(item, MENU_STRING)) && *cp) {
			while (*cp == ' ')
				cp++;
			xv_set(ip->encoding, PANEL_VALUE, cp,
			    XV_KEY_DATA, FORMAT_CLIENT_KEY,
			    xv_get(item, XV_KEY_DATA, FORMAT_CLIENT_KEY), NULL);
		}
	}

	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}
	return (item);
}

/* set channels text item from menu */
Menu_item
channel_notify(
	Menu_item		item,
	Menu_generate		op)
{
	char			*cp;
	Menu			m;
	Audio_hdr		hdr;
	format_panel_objects	*ip;

	ip = (format_panel_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	if (op == MENU_NOTIFY) {
		if ((cp = (char*)xv_get(item, MENU_STRING)) && *cp) {
			while (*cp == ' ')
				cp++;
			xv_set(ip->channels, PANEL_VALUE, cp,
			    XV_KEY_DATA, FORMAT_CLIENT_KEY,
			    xv_get(item, XV_KEY_DATA, FORMAT_CLIENT_KEY), NULL);
		}
	}

	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}
	return (item);
}

/*
 * Notify function for `enc_button'.
 */
void
enc_button_notify(Panel_item item, Event *event)
{
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

/*
 * Notify function for `rate_button'.
 */
void
rate_button_notify(Panel_item item, Event *event)
{
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

void
chan_button_notify(Panel_item item, Event *event)
{
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

void
add_button_notify(Panel_item item, Event *event)
{
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}


format_saveproc(ListMgr *lmp)
{
	struct format_panel_data *dp;
	format_panel_objects *ip;
	char val[XV_DEFAULTS_MAX_VALUE_SIZE];
	char buf[XV_DEFAULTS_MAX_VALUE_SIZE];
	char *fmtstr;
	struct format_list_entry *fle;
	Link *lp;
	int i;

	ip = (format_panel_objects *)lmp->ip;
	dp = (struct format_panel_data*)xv_get(ip->panel,
					       XV_KEY_DATA, FORMATKEY);

	val[0] = NULL;
	for (i = 0, lp = lmp->list->head;
	     lp && (i < lmp->list->num); lp=lp->next, i++) {
		if (fle = (struct format_list_entry*)lp->dp) {
			if (fle->readonly) {
				continue;
			}
			if ((fmtstr = audio_printhdr(&(fle->hdr))) == NULL) {
				continue;
			}
			sprintf(buf, "%s%s:%s",	(val[0] ? ";" : ""), 
				fle->label, fmtstr);
			if (val[0] == NULL) {
				strncpy(val, buf, XV_DEFAULTS_MAX_VALUE_SIZE);
			} else {
				strncat(val, buf, XV_DEFAULTS_MAX_VALUE_SIZE);
			}
			free(fmtstr);
		}
	}
	val[XV_DEFAULTS_MAX_VALUE_SIZE] = NULL;	/* just in case ... */

	AudPanel_Setdefault(dp->owner, "formatList", val);
	AudPanel_Writedefaults(dp->owner);
}


/*
 * Done function for `panel'.
 */
void
format_panel_done(Frame frame)
{
	format_panel_objects *ip = 
		(format_panel_objects *) xv_get(frame, XV_KEY_DATA, INSTANCE);
	struct format_panel_data *dp;

	dp = (struct format_panel_data *) xv_get(frame, XV_KEY_DATA, FORMATKEY);

	xv_set(frame, XV_SHOW, FALSE, NULL);
}

append_format(struct format_panel_data *dp, struct format_list_entry *fle)
{
	listmgr_append_link((Xv_opaque)dp->panel, (caddr_t)fle);
}

/*
 * Event function for `panel'.
 */
Notify_value
format_panel_event_callback(Xv_window win, Event *event, 
			    Notify_arg arg, Notify_event_type type)
{
	format_panel_objects *ip = 
		(format_panel_objects *) xv_get(win, XV_KEY_DATA, INSTANCE);
	
	/* gxv_start_connections DO NOT EDIT THIS SECTION */

	if (event_action(event) == WIN_RESIZE)
	{
	}
	
	/* gxv_end_connections */

	return (notify_next_event_func(win, (Notify_event) event, arg, type));
}
