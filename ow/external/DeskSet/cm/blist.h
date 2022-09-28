/*static  char sccsid[] = "@(#)blist.h 1.8 92/09/21 Copyr 1991 Sun Microsystems, Inc.";
        blist.h
*/

typedef struct {
	Frame           frame;
	Panel_item	username;
	Panel_item	list;
	Panel_item	changebutton;
} Browselist;

typedef struct browser_state {
	char			*cname;
	Pixrect			*glyph;
	struct browser_state	*next;
} BrowserState;

extern Boolean browselist_showing(), browselist_exists();
