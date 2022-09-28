#ifdef lint
#ifdef sccs
static char sccsid[] = "%Znds_listbx.c 1.5 90/02/08 Copyr 1989 Sun Micro";
#endif
#endif
/*
 *      Copyright (c) 1987, 1988, 1989 Sun Microsystems, Inc.
 *      All Rights Reserved.
 *
 *      Sun considers its source code as an unpublished, proprietary
 *      trade secret, and it is available only under strict license
 *      provisions.  This copyright notice is placed here only to protect
 *      Sun in the event the source is deemed a published work.  Dissassembly,
 *      decompilation, or other means of reducing the object code to human
 *      readable form is prohibited by the license agreement under which
 *      this code is provided to the user or company in possession of this
 *      copy.
 *
 *      RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *      Government is subject to restrictions as set forth in subparagraph
 *      (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *      clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *      NASA FAR Supplement.
 */

	/***************************************************************
	*******                                                  *******
	******  NOTE: please use the comment section at the end   ******
	******        of the program to document source changes.  ******
	*******                                                  *******
	***************************************************************/

#include <stdio.h>
/*
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <varargs.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include "xview/window_hs.h"

#include "xview/defaults.h"

#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
*/

#include "X11/Xlib.h"
#include "xview/panel.h"
#include "xview/font.h"
#include "xview/xview.h"
#include "ds_listbx.h"

extern void
list_clear_rgn(lp, low, high)
	Panel_item	lp;
	short		low;
	short		high;
{
        int     i;
  
        for (i=high-1; i >= low; i--) 
            xv_set(lp, PANEL_LIST_DELETE, i, NULL);

}

/* selecte all items in list */
extern void
list_select_all(lp)
	Panel_item	lp;
{
        int     i;
 
        for (i=(int)xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
		if (!xv_get(lp, PANEL_LIST_SELECTED, i))
            		xv_set(lp, PANEL_LIST_SELECT, i, TRUE, NULL);
}

extern void
list_deselect_all(lp)
	Panel_item	lp;
{
        int i;

        for (i=(int)xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
		if (xv_get(lp, PANEL_LIST_SELECTED, i))
            		xv_set(lp, PANEL_LIST_SELECT, i, FALSE, NULL);
}

/* given a string, set the item to 'selected' */
extern void
list_entry_select_string(lp, name)
	Panel_item lp;
	char *name;
{
	int i;
	char *fname;

	for (i = xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
	{
            fname = (char*)xv_get(lp, PANEL_LIST_STRING, i, NULL);		
            if (strcmp(fname, name) == 0)
                xv_set(lp, PANEL_LIST_SELECT, i, TRUE, NULL);
	}
}

/* given a number, set the item to 'selected' */
extern void
list_entry_select_n(lp, n)
	Panel_item lp;
	int n;
{

	if (lp != NULL && n >= 0)
           xv_set(lp, PANEL_LIST_SELECT, n, TRUE, NULL);
}

extern int
list_dup (lp, text)		/* is text entry already in list? */
	Panel_item	lp;
	char		*text;
{
	int i;	

	if (lp == NULL)
		return(FALSE);

	/*
	 * Speaking of gross, this searches for duplicate text
	 * sequentially.  Hopefully most people won't care if
	 * there are duplicate entries, and will not set LIST_DUPS
	 * to FALSE to often.
	 */
	for (i = xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
	    if (strcmp(text,(char*)xv_get(lp, PANEL_LIST_STRING, i))==0)
			return(TRUE);
	return(FALSE);
}

/* add string and/or glyph to scrolling list */
extern int
list_add_entry(lp, text, picture, client_data, position, duplicate)
	Panel_item	lp;
	char		*text;
	Pixrect		*picture;
	int   		client_data;
	int		position;
        int             duplicate;
{
	/* if list duplicates not allowed and duplicate found */
        if (!(duplicate) && list_dup(lp, text))
		return 0;

	xv_set(lp, PANEL_LIST_INSERT, position, NULL);

	if (text)
           xv_set(lp, PANEL_LIST_STRING, position,  text, 
	       NULL);
        if (picture)
           xv_set(lp, PANEL_LIST_GLYPH,  position,  picture,
               NULL);
	if (client_data)
           xv_set(lp, PANEL_LIST_CLIENT_DATA, position,  client_data,
   	       NULL);
        return TRUE;
}
list_add_entry_font(lp, text, client_data, position, font, dup)
	Panel_item      lp;
	char            *text;
	int             client_data;
	int             position;
	Xv_Font		font;
	int             dup;
{
	/* if list duplicates not allowed and duplicate found */
        if (!(dup) && list_dup(lp, text))
		return 0;

	xv_set(lp, PANEL_LIST_INSERT, position,
		PANEL_LIST_STRING, position,  text,
		PANEL_LIST_CLIENT_DATA, position,  client_data,
		PANEL_LIST_FONT, position, font,
		NULL);
}

/* delete n'th entry */
extern void
list_delete_entry_n(lp, n)
	Panel_item lp;
	int n;
{
        if (n >= 0)
            xv_set(lp, PANEL_LIST_DELETE, n, NULL);
}
	
/* Delete item given a string */
extern int
list_delete_entry(lp, string)
	Panel_item	lp;
	char*		string;
{
      int  i;
      char * fname;

      for (i=xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
      {
         fname = (char*)xv_get(lp, PANEL_LIST_STRING, i);	
         if (strcmp(fname, string) == 0)
         {
            xv_set(lp, PANEL_LIST_DELETE, i, NULL);
            return 0;
         }
      }
      return 1;
      
}
/* Delete all items in list and client data */
extern void
list_cd_flush (lp)
	Panel_item lp;
{
	int i;
	char *cd;

        for (i=xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--) {
		cd = (char*)xv_get(lp, PANEL_LIST_CLIENT_DATA, i);
		if (cd != NULL)
			free(cd);
	    	xv_set(lp, PANEL_LIST_DELETE, i, NULL);	
	}
}

/* Delete all items in list */
extern void
list_flush (lp)
	Panel_item lp;
{
	int i;

        for (i=xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
	    xv_set(lp, PANEL_LIST_DELETE, i, NULL);	

}

/* returns string associated with entry */
extern char *
list_get_entry(list, n)		/* return nth list entry in list */
	Panel_item list;  
	int n;
{
	return((char*)xv_get(list, PANEL_LIST_STRING, n));
}

/* call selection report routine for every item selected */
extern void 
list_items_selected(lp, selection_report_rtn)
	Panel_item lp;
 	PFI selection_report_rtn;
{
        int i; 
	char *entry_text;

	for (i = xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0;i--)
	    if (xv_get(lp, PANEL_LIST_SELECTED, i))
            {
		entry_text = (char*)xv_get(lp, PANEL_LIST_STRING, i);
		(*selection_report_rtn)(lp, entry_text, i);
            }
}

/* Delete all selected items */
extern void
list_delete_selected(lp)
        Panel_item      lp;	
{
        int     i;

        for (i=(int)xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--)
           if (xv_get(lp, PANEL_LIST_SELECTED, i))
	      xv_set(lp, PANEL_LIST_DELETE, i, NULL);	

}
/* returns true or false; if name is selected in list */
extern int
list_is_selected(lp, name)
	Panel_item lp;
	char *name;
{
	int i;
        char *list_name;

        i = xv_get(lp, PANEL_LIST_FIRST_SELECTED);
        while (i != -1) {
                list_name = (char*)xv_get(lp, PANEL_LIST_STRING, i);
                if (strcmp(name, list_name) == 0)
                        return 1;
                i = xv_get(lp, PANEL_LIST_NEXT_SELECTED, i);
        }
        return 0;
}

/* return 1 = the 'n'th item is Selected, return 0 = 'n'th 
item not selected */
extern int
list_item_selected(lp, n)
        Panel_item      lp;	
        int           n;
{
	return(xv_get(lp, PANEL_LIST_SELECTED, n));
}

/* return the number of items selected */
extern int
list_num_selected(lp)
	Panel_item      lp;
{
        int i, num = xv_get(lp, PANEL_LIST_NROWS);
        int num_selected = 0;

        for (i=0; i < num; i++)
           if (xv_get(lp, PANEL_LIST_SELECTED, i))
              num_selected++;

        return num_selected;
}

/* return client data associated with n'th item */
extern int
list_client_data(lp, n)
        Panel_item      lp;	
        int           n;
{
        int c_data;

        c_data = (int)xv_get(lp, PANEL_LIST_CLIENT_DATA, n);
	return(c_data);
}

/* copy from list_source to list_dest */
extern void
list_copy_list(list_source, list_dest)
	Panel_item list_source, list_dest;

{	int i, nrows;

	if ((list_source == NULL) || (list_dest == NULL))
		return;
	list_flush(list_dest);
	nrows = xv_get(list_source, PANEL_LIST_NROWS);
	for (i = 0; i < nrows; i++)
		list_add_entry(list_dest, xv_get(list_source, PANEL_LIST_STRING, i), 
			(Pixrect*)NULL, NULL, i, FALSE);
}
/* returns boolean, if in list, and row number */
extern int
list_in_list(lp, name, num)
	Panel_item lp;
	char *name;
	int *num;
{
	int i;
        char *list_name;

        *num = -1;
        for (i = xv_get(lp, PANEL_LIST_NROWS)-1; i >= 0; i--) {
                list_name = (char*)xv_get(lp, PANEL_LIST_STRING, i);
                if (strcmp(name, list_name) == 0) {
                        *num = i;
                        return 1;
                }
        }
        return 0;
}


/**************** << C O M M E N T   S E C T I O N  >> ****************

Name    Date    	Comments
----    ----    	-----------------------------------------------
knutson ???		original

mjb	6/89		Changed LIST_HEIGHT to LIST_PIX_HEIGHT &
			LIST_WIDTH to LIST_PIX_WIDTH (this will not
			confuse ian's fileview stuff).

mjb 	7/10/89		Changed switch LIST_PIX_HEIGHT & LIST_PIX_WIDTH
			options to setup list_rows & list_columns.

mjb	7/21/89		Modified list_flush() to set scrollbar to top.

mjb	7/26/89		Added LIST_DUPS attribute.  This governs whether
			duplicate text should be added to a list or not.
			Modified list_add_entry() to check this 
			attribute when inserting into a list.  Added 
			function list_dup() which returns TRUE if text
			already exists in list.

mjb	7/31/89		Added LIST_LINE_HEIGHT attribute.  This allows
			you to set the height of each line & should be 
			used when listing glyphs with each line listing.
			Default is size of current font's y value.

mjb	8/2/89		WARNING:  there seems to be a bug where you can 
			only pass 9 attributes to list_create() - others 
			will be ignored.

mjb	8/16/89		Removed extraneous code from list_repaint() which
			was causing an error on resizable list windows.
			
dipol	8/29/89		Change ds_listbx attributes to sunview type.

mjb	9/10/89		At Martin's suggestion, changed code to use memcopy
			instead of structure assignment in list_add_entry().

			Added list_batch_on() & list_batch_off() to 
			manually turn on & off the repaint operation when
			adding or deleting from a list.  Uses a new 
			attribute called LIST_BATCH, which is normally
			FALSE.

mjb	9/11/89		Modified list_batch_on() & list_batch_off() to
			use pw_batch_on & off respectively for further
			drawing speedup when using retained canvases.

mjb	9/26/89		Merged nannette's version of ds_listbx into the 
			DeskSet's version.  Removed #ifdef NEVER stmts.

mjb	10/5/89		Added code to make canvases non-retained.  This
			included changing list_create_internal() to 
			force canvases to be non-retained, and adding
			a function list_repaint_proc() for the canvas
			redraw procedure.

			I also fine-tuned list_repaint() by only 
			drawing items which can be seen in the visible
			window, and adding pw_lock & pw_unlock calls.

mjb	10/5/89		Had to back out a change in list_repaint().  The
			entire list is being painted again, even if
			only a portion is visible.  There is room for
			improvement here by setting up a way of view
			ing only the visible portion of the list, and
			having it work when scrolling too.

mjb	10/10/89	Added attribute LIST_SELECT_TEXT_ONLY to both
			ds_listbx.c & ds_listbx.h.  When TRUE, will only
			select (reverse) the text portion of the list
			entry, and not touch the glyph.  Modified
			reverse_item() to use this attribute.

mjb	10/11/89	Added a function called line_repaint() which
			will only repaint those lines you pass to
			it.  I modified both list_repaint_proc() and
			list_repaint() to use this function.

			As a byproduct of all these changes, listboxes
			can be adjustable or not, depending if the 
			canvas underneath is expandable, and attached
			to an adjustable frame.  Remember if you do
			a window_fit() around a frame, and it comes in
			contact with a canvas, the canvas becomes
			expandable.

mjb	10/20/89	Fixed text & glyph positioning in line_repaint().
			Fixed redraw bug in list_repaint().

mjb	11/27/89	Put VARARGSN in comments in front of list_get &
			list_set() to shut lint up.

isa     1/31/90         Added changes to reflect new font pkg. An
                        xview font object no longer points to a Xv_Font.
                        To access the pixfont, a xv_get(pf, FONT_PIXFONT)
                        is required.

bd	7/7/92		Add list_copy_list
bd	7/9/92		changed list_flush to list_cd_flush 
			in list_copy_list
bd	7/10/92		changed list_cd_flush to list_flush 
			in list_copy_list
bd	7/14/92		made list_dup extern
***********************************************************************/
 

