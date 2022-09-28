/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _LISTMGR_H
#define _LISTMGR_H

#ident	"@(#)listmgr.h	1.8	92/11/18 SMI"

#include <xview/xview.h>
#include <xview/panel.h>

#include "list.h"

typedef struct _listmgr {
	Panel_item	list_item;	/* the panel list item */
	Panel_item	text_item;	/* "primar" text item */
	Panel		panel;		/* panel that owns this stuff */
	Xv_opaque	ip;		/* guide instance ptr */
	List		*list;		/* ptr to head of list */
	caddr_t		cutbuf;		/* store last item in cut buffer */
	caddr_t		newitem;	/* values for a new item */
	int		(*sel_proc)();  /* when an item is selected */
	int		(*desel_proc)(); /* when an item is de-selected */
	int		(*insert_proc)(); /* when an item is inserted */
	int		(*change_proc)(); /* when an item is changed */
	int		(*delete_proc)(); /* when an item is deleted */
	int		(*store_proc)();  /* store list after a change */
	int		last_row; 	/* for trapping errors */
	int 		last_save;
	int		changeflag;     /* auto change on select? */
} ListMgr;

/* where to insert/paste things */
#define	I_BOTTOM	0
#define I_BEFORE	1
#define	I_AFTER		2
#define	I_TOP		3

/* get the LMGR_KEY from item, return if we can't get it  */
#define GET_LMP(lmp, obj, str) \
    if (!(lmp = listmgr_getkey(obj))) { \
		fprintf(stderr,"%s: can't get listmgr key\n", str); \
		return; \
    }

/* ditto, except return a particular value */
#define RGET_LMP(lmp, obj, str, retval) \
    if (!(lmp = listmgr_getkey(obj))) { \
		fprintf(stderr,"%s: can't get listmgr key\n", str); \
		return (retval); \
    }

/*
 * retrieve new value from a text item, compare to old string value.
 * if it's chnaged, incr changed flag and replace....
 */
#define GETNEWVAL(item, val, cp, chng) \
	if ((cp = (char*)xv_get(item, PANEL_VALUE)) && *cp) { \
		if (val && strcmp(val, cp)) { \
			free(val); \
			val = strdup(cp); \
			chng++; \
		} else if (!val) { \
			val = strdup(cp); \
			chng++; \
		} \
	} else { \
		if (val) { \
			free(val); \
			val = NULL; \
			chng++; \
		} \
	}


extern Attr_attribute LMGR_KEY;

 /* initialize the list mgr */
extern ListMgr *listmgr_init(Panel_item litem,/* list item */                
	     Panel_item titem,                /* "primary" text item */      
	     Panel panel,		      /* to attach key data to */    
	     Xv_opaque ip,		      /* guide instance ptr */       
	     List *list,		      /* linked list */              
	     int (*sel_proc) (/* ??? */),     /* when an item is selected */ 
	     int (*desel_proc) (/* ??? */),   /* "" de-selected */           
	     int (*insert_proc) (/* ??? */),  /* when an item is inserted */ 
	     int (*change_proc) (/* ??? */),  /* when an item is changed */  
	     int (*delete_proc) (/* ??? */),  /* when an item is deleted */  
	     int (*store_proc) (/* ??? */),   /* store list after change */  
	     int changeflag);		      /* auto-change when new item */
					      /* is selected? */             
extern ListMgr *listmgr_getkey(Xv_opaque item); /* get key data */

/* attach the apropriate callbacks to your buttons/menus */

extern Menu_item listmgr_cut(Menu_item item, Menu_generate op);
extern Menu_item listmgr_copy(Menu_item item, Menu_generate op);
extern Menu_item listmgr_paste(Menu_item item, Menu_generate op);
extern Menu_item listmgr_menu_delete(Menu_item item, Menu_generate op);
extern Menu_item listmgr_insert(Menu_item item, Menu_generate op);
extern Menu_item listmgr_add(Menu_item item, Menu_generate op);

extern int listmgr_notify(Panel_item item, 
			  char *string,
			  Xv_opaque client_data,
			  Panel_list_op op,
			  Event *event, int row);

extern void listmgr_create(Panel_item item, Event *event);
extern void listmgr_change(Panel_item item, Event *event);
extern void listmgr_delete(Panel_item item, Event *event);

/* same routines, but take lmp as first arg */

extern Menu_item do_listmgr_cut(ListMgr *lmp, Menu_item item,
				Menu_generate op);
extern Menu_item do_listmgr_copy(ListMgr *lmp, Menu_item item,
				 Menu_generate op);
extern Menu_item do_listmgr_paste(ListMgr *lmp, Menu_item item,
				  Menu_generate op);
extern Menu_item do_listmgr_menu_delete(ListMgr *lmp, Menu_item item,
					Menu_generate op);
extern Menu_item do_listmgr_insert(ListMgr *lmp, Menu_item item,
				   Menu_generate op);
extern Menu_item do_listmgr_add(ListMgr *lmp, Menu_item item,
				   Menu_generate op);

extern int do_listmgr_notify(ListMgr *lmp,
			     Panel_item item,
			     char *string,
			     Xv_opaque client_data,
			     Panel_list_op op,
			     Event *event,
			     int row);
extern void do_listmgr_create(ListMgr *lmp, Panel_item item, Event *event);
extern void do_listmgr_change();
extern void do_listmgr_delete(ListMgr *lmp, Panel_item item, Event *event);

extern int listmgr_select_item(Xv_opaque obj, char *s);

int listmgr_append_link(Panel_item obj, caddr_t dp);
int do_listmgr_append_link(ListMgr *lmp, caddr_t dp);

#endif /* _LISTMGR_H */
