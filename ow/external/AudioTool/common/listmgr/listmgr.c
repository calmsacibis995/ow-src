/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)listmgr.c	1.9	92/11/18 SMI"

/*
 * routines to handle editing a scrolling list associated with a linked list
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>

#include "listmgr.h"

/* XXX - gross hack */
/* cast an instance ptr to this ... */
struct guide_objects {
	Xv_opaque frame;
};

extern Attr_attribute INSTANCE;	/* attach LMGR_KEY key to this! */
Attr_attribute	LMGR_KEY = NULL;

/* public functions - and callbacks */

/*
 * initialize a list manager
 */


int listmgr_load (ListMgr *lmp);
int listmgr_do_delete (ListMgr *lmp, int sflag);
int get_insert_position (char *s);
int save_and_deselect (ListMgr *lmp, int i);
int listmgr_do_change (ListMgr *lmp);

extern int append_list_item (Panel_item list, char *s);
extern int replace_selected_list_item (Panel_item list, char *s);
extern int delete_selected_list_item (Panel_item list);
extern int replace_list_item (Panel_item list, char *s, int i);
extern int set_selected_list_index (Panel_item list, int index);
extern int get_selected_list_index (Panel_item list);
extern int insert_list_item (Panel_item list, char *s, int where);

ListMgr *
listmgr_init(Panel_item litem,			/* list item */
	     Panel_item titem,			/* "primary" text item */
	     Panel panel,			/* to attach key data to */
	     Xv_opaque ip,			/* guide instance ptr */
	     List *list,			/* linked list */
	     int (*sel_proc) (/* ??? */),	/* when an item is selected */
	     int (*desel_proc) (/* ??? */),	/* "" de-selected */
	     int (*insert_proc) (/* ??? */),    /* when an item is inserted */
	     int (*change_proc) (/* ??? */),	/* when an item is changed */
	     int (*delete_proc) (/* ??? */),	/* when an item is deleted */
	     int (*store_proc) (/* ??? */),	/* store list after a change */
	     int changeflag)			/* auto-change when new item */
{						/* is selected? */             
	struct guide_objects *gop;
	ListMgr *lmp;

	if (!LMGR_KEY) {
		LMGR_KEY = xv_unique_key();
	}
	
	if (!(lmp = (ListMgr *)calloc(1, sizeof(ListMgr)))) {
		return(NULL);
	}

	lmp->list_item = litem;
	lmp->text_item = titem;
	lmp->panel = panel;
	lmp->ip = ip;
	lmp->list = list;
	lmp->sel_proc = sel_proc;
	lmp->desel_proc = desel_proc;
	lmp->insert_proc = insert_proc;
	lmp->change_proc = change_proc;
	lmp->delete_proc = delete_proc;
	lmp->store_proc = store_proc;

#ifndef notdef
	/*
	 * attach key data to instance ptr (which will be unique)
	 */
	/* XXX - gross hack */
	gop = (struct guide_objects *)ip;
	xv_set(gop->frame, XV_KEY_DATA, LMGR_KEY, lmp, NULL);
#else

	/* attach key data to panel */
	xv_set(panel, XV_KEY_DATA, LMGR_KEY, lmp, NULL);
#endif
	/* make sure the list has the right attr's set (exclusive only) */
	xv_set(litem, 
	       PANEL_CHOOSE_ONE, TRUE,
	       PANEL_CHOOSE_NONE, FALSE,
	       NULL);

	listmgr_load(lmp);

	return (lmp);
}

/* alloc this much at a time to create avlist */

#define NUMTOALLOC  25

int
listmgr_load(ListMgr *lmp)
{
	Link *lp;
	int i, num;
	char **alist;		/* do it the fast way! */
	int asize = 0, leftover = 0;
	char *label;

	if (!(lmp->list && lmp->list->getlabel)) {
		return (-1);
	}

	alist = (char **) malloc(NUMTOALLOC * sizeof (char*));
	leftover = NUMTOALLOC-1;
	asize = NUMTOALLOC;

	alist[0] = (char*) PANEL_LIST_STRINGS;


	/* load the list with the items */
	for(lp = lmp->list->head, num=i=0; lp && (i<lmp->list->num);
	    lp=lp->next, i++) {
		if (label = lmp->list->getlabel(lmp->list, lp->dp)) {
			if (leftover<=0) {
				asize += NUMTOALLOC;
				leftover = NUMTOALLOC;
				alist = (char **)realloc(alist, 
						 asize * sizeof(char*));
			}
			alist[num+1] = label;
			leftover--;
			num++;
#ifdef notdef
			xv_set(lmp->list_item, 
			       PANEL_LIST_STRING, i, label,
			       NULL);
#endif
		}
	}

	if ((leftover-2) < 0) {
		alist = (char **)realloc(alist, asize+2);
	}
	alist[++num] = (char*)NULL;
	alist[++num] = (char*)NULL;

	xv_set(lmp->list_item, ATTR_LIST, alist, NULL);
	free(alist);

	/* set first item as selected */
	if (lmp->list->num) {
		set_selected_list_index(lmp->list_item, 0);
		if (lmp->sel_proc) {
			lmp->sel_proc(lmp, 0);
		}
	}
	return(0);
}

ListMgr *
listmgr_getkey(Xv_opaque item)
{
	Xv_opaque ip;
	struct guide_objects *gop;

	Xv_pkg *ptype;
	Menu_item mi;
	Event *e;

#ifndef notdef
	if (!(ip = xv_get(item, XV_KEY_DATA, INSTANCE))) {
		return(NULL);
	}
	gop = (struct guide_objects *)ip;

	return((ListMgr*)xv_get(gop->frame, XV_KEY_DATA, LMGR_KEY));
#else

	/*
	 * first see if it's a menu/menu item or other. if it's a menu
	 * or menu item, get it's owner (the button), then get its owner
	 * (the panel), then get the key....
	 */
	ptype = (Xv_pkg*)xv_get(item, XV_TYPE);
	if (!ptype) {
		return (NULL);
	} else if ((ptype == MENU) || (ptype == MENUITEM)
		   || (ptype == MENU_COMMAND_MENU)) {
		/* 
		 * find the panel item that owns of this sucka by 
		 * looking at the MENU_PARENT of each object until there
		 * is none.
		 */
#ifdef notdef
		while (mi = xv_get(item, MENU_PARENT)) {
			item = mi;
		}
#endif

		/* now get first event that brought menu up - to get panel */
		if (!(e = (Event*)xv_get(item, MENU_FIRST_EVENT))) {
			return(NULL);
		}
		item = e->ie_win;
	}

	while(item) {
		if ((Xv_pkg*)xv_get(item, XV_TYPE) == PANEL) {
			return((ListMgr*)xv_get(item, XV_KEY_DATA, LMGR_KEY));
		}
		item = xv_get(item, XV_OWNER);
	}

	return (NULL);
#endif
}

/*
 * Menu handler for `edit_menu (cut)'.
 */
Menu_item
listmgr_cut(Menu_item item, Menu_generate op)
{
	ListMgr *lmp;

	RGET_LMP(lmp, item, "listmgr_cut", item);

 	return(do_listmgr_cut(lmp, item, op));
}

Menu_item
do_listmgr_cut(ListMgr *lmp, Menu_item item, Menu_generate op)
{	
	Menu m;

	switch (op) {
#ifdef notdef
	case MENU_DISPLAY:
		if (get_selected_list_index(lmp->list_item) < 0) {
			xv_set(item, MENU_INACTIVE, TRUE, NULL);
		}
		break;
	case MENU_DONE:
		xv_set(item, MENU_INACTIVE, FALSE, NULL);
		break;
#endif
	case MENU_NOTIFY:
		listmgr_do_delete(lmp, 1);
		break;
	}

	/* make sure popup isn't dismissed */
	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}

	return item;
}

Menu_item
listmgr_menu_delete(Menu_item item, Menu_generate op)
{
	ListMgr *lmp;

	RGET_LMP(lmp, item, "listmgr_menu_delete", item);
	
	return(do_listmgr_menu_delete(lmp, item, op));
}

/*
 * Menu handler for `edit_menu (delete)'.
 */
Menu_item
do_listmgr_menu_delete(ListMgr *lmp, Menu_item item, Menu_generate op)
{
	Menu m;
	
	switch (op) {
#ifdef notdef
	case MENU_DISPLAY:
		if (get_selected_list_index(lmp->list_item) < 0) {
			xv_set(item, MENU_INACTIVE, TRUE, NULL);
		}
		break;
	case MENU_DONE:
		xv_set(item, MENU_INACTIVE, FALSE, NULL);
		break;
#endif
	case MENU_NOTIFY:
		listmgr_do_delete(lmp, 0);
		break;
	}

	/* make sure popup isn't dismissed */
	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}

	return item;
}

Menu_item
listmgr_copy(Menu_item item, Menu_generate op)
{
	ListMgr *lmp;
	Link *link;
	int i;

	RGET_LMP(lmp, item, "listmgr_copy", item);
	return(do_listmgr_copy(lmp, item, op));
}

/*
 * Menu handler for `edit_menu (copy)'.
 */
Menu_item
do_listmgr_copy(ListMgr *lmp, Menu_item item, Menu_generate op)
{
	Link *link;
	int i;
	Menu m;

	switch (op) {
#ifdef notdef
	case MENU_DISPLAY:
		if (get_selected_list_index(lmp->list_item) < 0) {
			xv_set(item, MENU_INACTIVE, TRUE, NULL);
		}
		break;
	case MENU_DONE:
		xv_set(item, MENU_INACTIVE, FALSE, NULL);
		break;
#endif
	case MENU_NOTIFY:
		if ((i = get_selected_list_index(lmp->list_item)) >= 0) {
			link = GetLink(lmp->list, i);
			if (lmp->list->delfcn) {
				lmp->list->delfcn(lmp->list, lmp->cutbuf);
			}
			lmp->cutbuf = CopyLinkData(lmp->list, link);
		}
		break;

	case MENU_NOTIFY_DONE:
		break;
	}

	/* make sure popup isn't dismissed */
	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}

	return item;
}

Menu_item
listmgr_paste(Menu_item item, Menu_generate op)
{
	ListMgr *lmp;
	
	RGET_LMP(lmp, item, "listmgr_paste", item);
	return(do_listmgr_paste(lmp, item, op));
}

/*
 * Menu handler for `edit_menu (paste)'.
 */
Menu_item
do_listmgr_paste(ListMgr *lmp, Menu_item item, Menu_generate op)
{
	caddr_t dp;
	char *s;
	char *label;
	int i, pos;
	Menu m;

	switch (op) {
	case MENU_DISPLAY:
		if (! lmp->cutbuf) {
			xv_set(item, MENU_INACTIVE, TRUE, NULL);
		} else {
			xv_set(item, MENU_INACTIVE, FALSE, NULL);
		}
		break;

	case MENU_NOTIFY:
		if (!lmp->cutbuf) {
			return item;
		}
		
		if (lmp->list->cpyfcn) {
			dp = lmp->list->cpyfcn(lmp->list, lmp->cutbuf);
		}
		if (lmp->list->getlabel) {
			s = lmp->list->getlabel(lmp->list, dp);
			pos = get_insert_position((char*)xv_get(item,
					MENU_STRING));
		
			i = insert_list_item(lmp->list_item, s, pos);
			InsertLink(lmp->list, s, dp, i);
#ifdef notdef
			DBGOUT(("pasting item %s, pos = %d, item # %d\n",
				s, pos, i));
#endif
			/* now select that item */
			set_selected_list_index(lmp->list_item, i);
			if (lmp->sel_proc) {
				lmp->sel_proc(lmp, i);
			}
			
			/* store list */
			if (lmp->store_proc) {
				lmp->store_proc(lmp);
			}
		}
		break;
	}

	/* make sure popup isn't dismissed */
	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}

	return item;
}

int
listmgr_notify(Panel_item item, char *string, Xv_opaque client_data, Panel_list_op op, Event *event, int row)
{
	ListMgr *lmp;

	RGET_LMP(lmp, item, "listmgr_notify", XV_ERROR);
	return(do_listmgr_notify(lmp, item, string, client_data, op, 
				 event, row));
}

/*
 * Notify callback function for `list1'.
 */
int
do_listmgr_notify(ListMgr *lmp, Panel_item item, char *string, 
		  Xv_opaque client_data, Panel_list_op op, 
		  Event *event, int row)
{
	int pos;

	switch(op) {
	case PANEL_LIST_OP_DESELECT:
		lmp->last_save = save_and_deselect(lmp, row);
		lmp->last_row = row;
		break;

	case PANEL_LIST_OP_SELECT:
		/* if last DESELECT was an error, re-select the previous row */
		if ((lmp->last_save == -1) && (lmp->last_row >= 0)) {
			set_selected_list_index(lmp->list_item, lmp->last_row);
		} else {
			if (lmp->sel_proc) {
				pos = get_selected_list_index(lmp->list_item);
#ifdef notdef
				DBGOUT(("selecting line %d\n", pos));
#endif
				lmp->sel_proc(lmp, pos);
			}
		}
		break;

	case PANEL_LIST_OP_VALIDATE:
		break;

	case PANEL_LIST_OP_DELETE:
		break;
	}
	
	return XV_OK;
}

void
listmgr_create(Panel_item item, Event *event)
{
	ListMgr *lmp;

	GET_LMP(lmp, item, "listmgr_create");

	do_listmgr_create(lmp, item, event);
}

/*
 * Notify callback function for `create'.
 */
void
do_listmgr_create(ListMgr *lmp, Panel_item item, Event *event)
{
	caddr_t dp = NULL;
	char *s;
	int pos, i;

	/* deselect item if one was selected */
	if ((i = get_selected_list_index(lmp->list_item)) >= 0) {
		if (save_and_deselect(lmp, i) == -1) {
			/* XXX - error - don't create a new one */
			return;
		}
	}

	if (lmp->list->newfcn) {
		dp = lmp->list->newfcn(lmp->list);
	}

	/* nothing to insert ... */
	if (!dp) {
		return;
	}

	/* if there's an insert proc, run it here */
	if (lmp->insert_proc) {
		/* XXX - check errors? */
		lmp->insert_proc(lmp, dp);
	}

	if (lmp->list->getlabel) {
		s = lmp->list->getlabel(lmp->list, dp);
		pos = append_list_item(lmp->list_item, s);
		AppendLink(lmp->list, s, dp);
	}

	set_selected_list_index(lmp->list_item, pos);

	if (lmp->sel_proc) {
		lmp->sel_proc(lmp, pos);
	}

	/* highlite the "primary" text item, if one was supplied */
	if (lmp->text_item) {
		xv_set(lmp->panel, PANEL_CARET_ITEM, lmp->text_item,
		       NULL);
		/* XXX - V3 only ... */
		xv_set(lmp->text_item, PANEL_TEXT_SELECT_LINE, NULL);
	}

	/* XXX - don't really want this here - it's not a "completed" entry */
#ifdef notdef
	/* store list */
	if (lmp->store_proc) {
		lmp->store_proc(lmp);
	}
#endif

	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

Menu_item
listmgr_insert(Menu_item item, Menu_generate op)
{
	ListMgr *lmp;

	RGET_LMP(lmp, item, "listmgr_insert", item);
	return(do_listmgr_insert(lmp, item, op));
}

/*
 * Menu handler for `insert_menu (Before)'.
 */
Menu_item
do_listmgr_insert(ListMgr *lmp, Menu_item item, Menu_generate op)
{
	caddr_t dp;
	char *label, *s;
	int i, pos;
	Menu m;
	
	/* make sure popup isn't dismissed */
	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}

	if (op == MENU_NOTIFY) {
		if ((i = get_selected_list_index(lmp->list_item)) >= 0) {
			if (save_and_deselect(lmp, i) == -1) {
				/* XXX - error - don't create a new one */
				return;
			}
		}

		if (lmp->list->newfcn) {
			dp = lmp->list->newfcn(lmp->list);
		}

		/* if there's an insert proc, run it here */
		if (lmp->insert_proc) {
			/* XXX - check errors? */
			lmp->insert_proc(lmp, dp);
		}

		if (lmp->list->getlabel) {
			s = lmp->list->getlabel(lmp->list, dp);
			pos = insert_list_item(lmp->list_item, s,
			       get_insert_position((char*)xv_get(item,
						 MENU_STRING)));
			InsertLink(lmp->list, s, dp, pos);
		}

		set_selected_list_index(lmp->list_item, pos);

		if (lmp->sel_proc) {
			lmp->sel_proc(lmp, pos);
		}
		/* highlite the "primary" text item, if one was supplied */
		if (lmp->text_item) {
			xv_set(lmp->panel, PANEL_CARET_ITEM, lmp->text_item,
			       NULL);
			/* XXX - V3 only ... */
			xv_set(lmp->text_item, PANEL_TEXT_SELECT_LINE, NULL);
		}

		/* store list */
		if (lmp->store_proc) {
			lmp->store_proc(lmp);
		}
	}

	return item;
}

/* 
 * for doing an add (take current items, fill in with values from prop sheet,
 * and insert at the right place).
 */
Menu_item
listmgr_add(Menu_item item, Menu_generate op)
{
	ListMgr *lmp;

	RGET_LMP(lmp, item, "listmgr_add", item);
	return(do_listmgr_add(lmp, item, op));
}

/*
 * Menu handler for `add_menu (Before)'.
 */
Menu_item
do_listmgr_add(ListMgr *lmp, Menu_item item, Menu_generate op)
{
	caddr_t dp;
	char *label, *s;
	int i, pos;
	Menu m;
	
	/* make sure popup isn't dismissed */
	if (m = xv_get(item, MENU_PARENT)) {
		xv_set(m, MENU_NOTIFY_STATUS, XV_ERROR, NULL);
	}


	if (op == MENU_NOTIFY) {
#ifdef notdef
		if ((i = get_selected_list_index(lmp->list_item)) >= 0) {
			if (save_and_deselect(lmp, i) == -1) {
				/* XXX - error - don't create a new one */
				return;
			}
		}
#endif
		if ((lmp->list->getlabel == NULL) || 
		    (lmp->list->newfcn == NULL)) {
			return(item);
		}

		/* create new item */
		dp = lmp->list->newfcn(lmp->list);

		/* if there's an insert proc, run it here */
		if (lmp->insert_proc) {
			/* XXX - check errors? */
			lmp->insert_proc(lmp, dp);
		}

		/* 
		 * now call the change proc to snarf up all the settings
		 * for this entry from the panel.
		 */
		if (lmp->change_proc) {
			if (lmp->change_proc(lmp, dp) == -1) {
				/* nope, didn't like it, nuke it */
				if (lmp->list->delfcn) {
					lmp->list->delfcn(lmp->list, dp);
				}
				return(item);
			}
		}

		/* insert label in scrolling list */
		s = lmp->list->getlabel(lmp->list, dp);
		pos = insert_list_item(lmp->list_item, s,
		       get_insert_position((char*)xv_get(item,
							 MENU_STRING)));
		/* insert link in to linked list */
		InsertLink(lmp->list, s, dp, pos);

		/* select that item */
		set_selected_list_index(lmp->list_item, pos);

		/* call the select proc (don't happen automatically) */
		if (lmp->sel_proc) {
			lmp->sel_proc(lmp, pos);
		}

		/* highlite the "primary" text item, if one was supplied */
#ifdef notdef
		if (lmp->text_item) {
			xv_set(lmp->panel, PANEL_CARET_ITEM, lmp->text_item,
			       NULL);
			/* XXX - V3 only ... */
			xv_set(lmp->text_item, PANEL_TEXT_SELECT_LINE, NULL);
		}
#endif
		/* store list */
		if (lmp->store_proc) {
			lmp->store_proc(lmp);
		}
	}

	return item;
}

/*
 * Notify callback function for `delete'.
 */
void
listmgr_delete(Panel_item item, Event *event)
{
	ListMgr *lmp;

	GET_LMP(lmp, item, "listmgr_delete");
	
	listmgr_do_delete(lmp, 0);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

void
do_listmgr_delete(ListMgr *lmp, Panel_item item, Event *event)
{
	listmgr_do_delete(lmp, 0);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

/*
 * Notify callback function for `change'. the 
 */
void
listmgr_change(Panel_item item, Event *event)
{
	ListMgr *lmp;

	GET_LMP(lmp, item, "listmgr_change");

	listmgr_do_change(lmp);
	xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
}

int
listmgr_do_change(ListMgr *lmp)
{
	Link *link;
	char *s;
	int i;
	int status;

	if ((i = get_selected_list_index(lmp->list_item)) >= 0) {
		link = GetLink(lmp->list, i);
		/* will change the link data */
		if (lmp->change_proc) {
			status = lmp->change_proc(lmp, link->dp);
			if (status == -1) {
				/* XXX - do something? */
				return (-1);
			}
		}
		if (lmp->list->getlabel) {
			/* update label on panel list */
			s = lmp->list->getlabel(lmp->list, link->dp);
			replace_selected_list_item(lmp->list_item, s);
			/* update label in linked list */
			ChangeLinkLabel(lmp->list, link, s);
		}
		/* store list */
		if (lmp->store_proc) {
			lmp->store_proc(lmp);
		}
	}
	return(XV_OK);
}

/* private functions... */

int
get_insert_position(char *s)
{
	/*
	 * given the a string (the label of a menu item), determine
	 * where to insert/paste and return the int value.
	 */

	if (!(s && *s)) {
		return (I_BOTTOM);
	}

	if (strcasecmp(s, "before")==0) {
		return (I_BEFORE);
	} else if (strcasecmp(s, "after") == 0) {
		return (I_AFTER);
	} else if (strcasecmp(s, "top") == 0) {
		return (I_TOP);
	}

	return(I_BOTTOM); /* the default */
}

listmgr_do_delete(ListMgr *lmp, int sflag)
{
	int i;
	Link *link;
	
	if ((i = get_selected_list_index(lmp->list_item)) >= 0) {
		/* delete item from panel list */
		delete_selected_list_item(lmp->list_item);

		/* save link data in cut buf */
		if (sflag) {
			link = GetLink(lmp->list, i);
			if (lmp->list->delfcn && lmp->cutbuf) {
				lmp->list->delfcn(lmp->list, lmp->cutbuf);
			}
			lmp->cutbuf = CopyLinkData(lmp->list, link);
		}
		DeleteLink(lmp->list, i);
		if (i >= lmp->list->num) {
			i = lmp->list->num-1;
		}

		/* select a new list item apropriately */
		set_selected_list_index(lmp->list_item, i);
		if (lmp->sel_proc) {
			lmp->sel_proc(lmp, i);
		}

		/* store list */
		if (lmp->store_proc) {
			lmp->store_proc(lmp);
		}
	}
	return(XV_OK);
}

int
save_and_deselect(ListMgr *lmp, int i)
{
	Link *link;
	char *s;
	int status = 0;


	if (lmp->changeflag) {
		link = GetLink(lmp->list, i);
		/* will change the link data */
		if (lmp->change_proc) {
			/* check if nothing changed, it changed, or error */
			switch(status = lmp->change_proc(lmp, link->dp)) {
			case 0:	/* no change */
				break;
			case 1:	/* changed */
				link = GetLink(lmp->list, i);
				if (lmp->list->getlabel) {
					s = lmp->list->getlabel(lmp->list, 
								link->dp);
					replace_list_item(lmp->list_item, 
							  s, i);
				}

				break;
			case -1:
				/* caller notifies user */
				return(-1);
			}
		}
	}

	if (lmp->desel_proc) {
		lmp->desel_proc(lmp);
	}

	return(status);
}

int
listmgr_select_item(Xv_opaque obj, char *s)
{
	ListMgr *lmp;
	Link *lp;
	int row;

	RGET_LMP(lmp, obj, "listmgr_select_item", -1);

	if ((row = find_list_item(lmp->list_item, s)) >= 0) {
		/* select a new list item apropriately */
		set_selected_list_index(lmp->list_item, row);
		if (lmp->sel_proc) {
			lmp->sel_proc(lmp, row);
		}
	}
}

int
listmgr_append_link(Panel_item obj, caddr_t dp)
{
	ListMgr *lmp;

	RGET_LMP(lmp, obj, "listmgr_append_link", -1);

	return(do_listmgr_append_link(lmp, dp));
}

int
do_listmgr_append_link(ListMgr *lmp, caddr_t dp)
{
	char *s;
	int pos, i;

	/* oops! */
	if (dp == NULL) {
		return(-1);
	}

	if (lmp->list->getlabel) {
		s = lmp->list->getlabel(lmp->list, dp);
		pos = append_list_item(lmp->list_item, s);
		AppendLink(lmp->list, s, dp);
	}

	return(0);
}

