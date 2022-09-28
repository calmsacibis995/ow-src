#include <stdio.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/scrollbar.h>
#include <xview/xv_xrect.h>
#include <xview/sel_pkg.h>


/* Global definitions
 *
 */

#define ORIGIN 20
#define HIGHLIGHT TRUE
#define UNHIGHLIGHT FALSE
#define UNMODIFIED 0
#define HELPPATHNAME "."


char  *string = "Select this text and paste it in some other window";
static XFontStruct *txtfont;
int    direction,ascent, descent;
XCharStruct overall;


int   min_x, max_x, top_y, bottom_y;
int   start_char_index, end_char_index; /* start & end of highlighting */
int   already_selected;
int   already_pasted;
char   seln_string[BUFSIZ]; /* to store the selection made in the canvas */
char  paste_str[BUFSIZ]; /* used to store the string pasted on the canvas */

Selection_owner   sel_owner;
Selection_item    sel_item;



void   start_highlight();
void   extend_highlight();
int    find_index();
void   reverse_video();
void   make_selection();
void   paste_selection();
void   paste_string();

Frame  frame;
Canvas canvas;
Panel  panel;
Panel_item panel_message;
Drawable  xid;
Xv_Server server;
Xv_Window paint_window;

main(argc,argv)
int  argc;
char *argv[];
{
  
  
  already_pasted = FALSE;

  server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);
 
  create_ui();

  xv_main_loop(frame);

}

/*canvas_repaint_proc: handles the exposure event on the canvas */

void canvas_repaint_proc(Canvas  canvas, Xv_Window  paint_win,
		Display *dpy, Window xwin, Xv_xrectlist *xrects)
{
  static GC  gc;
  XGCValues  gc_val;
  static int ready_to_draw = FALSE;
  XID xid;
  
  xid = (XID) xv_get(paint_win, XV_XID);

  if(!ready_to_draw)
  {
     if(( txtfont = XLoadQueryFont(dpy,"9x15")) == NULL)
          txtfont = XLoadQueryFont(dpy, "9x15"); 

  /* Find the bounding box for the string  "Hello World" */
 
  XQueryTextExtents(dpy, txtfont->fid, string, strlen(string),
        &direction, &ascent, &descent, &overall);

  min_x = ORIGIN - overall.lbearing;
  max_x = ORIGIN + overall.rbearing;
  top_y = ORIGIN - overall.ascent;
  bottom_y = ORIGIN + overall.descent;
 
  ready_to_draw = TRUE;
  gc_val.font = txtfont->fid;
  gc_val.foreground = BlackPixel(dpy, DefaultScreen(dpy));
  gc = XCreateGC(dpy, xid, GCForeground | GCFont, gc_val );
  }
  XDrawString(dpy, xid ,gc, ORIGIN, ORIGIN, string, strlen(string));

  /* If already the wholw string or a part are highlighted */
  if( already_selected)
  reverse_video(paint_win, start_char_index, end_char_index, HIGHLIGHT);
  
  /* If some text has been pasted in the canvas - repaint the text */
  if(already_pasted)
  paste_string(paint_win, paste_str);
} 
 
/* PROC: event_handle(): The event handler for the canvas */
 
void
event_handle(Xv_Window window, Event *event)
{
  
   switch( event_action(event)) {
  
   case ACTION_SELECT:
        if(event_is_down(event)) {
        start_highlight( window, event);
        }
        else if(event_is_up(event)) {
       }
       break;
    case LOC_DRAG: 
       extend_highlight(window, event);
       break;
    case ACTION_COPY:
       make_selection(window,event);
       break;
    case ACTION_PASTE:
       paste_selection(window,event);

  default: break;
      
  }
}

/* PROC: start_highlight(): It highlights the text on the canvas */ 
void
start_highlight(Xv_Window w, Event *evt)
{
  if((( evt->ie_locx >= min_x) &&  (evt->ie_locx <= max_x)) &&
     ((evt->ie_locy >= top_y) && (evt->ie_locy <= bottom_y)))
   {
     /* If some part of the string was highlighted, unhighlight
        it first and then set the already_selected variable to
        FASLE */
     if( already_selected)
     {
        reverse_video(w,start_char_index, end_char_index, UNHIGHLIGHT);
        already_selected = FALSE;
     }
     
     start_char_index = find_index(evt->ie_locx);
   }
}

/* PROC:find_index: Find the index into the string corresponding to the 
   current mouse drag location */

int
find_index(int xloc)
{
  int  index;
  
  index = (xloc - ORIGIN)/ txtfont->max_bounds.width;
  return(index);
}

/*PROC: reverse_video(): Take care of Highlighting and unhighlighting */

void
reverse_video(w, index1, index2, flag)
Xv_Window  w;
int        index1;
int        index2;
int        flag;
{

  GC  gc;
  XGCValues gc_val;
  Display  *dpy;
  XID       xid;
  int       start_char, end_char, len, beg_char_x;

  dpy = (Display *) xv_get(w, XV_DISPLAY);
  xid = (XID) xv_get(w, XV_XID);
  
  if( index1 < index2)
  {
    start_char = index1;
    end_char = index2;
  }
  else 
  {
    start_char = index2;
    end_char = index1;
  }
 /* Find the length of the string the user has selected */
  len = end_char - start_char +1;
 /* Find the correct x locatiojn of the firest charecter */ 
  beg_char_x = start_char * txtfont->max_bounds.width + ORIGIN	;


  gc_val.font = txtfont->fid;


  if (flag) /* Highlight */
  {
    gc_val.foreground = WhitePixel(dpy, DefaultScreen(dpy));
    gc_val.background = BlackPixel(dpy, DefaultScreen(dpy));
    gc = XCreateGC(dpy, xid, GCForeground | GCBackground | GCFont, gc_val );
    memset(seln_string, '\0', sizeof(seln_string));
    strncpy(seln_string, &string[start_char], len);
    XDrawImageString(dpy, xid, gc, beg_char_x, ORIGIN, seln_string,
                      strlen(seln_string));
  }
  else /* Unhighlight */
  {
    gc_val.foreground = BlackPixel(dpy, DefaultScreen(dpy));
    gc_val.background = WhitePixel(dpy, DefaultScreen(dpy));
    gc = XCreateGC(dpy, xid, GCForeground | GCBackground | GCFont, gc_val );
    XDrawImageString(dpy, xid, gc, beg_char_x, ORIGIN, seln_string,
                     strlen(seln_string));
  }
}
    
/*PROC: extend_highlight(): Extend the selection as the mouse is dragged */
void
extend_highlight(Xv_Window w, Event *evt)
{
  int  cur_index;

  /* The user has dragged the mouse in the bounding box of the 
     string " Hello World " */
  if((( evt->ie_locx >= min_x) &&  (evt->ie_locx <= max_x)) &&
     ((evt->ie_locy >= top_y) && (evt->ie_locy <= bottom_y)))
   {
     /* Find out the current charecter index */
    
   cur_index = find_index(evt->ie_locx);
 
  /* If the user has not already made a selection, then
     set end_char_index to the current value and highlight
     the string */
  
   if (! already_selected)
    {
     end_char_index = cur_index;
     reverse_video(w,start_char_index, end_char_index, HIGHLIGHT);
    }
   else
    {
   /* Unhighlight the previous selection and re-highlight the
      current selection */

    reverse_video(w,start_char_index,end_char_index, UNHIGHLIGHT);
    end_char_index = cur_index;
    reverse_video(w,start_char_index, end_char_index, HIGHLIGHT);
    }
   already_selected = TRUE;

  }

  /* The user has dragged the mouse outside the bounding
     box, so unhighlight the text that was previously inverted
  */
  else 
  {
    reverse_video(w,start_char_index, end_char_index, UNHIGHLIGHT);
    already_selected = FALSE;
  }
}

/* Make_selection : Make a selection and associate the "COPY" data
 * with a selection item  
 */

void
make_selection(Xv_Window w, Event *evt)
{
  static int  cvtproc(); 
  Atom        CLIPBOARD;

  CLIPBOARD = xv_get(server, SERVER_ATOM,
                               "CLIPBOARD");
  
 
  /* If the user has made the selection within the bounding box   
     create the selection owner, and associate data with it */
  if((( evt->ie_locx >= min_x) &&  (evt->ie_locx <= max_x)) &&
     ((evt->ie_locy >= top_y) && (evt->ie_locy <= bottom_y)))
   { 
     /* Creae the selection owner */   
     sel_owner = xv_create(frame, SELECTION_OWNER,
                           SEL_CONVERT_PROC, cvtproc,
                           SEL_RANK_NAME, "CLIPBOARD",
                           SEL_OWN, TRUE,
                           0);

     /* associate the highlited text(seln_string) with the     
        selection item 
     */
     sel_item = xv_create(sel_owner, SELECTION_ITEM,
                          SEL_DATA, seln_string,
                          0);
   
   }
}

/*PROC:cvtproc(): The conversion proc which handles the selection request's 
 and converts to the requested data format
*/

static int
cvtproc(Selection_owner sel_owner, Atom *type, Xv_opaque *data,
	long *length, int *format)
{
  int   strLen; 
  int   *intbuffer;
  char  *buffer;
  Atom   STRING, LENGTH;

  STRING = xv_get(server, SERVER_ATOM,
                               "STRING");
  LENGTH = xv_get(server, SERVER_ATOM,
                                "LENGTH"); 
   
  if( *type == STRING)
   {
     strLen = strlen(seln_string);
     buffer = (char *) malloc( (strLen + 1) * sizeof(char));
     (void) strncpy(buffer, seln_string, strLen);
     buffer[strLen] = '\0';
     *format = 8;
     *length = strLen;
     *type = STRING;
     *data = (Xv_opaque) buffer;
     free(buffer);
     return(TRUE);
   }
  
 else  if( *type == LENGTH)
   {
    intbuffer = (int *) malloc(sizeof(int));
    *intbuffer = (int) (strlen(seln_string));
    *data = (Xv_opaque) intbuffer;
    *length =1;
    *type = LENGTH;
    *format = 32;
     free(intbuffer);
     return(TRUE);
    }
  else {
     return(FALSE); 
    }
 }

/* PROC: paste_selection: This proc issues a selection request to the current
owner of the XA_CLIPBOARD, to get the data to paste in the canvas */

void
paste_selection(Xv_Window w, Event *evt)
{
  char 	*buffer; 
  int   length, format;
  void  replyProc();

  /* The selection requestor */
  Selection_requestor  sel_req;

  memset(paste_str, '\0', BUFSIZ -1);

  /* we want to paste something into the canvas */
  /* create a selection requestor */
  
  sel_req = xv_create(frame, SELECTION_REQUESTOR,
		SEL_REPLY_PROC, replyProc,
		SEL_RANK_NAME, "CLIPBOARD",
		SEL_TYPE_NAME, "STRING", 
		NULL);

 /* Post a request - Non Blocking request */ 
  sel_post_req(sel_req);
}

/* replyProc : Handles replies from the selection owner */

void
replyProc(Selection_requestor sel_req, Atom target, Atom type,
		Xv_opaque replyBuf, unsigned long len, int format)
{

  Atom		INCR,STRING; 
  int           incr_transfer = FALSE;
  fprintf(stdout,"Target = %d type= %d len = %d\n", target, type, len);
  
  if( len == SEL_ERROR) 
  {
    int errCode;
 
  memcpy((char *) &errCode, (char *) replyBuf, sizeof(int));
	switch(errCode) {
	case SEL_BAD_PROPERTY :
             fprintf(stderr,"ReplyProc: Bad property \n");
	     break;
	case SEL_BAD_CONVERSION :
	     fprintf(stderr,"ReplyProc: Bad conversion \n");
	     break;
	case SEL_BAD_TIME :
	     fprintf(stderr,"ReplyProc: Bad time \n");
	     break;
	case SEL_BAD_WIN_ID :
	     fprintf(stderr,"ReplyProc: Bad window \n");
	     break;
	case SEL_TIMEDOUT :
             fprintf(stderr,"ReplyProc: Timed out \n");
	     break;
        } /* end of switch */
   } /* end of if */

  if (len == 0) 
  {
    fprintf(stdout,"End of incremental data transfer \n");
    /* Now paste the string*/
    /* In this example, even the incremental transfer is limited by BUFSIZ
       This can be overcome by using a file instead of charecter buffer to
       sore the COPY/CUT selection */
    paste_string(paint_window,paste_str);
    already_pasted = TRUE;
    incr_transfer = FALSE;
    return;
  }
 
  /* Get the server Atom INCR */
  INCR = xv_get(server, SERVER_ATOM,
                "INCR"); 
  if( type == INCR)
  {
  long  size;
  memcpy((char *) &size, (char *) replyBuf, sizeof(long));
  fprintf(stdout,"Getting a incremental transfer of size %d\n", size);	
  incr_transfer = TRUE;
  }
  STRING = xv_get(server, SERVER_ATOM,
                "STRING"); 
  
  if((type == STRING) && len)
  {
  char *tmp;
  
  tmp = (char *) malloc(sizeof(char) * len);
  memcpy(tmp, (char *) replyBuf, sizeof(char) * len);
  strncat(paste_str, tmp, len);
  free(tmp);
  if(incr_transfer == FALSE) {
   paste_string(paint_window,paste_str);
   already_pasted = TRUE;
  }
   
  }
}
   

 /*PROC: paste_string : Does the actual pasting of the selection in the canvas
 */ 
void paste_string(Xv_Window w, char *buff)
{
  GC  gc;
  GContext  gcon;
  Display *dpy;
  XID      xid;
  int  screen;
  XWindowAttributes  win_attr;
  int   width, height;
  XFontStruct  *default_font;
  
  dpy = (Display *) xv_get(w, XV_DISPLAY);
  xid = (XID) xv_get(w, XV_XID);
  screen = DefaultScreen(dpy);
 
  gc = DefaultGC(dpy, screen);
  gcon = XGContextFromGC(gc);

  default_font = XQueryFont(dpy, gcon);
  
  /* clear the old text from the canvas before pasting the new string */

  XGetWindowAttributes(dpy, xid, &win_attr);
  
  width = win_attr.width - ORIGIN;
  height = default_font->ascent + default_font->descent;

  XClearArea(dpy, xid, ORIGIN, 50 - default_font->ascent, width,height,FALSE);
  XDrawImageString(dpy, xid, gc, ORIGIN, 50, buff, strlen(buff));  	
}


/* create_ui: This function creates the UI*/

create_ui()
{
  void   canvas_repaint_proc();
  void   event_handle();
  
  already_pasted = FALSE;
   

  frame = (Frame) xv_create(NULL, FRAME, 
                  XV_LABEL, "Selection service Copy/Paste Demo",
                  XV_WIDTH, 568,
                  XV_HEIGHT, 340,
                  NULL);
  panel = xv_create( frame, PANEL, 
                     XV_WIDTH, WIN_EXTEND_TO_EDGE,
                     XV_HEIGHT, 20,
                     NULL);
  
  panel_message = xv_create(panel, PANEL_MESSAGE,
                  PANEL_LABEL_BOLD, TRUE,
                  PANEL_LABEL_STRING,
                  "Select the text and try COPY/PASTE from the canvas below",
                  NULL);

  canvas = xv_create(frame, CANVAS,
		CANVAS_REPAINT_PROC, canvas_repaint_proc,
		CANVAS_X_PAINT_WINDOW, TRUE,
		NULL);
      
  paint_window = canvas_paint_window(canvas);

  xv_set(paint_window,
                WIN_CONSUME_EVENTS, LOC_DRAG, MS_LEFT, ACTION_SELECT,
                                    ACTION_COPY, ACTION_PASTE, NULL,
                WIN_EVENT_PROC, event_handle,
                XV_WIDTH, WIN_EXTEND_TO_EDGE,
                XV_HEIGHT, WIN_EXTEND_TO_EDGE,
                NULL);


  xid = (Drawable) xv_get(paint_window, XV_XID);
  window_fit(frame);
}
  


     


  
   
  
  

