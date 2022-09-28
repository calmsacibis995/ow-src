#ifndef	_XOL_LISTPANE_H
#define	_XOL_LISTPANE_H

#pragma	ident	"@(#)ListPane.h	302.5	93/04/14 include/Xol SMI"	/* scrolllist:include/Xol/ListPane.h 1.7 	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * New Resources:
 *
 * Name			Type		Default		Meaning
 * ----			----		-------		-------
 * XtNdelete		Token		n/a		delete item from list
 * XtNeditOn		Token		n/a		edit item in list
 * XtNeditOff		Boolean		FALSE		done editing
 * XtNfont		XFontStuct	---		font
 * XtNfontColor		Pixel		Black		font color
 * XtNinsert		Token		n/a		insert item into list
 * XtNmove		Token		n/a		move to node in tree
 * XtNrecomputeWidth	Boolean		TRUE		resize or live w/ size
 * XtNselectable	Boolean		True		SINGLE / MULTI_LEVEL
 * XtNtouch		Token		n/a		list item has changed
 * XtNupdateView	Boolean		TRUE		lock/unlock view
 * XtNview		Token		n/a		view list item
 * XtNviewHeight	Cardinal	---		# items desired in view
 *
 * The following were added to accomodate the need to specify
 * the preferred maximum and preferred minimum widths for the
 * ScrollingList (if their values are 0, then these are ignored):
 * XtNprefMaxWidth      Dimension       0               Maximum width for list
 * XtNprefMinWidth      Dimension       0               Minimum width for list
 */


#include <Xol/OpenLook.h>
#include <Xol/Primitive.h>		/* include superclasses' header */

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define	OL_B_LIST_ATTR_APPL	0x0000ffff
#define	OL_B_LIST_ATTR_CURRENT	0x00020000
#define	OL_B_LIST_ATTR_SELECTED	0x00040000
#define	OL_B_LIST_ATTR_FOCUS	0x00080000

/* for 1.0 compatibility */
#define	OL_LIST_ATTR_APPL	OL_B_LIST_ATTR_APPL
#define	OL_LIST_ATTR_CURRENT	OL_B_LIST_ATTR_CURRENT
#define	OL_LIST_ATTR_SELECTED	OL_B_LIST_ATTR_SELECTED

typedef struct _OlListItem {                    /* OLIT list item */
        OlDefine                label_type;
        OlStr                   label;
        XImage*                 glyph;
        OlBitMask               attr;
        XtPointer               user_data;
        unsigned char           mnemonic;
}                       OlListItem;

typedef struct _OlListToken*	OlListToken;     /* opaque item token */
typedef struct _OlListToken*	OlSlistItemPtr;  /* opaque item Pointer */

typedef struct _OlListDelete {			/* XtNuserDelete call_data */
	OlListToken*		tokens;
	Cardinal		num_tokens;
}                       OlListDelete;

#define OlItemLabelType (1L << 0)
#define OlItemLabel     (1L << 1)
#define OlItemImage     (1L << 2)
#define OlItemSensitive (1L << 3)
#define OlItemCurrent   (1L << 4)
#define OlItemMnemonic  (1L << 5)
#define OlItemUserData  (1L << 6)
#define OlItemFont      (1L << 7) 	    /* Not implemented */
#define OlItemForeground  (1L << 8)     /* Not implemented */
#define OlItemBackground  (1L << 9)     /* Not implemented */
#define OlItemPixmap  (1L << 10)        /* Not implemented */

typedef struct {
	long	flags; /* indicates which fields in this struct are valid */
	OlDefine  label_type;
	XtPointer item_label;
	XImage    *item_image;
	OlFont    item_font;	    /* Not implemented */ 
	Pixmap    item_pixmap;	    /* Not implemented */ 
	Pixel     item_foreground;	/* Not implemented */
	Pixel     item_background;	/* Not implemented */
	Boolean   item_sensitive;
	Boolean   item_current;
	unsigned char item_mnemonic;
	XtPointer user_data;
} OlSlistItemAttrs, *OlSlistItemAttrsPtr;

typedef struct {
	int 		reason;
	XEvent		*event;
	OlDefine	mode;
	OlSlistItemPtr item;
	int 		item_pos;  
	OlSlistItemPtr *cur_items;
	int			num_cur_items;
	int 		*cur_items_pos; /* Not implemented */
} OlSlistCallbackStruct;

typedef struct {
	int reason;
	XEvent *event;
	OlSlistItemPtr *user_del_items;
	int	num_del_items;
} OlSlistUserDelCallbackStruct;

#if	defined(__STDC__) || defined(__cplusplus)

void OlSlistDeleteItem(Widget widget, OlSlistItemPtr item);
void OlSlistDeleteItems(Widget widget, OlSlistItemPtr *items, int num_items);
void OlSlistDeleteAllItems(Widget widget);
void OlSlistMakeItemNotCurrent(Widget widget, OlSlistItemPtr item, Boolean issue_callback);
void OlSlistMakeAllItemsNotCurrent(Widget widget);
void OlSlistEditItem(Widget widget, Boolean insert, OlSlistItemPtr ref_item);
void OlSlistEndEdit(Widget widget);
Boolean OlSlistIsValidItem(Widget widget, OlSlistItemPtr item);
void OlSlistMakeItemCurrent(Widget widget, OlSlistItemPtr item, Boolean issue_callback);
void OlSlistSetItemAttrs(Widget w, OlSlistItemPtr item, OlSlistItemAttrsPtr item_attr);
void OlSlistGetItemAttrs(Widget w, OlSlistItemPtr item, OlSlistItemAttrsPtr item_attr);
OlSlistItemPtr OlSlistAddItem(Widget widget, 
	OlSlistItemAttrsPtr item_attr, OlSlistItemPtr ref_item);
void OlSlistUpdateView(Widget widget, Boolean ok);
void OlSlistTouchItem(Widget widget, OlSlistItemPtr item);
void OlSlistViewItem(Widget widget, OlSlistItemPtr item);
void OlSlistFirstViewableItem(Widget widget, OlSlistItemPtr item);
void OlSlistLastViewableItem(Widget widget, OlSlistItemPtr item);
OlSlistItemPtr OlSlistGetNextItem(Widget widget, OlSlistItemPtr item);
OlSlistItemPtr OlSlistGetPrevItem(Widget widget, OlSlistItemPtr item);
OlStr OlSlistGetItemLabel(Widget widget, OlSlistItemPtr item);
XImage *OlSlistGetItemImage(Widget widget, OlSlistItemPtr item);
OlDefine OlSlistGetItemType(Widget widget, OlSlistItemPtr item);
OlDefine OlSlistGetMode(Widget widget);
Boolean OlSlistIsItemCurrent(Widget widget, OlSlistItemPtr item);
Boolean OlSlistGetItemSensitivity(Widget widget, OlSlistItemPtr item);
XtPointer OlSlistGetItemUserData(Widget widget, OlSlistItemPtr item);

typedef OlListToken	(*OlApplAddItemProc)(Widget widget,
	OlListToken parent, OlListToken reference,
	OlListItem item);
			
typedef void		(*OlApplDeleteItemProc)(Widget widget,
	OlListToken token);
		
typedef void		(*OlApplEditCloseProc)(Widget widget);
		
typedef void		(*OlApplEditOpenProc)(Widget widget, Boolean insert,
	OlListToken reference);
			
typedef void		(*OlApplTouchItemProc)(Widget widget,
	OlListToken token);
		
typedef void		(*OlApplUpdateViewProc)(Widget widget, Boolean ok);
		
typedef void		(*OlApplViewItemProc)(Widget widget,
	OlListToken token);

#else	/* __STDC__ || __cplusplus */

void OlSlistDeleteItem();
void OlSlistDeleteItems();
void OlSlistDeleteAllItems();
void OlSlistMakeItemNotCurrent();
void OlSlistMakeAllItemsNotCurrent();
void OlSlistEditItem();
void OlSlistEndEdit();
Boolean OlSlistIsValidItem();
void OlSlistMakeItemCurrent();
void OlSlistSetItemAttrs();
void OlSlistGetItemAttrs();
OlSlistItemPtr OlSlistAddItem();
void OlSlistUpdateView();
void OlSlistViewItem();
void OlSlistTouchItem();
void OlSlistFirstViewableItem();
void OlSlistLastViewableItem();
OlSlistItemPtr OlSlistGetNextItem();
OlSlistItemPtr OlSlistGetPrevItem();
OlStr OlSlistGetItemLabel();
XImage *OlSlistGetItemImage();
OlDefine OlSlistGetItemType();
OlDefine OlSlistGetMode();
Boolean OlSlistIsItemCurrent();
Boolean OlSlistGetItemSensitivity();
XtPointer OlSlistGetItemUserData();

typedef OlListToken	(*OlApplAddItemProc)();
typedef void		(*OlApplDeleteItemProc)();
typedef void		(*OlApplEditCloseProc)();
typedef void		(*OlApplEditOpenProc)();
typedef void		(*OlApplTouchItemProc)();
typedef void		(*OlApplUpdateViewProc)();
typedef void		(*OlApplViewItemProc)();

#endif	/* __STDC__ || __cplusplus */


/* C Widget type definition */
typedef struct _ListPaneClassRec*	ListPaneWidgetClass;
typedef struct _ListPaneRec*		ListPaneWidget;


/* Class record pointer */
extern WidgetClass			listPaneWidgetClass;


#if	defined(__STDC__) || defined(__cplusplus)

extern OlListItem*	OlListItemPointer(OlListToken token);

#else	/* __STDC__ || __cplusplus */

extern OlListItem*	OlListItemPointer();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_LISTPANE_H */
