#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/keysym.h>
#include <Xol/OpenLookP.h>
#include <Xol/Stub.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ControlAre.h>
#include <X11/Xatom.h>
#include <Xol/StaticText.h>
#include <stdio.h>

#define TRUE	1
#define FALSE	0
#define ORIGIN 20
#define UNMODIFIED 0
#define HIGHLIGHT  TRUE 
#define UNHIGHLIGHT  FALSE 

Widget CreateDrawableCanvas();
void exposeHandler(), Highlight(), StartHighlight(); 
void ExtendHighlight(), MakeSelection();
void PasteSelection(), reverse_video();
void lose_seln(), requestorCB(), PasteString();
int find_index();
static Boolean convert_proc();

char *string = "Select this text and paste it in some other window";
char seln_str[BUFSIZ];
char *pastestr;
static XFontStruct *txtfont;
int direction, ascent, descent;
XCharStruct overall;
int max_x, min_x, top_y, bottom_y;
int already_selected = FALSE;
int already_pasted = FALSE;
int start_char_index, end_char_index;

/*===============================================================*/

main(int argc, char **argv)
{
	Widget toplevel, scrollwin, canvas, statictxt, cntrl;
	Arg arg[10];
	Mask mask;
	int n = 0;
	Display *dpy;
	Window window;
	XtAppContext	appContext;

	/* Create toplevel shell */

	OlToolkitInitialize((XtPointer) NULL);
	toplevel = XtAppInitialize(&appContext, "CanvasTest",
				(XrmOptionDescList) NULL,
				0, &argc, argv, (String *) NULL,
				(ArgList) NULL, 0);
        n = 0;
        XtSetArg(arg[n], XtNlayoutType, OL_FIXEDROWS); n++;
        XtSetArg(arg[n], XtNmeasure, 2); n++;
        cntrl = XtCreateWidget("Control", controlAreaWidgetClass,
                                toplevel, arg, n);
        n = 0;
        XtSetArg(arg[n], XtNstring, "Select the text and try COPY/PASTE from the canvas below"); n++;
        XtSetArg(arg[n], XtNheight, 20); n++;
        statictxt = XtCreateManagedWidget("staticText", staticTextWidgetClass,
                    cntrl, arg, n);
	/* Create canvas (Stub) and set its expose Method */

	canvas = CreateDrawableCanvas("canvas", cntrl, 568, 350);
	XtSetArg(arg[0], XtNexpose, 	exposeHandler);
	XtSetValues(canvas, arg[0], 1);

	/* Set up the stub (canvas) to look for all mouse button
	 * events and also any KeyPress events (currently the stub
	 * widget won't recognize any keypress events due to a bug).
	 * This bug has been fixed in OLIT 2.5.
	 */
	mask=(ButtonPressMask | ButtonReleaseMask | Button1MotionMask |
		KeyPressMask | KeyReleaseMask);
	XtAddEventHandler(canvas, mask,
				  FALSE, 
				  Highlight, 
				  NULL);
        XtManageChild(cntrl);
	XtRealizeWidget(toplevel);

    	XtMapWidget(toplevel);

    	/*
     	* Enter Event Processing Loop....
     	*/  
	XtAppMainLoop(appContext);
}

/*==============================================================*
 * CreateDrawableCanvas: function to create a widget which can  *
 * be used for rendering xlib graphics.                         *
 * This function returns the handle to the created Stub widget. *
 *==============================================================*/

Widget
CreateDrawableCanvas(char *name,Widget parent, int width, int height)
{
	Widget canvas;
	Arg arg[4];
	int n = 0;

	XtSetArg(arg[n], XtNwidth, 	(Dimension) width);	n++;
	XtSetArg(arg[n], XtNheight, 	(Dimension) height);	n++;

	canvas = XtCreateManagedWidget(name, stubWidgetClass, 
						parent, arg, n);

	return(canvas);
}

/*===============================================================*
 * exposeHandler: routine called on Exposure events on the canvas*
 * (i.e. the stub widget). 					 *
 *===============================================================*/

void 
exposeHandler(Widget w, XEvent *xevent, Region region)
{
	Display *display;
	static GC gc;
	XGCValues gc_values;
	Window paint_win;
	static int ready_to_draw = FALSE;
	
	display = XtDisplay(w);
	paint_win = XtWindow(w);

	if (!ready_to_draw)
	   {
	     if((txtfont = XLoadQueryFont(display, "9x15")) == NULL)
		txtfont = XLoadQueryFont(display, "9x15");

	/* Find out the bounding box for the string,  
	 * in the text type specified by the font id (fid)...
	 */
	     XQueryTextExtents(display, txtfont->fid, string, strlen(string),
		&direction, &ascent, &descent, &overall);
	     min_x = ORIGIN - overall.lbearing;
	     max_x = ORIGIN + overall.rbearing;
	     top_y = ORIGIN - overall. ascent;
	     bottom_y = ORIGIN + overall.descent;

	     ready_to_draw = TRUE;
     	     gc_values.font = txtfont->fid;
	     gc_values.foreground = BlackPixel(display, DefaultScreen(display));
	     gc = XtGetGC(w, GCForeground | GCFont, gc_values); 
	   }

	XDrawString(display, paint_win,gc,ORIGIN,ORIGIN,string,strlen(string));	

	/* If part or all of the string is highlighted, then we must draw 
	 * it again in reverse video when we get an exposure event.
	 */
	if (already_selected)  
	  reverse_video(w, start_char_index, end_char_index, HIGHLIGHT); 

	/* If we have already pasted some text on the stub (canvas)
	 * widget, then we should draw it again if the window is
	 * obscured and then unobscured.
	 */
	if (already_pasted)
	  PasteString(w, pastestr);
}


/*===============================================================*
 * This proc is called when any mouse button events occur within *
 * the stub widget.  We then branch to the proper function.      *
 *===============================================================*/

void 
Highlight(Widget w, caddr_t unused, XEvent *event) 
{

	KeySym keysym;

	switch (event->type) {
    	  case ButtonPress:
		if(event->xbutton.button == Button1)   
 		  StartHighlight(w, unused, event);
		break;
	  case MotionNotify:
 		ExtendHighlight(w, unused, event);
		break;
	/* Currently there is no way to get KeyPress events
	 * in a stub widget.  This is a bug (1047085).
	 */
	 case KeyPress:
		if((keysym = XLookupKeysym(event, UNMODIFIED)) == XK_F18) 
		  PasteSelection(w, unused, event);
                else
		if((keysym = XLookupKeysym(event, UNMODIFIED)) == XK_F16)
                 MakeSelection(w, unused,event); 
		break;
	 default: break;
	}
}

/*============================================================*
 * This proc is called when the user presses left mouse down  *
 * within the bounding box for the string.  At this point, we *
 * determine the value for the starting character index for   *
 * the selected string.					      *
 *============================================================*/

void 
StartHighlight(Widget w, caddr_t unused, XButtonEvent *event)
{

	/* The user has pressed the left mouse button within the 
	 * bounding box for the string. "
	 */
	if (((event->x >= min_x) && (event->x <= max_x)) && 
	  ((event->y >= top_y) && (event->y <= bottom_y)))
	  {
	/* If the user has previously made a selection of any part
	 * of the string. Then we need to unhighlight 
	 * that selection first before highlighting a new selection
	 * of the string "Select this..".  Initially already_selected is set to
	 * FALSE.
	 */
	    if (already_selected)
	      {
	        reverse_video(w, start_char_index, end_char_index, UNHIGHLIGHT);
	        already_selected = FALSE;
	      }
	    start_char_index = find_index(event->x);
	  }	
}

/*===============================================================*
 * This is called with each MotionNotify event.  This proc takes *
 * care of extending the highlighting as the user drags the left *
 * mouse withing the bounding box of the string, "select this ....".  *
 *===============================================================*/

void 
ExtendHighlight(Widget w, caddr_t unused, XMotionEvent *event)
{
   int cur_index;

	/* The user has dragged the left mouse button within the 
	 * bounding box for the string, "Select this ...".
	 */
	if (((event->x >= min_x) && (event->x <= max_x)) && 
		((event->y >= top_y) && (event->y <= bottom_y)))
	  {

	/* Find out the current index into the string, "Hello
	 * World" by passing in the current x location.
	 */
   	     cur_index = find_index(event->x);

	/* If the user has not already made a selection, then
	 * set end_char_index to be the current value and
	 * highlight the string.
	 */
   	     if (!already_selected)
     	     {
   	       end_char_index = cur_index;
	       reverse_video(w,start_char_index, end_char_index, HIGHLIGHT);
     	     }
             else
             {

	/* First unhighlight the last selection and re-highlight
	 * what currently is selected.  Store the current char
	 * index in end_char_index, so that we have this value
	 * saved for the next time we come into this proc to
	 * unhilight the last selection and re-highlight the new
	 * one.
	 */
   	       reverse_video(w,start_char_index,end_char_index,UNHIGHLIGHT);
   	       end_char_index = cur_index;
   	       reverse_video(w,start_char_index,end_char_index,HIGHLIGHT);
             }
	     already_selected = TRUE;
	   }
	/* The user has dragged the mouse outside of the bounding
	 * box for "Select this ...", so unhighlight the text that was
	 * previously inverted (white on black).
	 */
	else 
	  {
	    reverse_video(w, start_char_index, end_char_index, UNHIGHLIGHT);
	    already_selected = FALSE;
	  }
}

/*================================================================*
 * This proc takes care of asserting ownership over the selection *
 * by calling XtOwnSelection() and also registers 3 callbacks (of *
 * the last is NULL in this case) with the intrinsics.            *
 *================================================================*/

void 
MakeSelection(Widget w, caddr_t unused, XButtonEvent *event)
{ 
	/* The user has let up on the left mouse button within the 
	 * bounding box for the string, "Select this ...".
	 */
	if (((event->x >= min_x) && (event->x <= max_x)) && 
		((event->y >= top_y) && (event->y <= bottom_y)))
	  {
	    if (XtOwnSelection(w, XA_CLIPBOARD(XtDisplay(w)), event->time, 
		convert_proc, lose_seln, NULL) == FALSE)
	      {
	        XtWarning("cantext: failed to become selection owner;\
	    	    make a new selection.\n");
	      } 
          }
}

/*==============================================================*
 * This proc takes care of highlighting and unhighlighting a    *
 * string.  It is given the stub widget handle, two indices for *
 * the string array and a boolean value to determine whether to *
 * highlight or unhighlight.                                    *
 *==============================================================*/

void 
reverse_video(Widget w, int char_index1, int char_index2, int inverted)
{
	GC gc;
	XGCValues gc_val;
	int width, height;
	Display *display;
	Drawable drawable;
	int len, start_char, end_char, beg_char_x; 
	char unhilite_str[20];

	display = XtDisplay(w);
	drawable = XtWindow(w);

       	/* Check which of the two character indices is
         * greater than the other and then assign start_
         * char and end_char accordingly.
	 */
	if (char_index1 < char_index2)
	  {
	    start_char = char_index1;
	    end_char = char_index2;
	  }
	else
	  {
	    start_char = char_index2;
	    end_char = char_index1;
	  }

	/* Figure out the length of the string the user
	 * has selected. Then determine the correct
	 * beginning x location for the first character.
	 */
	len = end_char - start_char+ 1;
        beg_char_x = start_char* txtfont->max_bounds.width + ORIGIN;

	/* Use the same font that the string was originally
	 * drawn in.
	 */
	gc_val.font = txtfont->fid;

	/* If HIGHLIGHT is passed in, then make the foreground
	 * white and the background black. Also copy this part
	 * of the string into the global variable, seln_str, which
	 * will be passed to the requestor's (app which wants to
	 * paste) callback proc (i.e. will draw the string into
	 * its window).  If UNHIGHLIGHT is passed in, then do the 
	 * reverse coloring and unhighlight seln_str.
	 */
	if (inverted)   /* HIGHLIGHT */
	  {
	    gc_val.foreground = WhitePixel(display, DefaultScreen(display));
	    gc_val.background = BlackPixel(display, DefaultScreen(display));
	    gc = XtGetGC(w, GCForeground|GCBackground|GCFont, gc_val);
            memset(seln_str,'\0', sizeof(seln_str));
            strncpy(seln_str, &string[start_char], len);
	    XDrawImageString(display, drawable, gc, beg_char_x , ORIGIN, 
		seln_str, strlen(seln_str));
	  }
	else  		/* UNHIGHLIGHT */
	  {
	    gc_val.foreground = BlackPixel(display, DefaultScreen(display));
	    gc_val.background = WhitePixel(display, DefaultScreen(display));
	    gc = XtGetGC(w, GCForeground|GCBackground|GCFont, gc_val);
	    XDrawImageString(display, drawable, gc, beg_char_x , ORIGIN, 
		seln_str, strlen(seln_str));
	  }
}

/*================================================================*
 * This proc is registered by the call to XtOwnSelection().  It's *
 * called when the intrinsics sends us a SelectionRequest event.  *
 * This means that another client/widget wants to paste the text  *
 * that is currently selected (we are the owner) into it's window *
 * and this proc takes care of converting the selected data to the*
 * appropriate format (target) that the requestor is asking for   *
 *================================================================*/

static Boolean
convert_proc(
	Widget w,
	Atom *selection,
	Atom *target,
	Atom *type_return, 
	XtPointer *value_return,
	unsigned long *length_return,
	int *format_return)
{

	int     i;
        int     *intbuffer;
	char	*buffer;

        if ((*selection != XA_PRIMARY) && 
		(*selection != XA_CLIPBOARD(XtDisplay(w)))) 
	  {
                return (False);
          }

	if (*target == XA_STRING) 
	  {

	/* We need to check both the primary and clipboard   
	 * atoms, because regular intrinsic based apps, such 
	 * as xterm will use the XA_PRIMARY atom to pass the
	 * selected data, whereas XView uses the XA_CLIPBOARD
	 * atom to pass data.
 	 *
	 * NOTE: We just ask for the XA_CLIPBOARD atom for
	 * 	 now so that we can copy&paste between this
	 * 	 app and xview tools.  It is asked for in
	 * 	 the call to XtGetSelectionValue().
	 */
             if (*selection == XA_CLIPBOARD(XtDisplay(w)))
	       {
                  i = strlen(seln_str);
                  buffer = XtMalloc(1 + i);
                  (void) strncpy (buffer, seln_str, i);
                  buffer[i] = '\0';
                }
              else 
		{
                  return (FALSE);
		}
              *value_return = buffer;
              *length_return = i;         /* DON'T include null byte */
              *format_return = 8;
              *type_return = XA_STRING;
              return (TRUE);
           }
        else if (*target == XInternAtom (XtDisplay(w), "LENGTH", False)) 
	       {
                  intbuffer = (int *) XtMalloc(sizeof (int));
                  *intbuffer = (int) (strlen (seln_str));
                  *value_return = (XtPointer) intbuffer;
                  *length_return = 1;
                  *format_return = sizeof (int) * 8;
                  *type_return = (Atom) *target;
                  return (True);
               }
 
        return (False);
}

/*===============================================================*
 * This proc is called when the intrinsics sends a SelecionClear * 
 * event.  This means we need to take care of unhighlighting our *
 * string to show that we no longer the selection holder/owner.  *
 * This proc is registered with the call to XtOwnSelection().    *
 *===============================================================*/

void
lose_seln(Widget w, Atom selection)
{	
	/* We've lost the selection (i.e. the user has selected
	 * some other text, so we need to take care of unhigh-
	 * lighting our string and change already_selected to
	 * false.
	 */
   reverse_video(w, start_char_index, end_char_index, UNHIGHLIGHT);
   already_selected = FALSE;
}

/*===============================================================*
 * Called when the user presses middle mouse in the stub.  This  *
 * signifies that some text needs to be pasted into the stub.    *
 *===============================================================*/

void
PasteSelection(Widget w, caddr_t unused, XEvent *event)
{
	/* We want to paste something into the stub widget.
	 * Here we register requestorCB which will be called
	 * to do the actual pasting of text.
	 */
    XtGetSelectionValue(w, XA_CLIPBOARD(XtDisplay(w)), XA_STRING,
	requestorCB, event, event->xbutton.time);
}

/*===============================================================*
 * This proc is called when the user wants to paste (by pressing *
 * middle mouse down in the stub widget) into our window.  It is *
 * registered when XtGetSelection() is called.                   *
 *===============================================================*/

void
requestorCB(Widget w, XEvent *event, Atom *selection, Atom *type,
	caddr_t value, unsigned long *length, int *format)
{
	if ((*type == 0) /*XT_CONVERT_FAIL || (*length == 0)*/ ) {
	  XBell(XtDisplay(w), 100);
	  fprintf(stderr, "cantext_selnclip: no selection or selection\
		  timed out; try again\n");
	}

	/* The user hasn't selected anything yet, so display an
	 * error message stating so.
	 */
	if (*length == 0){
          XBell(XtDisplay(w), 100);   
          fprintf(stderr, "cantext_selnclip: zero length atom\
		Have you selected any text?\n");
	}
	else {

	/* Here we call the routine which actually rops the 
	 * string image onto the stub widget.
	 */
		PasteString(w, value);
		pastestr = XtMalloc(*length + 1);
		strncpy(pastestr, value, *length);
		pastestr[*length] = '\0';	
		already_pasted = TRUE;
	}
	XtFree(value);
}

/*===============================================================*
 * This routine takes care of actually ropping the string image  *
 * onto the stub widget.  For simplicity's sake, we simply will  *
 * rop the string to be pasted into our window starting at loca- *
 * tion (30, 60).						 * 
 *================================================================*/

void 
PasteString(Widget w, char *string)
{
  GC gc;
  GContext gcon;
  Display *display;
  Drawable drawable;
  int screen;
  XWindowAttributes win_attr;
  int width, height;
  XFontStruct *default_font;

   display = XtDisplay(w);
   drawable = XtWindow(w);
   screen = DefaultScreen(display);

   gc = DefaultGC(display, screen);
   gcon = XGContextFromGC(gc);
   default_font = XQueryFont(display, gcon); 

	/* Clear the old text from the stub widget before pasting
	 * the new string.
	 */

   XGetWindowAttributes(display, drawable, &win_attr);
   width = win_attr.width - ORIGIN;
   height = default_font->ascent + default_font->descent;

   XClearArea(display, drawable, ORIGIN, 60-default_font->ascent, 
	width, height, FALSE);
   XDrawImageString(display, drawable, gc, ORIGIN, 60, string, strlen(string));
}
  
/*===============================================================*
 * Find the index into the string array corresponding to the     *
 * current x location of the event (i.e. left mouse drag).       *
 *===============================================================*/

int 
find_index(int xloc)
{  
   int index;

   index = (xloc - ORIGIN)/txtfont->max_bounds.width; 
   return(index);
}

