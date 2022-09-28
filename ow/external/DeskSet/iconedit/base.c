#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/svrimage.h>
#include <xview/cms.h>
#include <xview/dragdrop.h>
#include <xview/notice.h>
#include <xview/file_chsr.h>
#include "gdd.h"
#include "ds_popup.h"
#include "base_gui.h"
#include "base.h"
#include "resources.h"
#include "file_gui.h"
#include "text_gui.h"
#include "tooltalk.h"

#define MAX_UNDOS 8

#define MAX_FOOTER_LENGTH 64

#define MONO 1
#define COLOR 0

#define GRID_OFF 0
#define GRID_ON 1

#define MSGFILE "SUNW_DESKSET_ICONEDIT"

File_chooser load_dialog = NULL;
File_chooser save_dialog = NULL;

text_popup_objects *text_popup;
int	canvas_dimension;


static int	text_popup_located = FALSE;

/* 
 * Remember, global data is your friend.  You always know where it is...
 */

GC gc, gc_rv, redraw_gc, fill_gc, big_gc, preview_gc, select_gc, preview_select_gc, grid_gc, moving_gc, preview_moving_gc;
Colormap cmap;
XColor green, blue, red, black, yellow, orange, violet, brown, white, current_pen;
Display *dpy;
Window preview_xid, big_bit_xid;
base_window_objects	*base_window; 
Xv_Cursor big_bit_cursor, eraser_cursor;
Xv_singlecolor xvblack, xvwhite;

Pixmap preview_pixmap, big_pixmap, moving_pixmap, moving_preview_pixmap;
XImage *edit_image, *rotate_image;
int rotate_x, rotate_y;

Undo_struct undo_images[MAX_UNDOS];

int current_color_state = COLOR;
int screen_depth;
int tt_running = FALSE;
Menu color_menu;
int current_undo, undos_allowed, redos_allowed, fill_pattern;
int Edited = 0;
int positioned = 0;
char	buffer[MAXPATHLEN];

/* globals for dealing withcut/copy/paste */

Selection_owner		Shelf_owner;
Selection_requestor	Shelf_requestor;
Selection_item		Shelf_item;

int grid_status = GRID_OFF;
int redo_possible = FALSE;
char cmdline_args[MAXPATHLEN] = "";
void init_globals();
void init_dragdrop();

unsigned char pattern_root_bits[] =  {0x01, 0x01, 0x04, 0x04};
unsigned char pattern_17_bits[] = {0x02, 0x08, 0x01, 0x04, 0x10};
unsigned char pattern_20_bits[] = {0x04, 0x01, 0x08, 0x02, 0x10};
unsigned char pattern_25_bits[] = {0x01, 0x04, 0x02, 0x08};
unsigned char pattern_50_bits[] = {0x01, 0x02};
unsigned char pattern_75_bits[] = {0x0e, 0x0b, 0x0d, 0x07};
unsigned char pattern_80_bits[] = {0xfd, 0xf7, 0xfe, 0xfb, 0xef};
unsigned char pattern_83_bits[] = {0xf7, 0xfe, 0xfb, 0xef, 0xfd};
unsigned char pattern_black_bits[] = {0x01};

Pixmap pattern_root, pattern_17, pattern_20, pattern_25, pattern_50, pattern_75, pattern_80, pattern_83, pattern_black;

extern int	selection_top;
extern int	selection_left;
extern int	selection_bottom;
extern int	selection_right;
extern int      wiping_on;
extern char	*file;
GC three_gc;
int icon_height, icon_width;
int preview_boarder_upper, preview_boarder_left;
int boarder_upper, boarder_lower, boarder_left, boarder_right;
int height_block, width_block;
int number_of_vert_lines, number_of_horz_lines;
int	Edit_mode = 0;

Xv_Server	server;
Panel_item      black_white_toggle;

unsigned short eraser_bits[] = {
  0x0000, 0x0000, 0x0000, 0x00FF, 0x0103, 0x0205, 0x040B, 0x0815, 
  0x102A, 0x2054, 0x40A8, 0xFF50, 0x81A0, 0x8140, 0x8180, 0xFF00
  };

XSegment x_segment_list[129], y_segment_list[129];

Server_image eraser_image, default_image;

extern void do_that_base_repaint_thing();
extern void reset_those_pesky_clip_masks();
extern void set_select_cliprects();
extern void selection_off();
extern  int do_that_save_thing();
extern void change_stuff_to_mono();
extern void change_stuff_to_color();
extern void grid_proc();
extern void set_the_size_info();
extern Menu_item select_black();
extern void change_icon_size();
extern void paste_reply_proc();
extern void invert_area();
extern void call_up_that_color_chooser();
extern void select_color_from_rgb();
extern void initialize_grid_lines();
extern void set_iconedit_label();
extern void b_w_notify_handler();
extern void b_w_event_handler();
extern char *gettext();


extern XVars X;
extern Vars v;
extern char *iconedit_res[];
extern int save_format;
char *excellent = "This program is dedicated to my good friends, Bill and Ted";

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute	INSTANCE;

void
main(argc, argv)
   int		argc;
   char		**argv;
{
  /*	base_window_objects	*base_window; */
  
  char bind_home[MAXPATHLEN];
  char working_dir[MAXPATHLEN];
  char	*init_file = NULL;
  extern int x_error_proc();
  int i;
  
  char *ptr;
  
  getcwd(working_dir, MAXPATHLEN);
  
  /*
   * Need to init Tooltalk before init_globals to
   * check if we need to allocate those cursor colors.
   */
  iconedit_tt_init( argc, argv );
  /*
   * Initialize XView.
   */
  server = xv_init(XV_USE_LOCALE, TRUE,
		   XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
		   XV_X_ERROR_PROC, x_error_proc,
		   0);
  
  ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
  bindtextdomain(MSGFILE, bind_home);
  textdomain(MSGFILE);
  
  for(i=0; i<argc; i++)
    if ( !strcmp(argv[i], "-v") )
      {
	fprintf(stdout, "iconedit %s\n", ds_relname());
	exit(0);
      }
  if (argc == 2)
    init_file = argv[1];
  else if (argc > 2)
    {
      printf(gettext("Iconedit can only take 1 file name as a parameter\n"));
      exit(1);
    }
  
  
  INSTANCE = xv_unique_key();
  
  /*
   * Initialize user interface components.
   */
  base_window = 
    base_window_objects_initialize(NULL, NULL);
  
  if (!xv_get(base_window->fill_setting, PANEL_INACTIVE)) 
    xv_set(base_window->fill_setting, PANEL_INACTIVE, TRUE, 0);

  text_popup = 
    text_popup_objects_initialize(NULL, base_window->window);
  
  iconedit_tt_init_complete();
  
  /* initialize some resource stuff... */
  iconedit_res[(int) R_HEIGHT]   = DGET("iconHeight");
  iconedit_res[(int) R_WIDTH]    = DGET("iconWidth");
  iconedit_res[(int) R_FILEFORM] = DGET("iconFormat");
  
  v = (Vars) (malloc(sizeof(IconVars))) ;
  
  v->appname = NULL;
  v->height = 64;
  v->width = 64;
  v->format = 1;    /* Default save format 1 icon, 2 xbm, 3 xpm...*/
  
  X = (XVars) (malloc(sizeof(XObject))) ;
  
  X->dpy  = (Display *)xv_get(base_window->window, XV_DISPLAY);
  X->home           = getenv("HOME") ;
  X->desksetDB      = NULL ;
  X->rDB            = NULL ;
  
  if ((ptr = strrchr(argv[0], '/')) != NULL)
    read_str(&v->appname, ptr+1) ;
  else read_str(&v->appname, argv[0]) ;
  
  
  load_resources() ;          /* Get resources from various places. */
  read_resources() ;          /* Read resources from merged database. */
  
  (void)init_globals();
  (void)init_dragdrop();
  
  if (screen_depth != 1)
    {
      ds_colors_init( dpy, base_window->window );
      
      color_menu = (Menu) xv_get(base_window->palette_bt, PANEL_ITEM_MENU); 
      
      if ( tt_running )
	{
	  xv_set( base_window->palette_bt, 
		 PANEL_ITEM_MENU, XV_NULL,
		 NULL);
	  xv_set( base_window->palette_bt, 
		 PANEL_NOTIFY_PROC, call_up_that_color_chooser,
		 NULL);
	}
      else
	{
	  xv_set( base_window->palette_bt,
		 PANEL_ITEM_MENU, 
		 base_palette_menu_create((caddr_t *)base_window, 
					  base_window->main_control ), 
		 NULL );
	  
	  /* reposition color choosers if tooltalk initialization 
	     fails, as the palette button becomes wider when it 
	     acquires a menu. */
	  
	  position_color_choosers(base_window);
	}
    }
  
  
  if (init_file)
    {
      char	*leaf_pointer = buffer;
      char	*tail_pointer = buffer;
      
      ds_expand_pathname(init_file, buffer);
      strcat(cmdline_args, init_file);

      load_dialog = 
        file_dialog_initialize(base_window->window, FILE_CHOOSER_OPEN);
      save_dialog = 
        file_dialog_initialize(base_window->window, FILE_CHOOSER_SAVEAS);
      
      if (*argv[1] == '/')
		{
		  while (*tail_pointer)
		    tail_pointer++;
		  
		  while (*tail_pointer != '/')
		    tail_pointer--;
		  
		  *tail_pointer = NULL;
		  tail_pointer++;
	          xv_set(load_dialog, 
	                 FILE_CHOOSER_DIRECTORY, buffer, 0);
	          xv_set(save_dialog, 
	                 FILE_CHOOSER_DIRECTORY, buffer, 0);
		  xv_set(save_dialog, 
		         FILE_CHOOSER_DOC_NAME, tail_pointer, 0);

		  *(tail_pointer - 1) = '/';
                  file = buffer;
                } 
      else
      {
	/* do something else */
	xv_set(load_dialog, 
	       FILE_CHOOSER_DIRECTORY, working_dir, 0);
        xv_set(save_dialog, 
	       FILE_CHOOSER_DIRECTORY, working_dir, 0);
        xv_set(save_dialog, 
               FILE_CHOOSER_DOC_NAME, buffer, 0);
        file = tail_pointer;
      }
      
      file_load();
    }
  set_iconedit_label();
  /*
   * Turn control over to XView.
   */
  xv_main_loop(base_window->window);
  iconedit_tt_quit();
  exit(0);
}

/*
 * Initialization for the pixmaps that hold the picture for repaint and 
 * dumping into a file, among other things...
 */

void
init_globals()
{

  int i;
  XGCValues       	 gc_vals;
  int             	 gc_flags;
  Xv_Font                my_font;
  Cms                    cms;
  XRectangle             big_rect[1], preview_rect[1];
  XRectangle             select_rect[1], moving_rect[1];
  Menu                   edit_menu, save_menu, view_menu, size_menu;
  char                   dash_pattern[2];
  int                    num_o_dashes;
  Visual                *visual;
  XGCValues              gcvals;
  extern int             lose_clipboard();
  
  xv_set(base_window->preview_canvas, 
	 CANVAS_RETAINED, FALSE,
	 NULL);

  xv_set(base_window->big_bit_canvas, 
	 CANVAS_RETAINED, FALSE,
	 NULL);

  preview_xid = (Window)xv_get(canvas_paint_window(base_window->preview_canvas)
			       , XV_XID);
  big_bit_xid = (Window)xv_get(canvas_paint_window(base_window->big_bit_canvas)
			       , XV_XID);
  
  dpy  = (Display *)xv_get(base_window->window, XV_DISPLAY);
  
  screen_depth = ( int )xv_get( base_window->window, WIN_DEPTH );
  
  Shelf_owner = xv_create(base_window->window, SELECTION_OWNER, 
			  SEL_RANK, xv_get(server, SERVER_ATOM, "CLIPBOARD"), 
			  SEL_LOSE_PROC, lose_clipboard,
			  0);
  Shelf_item = xv_create(Shelf_owner, SELECTION_ITEM, 0);
  Shelf_requestor = xv_create(base_window->window, SELECTION_REQUESTOR,
			      SEL_RANK, 
			      xv_get(server, SERVER_ATOM, "CLIPBOARD"),
			      SEL_REPLY_PROC,	
			      paste_reply_proc,
			      0);


  icon_width = v->width;
  icon_height = v->width;
  undo_images[current_undo%MAX_UNDOS].height = icon_height;
  undo_images[current_undo%MAX_UNDOS].width = icon_width;
  
  set_the_size_info();

  selection_top = 0;
  selection_left = 0;
  selection_bottom = 0;
  selection_right = 0;

  preview_pixmap = XCreatePixmap(dpy, preview_xid, 
				 138, 138, 
				 screen_depth);
  big_pixmap = XCreatePixmap(dpy, big_bit_xid, 
			     canvas_dimension, canvas_dimension, 
			     screen_depth);
  moving_pixmap = XCreatePixmap(dpy, big_bit_xid, 
				canvas_dimension, canvas_dimension, 
				screen_depth);
  moving_preview_pixmap = XCreatePixmap(dpy, preview_xid, 
					138, 138, 
					screen_depth);

  visual = ( Visual * )xv_get( base_window->window, XV_VISUAL );

  edit_image = XCreateImage(dpy, visual, 
			    screen_depth, ZPixmap, 
			    0, 0, 
			    128, 128, 
			    8, 0);
  rotate_image = XCreateImage(dpy, visual, 
			      screen_depth, ZPixmap, 
			      0, 0, 
			      128, 128, 
			      8, 0);
  
  if ( edit_image == 0)
    {
      fprintf(stderr, gettext("base: XCreateImage allocation failed\n"));
    }
  edit_image->data = (char *)calloc(128, edit_image->bytes_per_line);
  if (edit_image->data == 0)
    {
      fprintf(stderr, 
	      gettext("base: memory allocation for image data failed\n"));
    }

  if ( rotate_image == 0)
    {
      fprintf(stderr, 
	      gettext("base: XCreateImage allocation failed\n"));
    }
  rotate_image->data = (char *)calloc(128, rotate_image->bytes_per_line);
  if (rotate_image->data == 0)
    {
      fprintf(stderr, 
	      gettext("base: memory allocation for image data failed\n"));
    }
  
  for (i=0; i < MAX_UNDOS; i++)
    {
      undo_images[i].image = XCreateImage(dpy, visual, 
					  screen_depth, ZPixmap, 
					  0, 0, 
					  128, 128, 
					  8, 0);
      if ( undo_images[i].image == 0)
	{
	  fprintf(stderr, 
		  gettext("base: XCreateImage allocation failed for undo image\n"));
	}
      undo_images[i].image->data = 
	(char *)calloc(128, undo_images[i].image->bytes_per_line);
      if (undo_images[i].image->data == 0)
	{
	  fprintf(stderr, 
		  gettext("base: memory allocation for image data failed for undo image\n"));
	}
    }

  current_undo = 0;
  undos_allowed = 0;
  redos_allowed = 0;
  Edited = 0;
  undo_images[current_undo%MAX_UNDOS].edited = 0;
  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), MENU_INACTIVE, TRUE, NULL);
  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), MENU_INACTIVE, TRUE, NULL);
  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 8), MENU_INACTIVE, TRUE, NULL);

  set_iconedit_footer();

  save_format = v->format;

  if (save_format != 3)
    undo_images[current_undo%MAX_UNDOS].state = MONO;
  else
    undo_images[current_undo%MAX_UNDOS].state = COLOR;


  cmap = xv_get( xv_get( base_window->window, WIN_CMS ), CMS_CMAP_ID );

  if ( !tt_running && ( screen_depth > 1 ) ) {

  green.red = 11520; green.green = 58624; green.blue = 23296;
  blue.red = 11520; blue.green = 23296; blue.blue = 58624;
  red.red = 58624; red.green = 11520; red.blue = 11520;
  white.red = 65280; white.green = 65280; white.blue = 65280;
  black.red = 0; black.green = 0; black.blue = 0;
  orange.red = 58624; orange.green = 45312; orange.blue = 11520;
  yellow.red = 56320; yellow.green = 58624; yellow.blue = 11520;
  violet.red = 58624; violet.green = 35072; violet.blue = 52736;
  brown.red = 48896; brown.green = 9728; brown.blue = 9728;

  if (XAllocColor( dpy, cmap, &green) == 0)
    {
      printf(gettext("green allocation didn't work\n"));
    }
  if (XAllocColor( dpy, cmap, &blue) == 0)
    {
      printf(gettext("blue allocation didn't work\n"));
    }
  if (XAllocColor( dpy, cmap, &red) == 0)
    {
      printf(gettext("red allocation didn't work\n"));
    }
  if (screen_depth != 1)
    {
      if (XAllocColor( dpy, cmap, &white) == 0)
	{
	  printf(gettext("white allocation didn't work\n"));
	}
      if (XAllocColor( dpy, cmap, &black) == 0)
	{
	  printf(gettext("black allocation didn't work\n"));
	}
    }
  if (XAllocColor( dpy, cmap, &orange) == 0)
    {
      printf(gettext("orange allocation didn't work\n"));
    }
  if (XAllocColor( dpy, cmap, &yellow) == 0)
    {
      printf(gettext("yellow allocation didn't work\n"));
    }
  if (XAllocColor( dpy, cmap, &violet) == 0)
    {
      printf(gettext("violet allocation didn't work\n"));
    }
  if (XAllocColor( dpy, cmap, &brown) == 0)
    {
      printf(gettext("brown allocation didn't work\n"));
    }
  
}
  else {
    Cms             xview_cms;
    unsigned long   *temp_colors;
    
    xview_cms = (Cms)xv_create(NULL, CMS,
			       CMS_SIZE, 2,
			       CMS_NAMED_COLORS, "white", "black", NULL,
			       NULL);
    
    temp_colors = (unsigned long *)xv_get(xview_cms, CMS_INDEX_TABLE);
    
    white.pixel = temp_colors[0];
    black.pixel = temp_colors[1];
    
  } /* if ( tt_running ) */
  
  big_bit_cursor = (Xv_Cursor)xv_get(base_window->big_bit_canvas, WIN_CURSOR);
  xvblack.red = 0; xvblack.green = 0; xvblack.blue = 0;
  xvwhite.red = 255; xvwhite.green = 255; xvwhite.blue = 255;
  
  eraser_image = (Server_image)xv_create(NULL, SERVER_IMAGE,
					 SERVER_IMAGE_BITS, 
					 (short *)eraser_bits,
					 SERVER_IMAGE_DEPTH, 1,
					 XV_WIDTH, 16,
					 XV_HEIGHT, 16,
					 NULL);
  
  eraser_cursor = (Xv_Cursor)xv_create(NULL, CURSOR, 
				       CURSOR_IMAGE, eraser_image,
				       CURSOR_FOREGROUND_COLOR, &xvblack,
				       CURSOR_BACKGROUND_COLOR, &xvwhite,
				       CURSOR_XHOT, 0,
				       CURSOR_YHOT, 15,
				       CURSOR_OP, PIX_SRC^PIX_DST,
				       NULL);

  gc = XCreateGC( dpy, xv_get( base_window->window, XV_XID ), 0, &gcvals );
  XSetForeground(dpy, gc, black.pixel);
  XSetBackground(dpy, gc, white.pixel);
  current_pen.pixel = black.pixel;
  
  gc_rv = XCreateGC(dpy, big_bit_xid, 0, 0);
  XSetBackground(dpy, gc_rv, black.pixel);
  XSetForeground(dpy, gc_rv, white.pixel);

  select_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  XSetBackground(dpy, select_gc, black.pixel);
  XSetForeground(dpy, select_gc, white.pixel);
  
  redraw_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  
  XFillRectangle( dpy, preview_pixmap, gc_rv, 
		 0 , 0, 138, 138);
  XFillRectangle( dpy, big_pixmap, gc_rv, 
		 0, 0, canvas_dimension, canvas_dimension); 
  XFillRectangle( dpy, moving_pixmap, gc_rv, 
		 0, 0, canvas_dimension, canvas_dimension); 
  XFillRectangle( dpy, moving_preview_pixmap, gc_rv, 
		 0, 0, 138, 138);

  XDrawRectangle(dpy, big_bit_xid, redraw_gc, 
		 boarder_left -1, boarder_upper -1, 
		 canvas_dimension - ( boarder_left + boarder_right) +1, 
		 canvas_dimension - ( boarder_upper + boarder_lower) +1);
  XDrawRectangle(dpy, big_pixmap, redraw_gc, 
		 boarder_left -1, 
		 boarder_upper -1, 
		 canvas_dimension - ( boarder_left + boarder_right) +1, 
		 canvas_dimension - ( boarder_upper + boarder_lower) +1);

  big_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  preview_gc = XCreateGC(dpy, big_bit_xid, 0, 0);

  grid_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  XSetForeground(dpy, grid_gc, black.pixel);
  XSetBackground(dpy, grid_gc, white.pixel);

  dash_pattern[0] = 2;   dash_pattern[1] = 2; num_o_dashes = 2;
  XSetDashes(dpy, grid_gc, 0, dash_pattern, num_o_dashes);

  preview_select_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  XSetBackground(dpy, preview_select_gc, black.pixel);
  XSetForeground(dpy, preview_select_gc, white.pixel);

  moving_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  preview_moving_gc = XCreateGC(dpy, big_bit_xid, 0, 0);

  XSetForeground(dpy, preview_gc, black.pixel);
  XSetBackground(dpy, preview_gc, white.pixel);

  fill_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  XSetForeground(dpy, fill_gc, black.pixel);
  XSetBackground(dpy, fill_gc, white.pixel);

  my_font = (Xv_Font)xv_create(0,FONT,
			       
#ifdef OW_I18N
			       FONT_FAMILY, FONT_FAMILY_SANS_SERIF,
#else
			       FONT_FAMILY, FONT_FAMILY_LUCIDA_FIXEDWIDTH,
#endif
			       FONT_STYLE, FONT_STYLE_NORMAL,
			       FONT_SCALE, FONT_SCALE_DEFAULT,
			       0);
  
  cms = (Cms)xv_get((Xv_Window)xv_get(base_window->big_bit_canvas, 
				      CANVAS_NTH_PAINT_WINDOW, 0), 
		    WIN_CMS);

  gc_vals.font = ((XFontStruct *)xv_get(my_font, FONT_INFO))->fid;
  gc_vals.function = GXxor;
  gc_vals.foreground = xv_get(cms, CMS_FOREGROUND_PIXEL) 
    ^ xv_get(cms, CMS_BACKGROUND_PIXEL);
  gc_vals.background = xv_get(cms, CMS_BACKGROUND_PIXEL);
  gc_vals.fill_style = FillSolid; 
  
  gc_flags = GCFont | GCFunction | GCForeground | GCBackground |
    GCFillStyle;
  
  three_gc = XCreateGC(dpy, big_bit_xid, gc_flags, &gc_vals); 

  reset_those_pesky_clip_masks();

  undo_images[current_undo%MAX_UNDOS].image = 
    XGetImage(dpy, preview_pixmap, 
	      preview_boarder_left, 
	      preview_boarder_upper, 
	      icon_width, 
	      icon_height, 
	      -1, ZPixmap);

  pattern_root = XCreateBitmapFromData( dpy, big_bit_xid, 
				       (char *)pattern_root_bits, 4, 4);
  pattern_17 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_17_bits, 5, 5);
  pattern_20 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_20_bits, 5, 5);
  pattern_25 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_25_bits, 4, 4);
  pattern_50 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_50_bits, 2, 2);
  pattern_75 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_75_bits, 4, 4);
  pattern_80 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_80_bits, 5, 5);
  pattern_83 = XCreateBitmapFromData( dpy, big_bit_xid, 
				     (char *)pattern_83_bits, 5, 5);
  pattern_black = XCreateBitmapFromData( dpy, big_bit_xid, 
					(char *)pattern_black_bits, 1, 1);

  
  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), MENU_INACTIVE, TRUE, NULL);
  
  view_menu = xv_get(base_window->view_bt, PANEL_ITEM_MENU);

  black_white_toggle = xv_create(base_window->main_control, PANEL_CHOICE,
				 XV_SHOW, FALSE,
				 XV_Y, xv_row(base_window->main_control, 0),
				 PANEL_LAYOUT, PANEL_VERTICAL,
				 PANEL_CHOICE_NROWS, 1,
				 PANEL_NOTIFY_PROC, b_w_notify_handler,
				 PANEL_EVENT_PROC, b_w_event_handler,
				 XV_HELP_DATA, "iconedit:BlackWhiteToggle",
				 PANEL_CHOICE_STRINGS,
				 gettext("Black") ,
				 gettext("White") ,
				 0,
				 PANEL_VALUE, 0,
				 NULL);
  
  position_color_choosers(base_window);
  
  
  if (screen_depth ==1 )
    {
      current_color_state = MONO;
      xv_set(base_window->color_mono_setting, PANEL_INACTIVE, TRUE, NULL);
      change_stuff_to_mono();
    }
  initialize_grid_lines();
}


void
set_the_size_info()
{

  if (icon_height > icon_width)
    {
      if ( icon_height > 64 )  
	height_block = (canvas_dimension -1)/icon_height;
      else 
	height_block = (canvas_dimension -65)/icon_height;
      width_block = height_block;
    }
  else
    {
      if (icon_width > 64 )
	width_block = (canvas_dimension -1)/icon_width;
      else 
	width_block = (canvas_dimension -65)/icon_width;
      height_block = width_block;
    }
  /*
    fprintf(stderr, "height_block is %d\n", height_block);
    */
  
  boarder_upper = ((canvas_dimension-1)-(icon_height*height_block))/2;
  boarder_lower = (((canvas_dimension-1)-(icon_height*height_block)) +1)/2;


  /*
    fprintf(stderr, "width_block is %d\n", width_block);
    */
  
  boarder_left = ((canvas_dimension -1) - (icon_width * width_block)) / 2;
  boarder_right = (((canvas_dimension -1) - (icon_width * width_block)) +1)/ 2;

  /*
    fprintf(stderr, "boarder_left is %d\n", boarder_left);
    fprintf(stderr, "boarder_upper is %d\n", boarder_upper);
    fprintf(stderr, "boarder_right is %d\n", boarder_right);
    fprintf(stderr, "boarder_lower is %d\n", boarder_lower);
    */

  preview_boarder_left = (138 - icon_width) / 2;
  preview_boarder_upper = (138 - icon_height) / 2;

  /*
    fprintf(stderr, "preview_boarder_left is %d\n", preview_boarder_left);
    fprintf(stderr, "preview_boarder_upper is %d\n", preview_boarder_upper);
    */  
}


Menu_item
show_file_popup_save(item, op)
   Menu_item	item;
   Menu_generate	op;
{
  extern Menu_item show_file_popup();

  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    if (!save_dialog)
      save_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_SAVEAS);
    if (!load_dialog)
      load_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_OPEN);
    if (!positioned)
      {
	  ds_position_popup(base_window->window, save_dialog, 
			    DS_POPUP_LOR);
	  positioned = TRUE;
      }
    xv_set(save_dialog, XV_SHOW, TRUE, 0);
    show_file_popup(item, op);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

Menu_item
show_file_popup_load(item, op)
	Menu_item	item;
	Menu_generate	op;
{
  extern Menu_item show_file_popup();
  static int positioned = 0;
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    if (!load_dialog)
      load_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_OPEN);
    if (!save_dialog)
      save_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_SAVEAS);

    if (!positioned)
      {
	  ds_position_popup(base_window->window, load_dialog, 
			    DS_POPUP_LOR);
	  positioned = TRUE;
      }
    xv_set(load_dialog, XV_SHOW, TRUE, 0);
/* set load to the default  */
    show_file_popup(item, op);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_file_menu (Load/Save/Browse...)'.
 */
Menu_item
show_file_popup(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	
	switch (op) {
	case MENU_DISPLAY:
	  break;
	  
	case MENU_DISPLAY_DONE:
	  break;
	  
	case MENU_NOTIFY:
	  file_initialize();
	  break;
	  
	case MENU_NOTIFY_DONE:
	  break;
	}
  return item;
}

undo_last_edit()
   
{
  long pixel, temp_pixel, pixel_old;
  int x, y, corner_x, corner_y, items, i;
  
  Menu color_menu;	
  Menu edit_menu;

  xv_set(base_window->window, FRAME_BUSY, TRUE, NULL);
  
  if (current_undo > 0) 
    if (undos_allowed > 0)
      {
	undo_images[current_undo%MAX_UNDOS].height = icon_height;
	undo_images[current_undo%MAX_UNDOS].width = icon_width;
	current_undo = current_undo -1;
	
	undos_allowed = undos_allowed -1;
	
	if (undos_allowed == 0)
	  {
	    edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	    xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		   MENU_INACTIVE, TRUE, NULL);
	  }
	
	if (redo_possible == FALSE)
	  {
	    redo_possible = TRUE;
	    
	    edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	    xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		   MENU_INACTIVE, FALSE, NULL);
	  }
	Edited = undo_images[current_undo%MAX_UNDOS].edited;
	redos_allowed++;
	
	temp_pixel = current_pen.pixel;

	if (((undo_images[current_undo%MAX_UNDOS].width != icon_width) || 
	     (undo_images[current_undo%MAX_UNDOS].height != icon_height))
	    && ( current_undo != 0))
	  {
	    icon_width = undo_images[current_undo%MAX_UNDOS].width;
	    icon_height = undo_images[current_undo%MAX_UNDOS].height;
	    
	    XFillRectangle( dpy, preview_pixmap, gc_rv, 
			   0 , 0, 138, 138);
	    XFillRectangle( dpy, big_pixmap, gc_rv, 
			   0, 0, canvas_dimension, canvas_dimension); 
	    XFillRectangle( dpy, preview_xid, gc_rv, 
			   0 , 0, 138, 138);
	    XFillRectangle( dpy, big_bit_xid, gc_rv, 
			   0, 0, canvas_dimension, canvas_dimension); 
	    
	    set_the_size_info();
	    do_that_base_repaint_thing(undo_images[current_undo%MAX_UNDOS].image);
	    reset_those_pesky_clip_masks();
	    initialize_grid_lines();
	    grid_proc(grid_status);
	    

	  }
	else
	  {		    
	    for (x = 0; x < icon_width; x++)
	      {
		corner_x = (x * width_block) + boarder_left +1;
		for (y = 0; y < icon_height; y++)
		  {
		    corner_y = (y * height_block) + boarder_upper +1;
		    pixel = XGetPixel(undo_images[current_undo%MAX_UNDOS].image, x, y);
		    pixel_old = XGetPixel(undo_images[(current_undo+1)%MAX_UNDOS].image, x, y);
		    if (!(pixel == pixel_old))
		      {
			current_pen.pixel = pixel;
			XSetForeground(dpy, redraw_gc, current_pen.pixel);
			XFillRectangle(dpy, big_bit_xid, redraw_gc, 
				       corner_x, corner_y, 
				       width_block-1, 
				       height_block-1);	    
			XFillRectangle(dpy, big_pixmap, redraw_gc, 
				       corner_x, corner_y, 
				       width_block-1, 
				       height_block-1);	    
			XDrawPoint(dpy, preview_pixmap, redraw_gc, 
				   x+preview_boarder_left, 
				   y+preview_boarder_upper);
			XDrawPoint(dpy, preview_xid, redraw_gc, 
				   x+preview_boarder_left, 
				   y+preview_boarder_upper);
		      }
		  }
	      }
	  }
	
	XSetForeground(dpy, redraw_gc, black.pixel);
	current_pen.pixel = temp_pixel;
	
	XDrawRectangle(dpy, big_bit_xid, redraw_gc, 
		       boarder_left -1, 
		       boarder_upper -1, 
		       canvas_dimension - ( boarder_left + boarder_right) +1, 
		       canvas_dimension - ( boarder_upper + boarder_lower) +1);
	XDrawRectangle(dpy, big_pixmap, redraw_gc, 
		       boarder_left -1, 
		       boarder_upper -1, 
		       canvas_dimension - ( boarder_left + boarder_right) +1, 
		       canvas_dimension - ( boarder_upper + boarder_lower) +1);
	
	
	current_color_state = undo_images[current_undo%MAX_UNDOS].state;
	
	if ((undo_images[(current_undo+1)%MAX_UNDOS].state == COLOR) 
	    && ( current_color_state == MONO ))
	  {
	    change_stuff_to_mono();
	  }
	else
	  if ((undo_images[(current_undo+1)%MAX_UNDOS].state == MONO) 
	      && ( current_color_state == COLOR )) 
	    {
	      change_stuff_to_color();
	    }
      }
  
  selection_off();
  set_iconedit_footer();
  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
  /*
   *	  fputs("undo couldn't repaint last image...\n", stderr);
   */
}

/*
 * Menu handler for `bw_edit_menu (Undo)'.
 */
Menu_item
  undo(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;

  case MENU_NOTIFY:
    undo_last_edit();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_edit_menu (Invert...)'.
 */
Menu_item
  invert(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    
    /* insert inversion code here... */
    invert_area();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_edit_menu (Clear...)'.
 */
Menu_item
  clear(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
	
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: clear: MENU_NOTIFY\n", stderr);
     */
    
    clear_area();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_edit_menu (Copy...)'.
 */
Menu_item
  copy(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
	
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: copy: MENU_NOTIFY\n", stderr);
     */
    
    copy_to_shelf();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_edit_menu (Paste...)'.
 */
Menu_item
  paste(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  Event	*ptr;
	
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: paste: MENU_NOTIFY\n", stderr);
     */
    
    ptr = (Event *) xv_get(xv_get(item, MENU_PARENT), MENU_FIRST_EVENT);
    if (event_window(ptr) == base_window->strip_control)
      {
	int	newx, newy;
	Window child;
	
	XTranslateCoordinates(dpy, 
			      (Window)xv_get(base_window->strip_control, 
					     XV_XID), 
			      (Window)xv_get(base_window->big_bit_canvas, 
					     XV_XID), 
			      event_x(ptr), 
			      event_y(ptr), 
			      &newx, 
			      &newy, 
			      &child);
	paste_from_shelf(newx + boarder_left, 
			 newy + boarder_upper);
      }
    else
      paste_from_shelf(boarder_left, boarder_upper);
    
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_grid_menu (Toggle)'.
 */
Menu_item
  grid_toggle(item, op)
Menu_item	item;
Menu_generate	op;
{
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: grid_toggle: MENU_NOTIFY\n", stderr);
     */
    if (grid_status == GRID_ON)
      {	
	grid_status = GRID_OFF;
	grid_proc(GRID_OFF);
	xv_set( xv_get( xv_get(base_window->view_bt, 
			       PANEL_ITEM_MENU),
		       MENU_NTH_ITEM, 1), 
	       MENU_STRING, gettext("Grid On"), NULL);
      }
    else
      {	
	grid_status = GRID_ON;
	grid_proc(GRID_ON);
	xv_set( xv_get( xv_get(base_window->view_bt, 
			       PANEL_ITEM_MENU),
		       MENU_NTH_ITEM, 1), 
	       MENU_STRING, gettext("Grid Off"), NULL);
      }
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_grid_menu (Off)'.
 */
Menu_item
  grid_off(item, op)
Menu_item	item;
Menu_generate	op;
{
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: grid_off: MENU_NOTIFY\n", stderr);
     */
    if (grid_status == GRID_ON)
      {	
	grid_status = GRID_OFF;
	grid_proc(GRID_OFF);
      }
    
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_size_menu (16)'.
 */
Menu_item
  size_16(item, op)
Menu_item	item;
Menu_generate	op;
{
  Menu format_menu;
  Menu props_menu;
  
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;

  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: size_16: MENU_NOTIFY\n", stderr);
     */
    
    change_icon_size(16, 16);
    v->height = 16;
    v->width = 16;
    props_menu = xv_get(base_window->props_bt, PANEL_ITEM_MENU);
    format_menu = 
      xv_get(xv_get(props_menu, MENU_NTH_ITEM, 2), 
	     MENU_PULLRIGHT);
    xv_set(format_menu, MENU_DEFAULT_ITEM, 
	   xv_get(format_menu, MENU_NTH_ITEM, 1), NULL);
    
    write_resources();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_size_menu (32)'.
 */
Menu_item
  size_32(item, op)
Menu_item	item;
Menu_generate	op;
{
  Menu format_menu;
  Menu props_menu;
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
	
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;

  case MENU_NOTIFY:
    /*
     *		fputs("base: size_32: MENU_NOTIFY\n", stderr);
     */
    
    change_icon_size(32, 32);
    v->height = 32;
    v->width = 32;
    props_menu = xv_get(base_window->props_bt, PANEL_ITEM_MENU);
    format_menu = xv_get(xv_get(props_menu, MENU_NTH_ITEM, 2), 
			 MENU_PULLRIGHT);
    xv_set(format_menu, MENU_DEFAULT_ITEM, 
	   xv_get(format_menu, MENU_NTH_ITEM, 2), NULL);
    write_resources();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_size_menu (48)'.
 */
Menu_item
  size_48(item, op)
Menu_item	item;
Menu_generate	op;
{
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
  Menu format_menu;
  Menu props_menu;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: size_48: MENU_NOTIFY\n", stderr);
     */
    change_icon_size(48, 48);
    v->height = 48;
    v->width = 48;
    props_menu = xv_get(base_window->props_bt, PANEL_ITEM_MENU);
    format_menu = xv_get(xv_get(props_menu, MENU_NTH_ITEM, 2), 
			 MENU_PULLRIGHT);
    xv_set(format_menu, MENU_DEFAULT_ITEM, 
	   xv_get(format_menu, MENU_NTH_ITEM, 3), NULL);
    write_resources();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_size_menu (64)'.
 */
Menu_item
  size_64(item, op)
Menu_item	item;
Menu_generate	op;
{
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
  Menu format_menu;
  Menu props_menu;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: size_64: MENU_NOTIFY\n", stderr);
     */
    change_icon_size(64, 64);
    v->height = 64;
    v->width = 64;
    props_menu = xv_get(base_window->props_bt, PANEL_ITEM_MENU);
    format_menu = xv_get(xv_get(props_menu, MENU_NTH_ITEM, 2), 
			 MENU_PULLRIGHT);
    xv_set(format_menu, MENU_DEFAULT_ITEM, 
	   xv_get(format_menu, MENU_NTH_ITEM, 4), NULL);
    write_resources();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_size_menu (128)'.
 */
Menu_item
  size_128(item, op)
Menu_item	item;
Menu_generate	op;
{
  Xv_opaque ip = (Xv_opaque) xv_get(item, XV_KEY_DATA, INSTANCE);
  Menu format_menu;
  Menu props_menu;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: size_128: MENU_NOTIFY\n", stderr);
     */
    change_icon_size(128, 128);
    v->height = 128;
    v->width = 128;
    props_menu = xv_get(base_window->props_bt, PANEL_ITEM_MENU);
    format_menu = xv_get(xv_get(props_menu, MENU_NTH_ITEM, 2), 
			 MENU_PULLRIGHT);
    xv_set(format_menu, MENU_DEFAULT_ITEM, 
	   xv_get(format_menu, MENU_NTH_ITEM, 5), NULL);
    write_resources();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_op_menu (Source (Overwrite Canvas))'.
 */
Menu_item
  fill_op_source(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: fill_op_source: MENU_NOTIFY\n", stderr);
     */
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_op_menu (Or (Union))'.
 */
Menu_item
  fill_op_or(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: fill_op_or: MENU_NOTIFY\n", stderr);
     */
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_op_menu (Xor (Exclusive Or))'.
 */
Menu_item
  fill_op_xor(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
/*
 *		fputs("base: fill_op_xor: MENU_NOTIFY\n", stderr);
 */
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_op_menu (And (Intersection))'.
 */
Menu_item
  fill_op_and(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
	
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: fill_op_and: MENU_NOTIFY\n", stderr);
     */
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Event callback function for `palette_bt'.
 */
void
  show_palette(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  /*
   *	fprintf(stderr, "base: show_palette: event %d\n", event_id(event)); 
   */
  panel_default_handle_event(item, event);
}

/*
 * Notify callback function for `brush_setting'.
 */
void
  brush_sel(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  extern Font text_font;
  int	old_edit_mode = Edit_mode;
  
  /*
   *	fprintf(stderr, "base: brush_sel: value: %u\n", value);
   */
  
  Edit_mode = value;
  
  /* Change cursor? */
  
  if (Edit_mode == 7)
    xv_set(canvas_paint_window(base_window->big_bit_canvas), 
	   WIN_CURSOR, eraser_cursor, 
	   NULL);		 
  else
    xv_set(canvas_paint_window(base_window->big_bit_canvas), 
	   WIN_CURSOR, big_bit_cursor, 
	   NULL);
  
  /* determine whether we are in marquis selection mode */
  
  if (value == 6)
    {
      selection_on(base_window->big_bit_canvas);
    }
  if (value == 1)
    {
      selection_off();
    }
  if (value == 2)
    {
      selection_off();
    }
  if (value == 3)
    {
      selection_off();
    }
  if (value == 4)
    {
      selection_off();
    }
  else
    {
      selection_off();
      if (value == 5)
	{
	  text_draw_init();
	  if (!text_popup_located)
	    {
	      ds_position_popup(base_window->window,
				text_popup->popup, DS_POPUP_LOR);
	      text_popup_located = TRUE;
	    }
	  xv_set(text_popup->popup, XV_SHOW, TRUE, 0);
	  
	  if (!text_font)
	    {
	      xv_set(item, PANEL_VALUE, old_edit_mode, 0);
	      value = Edit_mode = old_edit_mode;
	    }
	  
	}
    }
  
  /* determine whether we should have the fill patterns active */
  
  switch (value) {
    
  case 0:
  case 1:
  case 5:
  case 6:
  case 7:
    if (!xv_get(ip->fill_setting, PANEL_INACTIVE))
      xv_set(ip->fill_setting, PANEL_INACTIVE, TRUE, 0);
    break;
    
  default:
    if (xv_get(ip->fill_setting, PANEL_INACTIVE))
      xv_set(ip->fill_setting, PANEL_INACTIVE, FALSE, 0);
    break;
  }
}

/*
 * Notify callback function for `fill_setting'.
 */
void
fill_sel(item, value, event)
	Panel_item	item;
	int		value;
	Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  fill_pattern = value;
  
  switch (value) {
    
  case 0:
    XSetStipple(dpy, fill_gc, pattern_black); 
    break;
  case 1: 
    XSetStipple(dpy, fill_gc, pattern_17);
    break;
  case 2:
    XSetStipple(dpy, fill_gc, pattern_20);
    break;
  case 3:
    XSetStipple(dpy, fill_gc, pattern_25);
    break;
  case 4:
    XSetStipple(dpy, fill_gc, pattern_root);
    break;
  case 5:
    XSetStipple(dpy, fill_gc, pattern_50);
    break;
  case 6:
    XSetStipple(dpy, fill_gc, pattern_75);
    break;
  case 7:
    XSetStipple(dpy, fill_gc, pattern_80);
    break;
  case 8:
    XSetStipple(dpy, fill_gc, pattern_83);
    break;
  case 9:
    XSetStipple(dpy, fill_gc, pattern_black);
    break;
  default:
    break;
	}

}

/*
 * Event callback function for `move_left_bt'.
 */
void
  move_left(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  int x, y, w, h, move_all;
  int preview_x, preview_y, preview_w, preview_h;
  Menu edit_menu;
  XRectangle temp_rect[1];
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT ) 
    {
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
      
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = x; temp_rect[0].y = y; 
	  temp_rect[0].width = w; temp_rect[0].height = h;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_OFF);
	}
      
      preview_x = ((x - boarder_left) / width_block) + preview_boarder_left; 
      preview_y = ((y - boarder_upper) / height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;
      
      XCopyArea(dpy, big_pixmap, moving_pixmap, select_gc, 
		x, y, 
		w, h, 
		x, y);
      XCopyArea(dpy, preview_pixmap, moving_preview_pixmap, preview_gc, 
		preview_x, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);

      selection_left = selection_left - width_block;
      selection_right = selection_right - width_block;	    
      
      x = x - width_block;
      preview_x = preview_x - 1;
      
      XFillRectangle(dpy, big_bit_xid, select_gc, 
		     x + w - 1, 
		     y, 
		     width_block +1, 
		     h +1); 
      XFillRectangle(dpy, big_pixmap, select_gc, 
		     x + w - 1, 
		     y, width_block +1, 
		     h +1); 
      
      XDrawLine( dpy, preview_pixmap, preview_select_gc, 
		preview_x + preview_w, 
		preview_y, 
		preview_x + preview_w, 
		preview_y + preview_h -1); 
      XDrawLine( dpy, preview_xid, preview_select_gc, 
		preview_x + preview_w, 
		preview_y, 
		preview_x + preview_w, 
		preview_y + preview_h -1); 
      
      XCopyArea( dpy, moving_pixmap, big_pixmap, select_gc, 
		x + width_block, y, 
		w, h, 
		x, y);
      XCopyArea( dpy, moving_pixmap, big_bit_xid, select_gc, 
		x + width_block, y, 
		w, h, 
		x, y);

      XCopyArea( dpy, moving_preview_pixmap, preview_pixmap, preview_gc, 
		preview_x +1, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      XCopyArea( dpy, moving_preview_pixmap, preview_xid, preview_gc, 
		preview_x +1, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      
      current_undo++;
      Edited++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      
      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = 0; temp_rect[0].y = 0; 
	  temp_rect[0].width = 1000; temp_rect[0].height = 1000;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_ON);
	}
      
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);
      
      
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      set_iconedit_footer();
    }
  
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `move_right_bt'.
 */
void
  move_right(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  int x, y, w, h, move_all;
  int preview_x, preview_y, preview_w, preview_h;
  Menu edit_menu;
  XRectangle temp_rect[1];
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, 
		     selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
      
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = x; temp_rect[0].y = y; 
	  temp_rect[0].width = w; temp_rect[0].height = h;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_OFF);
	}
      
      preview_x = ((x - boarder_left) / width_block) + preview_boarder_left; 
      preview_y = ((y - boarder_upper) / height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;
      
      XCopyArea( dpy, big_pixmap, moving_pixmap, select_gc, x, y, w, h, x, y);
      XCopyArea( dpy, preview_pixmap, moving_preview_pixmap, preview_gc, 
		preview_x, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      
      selection_left = selection_left + width_block;
      selection_right = selection_right + width_block;	    
      
      x = x + width_block;
      preview_x = preview_x + 1;
      
      XFillRectangle(dpy, big_bit_xid, select_gc, 
		     x - width_block, y, 
		     width_block +1, h +1); 
      XFillRectangle(dpy, big_pixmap, select_gc, 
		     x - width_block, y, 
		     width_block +1, h +1); 
      
      XDrawLine(dpy, preview_pixmap, preview_select_gc, 
		preview_x -1, preview_y, 
		preview_x -1, preview_y + preview_h -1); 
      XDrawLine(dpy, preview_xid, preview_select_gc, 
		preview_x -1, preview_y, 
		preview_x -1, preview_y + preview_h -1); 
      
      XCopyArea(dpy, moving_pixmap, big_pixmap, select_gc, 
		x - width_block, y, 
		w, h, 
		x, y);
      XCopyArea(dpy, moving_pixmap, big_bit_xid, select_gc, 
		x - width_block, y, 
		w, h, 
		x, y);

      XCopyArea(dpy, moving_preview_pixmap, preview_pixmap, preview_gc, 
		preview_x -1, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      XCopyArea(dpy, moving_preview_pixmap, preview_xid, preview_gc, 
		preview_x -1, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);

      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);

      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = 0; temp_rect[0].y = 0; 
	  temp_rect[0].width = 1000; temp_rect[0].height = 1000;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_ON);
	}
      
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      
      set_iconedit_footer();
    }
  
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `move_up_bt'.
 */
void
  move_up(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  int x, y, w, h, move_all;
  int preview_x, preview_y, preview_w, preview_h;
  Menu edit_menu;
  XRectangle temp_rect[1];
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {	
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
      
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = x; temp_rect[0].y = y; 
	  temp_rect[0].width = w; temp_rect[0].height = h;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_OFF);
	}
      
      preview_x = ((x - boarder_left) / width_block) + preview_boarder_left; 
      preview_y = ((y - boarder_upper) / height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;

      XCopyArea( dpy, big_pixmap, moving_pixmap, select_gc, x, y, w, h, x, y);
      XCopyArea( dpy, preview_pixmap, moving_preview_pixmap, preview_gc, 
		preview_x, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      
      selection_top = selection_top - height_block;
      selection_bottom = selection_bottom - height_block;
      
      y = y - height_block;
      preview_y = preview_y - 1;
      
      XFillRectangle(dpy, big_bit_xid, select_gc, 
		     x, y + h -1, 
		     w +1, height_block +1); 
      XFillRectangle(dpy, big_pixmap, select_gc, 
		     x, y + h -1, 
		     w +1, height_block +1); 
      
      XDrawLine( dpy, preview_pixmap, preview_select_gc, 
		preview_x, preview_y + preview_h, 
		preview_x + preview_w -1, preview_y + preview_h); 
      XDrawLine( dpy, preview_xid, preview_select_gc, 
		preview_x, preview_y + preview_h, 
		preview_x + preview_w -1, preview_y + preview_h); 
      
      XCopyArea( dpy, moving_pixmap, big_pixmap, select_gc, 
		x, y + height_block, 
		w, h, 
		x, y);
      XCopyArea( dpy, moving_pixmap, big_bit_xid, select_gc, 
		x, y + height_block, 
		w, h, 
		x, y);

      XCopyArea( dpy, moving_preview_pixmap, preview_pixmap, preview_gc, 
		preview_x, preview_y +1, 
		preview_w, preview_h, 
		preview_x, preview_y);
      XCopyArea( dpy, moving_preview_pixmap, preview_xid, preview_gc, 
		preview_x, preview_y +1, 
		preview_w, preview_h, 
		preview_x, preview_y);

      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);

      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = 0; temp_rect[0].y = 0; 
	  temp_rect[0].width = 1000; temp_rect[0].height = 1000;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_ON);
	}
      
      
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      set_iconedit_footer();
    }
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `move_down_bt'.
 */
void
  move_down(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  int x, y, w, h, move_all;
  int preview_x, preview_y, preview_w, preview_h;
  Menu edit_menu;
  XRectangle temp_rect[1];
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
      
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = x; temp_rect[0].y = y; 
	  temp_rect[0].width = w; temp_rect[0].height = h;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_OFF);
	}
      
      preview_x = ((x - boarder_left) / width_block) + preview_boarder_left; 
      preview_y = ((y - boarder_upper) / height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;
      
      XCopyArea(dpy, big_pixmap, moving_pixmap, select_gc, 
		x, y, 
		w, h, 
		x, y);
      XCopyArea(dpy, preview_pixmap, moving_preview_pixmap, preview_gc, 
		preview_x, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      
      selection_top = selection_top + width_block;
      selection_bottom= selection_bottom + width_block;	    
      
      y = y + height_block;
      preview_y = preview_y + 1;
      
      XFillRectangle(dpy, big_bit_xid, select_gc, 
		     x, y - height_block, 
		     w +1, height_block +1); 
      XFillRectangle(dpy, big_pixmap, select_gc, 
		     x, y - height_block, 
		     w +1, height_block +1); 
      
      XDrawLine(dpy, preview_pixmap, preview_select_gc, 
		preview_x, preview_y -1, 
		preview_x + preview_w -1, preview_y -1); 
      XDrawLine(dpy, preview_xid, preview_select_gc, 
		preview_x, preview_y -1, 
		preview_x + preview_w -1, preview_y -1); 
      
      XCopyArea(dpy, moving_pixmap, big_pixmap, select_gc, 
		x, y - height_block, 
		w, h, 
		x, y);
      XCopyArea(dpy, moving_pixmap, big_bit_xid, select_gc, 
		x, y - height_block, 
		w, h, 
		x, y);
      
      XCopyArea(dpy, moving_preview_pixmap, preview_pixmap, preview_gc, 
		preview_x, preview_y -1, 
		preview_w, preview_h, 
		preview_x, preview_y);
      XCopyArea(dpy, moving_preview_pixmap, preview_xid, preview_gc, 
		preview_x, preview_y -1, 
		preview_w, preview_h, 
		preview_x, preview_y);

      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);
      
      if (grid_status == GRID_ON)
	{
	  temp_rect[0].x = 0; temp_rect[0].y = 0; 
	  temp_rect[0].width = 1000; temp_rect[0].height = 1000;  
	  XSetClipRectangles( dpy, gc_rv, 0, 0, temp_rect, 1, YSorted);
	  grid_proc(GRID_ON);
	}
      
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      
      set_iconedit_footer();
    }
  
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `horzflip_bt'.
 */
void
  horz_flip(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  int x, y, w, h, a, b, corner_x, corner_y, move_all;
  int preview_x, preview_y, preview_w, preview_h;
  Menu edit_menu;
  long pixel, temp_pixel;
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {
      xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
      
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
      
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      preview_x = ((x - boarder_left) / width_block) + preview_boarder_left; 
      preview_y = ((y - boarder_upper) / height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;

      edit_image = XGetImage(dpy, preview_pixmap, 
			     preview_x, preview_y, 
			     preview_w, preview_h, 
			     -1, ZPixmap); 

      temp_pixel = current_pen.pixel;
      
      for (a = 0; a < preview_w; a++)
	for (b = 0; b < preview_h; b++)
	  {
	    corner_x = x + (a * width_block);		    
	    corner_y = y + (b * height_block);
	    pixel = XGetPixel(edit_image, preview_w-(a+1), b);
	    current_pen.pixel = pixel;
	    XSetForeground(dpy, redraw_gc, current_pen.pixel);
	    XFillRectangle(dpy, big_bit_xid, redraw_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);	    
	    XFillRectangle(dpy, big_pixmap, redraw_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);	    
	    XDrawPoint(dpy, preview_pixmap, redraw_gc, 
		       a+preview_x, b+preview_y);
	    XDrawPoint(dpy, preview_xid, redraw_gc, 
		       a+preview_x, b+preview_y);
	  }
      
      XSetForeground(dpy, redraw_gc, black.pixel);
      
      XDrawRectangle(dpy, big_bit_xid, redraw_gc, 
		     boarder_left -1, boarder_upper -1, 
		     canvas_dimension - ( boarder_left + boarder_right) +1, 
		     canvas_dimension - ( boarder_upper + boarder_lower) +1);
      XDrawRectangle(dpy, big_pixmap, redraw_gc, 
		     boarder_left -1, boarder_upper -1, 
		     canvas_dimension - ( boarder_left + boarder_right) +1, 
		     canvas_dimension - ( boarder_upper + boarder_lower) +1);
      
      current_pen.pixel = temp_pixel;
      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      
      set_iconedit_footer();
      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
    }
  
  panel_default_handle_event(item, event);

}

/*
 * Event callback function for `vertflip_bt'.
 */
void
  vert_flip(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);

  int x, y, w, h, a, b, corner_x, corner_y, move_all;
  int preview_x, preview_y, preview_w, preview_h;
  Menu edit_menu;
  long pixel, temp_pixel;
  
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {
      xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 

      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right  
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom  
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      
      preview_x = ((x - boarder_left) / width_block) + preview_boarder_left; 
      preview_y = ((y - boarder_upper) / height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;
      
      edit_image = XGetImage(dpy, preview_pixmap, 
			     preview_x, preview_y, 
			     preview_w, preview_h, 
			     -1, ZPixmap); 
      
      temp_pixel = current_pen.pixel;
      
      for (a = 0; a < preview_w; a++)
	for (b = 0; b < preview_h; b++)
	  {
	    corner_x = x + (a * width_block);		    
	    corner_y = y + (b * height_block);
	    pixel = XGetPixel(edit_image, a, preview_h - (b+1));
	    current_pen.pixel = pixel;
	    XSetForeground(dpy, redraw_gc, current_pen.pixel);
	    XFillRectangle(dpy, big_pixmap, redraw_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);	    
	    XDrawPoint(dpy, preview_pixmap, redraw_gc, 
		       a+preview_x, b+preview_y);
	  }
      
      XCopyArea(dpy, big_pixmap, big_bit_xid, redraw_gc, x, y, w, h, x, y);
      XCopyArea(dpy, preview_pixmap, preview_xid, redraw_gc, 
		preview_x, preview_y, 
		preview_w, preview_h, 
		preview_x, preview_y);
      
      XSetForeground(dpy, redraw_gc, black.pixel);
      
      XDrawRectangle(dpy, big_bit_xid, redraw_gc, 
		     boarder_left -1, boarder_upper -1, 
		     canvas_dimension - ( boarder_left + boarder_right) +1, 
		     canvas_dimension - ( boarder_upper + boarder_lower) +1);
      XDrawRectangle(dpy, big_pixmap, redraw_gc, 
		     boarder_left -1, boarder_upper -1, 
		     canvas_dimension - ( boarder_left + boarder_right) +1, 
		     canvas_dimension - ( boarder_upper + boarder_lower) +1);
      
      current_pen.pixel = temp_pixel;
      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      
      set_iconedit_footer();
      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
    }
  
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `counter_rot_bt'.
 */
void
  counter_rot(item, event)
Panel_item	item;
Event		*event;
{

  int x, y, w, h, corner_x, corner_y;
  int preview_x, preview_y, preview_w, preview_h;
  int mid_prev_x, mid_prev_y, corner_prev_x, corner_prev_y;
  int a, b, original_height_counter, original_width_counter, move_all;
  long int lastpixel, pixel;
  
  Menu edit_menu;
  
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  /*
   *	fprintf(stderr, "base: counter_rot: event %d\n", event_id(event));
   */
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {
      xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
      
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      x = x - boarder_left; y = y - boarder_upper;
      
      preview_x = (x/width_block) + preview_boarder_left; 
      preview_y = (y/height_block) + preview_boarder_upper; 
      preview_w = w/width_block; 
      preview_h = h/height_block;
      
      mid_prev_x = preview_x + (preview_w/2);
      mid_prev_y = preview_y + (preview_h/2);
      
      corner_prev_x = mid_prev_x - preview_h/2;
      corner_prev_y = mid_prev_y - preview_w/2;
      
      if (move_all == 1)
	rotate_image = XGetImage(dpy, preview_pixmap, 
				 preview_x, preview_y, 
				 preview_w, preview_h, 
				 -1, ZPixmap); 
      
      XSetForeground(dpy, moving_gc, white.pixel);
      XSetForeground(dpy, preview_moving_gc, white.pixel);
      
      XFillRectangle(dpy, big_pixmap, moving_gc, 
		     x+boarder_left, y+boarder_upper, 
		     w+1, h+1);
      XFillRectangle(dpy, big_bit_xid, moving_gc, 
		     x+boarder_left, y+boarder_upper, 
		     w+1, h+1);
      XFillRectangle(dpy, preview_xid, preview_moving_gc, 
		     preview_x, preview_y, 
		     preview_w, preview_h);
      XFillRectangle(dpy, preview_pixmap, preview_moving_gc, 
		     preview_x, preview_y, 
		     preview_w, preview_h);
      
      grid_proc(grid_status);
      
      for (original_height_counter = 0; 
	   original_height_counter < preview_h; 
	   original_height_counter++)
	for (original_width_counter = 0; 
	     original_width_counter < preview_w; 
	     original_width_counter++)
	  {
	    corner_x = ((corner_prev_x - preview_boarder_left) * width_block) 
	      + boarder_left + (original_height_counter * width_block);
	    corner_y = ((corner_prev_y - preview_boarder_upper) * height_block)
	      + boarder_upper + (original_width_counter * height_block);
	    pixel = XGetPixel(rotate_image, 
			      (preview_w - 1) - original_width_counter, 
			      original_height_counter);
	    XSetForeground(dpy, moving_gc, pixel);
	    XSetForeground(dpy, preview_moving_gc, pixel);
	    XFillRectangle(dpy, big_bit_xid, moving_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);
	    XFillRectangle(dpy, big_pixmap, moving_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);
	    XDrawPoint(dpy, preview_xid, preview_moving_gc, 
		       corner_prev_x + original_height_counter, 
		       corner_prev_y + original_width_counter);  
	    XDrawPoint(dpy, preview_pixmap, preview_moving_gc, 
		       corner_prev_x + original_height_counter, 
		       corner_prev_y + original_width_counter); 
	  }
      
      x = x + boarder_left;    y = y + boarder_upper;
      
      selection_left = ((corner_prev_x - preview_boarder_left)*width_block) 
	+boarder_left ;
      selection_top = ((corner_prev_y - preview_boarder_upper)*height_block) 
	+boarder_upper;
      selection_right = selection_left + (preview_h * height_block) ;
      selection_bottom = selection_top + (preview_w * width_block) ;
      
      if (selection_top < boarder_upper) 
	selection_top = boarder_upper;
      else if (selection_top > (canvas_dimension - boarder_lower)) 
	selection_top = (canvas_dimension - boarder_lower) -1;
      if (selection_left < boarder_left) 
	selection_left = boarder_left;
      else if (selection_left > (canvas_dimension - boarder_right)) 
	selection_left = (canvas_dimension - boarder_right) -1;
      if (selection_right < boarder_left) 
	selection_right = boarder_left;
      else if (selection_right > ( canvas_dimension - boarder_right)) 
	selection_right = (canvas_dimension - boarder_right) -1;
      if (selection_bottom < boarder_upper) 
	selection_bottom = boarder_upper;
      else if (selection_bottom > (canvas_dimension - boarder_lower)) 
	selection_bottom = (canvas_dimension - boarder_lower) -1;
      
      
      rotate_image = 
	XGetImage(dpy, preview_pixmap, 
		  ((((selection_left > selection_right) 
		     ? selection_right 
		     : selection_left ) 
		    - boarder_left)/width_block)+preview_boarder_left,
		  ((((selection_top > selection_bottom) 
		     ? selection_bottom 
		     : selection_top ) 
		    - boarder_upper)/height_block)+preview_boarder_upper,
		  (selection_left < selection_right) 
		  ? (selection_right - selection_left)/width_block 
		  : (selection_left - selection_right)/width_block,
		  (selection_top < selection_bottom) 
		  ? (selection_bottom - selection_top)/height_block 
		  : (selection_top - selection_bottom)/height_block, 
		  -1, ZPixmap); 
      


      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      
      undo_images[current_undo%MAX_UNDOS].image = 
	XGetImage(dpy, preview_pixmap, 
		  preview_boarder_left, preview_boarder_upper, 
		  icon_width, icon_height, 
		  -1, ZPixmap);	    
      
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
    }
  
  set_iconedit_footer();
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `clock_rot_bt'.
 */
void
  clock_rot(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects *ip = (base_window_objects *) xv_get(item, 
							   XV_KEY_DATA, 
							   INSTANCE);
  
  int x, y, w, h, corner_x, corner_y;
  int preview_x, preview_y, preview_w, preview_h;
  int mid_prev_x, mid_prev_y, corner_prev_x, corner_prev_y;
  int original_height_counter, original_width_counter, move_all;
  long int lastpixel, pixel;
  
  Menu edit_menu;
  
  if ( event_is_up(event) && event_action(event) == ACTION_SELECT )
    {
      xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
      XDrawRectangle(dpy, big_bit_xid, select_gc, 
		     selection_left, selection_top, 
		     selection_right - selection_left, 
		     selection_bottom - selection_top); 
	    
      if (((selection_left == 0) && (selection_right == 0)) 
	  && ((selection_top == 0) && (selection_bottom == 0)))
	{
	  move_all = 1;
	  selection_left = boarder_left; 
	  selection_top = boarder_upper; 
	  selection_right = (canvas_dimension - boarder_right); 
	  selection_bottom = (canvas_dimension - boarder_lower);
	}
      else 
	move_all = 0;
      
      x = selection_left < selection_right ? selection_left : selection_right;
      y = selection_top < selection_bottom ? selection_top : selection_bottom;
      
      w = selection_left < selection_right 
	? selection_right - selection_left 
	  : selection_left - selection_right; 
      h = selection_top < selection_bottom 
	? selection_bottom - selection_top 
	  : selection_top - selection_bottom;
      
      x = x - boarder_left; 
      y = y - boarder_upper;
      
      preview_x = (x/width_block) + preview_boarder_left; 
      preview_y = (y/height_block) + preview_boarder_upper;
      preview_w = w/width_block; 
      preview_h = h/height_block;
      
      mid_prev_x = preview_x + (preview_w/2);
      mid_prev_y = preview_y + (preview_h/2);
      
      corner_prev_x = mid_prev_x - preview_h/2;
      corner_prev_y = mid_prev_y - preview_w/2;
      
      if (move_all == 1)
	rotate_image = XGetImage(dpy, preview_pixmap, 
				 preview_x, preview_y, 
				 preview_w, preview_h, 
				 -1, ZPixmap); 
      
      XSetForeground(dpy, moving_gc, white.pixel);
      XSetForeground(dpy, preview_moving_gc, white.pixel);
      
      XFillRectangle(dpy, big_bit_xid, moving_gc, 
		     x+ boarder_left, y+ boarder_upper, 
		     w+1, h+1);
      XFillRectangle(dpy, big_pixmap, moving_gc, 
		     x+boarder_left, y+boarder_upper, 
		     w+1, h+1);
      XFillRectangle(dpy, preview_xid, preview_moving_gc, 
		     preview_x, preview_y, 
		     preview_w, preview_h);
      XFillRectangle(dpy, preview_pixmap, preview_moving_gc, 
		     preview_x, preview_y, 
		     preview_w, preview_h);
      
      grid_proc(grid_status);
      
      for (original_height_counter = 0; 
	   original_height_counter < preview_h; 
	   original_height_counter++)
	for (original_width_counter = 0; 
	     original_width_counter < preview_w; 
	     original_width_counter++)
	  {
	    corner_x = ((corner_prev_x - preview_boarder_left) * width_block) 
	      + boarder_left + (original_height_counter * width_block);
	    corner_y = ((corner_prev_y - preview_boarder_upper) * height_block)
	      + boarder_upper + (original_width_counter * height_block);
	    pixel = XGetPixel(rotate_image, 
			      original_width_counter, 
			      (preview_h -1) - original_height_counter);
	    XSetForeground(dpy, moving_gc, pixel);
	    XSetForeground(dpy, preview_moving_gc, pixel);
	    XFillRectangle(dpy, big_bit_xid, moving_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);
	    XDrawPoint(dpy, preview_pixmap, preview_moving_gc, 
		       corner_prev_x + original_height_counter, 
		       corner_prev_y + original_width_counter);
	    XFillRectangle(dpy, big_pixmap, moving_gc, 
			   corner_x+1, corner_y+1, 
			   width_block-1, height_block-1);
	    XDrawPoint(dpy, preview_xid, preview_moving_gc, 
		       corner_prev_x + original_height_counter, 
		       corner_prev_y + original_width_counter);
	  }
      
      x = x + boarder_left;    
      y = y + boarder_upper;
      
      selection_left = ((corner_prev_x - preview_boarder_left)*width_block) 
	+boarder_left ;
      selection_top = ((corner_prev_y - preview_boarder_upper)*height_block) 
	+boarder_upper ;
      selection_right = selection_left + (preview_h * height_block) ;
      selection_bottom = selection_top + (preview_w * width_block) ;
      
      if (selection_top < boarder_upper) 
	selection_top = boarder_upper;
      else if (selection_top > (canvas_dimension - boarder_lower)) 
	selection_top = (canvas_dimension - boarder_lower) -1;

      if (selection_left < boarder_left) 
	selection_left = boarder_left;
      else if (selection_left > (canvas_dimension - boarder_right)) 
	selection_left = (canvas_dimension - boarder_right) -1;

      if (selection_right < boarder_left) 
	selection_right = boarder_left;
      else if (selection_right > ( canvas_dimension - boarder_right)) 
	selection_right = (canvas_dimension - boarder_right) -1;

      if (selection_bottom < boarder_upper) 
	selection_bottom = boarder_upper;
      else if (selection_bottom > (canvas_dimension - boarder_lower)) 
	selection_bottom = (canvas_dimension - boarder_lower) -1;
      
      rotate_image = 
	XGetImage(dpy, preview_pixmap, 
		  ((((selection_left > selection_right) 
		     ? selection_right 
		     : selection_left ) - boarder_left) / width_block) 
		  + preview_boarder_left,
		  ((((selection_top > selection_bottom) 
		     ? selection_bottom 
		     : selection_top ) - boarder_upper) / height_block) 
		  + preview_boarder_upper,
		  (selection_left < selection_right) 
		  ? (selection_right - selection_left) / width_block 
		  : (selection_left - selection_right) / width_block,
		  (selection_top < selection_bottom) 
		  ? (selection_bottom - selection_top) / height_block 
		  : (selection_top - selection_bottom) / height_block,
		  -1, ZPixmap); 

      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited++;
      current_undo++;
      undo_images[current_undo%MAX_UNDOS].edited++;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
      
      undo_images[current_undo%MAX_UNDOS].image 
	= XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, 
		    -1, ZPixmap);	    
      if (redo_possible == TRUE)
	{
	  redo_possible = FALSE;
	  redos_allowed = 0;
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
		 MENU_INACTIVE, TRUE, NULL);
	}
      if (move_all == 1)
	{
	  selection_left = 0; 
	  selection_top = 0; 
	  selection_right = 0; 
	  selection_bottom = 0;
	}
      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
    }
  
  set_iconedit_footer();
  panel_default_handle_event(item, event);
}

/*
 * Repaint callback function for `preview_canvas'.
 */
void
  repaint_preview_canvas(canvas, paint_window, display, xid, rects)
Canvas		canvas;
Xv_window	paint_window;
Display		*display;
Window		xid;
Xv_xrectlist	*rects;
{
  /*
   *	fputs("base: repaint_preview_canvas\n", stderr);
   */
  XCopyArea(dpy, preview_pixmap, preview_xid, gc, 0, 0, 138, 138, 0, 0); 
}


void
  paint_square(win, event)
Xv_window win;
Event     *event;
{
  int x, y, corner_x, corner_y;
  Menu edit_menu;
  
  x =  event->ie_locx;
  y =  event->ie_locy;
  
  if (((x>=boarder_left) && (x<canvas_dimension-(boarder_right+1))) 
      && ((y>=boarder_upper) && (y<canvas_dimension-(boarder_lower+1)))) 
    {
      
      corner_x = (((x-boarder_left)/width_block)*width_block) 
	+ boarder_left +1;
      corner_y = (((y-boarder_upper)/height_block)*height_block 
		  + boarder_upper +1);
      
      if (event_left_is_down(event))
	{
	  XFillRectangle(dpy, big_bit_xid, gc, 
			 corner_x, corner_y, 
			 width_block-1, height_block-1);
	  XFillRectangle(dpy, big_pixmap, gc, 
			 corner_x, corner_y, 
			 width_block-1, height_block-1);
	  XDrawPoint(dpy, preview_xid, gc, 
		     ((corner_x - boarder_left) / height_block) 
		     + preview_boarder_left, 
		     ((corner_y - boarder_upper) / width_block) 
		     + preview_boarder_upper);
	  XDrawPoint(dpy, preview_pixmap, gc, 
		     ((corner_x - boarder_left)/height_block) 
		     + preview_boarder_left, 
		     ((corner_y - boarder_upper) / width_block) 
		     + preview_boarder_upper);
	  
	}
      if (event_middle_is_down(event))
	{
	  XFillRectangle(dpy, big_bit_xid, gc_rv, 
			 corner_x, corner_y, 
			 width_block-1, height_block-1);
	  XFillRectangle(dpy, big_pixmap, gc_rv, 
			 corner_x, corner_y, 
			 width_block-1, height_block-1);
	  XDrawPoint(dpy, preview_xid, gc_rv, 
		     ((corner_x - boarder_left) / height_block) 
		     + preview_boarder_left, 
		     ((corner_y - boarder_upper) / width_block) 
		     + preview_boarder_upper);
	  XDrawPoint(dpy, preview_pixmap, gc_rv, 
		     ((corner_x - boarder_left) / height_block) 
		     + preview_boarder_left, 
		     ((corner_y - boarder_upper) / width_block) 
		     + preview_boarder_upper);
	}
      if (( event_is_up(event) && event_action(event) == ACTION_SELECT ) 
	  || ( event_is_up(event) && event_action(event) == ACTION_ADJUST ))
	{
	  
	  undo_images[current_undo%MAX_UNDOS].state = current_color_state;
	  undo_images[current_undo%MAX_UNDOS].height = icon_height;
	  undo_images[current_undo%MAX_UNDOS].width = icon_width;
	  if (undos_allowed < (MAX_UNDOS -1))
	  undos_allowed++;
	  
	  Edited++;
	  current_undo++;
	  undo_images[current_undo%MAX_UNDOS].edited++;
	  
	  if (undos_allowed == 1)
	    {
	      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		     MENU_INACTIVE, FALSE, NULL);
	    }
	  
	  redos_allowed = 0;
	  
	  set_iconedit_footer();
	  
	  undo_images[current_undo%MAX_UNDOS].image = 
	    XGetImage(dpy, preview_pixmap, 
		      preview_boarder_left, preview_boarder_upper, 
		      icon_width, icon_height, 
		      -1, ZPixmap);
	}
    }
}

/*
 * Event callback function for `big_bit_canvas'.
 */
Notify_value
  preview_canvas_event(win, event, arg, type)
Xv_window	win;
Event		*event;
Notify_arg	arg;
Notify_event_type type;
{
  if (event_action(event) == ACTION_HELP && event_is_down(event)) 
    {
      xv_help_show(win, "iconedit:PreviewWindow", event);
    }
  
  return notify_next_event_func(win, (Notify_event) event, arg, type);
}

/*
 * Event callback function for `big_bit_canvas'.
 */
Notify_value
  big_canvas_event(win, event, arg, type)
Xv_window	win;
Event		*event;
Notify_arg	arg;
Notify_event_type type;
{
  
  int newx, newy;
  Window child;
  base_window_objects	*ip = (base_window_objects *) xv_get(win, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  /*
   *	fprintf(stderr, "base: big_canvas_event: event %d\n", event_id(event));
   */	
  
  if (event_action(event) == ACTION_MENU && event_is_down(event)) {
    Menu   edit_menu  = xv_get(base_window->edit_bt, 
			       PANEL_ITEM_MENU);	  
    if (win != base_window->strip_control)
      {
	
	XTranslateCoordinates(dpy, 
			      (Window)xv_get(base_window->big_bit_canvas, 
					     XV_XID), 
			      (Window)xv_get(base_window->strip_control, 
					     XV_XID), 
			      event_x(event), 
			      event_y(event), 
			      &newx, 
			      &newy, 
			      &child);
	
	event_set_x(event, newx);
	event_set_y(event, newy);
	
	event_set_window(event, base_window->strip_control);
      }
    
    
    if (edit_menu)
      menu_show(edit_menu, base_window->strip_control, event, 0);
    
  }
  else if (event_action(event) == ACTION_CUT && event_is_down(event))
    cut_to_shelf();
  else if (event_action(event) == ACTION_COPY && event_is_down(event))
    copy_to_shelf();
  else if (event_action(event) == ACTION_PASTE && event_is_down(event))
    paste_from_shelf(event->ie_locx, event->ie_locy);
  else if ((event_action(event) == ACTION_REDO) && event_is_down(event))
    redo_last_edit();
  else if ((event_action(event) == ACTION_UNDO) && event_is_down(event))
    undo_last_edit();
  else if ((event_action(event) == ACTION_HELP) && event_is_down(event))
    xv_help_show(win, "iconedit:Canvas", event);
  else 
    {
      
      switch (Edit_mode) {
      case 0:
	XSetForeground(dpy, gc, current_pen.pixel);
	(void)paint_square(win, event);
	break;
      case 1:
	XSetForeground(dpy, gc, current_pen.pixel);
	XSetFillStyle(dpy, fill_gc, FillSolid);
      line_interpose_proc(win, event, arg, type);
	break;
      case 2:
	XSetFillStyle(dpy, fill_gc, FillStippled);
	box_interpose_proc(win, event, arg, type);
	break;
      case 3:
	XSetForeground(dpy, gc, current_pen.pixel);
	XSetFillStyle(dpy, fill_gc, FillStippled);
	circle_interpose_proc(win, event, arg, type);
	break;
      case 4:
	XSetForeground(dpy, gc, current_pen.pixel);
	XSetFillStyle(dpy, fill_gc, FillStippled);
	ellipse_interpose_proc(win, event, arg, type);
	break;
      case 5:
	XSetForeground(dpy, gc, current_pen.pixel);
	XSetFillStyle(dpy, fill_gc, FillSolid);
	text_interpose_proc(win, event, arg, type);
	break;
      case 6:
	XSetFillStyle(dpy, fill_gc, FillStippled);
	select_interpose_proc(win, event, arg, type);
	break;
      case 7:
	XSetForeground(dpy, gc, white.pixel);
	XSetFillStyle(dpy, fill_gc, FillStippled);
	(void)paint_square(win, event);
	break;
      }
    }
  return notify_next_event_func(win, (Notify_event) event, arg, type);
}

/*
 * Repaint callback function for `big_bit_canvas'.
 */
void
  repaint_big_canvas(canvas, paint_window, display, xid, rects)
Canvas		canvas;
Xv_window	paint_window;
Display		*display;
Window		xid;
Xv_xrectlist	*rects;
{
  
  long pixel, temp_pixel;
  int x, y, corner_x, corner_y;
  
  /*
   *  fputs("base: repaint_big_canvas\n", stderr);
   */
  
  
  XCopyArea(dpy, big_pixmap, big_bit_xid, gc, 0, 0, canvas_dimension, canvas_dimension, 0, 0); 	
  
  
  XSetForeground(dpy, redraw_gc, black.pixel);
  /*
    XDrawRectangle(dpy, big_bit_xid, gc, 31, 31, 450, 450);
    XDrawRectangle(dpy, big_bit_xid, redraw_gc, 31, 31, 450, 450);
    XDrawRectangle(dpy, big_pixmap, redraw_gc, 31, 31, 450, 450);
    */
  
  XDrawRectangle(dpy, big_bit_xid, redraw_gc, 
		 boarder_left -1, boarder_upper -1, 
		 canvas_dimension - ( boarder_left + boarder_right) +1, 
		 canvas_dimension - ( boarder_upper + boarder_lower) +1);
  XDrawRectangle(dpy, big_pixmap, redraw_gc, 
		 boarder_left -1, boarder_upper -1, 
		 canvas_dimension - ( boarder_left + boarder_right) +1, 
		 canvas_dimension - ( boarder_upper + boarder_lower) +1);
  
}

/*
 * Menu handler for `palette_menu'.
 */
Menu
  color_selection(menu, op)
Menu		menu;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(menu, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: color_selection: MENU_NOTIFY\n", stderr);
     */
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return menu;
}

void
  select_color_from_rgb(red, green, blue)
int red;
int green;
int blue;
{
  
  XColor new_color;
  static Xv_singlecolor xv_new_color;
  
  xv_new_color.red = red;
  xv_new_color.green = green;
  xv_new_color.blue = blue; 

  new_color.red = red * 256;
  new_color.green = green * 256;
  new_color.blue = blue * 256; 

  if (( current_color_state == COLOR)
      || ((red == 0) && (green == 0) && (blue == 0))
      || ((red == 255) && (green == 255) && (blue == 255)))
    
    {
      if (XAllocColor(dpy, cmap, &new_color) == 0)
	{
	  printf(gettext("new color allocation didn't work\n"));
	}
      else
	{
	  XSetForeground(dpy, gc, new_color.pixel);
	  XSetForeground(dpy, fill_gc, new_color.pixel);
	  current_pen.pixel = new_color.pixel;
	  
	  xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xv_new_color, NULL);
	  
	  if (((red * 0.41) + (green * 0.33) + (blue * 0.26)) < 128 )
	    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
	  else
	    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvblack, NULL);
	  
	}
    }
  else 
    notice_prompt(base_window->window, NULL,
		  NOTICE_MESSAGE_STRINGS,
		  gettext("You may not set a pen to be color for monochrome drawing"),
		  0,
		  NOTICE_BUTTON_NO, gettext("Continue"),
		  0);

}
/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_black(item, op)
Menu_item	item;
Menu_generate	op;
{
  /*	base_window_objects * ip = (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE); */
  
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_black: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, black.pixel);
    XSetForeground(dpy, fill_gc, black.pixel);
    current_pen.pixel = black.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvblack, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_red(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvred;
  
  base_window_objects * ip = (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  xvred.red = red.red/256; 
  xvred.green = red.green/256; 
  xvred.blue = red.blue/256;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_red: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, red.pixel);
    XSetForeground(dpy, fill_gc, red.pixel);
    current_pen.pixel = red.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvred, NULL);		
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_green(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvgreen;
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  xvgreen.red = green.red/256; 
  xvgreen.green = green.green/256; 
  xvgreen.blue = green.blue/256;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_green: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, green.pixel);
    XSetForeground(dpy, fill_gc, green.pixel);
    current_pen.pixel = green.pixel;
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvblack, NULL);
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvgreen, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_blue(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvblue;
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  xvblue.red = blue.red/256; 
  xvblue.green = blue.green/256; 
  xvblue.blue = blue.blue/256;
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_blue: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, blue.pixel);
    XSetForeground(dpy, fill_gc, blue.pixel);
    current_pen.pixel = blue.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvblue, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}


/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_orange(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvorange;
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  xvorange.red = orange.red/256; 
  xvorange.green = orange.green/256; 
  xvorange.blue = orange.blue/256;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_orange: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, orange.pixel);
    XSetForeground(dpy, fill_gc, orange.pixel);
    current_pen.pixel = orange.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvorange, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_yellow(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvyellow;
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  xvyellow.red = yellow.red/256; 
  xvyellow.green = yellow.green/256; 
  xvyellow.blue = yellow.blue/256;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_yellow: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, yellow.pixel);
    XSetForeground(dpy, fill_gc, yellow.pixel);
    current_pen.pixel = yellow.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvyellow, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvblack, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_violet(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvviolet;
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  xvviolet.red = violet.red/256; 
  xvviolet.green = violet.green/256; 
  xvviolet.blue = violet.blue/256;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_violet: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, violet.pixel);
    XSetForeground(dpy, fill_gc, violet.pixel);
    current_pen.pixel = violet.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvviolet, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_brown(item, op)
Menu_item	item;
Menu_generate	op;
{
  static Xv_singlecolor xvbrown;
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  xvbrown.red = brown.red/256; 
  xvbrown.green = brown.green/256; 
  xvbrown.blue = brown.blue/256;
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_brown: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, brown.pixel);
    XSetForeground(dpy, fill_gc, brown.pixel);
    current_pen.pixel = brown.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvbrown, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvwhite, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `palette_menu (icon/swatch_black.icon16)'.
 */
Menu_item
  select_white(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = (base_window_objects *) xv_get(item, 
							    XV_KEY_DATA, 
							    INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    /*
     *		fputs("base: select_white: MENU_NOTIFY\n", stderr);
     */
    XSetForeground(dpy, gc, white.pixel);
    XSetForeground(dpy, fill_gc, white.pixel);
    current_pen.pixel = white.pixel;
    xv_set(big_bit_cursor, CURSOR_FOREGROUND_COLOR, &xvwhite, NULL);
    xv_set(big_bit_cursor, CURSOR_BACKGROUND_COLOR, &xvblack, NULL);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Event callback function for `black_white_setting'.
 */
void
  b_w_event_handler(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  /*
   * fprintf(stderr, "base: b_w_event_handler: event %d\n", event_id(event));
   */
  panel_default_handle_event(item, event);
}

/*
 * Event callback function for `color_mono_setting'.
 */
void
  color_mono_event_handler(item, event)
Panel_item	item;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  /*
   * fprintf(stderr, "base: color_mono_event_handler: event %d\n", 
             event_id(event));
   */
  panel_default_handle_event(item, event);
}

/*
 * Notify callback function for `black_white_setting'.
 */
void
  b_w_notify_handler(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  if (value == 0) 
    select_color_from_rgb(0, 0, 0);
  else 
    if (value == 1)
      select_color_from_rgb(255, 255, 255);
}


/*
 * Notify callback function for `color_mono_setting'.
 */
void
  color_mono_notify_handler(item, value, event)
Panel_item	item;
int		value;
Event		*event;
{
  base_window_objects	*ip = (base_window_objects *) xv_get(item, 
							     XV_KEY_DATA, 
							     INSTANCE);
  
  GC write_gc;
  unsigned long value_mask;
  XGCValues values;
  int x, y, status, items, i;
  Menu color_menu, edit_menu;
  int	answer;
  extern char loaded_file[];
  
  if (((value == MONO) && ( current_color_state == COLOR )) &&
      (Edited || strlen(loaded_file)))
    {
      answer = notice_prompt(base_window->window, NULL,
			     NOTICE_MESSAGE_STRINGS,
			     gettext("Changing to a monochrome image will lose your color information.\nYou may continue, losing that information, or cancel, retaining the color information.\nIf you continue, the original image may be restored using Undo."),
			     0,
			     NOTICE_BUTTON_YES, gettext("Cancel"),
			     NOTICE_BUTTON_NO, gettext("Continue"),
			     0);
      
      
      if (answer == NOTICE_YES)
	{
	  xv_set(item, PANEL_VALUE, COLOR, 0);
	  return;
	}
    }
  
  xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
  
  if ((value == MONO) && ( current_color_state == COLOR ))
    {
      
      
      current_color_state = MONO;
      
      select_color_from_rgb(0, 0, 0);
      
      write_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
      
      value_mask = 0xFFFF;
      
      status = XGetGCValues(dpy, write_gc, value_mask, &values);
      
      XSetForeground(dpy, write_gc, black.pixel);	  
      XSetBackground(dpy, write_gc, white.pixel);	  
      
      /* if there was no file loaded, and the image had not 
	 been previously edited, don't count this transition as
	 an edit. */
      
      if (Edited || strlen(loaded_file)) {
	edit_image = XGetImage(dpy, preview_pixmap, 
			       preview_boarder_left, 
			       preview_boarder_upper, 
			       icon_width, icon_height, -1, ZPixmap); 
	
	undo_images[(current_undo+1)%MAX_UNDOS].image = 
	  XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, -1, ZPixmap);
	undo_images[current_undo%MAX_UNDOS].height = icon_height;
	undo_images[current_undo%MAX_UNDOS].width = icon_width;
	
	if (undos_allowed < (MAX_UNDOS -1))
	  undos_allowed++;
	
	Edited++;
	current_undo++;
	undo_images[current_undo%MAX_UNDOS].edited++;
	
	redos_allowed = 0;
	
	if (undos_allowed == 1)
	  {
	    edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	    xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		   MENU_INACTIVE, FALSE, NULL);
	  }
      }
      
      set_iconedit_footer();
      
      for (y = 0; y < icon_height; y++)
	for (x = 0; x < icon_width; x++)
	  {
	    if ((white.pixel != XGetPixel(edit_image, x, y)) && 
		(xv_get( xv_get( base_window->window, WIN_CMS ), 
			CMS_BACKGROUND_PIXEL ) 
		 != XGetPixel(edit_image, x, y)))
	      {
		XPutPixel(edit_image, x, y, black.pixel);
		XPutPixel(undo_images[(current_undo)%MAX_UNDOS].image,
			  x, y, black.pixel);
	      }
	  }
      do_that_base_repaint_thing(undo_images[current_undo%MAX_UNDOS].image);
      undo_images[current_undo%MAX_UNDOS].state = MONO;
      XFreeGC(dpy, write_gc);
      change_stuff_to_mono();
    }
  else
    if ((value == COLOR) && ( current_color_state == MONO ))
      {
	current_color_state = COLOR;
	change_stuff_to_color();
      }
  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
  
}


/*
 * Menu handler for `bw_file_menu (Save)'.
 */
Menu_item
  save_menu_proc(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = 
    (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    do_that_save_thing();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `bw_edit_menu (Cut)'.
 */
Menu_item
  cut(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = 
    (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    cut_to_shelf();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

redo_last_edit()
   
{
  
  long pixel, temp_pixel, pixel_old;
  int x, y, corner_x, corner_y, items, i;
  
  Menu color_menu;	
  Menu edit_menu;
  
  xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
  redos_allowed = redos_allowed -1;
  if (redos_allowed == 0)
    {
      redo_possible = FALSE;
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
	     MENU_INACTIVE, TRUE, NULL);
    }
  undo_images[current_undo%MAX_UNDOS].height = icon_height;
  undo_images[current_undo%MAX_UNDOS].width = icon_width;
  
  if (undos_allowed < (MAX_UNDOS -1))
    undos_allowed++;
  
  current_undo++;
  Edited = undo_images[current_undo%MAX_UNDOS].edited;
  
  
  if (undos_allowed == 1)
    {
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
	     MENU_INACTIVE, FALSE, NULL);
    }
  
	    
  temp_pixel = current_pen.pixel;	
  if ((undo_images[current_undo%MAX_UNDOS].width != icon_width) || 
      (undo_images[current_undo%MAX_UNDOS].height != icon_height))
    {
      icon_width = undo_images[current_undo%MAX_UNDOS].width;
      icon_height = undo_images[current_undo%MAX_UNDOS].height;
      
      XFillRectangle( dpy, preview_pixmap, gc_rv, 
		     0 , 0, 
		     138, 138);
      XFillRectangle( dpy, big_pixmap, gc_rv, 
		     0, 0, 
		     canvas_dimension, canvas_dimension); 
      XFillRectangle( dpy, preview_xid, gc_rv, 
		     0 , 0, 
		     138, 138);
      XFillRectangle( dpy, big_bit_xid, gc_rv, 
		     0, 0, 
		     canvas_dimension, canvas_dimension); 
      
      set_the_size_info();
      
      do_that_base_repaint_thing(undo_images[current_undo%MAX_UNDOS].image);
      reset_those_pesky_clip_masks();
      initialize_grid_lines();
      grid_proc(grid_status);
      
    }
  else
    {	    
      
      
      for (x = 0; x < icon_width; x++)
	{
	  corner_x = (x * width_block) + boarder_left +1;
	  for (y = 0; y < icon_height; y++)
	    {
	      corner_y = (y * height_block) + boarder_upper +1;
	      pixel = 
		XGetPixel(undo_images[current_undo%MAX_UNDOS].image, 
			  x, y);
	      pixel_old = 
		XGetPixel(undo_images[(current_undo-1)%MAX_UNDOS].image, 
			  x, y);
	      if (!(pixel == pixel_old))
		{
		  current_pen.pixel = pixel;
		  XSetForeground(dpy, redraw_gc, current_pen.pixel);
		  XFillRectangle(dpy, big_bit_xid, redraw_gc, 
				 corner_x, corner_y, 
				 width_block-1, height_block-1);	    
		  XFillRectangle(dpy, big_pixmap, redraw_gc, 
				 corner_x, corner_y, 
				 width_block-1, height_block-1);	    
		  XDrawPoint(dpy, preview_pixmap, redraw_gc, 
			     x+preview_boarder_left, y+preview_boarder_upper);
		  XDrawPoint(dpy, preview_xid, redraw_gc, 
			     x+preview_boarder_left, y+preview_boarder_upper);
		}
	    }
	}
    }
  XSetForeground(dpy, redraw_gc, black.pixel);
  XDrawRectangle(dpy, big_bit_xid, redraw_gc, 
		 boarder_left -1, boarder_upper -1, 
		 canvas_dimension - ( boarder_left + boarder_right) +1, 
		 canvas_dimension - ( boarder_upper + boarder_lower) +1);
  XDrawRectangle(dpy, big_pixmap, redraw_gc, 
		 boarder_left -1, boarder_upper -1, 
		 canvas_dimension - ( boarder_left + boarder_right) +1, 
		 canvas_dimension - ( boarder_upper + boarder_lower) +1);
  
  
  current_pen.pixel = temp_pixel;
  
  current_color_state = undo_images[current_undo%MAX_UNDOS].state;
  
  if ((undo_images[(current_undo-1)%MAX_UNDOS].state == COLOR) && 
      ( current_color_state == MONO ))
    {
      change_stuff_to_mono();
    }
  else
    if ((undo_images[(current_undo-1)%MAX_UNDOS].state == MONO) && 
	( current_color_state == COLOR )) 
      {
	change_stuff_to_color();
      }
  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
  set_iconedit_footer();
}

/*
 * Menu handler for `bw_edit_menu (Redo)'.
 */
Menu_item
  redo(item, op)
Menu_item	item;
Menu_generate	op;
{
  base_window_objects * ip = 
    (base_window_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    redo_last_edit();
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

void 
  change_stuff_to_mono()
{
  
  int items, i;
  Menu color_menu;	
  Menu save_menu, edit_menu;
  
  /* Set pen to black */
  
  select_black(NULL, MENU_NOTIFY);
  
  /* Set colors on palette to only black and white */
  
  if (! tt_running )
    {
      color_menu = xv_get(base_window->palette_bt, PANEL_ITEM_MENU);
      items = xv_get(color_menu, MENU_NITEMS);
      for (i = 2; i < items; i++)
	xv_set(xv_get(color_menu, MENU_NTH_ITEM, i), 
	       MENU_INACTIVE, TRUE, NULL);
    }
  else
    {
      xv_set(base_window->palette_bt, XV_SHOW, FALSE, 0);
      xv_set(black_white_toggle, XV_SHOW, TRUE, 0);
      xv_set(black_white_toggle, PANEL_VALUE, 0, NULL);
      select_color_from_rgb(0, 0, 0);
    }
  
  
  /* Set toggle to mono */
  
  xv_set(base_window->color_mono_setting, PANEL_VALUE, 1, NULL);
  
  edit_menu = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 8), MENU_INACTIVE, FALSE, NULL);  
  
  if ( save_format == 3 )
    save_format = 4;
}

void 
  change_stuff_to_color()
{
  
  int items, i;
  Menu color_menu;	
  Menu save_menu, edit_menu;
  

  /* Set colors on palette to all */
  
  if (screen_depth != 1)
    {
      if (!tt_running)
	{
	  color_menu = xv_get(base_window->palette_bt, PANEL_ITEM_MENU);
	  items = xv_get(color_menu, MENU_NITEMS);
	  for (i = 2; i < items; i++)
	    xv_set(xv_get(color_menu, MENU_NTH_ITEM, i), 
		   MENU_INACTIVE, FALSE, NULL);
	}
      else 
	{
	  xv_set(black_white_toggle, XV_SHOW, FALSE, 0);
	  xv_set(base_window->palette_bt, XV_SHOW, TRUE, 0);
	}
            
      /* Set toggle to color */
      
      xv_set(base_window->color_mono_setting, PANEL_VALUE, 0, NULL);
      
      /* Set save option to only Color X Pixmap */
      
      edit_menu = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 8), MENU_INACTIVE, TRUE, NULL);
      
      if ( save_format == 4 )
	save_format = 3;
    }
  else
    {
      xv_set(base_window->color_mono_setting, PANEL_VALUE, 1, NULL);
      current_color_state = MONO;
    }
}

void
  grid_proc(desired_state)
     int desired_state;
{
  
  if (desired_state == GRID_ON)
    {
      XDrawSegments(dpy, big_bit_xid, grid_gc, 
		    x_segment_list, number_of_vert_lines);
      XDrawSegments(dpy, big_pixmap, grid_gc, 
		    x_segment_list, number_of_vert_lines);
      XDrawSegments(dpy, big_bit_xid, grid_gc, 
		    y_segment_list, number_of_vert_lines);
      XDrawSegments(dpy, big_pixmap, grid_gc, 
		    y_segment_list, number_of_vert_lines);
    }
  else
    {
      XDrawSegments(dpy, big_bit_xid, gc_rv, 
		    x_segment_list, number_of_horz_lines);
      XDrawSegments(dpy, big_pixmap, gc_rv, 
		    x_segment_list, number_of_horz_lines);
      XDrawSegments(dpy, big_bit_xid, gc_rv, 
		    y_segment_list, number_of_horz_lines);
      XDrawSegments(dpy, big_pixmap, gc_rv, 
		    y_segment_list, number_of_horz_lines);
    }
}

void 
  initialize_grid_lines()
{
  int i;
  int count;
  
  count = icon_width;
  
  for (i = 0; i < count+1 ; i++)
    {
      x_segment_list[i].x1 = boarder_left + ( i * 4 * width_block);
      x_segment_list[i].x2 = boarder_left + ( i * 4 * width_block);
      
      x_segment_list[i].y1 = boarder_upper;
      x_segment_list[i].y2 = (canvas_dimension - boarder_lower) -1;
    }
  number_of_vert_lines = i;
  
  count = icon_height; 
  
  for (i = 0; i < count+1; i = i + 4)
    {
      y_segment_list[i].x1 = boarder_left;
      y_segment_list[i].x2 = (canvas_dimension - boarder_right) -1 ;
      
      y_segment_list[i].y1 = boarder_upper + ( i * height_block);
      y_segment_list[i].y2 = boarder_upper + ( i * height_block);
    }
  number_of_horz_lines = i;
  
}

void
  change_icon_size(new_width, new_height)
int new_width;
int new_height;
{
  
  
  Pixmap holding_pixmap;
  XRectangle big_rect[1], preview_rect[1], select_rect[1], moving_rect[1];
  Menu edit_menu;
  
  xv_set(base_window->window, FRAME_BUSY, TRUE, NULL);

  holding_pixmap = XCreatePixmap(dpy, preview_xid, 128, 128, screen_depth);

  XFillRectangle( dpy, holding_pixmap, gc_rv, 0, 0, 129, 129);

  XPutImage(dpy, holding_pixmap, gc, 
	    undo_images[current_undo%MAX_UNDOS].image,
	    0, 0, 
	    0, 0, 
	    128, 128);
  
  undo_images[current_undo%MAX_UNDOS].state = current_color_state;
  undo_images[current_undo%MAX_UNDOS].height = icon_height;
  undo_images[current_undo%MAX_UNDOS].width = icon_width;
  
  if (undos_allowed < (MAX_UNDOS -1))
    undos_allowed++;
  
  Edited++;
  current_undo++;
  undo_images[current_undo%MAX_UNDOS].edited++;
  
  if (undos_allowed == 1)
    {
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), MENU_INACTIVE, FALSE, NULL);
    }
  
  if (redo_possible == TRUE)
    {
      redo_possible = FALSE;
      redos_allowed = 0;
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), MENU_INACTIVE, TRUE, NULL);
    }
  
  undo_images[current_undo%MAX_UNDOS].image =  
    XGetImage( dpy, holding_pixmap, 0, 0, 128, 128, -1, ZPixmap);

  icon_width = new_width;
  icon_height = new_height;

  set_iconedit_footer();

  XFillRectangle( dpy, preview_pixmap, gc_rv, 
		 0 , 0, 
		 138, 138);
  XFillRectangle( dpy, big_pixmap, gc_rv, 
		 0, 0, 
		 canvas_dimension, canvas_dimension); 
  XFillRectangle( dpy, preview_xid, gc_rv, 
		 0 , 0, 
		 138, 138);
  XFillRectangle( dpy, big_bit_xid, gc_rv, 
		 0, 0, 
		 canvas_dimension, canvas_dimension); 
  
  set_the_size_info();
  
  reset_those_pesky_clip_masks();
  
  set_select_cliprects();
  initialize_grid_lines();
  grid_proc(grid_status);
  selection_off();
  
  do_that_base_repaint_thing(undo_images[current_undo%MAX_UNDOS].image);
  
  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
}

void
  reset_those_pesky_clip_masks()
{
  
  XRectangle big_rect[1], preview_rect[1], select_rect[1], moving_rect[1];
  
  big_rect[0].x = boarder_left; 
  big_rect[0].y = boarder_upper; 
  big_rect[0].width = canvas_dimension -( boarder_left + boarder_right); 
  big_rect[0].height = canvas_dimension - ( boarder_upper + boarder_lower);
  
  select_rect[0].x = boarder_left; select_rect[0].y = boarder_upper; 
  select_rect[0].width = canvas_dimension -( boarder_left + boarder_right); 
  select_rect[0].height = canvas_dimension - ( boarder_upper + boarder_lower);
  
  preview_rect[0].x = preview_boarder_left; 
  preview_rect[0].y = preview_boarder_upper; 
  preview_rect[0].width = icon_width; 
  preview_rect[0].height = icon_height;

  moving_rect[0].x = boarder_left +1; 
  moving_rect[0].y = boarder_upper + 1; 
  moving_rect[0].width = canvas_dimension -( boarder_left + boarder_right +2); 
  moving_rect[0].height = canvas_dimension -( boarder_upper+boarder_lower +2);
  
  XSetClipRectangles( dpy, big_gc, 0, 0, big_rect, 1, YSorted);
  XSetClipRectangles( dpy, preview_gc, 0, 0, preview_rect, 1, YSorted);
  XSetClipRectangles( dpy, select_gc, 0, 0, select_rect, 1, YSorted);
  XSetClipRectangles( dpy, preview_select_gc, 0, 0, preview_rect, 1, YSorted);
  XSetClipRectangles( dpy, moving_gc, 0, 0, moving_rect, 1, YSorted);
  XSetClipRectangles( dpy, preview_moving_gc, 0, 0, preview_rect, 1, YSorted);
  XSetClipRectangles( dpy, fill_gc, 0, 0, preview_rect, 1, YSorted); 
  XSetClipRectangles( dpy, three_gc, 0, 0, big_rect, 1, YSorted); 
  
}

void 
  call_up_that_color_chooser( item, event )
Panel_item  item;
Event      *event;
{
  if ( tt_running )
    {
      /* Beautiful */
      iconedit_tt_get_color( item, event );
    }
  else
    {
      /* !Beautiful */
      xv_set(base_window->window, 
	     FRAME_LEFT_FOOTER, gettext("Color Chooser not responding"), 
	     NULL);		
      xv_set( base_window->palette_bt, 
	     PANEL_ITEM_MENU, color_menu,
	     NULL);
    }
}

void
  invert_area()
{
  int x, y, w, h, a, b;
  int corner_x, corner_y;
  int preview_x, preview_y, preview_w, preview_h;
  int invert_all;
  long int pixel;
  
  Menu edit_menu;
  
  xv_set(base_window->window, FRAME_BUSY, TRUE, 0);
  
  XDrawRectangle(dpy, big_bit_xid, select_gc, selection_left, 
		 selection_top, selection_right - selection_left, 
		 selection_bottom - selection_top); 
  
  if (((selection_left == 0) && (selection_right == 0)) 
      && ((selection_top == 0) && (selection_bottom == 0)))
    {
      invert_all = TRUE;
      selection_left = boarder_left; 
      selection_top = boarder_upper; 
      selection_right = (canvas_dimension - boarder_right); 
      selection_bottom = (canvas_dimension - boarder_lower);
    }
  else
    invert_all = FALSE;
  
  x = selection_left < selection_right 
    ? selection_left
      : selection_right;
  
  y = selection_top < selection_bottom 
    ? selection_top  
      : selection_bottom;
  
  w = selection_left < selection_right 
    ? selection_right - selection_left 
      : selection_left - selection_right; 
  
  h = selection_top < selection_bottom 
    ? selection_bottom - selection_top 
      : selection_top - selection_bottom;
  
  preview_x = ( x - boarder_left )/width_block; 
  preview_y = ( y - boarder_upper )/height_block; 
  preview_w = w/width_block; 
  preview_h = h/height_block;
  
  edit_image = XGetImage(dpy, preview_pixmap, 
			 preview_x + preview_boarder_left, 
			 preview_y + preview_boarder_upper,
			 preview_w, preview_h, -1, ZPixmap); 
  
  XSetForeground(dpy, redraw_gc, black.pixel);
  
  for (a = 0; a < preview_w; a++)
    for (b = 0; b < preview_h; b++)
      {
	corner_x = x + (a * width_block);		    
	corner_y = y + (b * height_block);
	pixel = XGetPixel(edit_image, a, b);
	if ( pixel == white.pixel )
	  {	      
	    XFillRectangle(dpy, big_bit_xid, redraw_gc, 
			   corner_x+1, 
			   corner_y+1,
			   width_block-1, height_block-1);	    
	    XFillRectangle(dpy, big_pixmap, redraw_gc, 
			   corner_x+1, 
			   corner_y+1,
			   width_block-1, height_block-1);	    
	    XDrawPoint(dpy, preview_pixmap, redraw_gc, 
		       a + preview_x + preview_boarder_left, 
		       b + preview_y + preview_boarder_upper);
	    XDrawPoint(dpy, preview_xid, redraw_gc, 
		       a + preview_x + preview_boarder_left, 
		       b + preview_y + preview_boarder_upper);
	  }
	else
	  {	      
	    XFillRectangle(dpy, big_bit_xid, gc_rv, 
			   corner_x+1, 
			   corner_y+1,
			   width_block-1, height_block-1);	    
	    XFillRectangle(dpy, big_pixmap, gc_rv, 
			   corner_x+1, 
			   corner_y+1,
			   width_block-1, height_block-1);	    
	    XDrawPoint(dpy, preview_pixmap, gc_rv, 
		       a + preview_x + preview_boarder_left, 
		       b + preview_y + preview_boarder_upper);
	    XDrawPoint(dpy, preview_xid, gc_rv, 
		       a + preview_x + preview_boarder_left, 
		       b + preview_y + preview_boarder_upper);
	  }
      }
  
  
  undo_images[current_undo%MAX_UNDOS].state = current_color_state;
  undo_images[current_undo%MAX_UNDOS].height = icon_height;
  undo_images[current_undo%MAX_UNDOS].width = icon_width;
  current_undo++;
  if (undos_allowed < (MAX_UNDOS -1))
    undos_allowed++;
  
  Edited++;
  undo_images[current_undo%MAX_UNDOS].edited++;
  
  if (undos_allowed == 1)
    {
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
	     MENU_INACTIVE, FALSE, NULL);
    }
  
  undo_images[current_undo%MAX_UNDOS].image = 
    XGetImage(dpy, preview_pixmap, 
	      preview_boarder_left, preview_boarder_upper, 
	      icon_width, icon_height, -1, ZPixmap) ;
  
  grid_proc(grid_status);
  
  if (redo_possible == TRUE)
    {
      redo_possible = FALSE;
      redos_allowed = 0;
      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 3), 
	     MENU_INACTIVE, TRUE, NULL);
    }
  if (invert_all)
    {
      selection_left = 0; 
      selection_top = 0; 
      selection_right = 0; 
      selection_bottom = 0;
    }
  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
  
  set_iconedit_footer();
}

