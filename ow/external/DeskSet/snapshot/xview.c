
#ifndef lint
static char sccsid[] = "@(#)xview.c	3.27 11/26/96 Copyright 1987-1990 Sun Microsystem, Inc." ;
#endif

/*  Snapshot - graphics dependent routines.
 *
 *  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

#include <sys/stat.h>
#include <unistd.h>
#include <xview/file_chsr.h>
#include "snapshot.h"
#include "ds_popup.h"
#include "dstt.h"
#include "xdefs.h"


static unsigned short icon_data[] = {
#include "snapshot.icon"
} ;

static unsigned short icon_mask_image[] = {
#include "snapshot.mask.icon"
} ;

static unsigned short source_drag_image[] = {
#include "dupedoc_drag.icon"
} ;

static unsigned short source_drop_image[] = {
#include "dupedoc_drop.icon"
} ;

static unsigned short confirm_data[16] = {
  0x1FF8, 0x3FFC, 0x336C, 0x336C, 0x336C, 0x336C, 0x336C, 0x336C,
  0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FC4, 0x3FFC, 0x1FF8
} ;

static unsigned short select_data[16] = {
  0x2000, 0x7000, 0xF800, 0x7C01, 0x3E01, 0x1F03, 0x0F83, 0x07C3,
  0x03E3, 0x01F3, 0x00FF, 0x007F, 0x003F, 0x003F, 0x07FF, 0x1FFF
} ;

static unsigned short blank_data[16] = {
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
} ;

XVars X ;
Panel_item load_button;
Panel_item save_button;
Panel_item print_button;

extern Vars v ;
extern DNDVars DND ;
extern char *hstr[] ;
extern char *mstr[] ;
extern char *mpstrs[] ;
extern char *ostr[] ;
extern char *Pstrs[] ;

extern int computegrasp P((Rect *, int, int)) ;
extern int load_from_dragdrop P((Xv_server, Event *)) ;

extern void dofeedback P((int, int, Rect *, int *)) ;

extern int SNAPSHOT_DB_HW ; /* Workaround flag for ESC 502957, BUGID 1222996 */

static void create_viewwin P(()) ;
static void delay_choice P((Panel_item, int, Event *)) ;
static void do_snap_proc P((Panel_item, Event *)) ;
static void draw_view_canvas P((Canvas, Xv_Window, Display *,
                                Xv_Window, Xv_xrectlist *)) ;
#ifdef FILECHOOSER
static int  load_callback P((File_chooser, char *, char *, Xv_opaque));
static int  save_callback P((File_chooser, char *, struct stat *));
#else
static void load_save_snap P((Panel_item, Event *)) ;
#endif
static void load_snap_proc P((Panel_item, Event *)) ;
#ifdef FILECHOOSER
static void make_open_filechooser P(()) ;
static void make_save_filechooser P(()) ;
#else
static void make_ls_panel P(()) ;
#endif
static void make_print_panel P(()) ;
static void print_options P((Panel_item, Event *)) ;
static void print_proc P((Panel_item, Event *)) ;
static void save_snap_proc P((Panel_item, Event *)) ;
static void set_hide_choice P((Panel_item, int, Event *)) ;
static void snap_scaleto_proc P((Panel_item, int, Event *)) ;
static void snap_destination_proc P((Panel_item, int, Event *)) ;
static void specify_position_proc P((Panel_item, int, Event *)) ;
static void view_snap_proc P((Panel_item, Event *)) ;
static void xerror_interpose P((Display *, XErrorEvent *)) ;

static Notify_value canvas_resize P((Canvas, int, int)) ;
static Notify_value time_delay P(()) ;

static Menu create_print_pulldown P(()) ;
static Panel_setting dir_proc P((Panel_item, Event *)) ;
static XRectangle compress_repaint P((Xv_Window, Xv_xrectlist *)) ;


void
activate_item(pitype, val)   /* [In]activate a panel item. */
enum pi_type pitype ;
int val ;
{
  XV_SET(X->pitems[(int) pitype], PANEL_INACTIVE, !val, 0) ;
}


/*  Add the new colors to the canvas colormap, and adjust the image data
 *  to match the colormap.
 */

void
add_colors(image)
image_t *image ;
{
  Xv_singlecolor *colors ;
  unsigned char *iptr ;
  unsigned long *pixel_table ;
  int c, x, y ;

  iptr = image->data ;
  colors = (Xv_singlecolor *)
    LINT_CAST(malloc((unsigned) (sizeof(Xv_singlecolor) * image->cmapused))) ;
 
/* Transfer colormap data to Xv_singlecolor array */

  for (c = 0; c < image->cmapused; c++)
    {
      colors[c].red   = image->red[c] ;
      colors[c].green = image->green[c] ;
      colors[c].blue  = image->blue[c] ;
    }
 
  create_viewwin() ;            /* Create viewing window canvas. */

  /* Create the cms from the colors */
  X->cms = (Cms) xv_create(xv_get(X->canvas, XV_SCREEN), CMS,
                        CMS_SIZE,                  image->cmapused,
                        CMS_COLORS,                colors,
                        0) ;

  if (!X->cms)
    { 
      FPRINTF(stderr, "Unable to create colormap, aborting\n") ;
      exit(-1) ;
    }
 
  pixel_table = (unsigned long *) xv_get(X->cms, CMS_INDEX_TABLE) ;
 
/* Adjust image data to the new pixels */

  for (y = 0; y < image->height; y++)
    {
      for (x = 0; x < image->width; x++)
        {
          *iptr = pixel_table[(unsigned char) *iptr] ;
          iptr++ ;
        }
      iptr += (image->bytes_per_line - image->width);
    }
 
/* Set the cms on the canvas */

  XV_SET(X->canvas, WIN_CMS, X->cms, NULL) ;

  X->cmap = xv_get (xv_get (X->canvas, WIN_CMS), CMS_CMAP_ID);
}


/*ARGSUSED*/
static Notify_value
canvas_resize(canvas, width, height)
Canvas canvas ;
int width, height ;
{

/*  The resize handler is only here to compress the resize events. If you
 *  don't have a resize handler, it seems you get multiple resize events in
 *  the interposed event handler.
 */

  return(NOTIFY_DONE) ;
}


static XRectangle
compress_repaint(xid, xrects)
Xv_Window xid ;
Xv_xrectlist *xrects ;
{
  XEvent ev ;
  XRectangle *xr = xrects->rect_array ;
  XRectangle clip ;

  int i ;
  int minx = xr->x ;
  int miny = xr->y ;
  int maxx = minx + xr->width ;
  int maxy = miny + xr->height ;

/* Look through this expose event building the bbox */

  for (i = 1, xr++; i < xrects->count; i++, xr++)
    {
      if (xr->x < minx)                      minx = xr->x ;
      if (xr->y < miny)                      miny = xr->y ;
      if ((int) (xr->x + xr->width) > maxx)  maxx = xr->x + xr->width ;
      if ((int) (xr->y + xr->height) > maxy) maxy = xr->y + xr->height ;
    }

  XSync(X->dpy, 0) ;

/* Look through pending expose events building the bbox */

  while (XPending(X->dpy) && (XPeekEvent(X->dpy, &ev),
         (ev.type == Expose && ev.xany.window == xid)))
    {
      XNextEvent(X->dpy, &ev) ;
      if (ev.xexpose.x < minx) minx = ev.xexpose.x ;
      if (ev.xexpose.y < miny) miny = ev.xexpose.y ;
      if ((int) (ev.xexpose.x + ev.xexpose.width) > maxx)
        maxx = ev.xexpose.x + ev.xexpose.width ;
      if ((int) (ev.xexpose.y + ev.xexpose.height) > maxy)
        maxy = ev.xexpose.y + ev.xexpose.height ;
    }

/* Confine drawing to the extent of the damage */

  clip.x      = minx ;
  clip.y      = miny ;
  clip.width  = maxx - minx ;
  clip.height = maxy - miny ;
  return(clip) ;
}


int
confirm(s)        /* Request confirmation from the user.  */
char *s ;
{
  Event ie ;
  int status ;
 
  status = notice_prompt(X->frames[(int) F_MAIN], &ie,
                         NOTICE_MESSAGE_STRINGS, s, 0,
                         NOTICE_BUTTON_YES,      ostr[(int) O_CONTINUE],
                         NOTICE_BUTTON_NO,       ostr[(int) O_CANCEL],
                         0) ;
  return((status == NOTICE_YES) ? TRUE : FALSE) ;
}


void
copyrect()
{
  Fullscreen view_fullscreen ;
  Display *display ;
  XColor xcolors[256] ;
  register int i ;
  int multiVis;
  int get_image = TRUE;
  image_t *image;
  struct rasterfile rh_tmp;
  int current_depth;

/*
 * Initialize padding info...
 */

  init_pad ();

/*  The following method of capture should really only be used
 *  in timed dumps. otherwise, do the copy under fullscreen locks
 */

  view_fullscreen = xv_create(0, FULLSCREEN,
        FULLSCREEN_GRAB_SERVER, TRUE,
        FULLSCREEN_GRAB_POINTER, FALSE,
        FULLSCREEN_GRAB_KEYBOARD, FALSE,
        0) ;

  if (X->ximage != NULL) {
     if (v->image != NULL) {
        if ((unsigned char *) X->ximage->data == v->image->data) {
	   ck_zfree((char *) v->image->data, v->image->used_malloc);
           v->image->data = (unsigned char *) NULL;
	   X->ximage->data = (char *) NULL;
	   }
	else {
	   ck_zfree((char *) X->ximage->data, FALSE);
	   X->ximage->data = (char *) NULL;
	   }
	}
     else {
	ck_zfree((char *) X->ximage->data, FALSE);
	X->ximage->data = (char *) NULL;
	}
     XDestroyImage(X->ximage) ;
     }
 
  if (v->num_vis > 1) {
 
/*
 * Reset multivisual variables.
 */
 
     mvReset ();
 
/*
 * Get a list of intersecting windows, with their visual details.
 */
 
     mvWalkTree(X->root, 0, 0, X->rect[(int) R_BOX].r_left,
                               X->rect[(int) R_BOX].r_top,
                               X->rect[(int) R_BOX].r_width,
                               X->rect[(int) R_BOX].r_height);
      
 
/*
 * Check if we have a multiVis on our hands.  If the SNAPSHOT_DB_HW
 * environment variable is set then enter the multivis code regardless.
 * This is to force snapshot to do an XGetImage() on each window instead
 * of the root.  This gets around a problem with hardware double-buffering
 * (ESC 502957, BUGID 1222996).
 */

    
     multiVis = IsMultiVis(mvIsMultiVis());
     if (multiVis || SNAPSHOT_DB_HW) {
 
/*
 * Create an Image where mvDoWindows.. will operate.
 */
 
        if (!mvCreatImg( X->rect[(int) R_BOX].r_width,
                         X->rect[(int) R_BOX].r_height,
                         X->rect[(int) R_BOX].r_left,
                         X->rect[(int) R_BOX].r_top)) {
           fprintf(stderr, "Snapshot: mvCreatImg failed\n");
           exit(1);
           }
        mvDoWindowsFrontToBack();
 
/*
 * Convert the map into an XImage.
 */
        image = CreateImageFromImg ();
        X->ximage = gen_image (image);
        free_image (image);
        get_image = FALSE;
        XV_DESTROY(view_fullscreen) ;
        XSync(X->dpy, FALSE) ;
        }

 
/*
 * We don't have a multivisual problem on our hands. Although we
 * have a system that supports multiple visuals, the  region/screen/windows
 * snapped only uses one of the visuals, so we can use XGetImage to get
 * the pixels values for that region.
 */
 
     else {
        current_depth = mvDepth ();
        X->cmap = mvColormap ();
        }
 
     }    
 
/*
 * Our system only supports one visual, so we can use XGetImage.
 */
 
  else  {
     current_depth = v->depth;
 
/*
 * This is getting the colormap of the base frame... we can't use
 * DefaultColormap now.
 */
 
     X->cmap = xv_get (xv_get (X->frames [(int) F_MAIN], WIN_CMS),
                                   CMS_CMAP_ID);
 
     }
 
  if (get_image == TRUE) {
 
     X->ximage = (XImage *) XGetImage(X->dpy, X->rxid,
                                   X->rect[(int) R_BOX].r_left,
                                   X->rect[(int) R_BOX].r_top,
                                   X->rect[(int) R_BOX].r_width,
                                   X->rect[(int) R_BOX].r_height, AllPlanes,
                                   (current_depth == 1) ? XYPixmap : ZPixmap) ;
 
     XV_DESTROY(view_fullscreen) ;
     XSync(X->dpy, FALSE) ;
  
     set_current_pad (X->ximage->depth, X->ximage->bitmap_pad);
     if (X->ximage->depth == 4)
        set_current_pad (8, X->ximage->bitmap_pad);
 
     image                 = new_image() ;
     image->width          = X->rect[(int) R_BOX].r_width ;
     image->height         = X->rect[(int) R_BOX].r_height ;
 
/*
 * Image depth should be set to depth returned from XGetImage, not the
 * depth of the window (may be different) - this is where we need to
 * add support for images that cross visual boundaries.
 */

     image->depth          = X->ximage->depth ;
     image->bytes_per_line = X->ximage->bytes_per_line ;
     image->data = (unsigned char *) ck_zmalloc (image->height *
						 image->bytes_per_line);
     MEMCPY ((char *) image->data, (char *) X->ximage->data,
			image->height * image->bytes_per_line);
     free (X->ximage->data);
     X->ximage->data = (char *) image->data;
 
/*
 * If the depth is 4 or 8, then we need to get pixel values from
 * the colormap. If it's 24, then we have the rgb values.
 */
 
       if ( (image->depth == 4) || (image->depth == 8) )
       {
         int cmap_size = CMAP_SIZE;
 
         if (image->depth == 4)
            cmap_size = 16;
	 else if (image->depth == 8)
	    cmap_size = X->visual->map_entries;

         display = (Display *) xv_get(X->frames[(int) F_ROOT], XV_DISPLAY) ;
 
         for (i = 0; i <= cmap_size - 1; i++) xcolors[i].pixel = i ;
 
         XQueryColors(display, X->cmap, xcolors, cmap_size) ;

         image->red   = (unsigned char *) malloc(cmap_size) ;
         image->green = (unsigned char *) malloc(cmap_size) ;
         image->blue  = (unsigned char *) malloc(cmap_size) ;
         for (i = 0; i <= cmap_size - 1; i++)
           {
             image->red[i]   = xcolors[i].red   >> 8 ;
             image->green[i] = xcolors[i].green >> 8 ;
             image->blue[i]  = xcolors[i].blue  >> 8 ;
           }

         image->cmaptype = RMT_EQUAL_RGB ;
         image->cmapused = cmap_size ;
	 if (image->depth == v->depth)
	    add_colors (image);
       }
 
     if (image->depth != v->depth)  {
        int length;
 
        length = image->bytes_per_line * image->height;
        image->data = (unsigned char *) ck_zmalloc ((size_t) length);
        MEMCPY ((char *) image->data, (char *) X->ximage->data, length);
        if (image->depth >= 24) {
           rh_tmp.ras_magic     = RAS_MAGIC;
           rh_tmp.ras_width     = image->width ;
           rh_tmp.ras_height    = image->height ;
           rh_tmp.ras_depth     = 32 ;
           rh_tmp.ras_type      = RT_STANDARD ;
           rh_tmp.ras_length    = image->bytes_per_line * image->height;
           rh_tmp.ras_maptype   = RMT_NONE ;
           rh_tmp.ras_maplength = 0 ;
 
           adjust_image (&rh_tmp, image->data, rh_tmp.ras_length);
	   image->depth = 32;
           }
	if (X->ximage != NULL) {
	   if (v->image != NULL) {
	      if ((unsigned char *) X->ximage->data == v->image->data) {
	         ck_zfree((char *) v->image->data, v->image->used_malloc);
	         v->image->data = (unsigned char *) NULL;
	         X->ximage->data = (char *) NULL;
	         }
	      else {
	         ck_zfree((char *) X->ximage->data, FALSE);
	         X->ximage->data = (char *) NULL;
	         }
	      }
	   else {
	      ck_zfree((char *) X->ximage->data, FALSE);
	      X->ximage->data = (char *) NULL;
	      }
	   XDestroyImage (X->ximage);
	   }
        X->ximage = gen_image (image);
        free_image (image);
        }
 
     else {
        free_image(v->image) ;
        v->image = image;
        }
     }
 
  message(mstr[(int) M_SNAPSUCS]) ;
 
  if (get_int_val(PI_HIDEWIN)) frame_show(F_ALL, TRUE) ;
  v->havename = FALSE ;
  set_namestripe ();
  v->ls_status = ISSNAP ;

  if (v->ttid != (char *) NULL)
     viewfile ();
  else if ((X->frames[(int) F_VIEW] != NULL) &&
      	   (xv_get(X->frames[(int) F_VIEW], XV_SHOW) == TRUE)) 
     old_viewfile() ;

/*
 * We got an image.. let the user drag it out now if he wants to...
 */

  xv_set (X->pitems [(int) PI_DRAG], PANEL_DROP_FULL, TRUE, NULL);

/*
**	Perform gamma correction if required.
**	Default gamma value is 1.0
**	which results in no change to the image.
*/
	if (X->ximage->depth == 24) {
		XImage *xim = X->ximage;
		int i, j;
		long pixel;
		int r, g, b;

		for (j = 0; j < xim->height; j++) {
			for (i = 0; i < xim->width; i++) {
				pixel = xim->f.get_pixel(xim, i, j);
				r = pixel & 0xff;
				g = (pixel >> 8) & 0xff;
				b = (pixel >> 16) & 0xff;
				pixel = v->gamma_tbl[r] + (v->gamma_tbl[g] << 8)
										 + (v->gamma_tbl[b] << 16);
				xim->f.put_pixel(xim, i, j, pixel);
			}
		}
	}
}


static Menu
create_print_pulldown()
{
  Menu menu ;

  menu = xv_create(XV_NULL,             MENU,
                   MENU_GEN_PIN_WINDOW,
                     X->frames[(int) F_MAIN], ostr[(int) O_POPT],
                   MENU_ITEM,
                     MENU_STRING,       ostr[(int) O_PMITEM],
                     MENU_NOTIFY_PROC,  print_proc,
#ifdef KYBDACC
                        MENU_ACCELERATOR, "coreset Print",
#endif
                     XV_HELP_DATA,      hstr[(int) H_PRINT],
                     0,
                   MENU_ITEM,
                     MENU_STRING,       ostr[(int) O_OMITEM],
                     MENU_ACTION_PROC,  print_options,
                     XV_HELP_DATA,      hstr[(int) H_MENUOPT],
                     0,
                   0) ;
  return(menu) ;
}


static void
create_viewwin()
{
  Canvas_paint_window cpw ;

  if (X->frames[(int) F_VIEW] != NULL) return ;

  X->frames[(int) F_VIEW] = xv_create(X->frames[(int) F_MAIN], FRAME_CMD,
                                      FRAME_LABEL,     ostr[(int) O_VIEW],
                                      FRAME_DONE_PROC,          frame_done,
                                      FRAME_CMD_PUSHPIN_IN,     TRUE,
                                      FRAME_SHOW_RESIZE_CORNER, TRUE,
                                      FRAME_SHOW_FOOTER,        FALSE,
                                      XV_SHOW,                  FALSE,
                                      0) ;

  X->canvas = xv_create(X->frames[(int) F_VIEW], CANVAS,
                        XV_X,                    0,
                        XV_Y,                    0,
                        XV_VISUAL,		 X->visual,
			WIN_DEPTH,		 v->depth,
                        CANVAS_AUTO_CLEAR,       TRUE,
                        CANVAS_X_PAINT_WINDOW,   TRUE,
                        CANVAS_REPAINT_PROC,     draw_view_canvas,
                        CANVAS_RESIZE_PROC,      canvas_resize,
                        CANVAS_RETAINED,         FALSE,
                        0) ;
 
  cpw = canvas_paint_window(X->canvas) ;
  X->vxid = (Drawable) xv_get(cpw, XV_XID) ;

  X->gc_mask = GCForeground | GCBackground ;
  X->gc_val.foreground = X->foregnd ;
  X->gc_val.background = X->backgnd ;
  X->viewgc = XCreateGC(X->dpy, X->vxid, X->gc_mask, &X->gc_val) ;

  XV_SET(cpw, WIN_BIT_GRAVITY, ForgetGravity, 0) ;
}


/*ARGSUSED*/
static void
delay_choice(item, value, event)
Panel_item item ;
int value ;
Event *event ;
{
  int dval ;

  if ((int) xv_get(X->pitems[(int) PI_BELL], PANEL_INACTIVE) != TRUE)
    v->timer_bell_on = get_int_val(PI_BELL) ;

  dval = get_int_val(PI_DELAY) ;
  if (v->hide && dval < 3)
    {

/* Force the delay to be 8 sec. if application is to be hidden. */

      dval = 3 ;
      XV_SET(X->pitems[(int) PI_DELAY], PANEL_VALUE, dval, 0) ;
      message(mstr[(int) M_DELAY]) ;
    }

  if (dval == 0)
    {
      v->snapshot_delay = 0 ;
      XV_SET(X->pitems[(int) PI_BELL],
             PANEL_VALUE, 0,
             PANEL_INACTIVE, TRUE,
             0) ;
    }
  else
    { 
      v->snapshot_delay = (1 << dval) ;
      XV_SET(X->pitems[(int) PI_BELL],
             PANEL_VALUE, v->timer_bell_on,
             PANEL_INACTIVE, FALSE,
             0) ;
    }
}


void
destroy_frame(ftype)
enum frame_type ftype ;
{
  XV_DESTROY(X->frames[(int) ftype]) ;
}


static Panel_setting
dir_proc(item, event)
Panel_item item ;
Event *event ;
{
  int id = event_id(event) ;
   
  if (id == '\r' || id == '\t' || id == '\n') CHANGE_DIR() ;
  return(panel_text_notify(item, event)) ;
}


void
disable_timer()
{
  notify_set_itimer_func((Notify_client) &X->handle,
                         time_delay, ITIMER_REAL, NULL, 0) ;
}


int
do_overwrite(otype, current_op)    /* Display notice with overwrite message. */
enum ostr_type otype ;
int current_op;
{
  int status = 0 ;
  char no_string [1024];

  switch (current_op) {
    case ISSNAP:  strcpy (no_string, ostr[(int) O_SCANCEL]);
		  break;
    case ISLOAD:  strcpy (no_string, ostr[(int) O_LCANCEL]);
		  break;
    case ISDRAG:  strcpy (no_string, ostr[(int) O_DCANCEL]);
		  break;
    case ISQUIT:  strcpy (no_string, ostr[(int) O_QCANCEL]);
    default:	  break;
    }

  if (otype == O_FOVERWRITE)
    {
      status = notice_prompt(X->frames[(int) F_MAIN], NULL,
                             NOTICE_MESSAGE_STRINGS,
                               ostr[(int) otype],
                               0,
                             NOTICE_BUTTON_YES, ostr[(int) O_YES],
                             NOTICE_BUTTON_NO,  ostr[(int) O_NO],
                             0) ;
      if (status == NOTICE_YES) return(TRUE) ;
    }
  else if (otype == O_LOVERWRITE)
    {
      if (current_op == ISQUIT)
         status = notice_prompt(X->frames[(int) F_MAIN], NULL,
                             NOTICE_MESSAGE_STRINGS,
                               ostr[(int) otype],
                               0,
                             NOTICE_BUTTON_YES, ostr[(int) O_QLDISCARD],
                             NOTICE_BUTTON_NO,  no_string,
                             0) ;
      else
         status = notice_prompt(X->frames[(int) F_MAIN], NULL,
                             NOTICE_MESSAGE_STRINGS,
                               ostr[(int) otype],
                               0,
                             NOTICE_BUTTON_YES, ostr[(int) O_LDISCARD],
                             NOTICE_BUTTON_NO,  no_string,
                             0) ;
      if (status == NOTICE_YES) return(TRUE) ;
      if (status == NOTICE_NO)
        XV_SET(X->frames[(int) F_MAIN], FRAME_LEFT_FOOTER, "", 0) ;
    }
  else if (otype == O_SOVERWRITE)
    {
      if (current_op == ISQUIT)
         status = notice_prompt(X->frames[(int) F_MAIN], NULL,
                             NOTICE_MESSAGE_STRINGS,
                               ostr[(int) otype],
                               0,  
                             NOTICE_BUTTON_YES, ostr[(int) O_QSDISCARD],
                             NOTICE_BUTTON_NO,  no_string,
			     0);
      else
         status = notice_prompt(X->frames[(int) F_MAIN], NULL,
                             NOTICE_MESSAGE_STRINGS,
                               ostr[(int) otype],
                               0,  
                             NOTICE_BUTTON_YES, ostr[(int) O_SDISCARD],
                             NOTICE_BUTTON_NO,  no_string,
                             0) ;
      if (status == NOTICE_YES) return(TRUE) ;
      if (status == NOTICE_NO)
        XV_SET(X->frames[(int) F_MAIN], FRAME_LEFT_FOOTER, "", 0) ;
    }

  if (status == NOTICE_FAILED)
    {
      XV_SET(X->frames[(int) F_MAIN], FRAME_LEFT_FOOTER,
             ostr[(int) O_ALERT1],
             0) ;
      FPRINTF(stderr, ostr[(int) O_ALERT2]) ;
      FPRINTF(stderr, ostr[(int) O_ALERT3]) ;
      FPRINTF(stderr, ostr[(int) O_ALERT4]) ;
      FPRINTF(stderr, ostr[(int) O_ALERT5]) ;
    }
  return(FALSE) ;
}


/*ARGSUSED*/
static void
do_snap_proc(item, event)
Panel_item item ;
Event *event ;
{
  int value ;

  XV_SET(X->pitems[(int) PI_VIEW], PANEL_INACTIVE, TRUE, 0) ;
  value = (int) xv_get(X->pitems[(int) PI_STYPE], PANEL_VALUE) ;
  if (!get_int_val(PI_HIDEWIN)) message(mstr[(int) M_TAKING]) ;
  switch (value)
    {
      case DUMP_WINDOW : do_window_dump() ;
                         break ;
      case DUMP_RECT   : do_rect_dump() ;
                         break ;
      case DUMP_SCREEN : do_screen_dump() ;
    }
  v->filetype = RASTERFILE;
  XV_SET(X->pitems[(int) PI_VIEW], PANEL_INACTIVE, FALSE, 0) ;
}


void
do_sync(ftype)
enum frame_type ftype ;
{
  XV_SET(XV_SERVER_FROM_WINDOW(X->frames[(int) ftype]), SERVER_SYNC, FALSE, 0) ;
}


void
drawbox(r)
Rect *r ;
{
  XDrawLine(X->dpy, X->rxid, X->rootgc,
            r->r_left,     r->r_top,         rect_right(r), r->r_top) ;
  XDrawLine(X->dpy, X->rxid, X->rootgc,
            rect_right(r), r->r_top+1,       rect_right(r), rect_bottom(r)-1) ;
  XDrawLine(X->dpy, X->rxid, X->rootgc,
            rect_right(r), rect_bottom(r),   r->r_left,     rect_bottom(r)) ;
  XDrawLine(X->dpy, X->rxid, X->rootgc,
            r->r_left,     rect_bottom(r)-1, r->r_left,     r->r_top+1) ;
}


/*ARGSUSED*/
static void
draw_view_canvas(canvas, window, display, xid, xrects)
Canvas canvas ;             /* Canvas handle */
Xv_Window window ;          /* Pixwin/Paint window */
Display *display ;          /* Server connection */
Xv_Window xid ;             /* X11 window handle */
Xv_xrectlist *xrects ;      /* Damage rectlist */
{
  XRectangle xr ;
  int h, w ;

  w = (int) xv_get(window, XV_WIDTH) ;
  h = (int) xv_get(window, XV_HEIGHT) ;

  XSetFunction(X->dpy, X->viewgc, GXset) ;
  if (xrects)
    {
      xr = compress_repaint(xid, xrects) ;
      XFillRectangle(X->dpy, X->vxid, X->viewgc,
                     xr.x, xr.y, xr.width, xr.height) ;
    }
  else XFillRectangle(X->dpy, X->vxid, X->viewgc, 0, 0, w, h) ;

  XSetFunction(X->dpy, X->viewgc, GXcopy) ;
  if (xrects)
    {
      w = X->rect[(int) R_BOX].r_width ;
      if (((int) xr.width)  < ((int) X->rect[(int) R_BOX].r_width))
        w = xr.width ;
      h = X->rect[(int) R_BOX].r_height ;
      if (((int) xr.height) < ((int) X->rect[(int) R_BOX].r_height))
        h = xr.height ;
      XPutImage(X->dpy, X->vxid, X->viewgc, X->ximage,
                xr.x, xr.y, xr.x, xr.y, w, h) ;
    }
  else
    XPutImage(X->dpy, X->vxid, X->viewgc, X->ximage, 0, 0, 0, 0,
              X->rect[(int) R_BOX].r_width, X->rect[(int) R_BOX].r_height) ;
}


int
drop_target_notify_proc (item, value, event)
Panel_item	 item;
unsigned int	 value;
Event		*event;
{
  switch (event_action(event))
    {
      case LOC_DRAG :

           SET_FOOTER("") ;
           switch (value)
             {
               case XV_OK       : SET_FOOTER(MGET("Drag and Drop: Began")) ;                                      break ;
 
               case DND_TIMEOUT :
                                  SET_FOOTER(MGET("Drag and Drop: Timed Out")) ;                                  break ;
 
               case DND_ILLEGAL_TARGET :
                             SET_FOOTER(MGET("Drag and Drop: Illegal Target")) ;                             break ;
 
               case DND_SELECTION :
                              SET_FOOTER(MGET("Drag and Drop: Bad Selection")) ;                              break ;
 
               case DND_ROOT    :
                                SET_FOOTER(MGET("Drag and Drop: Root Window")) ;                                break ;
 
               case XV_ERROR    :
                                SET_FOOTER(MGET("Drag and Drop: Failed")) ;
             }

	   break;

      case ACTION_DRAG_COPY :
      case ACTION_DRAG_MOVE :

/*
 * Snapshot should only accept ACTION_DRAG_COPY.. so change
 * the event to this just in case it was ACTION_DRAG_MOVE.
 */

	   event->action = ACTION_DRAG_COPY;

	   if (value != XV_ERROR)
	      if (check_overwrite (ISDRAG) == FALSE)
		 stop_dragdrop ();
	      else
                 load_from_dragdrop(X->server, event) ;
  	   else
             {
               if (v->debug_on) printf ("drop error\n") ;
             }
    }
  return(XV_ERROR) ;
}    

void
frame_done()
{
  XV_SET(X->frames[(int) F_VIEW], XV_SHOW, FALSE, 0) ;
}


void
frame_show(ftype, val)    /* Display or hide given frame. */
enum frame_type ftype ;
int val ;
{ 
  static int popup_show [MAXFRAMES] = {0, 0, 0, 0, 0} ;
  static int popup_pin [MAXFRAMES] = {0, 0, 0, 0, 0} ;

/*  The XV_SHOW operation seems to re-show the main snapshot panel in a
 *  random position. In order to get around this problem, the position of
 *  the snapshot panel is saved when the window is un-shown, and used when
 *  the window is re-shown. Sigh!
 */

  if (ftype == F_ALL)  {
     if (val == TRUE) {
	if (xv_get (X->frames[(int) F_MAIN], XV_SHOW) == FALSE)
           frame_set_rect(X->frames[(int) F_MAIN], &X->rect[(int) R_MAIN]) ;
        if (popup_show[(int) F_PRINT]) {
	   if (popup_pin [(int) F_PRINT]) 
  	      XV_SET (X->frames[(int) F_PRINT], FRAME_CMD_PUSHPIN_IN, TRUE, 0);	
           XV_SET(X->frames[(int) F_PRINT], XV_SHOW, val, 0) ;
	   }
#ifdef FILECHOOSER
        if (popup_show[(int) F_LOAD]) {
	   if (popup_pin [(int) F_LOAD]) 
  	      XV_SET (X->frames[(int) F_LOAD], 
				FRAME_CMD_PUSHPIN_IN, TRUE, 0);	
           XV_SET(X->frames[(int) F_LOAD], XV_SHOW, val, 0) ;
	   }
        if (popup_show[(int) F_SAVE]) {
	   if (popup_pin [(int) F_SAVE]) 
  	      XV_SET (X->frames[(int) F_SAVE], 
				FRAME_CMD_PUSHPIN_IN, TRUE, 0);	
           XV_SET(X->frames[(int) F_SAVE], XV_SHOW, val, 0) ;
	   }
#else
        if (popup_show[(int) F_LOADSAVE]) {
	   if (popup_pin [(int) F_LOADSAVE]) 
  	      XV_SET (X->frames[(int) F_LOADSAVE], 
				FRAME_CMD_PUSHPIN_IN, TRUE, 0);	
           XV_SET(X->frames[(int) F_LOADSAVE], XV_SHOW, val, 0) ;
	   }
#endif
        if (popup_show[(int) F_VIEW]) {
	   if (popup_pin [(int) F_VIEW]) 
  	      XV_SET (X->frames[(int) F_VIEW], FRAME_CMD_PUSHPIN_IN, TRUE, 0);	
           XV_SET(X->frames[(int) F_VIEW], XV_SHOW, val, 0) ;
	   }
        }
     else  {
        frame_get_rect(X->frames[(int) F_MAIN], &X->rect[(int) R_MAIN]) ;
        if (X->frames [(int) F_PRINT] != NULL) {
	   popup_show [(int) F_PRINT] = 
			(int) xv_get (X->frames [(int) F_PRINT], XV_SHOW);
	   popup_pin [(int) F_PRINT] =
		(int) xv_get (X->frames [(int) F_PRINT], FRAME_CMD_PUSHPIN_IN);
	   XV_SET(X->frames[(int) F_PRINT], FRAME_CMD_PUSHPIN_IN, FALSE, 0);
           XV_SET(X->frames[(int) F_PRINT], XV_SHOW, val, 0) ;
	   }
#ifdef FILECHOOSER
        if (X->frames [(int) F_LOAD] != NULL) {
	   popup_show [(int) F_LOAD] = 
			(int) xv_get (X->frames [(int) F_LOAD], XV_SHOW);
	   popup_pin [(int) F_LOAD] =
	     (int) xv_get (X->frames [(int) F_LOAD], FRAME_CMD_PUSHPIN_IN);
	   XV_SET(X->frames[(int) F_LOAD], FRAME_CMD_PUSHPIN_IN, FALSE, 0);
           XV_SET(X->frames[(int) F_LOAD], XV_SHOW, val, 0) ;
	   }
        if (X->frames [(int) F_SAVE] != NULL) {
	   popup_show [(int) F_SAVE] = 
			(int) xv_get (X->frames [(int) F_SAVE], XV_SHOW);
	   popup_pin [(int) F_SAVE] =
	     (int) xv_get (X->frames [(int) F_SAVE], FRAME_CMD_PUSHPIN_IN);
	   XV_SET(X->frames[(int) F_SAVE], FRAME_CMD_PUSHPIN_IN, FALSE, 0);
           XV_SET(X->frames[(int) F_SAVE], XV_SHOW, val, 0) ;
	   }
#else
        if (X->frames [(int) F_LOADSAVE] != NULL) {
	   popup_show [(int) F_LOADSAVE] = 
			(int) xv_get (X->frames [(int) F_LOADSAVE], XV_SHOW);
	   popup_pin [(int) F_LOADSAVE] =
	     (int) xv_get (X->frames [(int) F_LOADSAVE], FRAME_CMD_PUSHPIN_IN);
	   XV_SET(X->frames[(int) F_LOADSAVE], FRAME_CMD_PUSHPIN_IN, FALSE, 0);
           XV_SET(X->frames[(int) F_LOADSAVE], XV_SHOW, val, 0) ;
	   }
#endif
        if (X->frames [(int) F_VIEW] != NULL) {
	   popup_show [(int) F_VIEW] = 
			(int) xv_get (X->frames [(int) F_VIEW], XV_SHOW);
	   popup_pin [(int) F_VIEW] =
		(int) xv_get (X->frames [(int) F_VIEW], FRAME_CMD_PUSHPIN_IN);
	   XV_SET(X->frames[(int) F_VIEW], FRAME_CMD_PUSHPIN_IN, FALSE, 0);
           XV_SET(X->frames[(int) F_VIEW], XV_SHOW, val, 0) ;
	   }
        }

     XV_SET(X->frames[(int) F_MAIN], XV_SHOW, val, 0) ;

     }
  else
     XV_SET(X->frames[(int) ftype], XV_SHOW, val, 0) ;
}

void
free_fullscreen()
{
  XV_DESTROY(X->fs) ;
  XSync (X->dpy, FALSE);
}


XImage *
gen_image(image)
image_t *image ;
{
  image_t *old, *new ;
  XImage *ximage ;
  char str[MAXPATHLEN] ;
  unsigned char *newdata;
  unsigned char *dptr;
  unsigned char *optr;
  int d ;
  int i, j, k;

  d = image->depth ;
  if (v->image != NULL) free_image(v->image) ;
  switch (v->depth)                    /* Screen depth. */
    {
      case 1  : switch (d)             /* Image depth. */
                  {
                    case 1  : v->image = copy_image (image) ;
                              break ;
		    case 4  : 
                    case 8  : message(mstr[(int) M_DITHER8]) ;
                              v->image = dither8to1(image) ;
                              break ;
                    case 24 :
                    case 32 : message(mstr[(int) M_DITHER24]) ;
                              old = dither24to8(image) ;
                              message(mstr[(int) M_DITHER8]) ;
                              v->image = dither8to1(old) ;
                              free_image(old) ;
                              break ;
                    default : SPRINTF(str, ostr[(int) O_BADIDEP], d) ;
                              message(str) ;
                              return((XImage *) NULL) ;
                  }
                ximage = XCreateImage(X->dpy, X->visual, 1, XYPixmap, 0,
                                      (char *) v->image->data, v->image->width,
                                      v->image->height, 
				      pad_value (v->depth), 0) ;
                break ;

      case 4  : switch (d)             /* Image depth. */
                  {
                    case 1  : message(mstr[(int) M_EXPAND8]) ;
                              v->image = expand1to8(image) ;
                              break ;
		    case 4  :
                    case 8  : message(mstr[(int) M_COMPRESS]) ;
                              if (v->isgray)
                                {
                                  new      = compress(image) ;
                                  message(mstr[(int) M_GRAY]) ;
                                  v->image = gray(new) ;
                                  free_image(new) ;
				}  
			      else {
                                   v->image = compress (image);
                                   if (v->image->cmapused > 16)  {
                                      message (mstr[(int) M_DITHER8]) ;
                                      new = dither8to1 (v->image) ;
                                      free_image (v->image);
                                      message(mstr[(int) M_EXPAND8]) ;
                                      v->image = expand1to8 (new) ;
                                      free_image (new) ;
                                      }
                                }
                              break;
                    case 24 :
                    case 32 : message(mstr[(int) M_DITHER24]) ;
                              old = dither24to8(image) ;
                              message(mstr[(int) M_COMPRESS]) ;
                              if (v->isgray)
                                {
                                  new = compress(old) ;
                                  message(mstr[(int) M_GRAY]) ;
                                  v->image = gray(new) ;
                                  free_image(new);
                                }
                              else {
                                   v->image = compress (old);
                                   if (v->image->cmapused > 16) {
                                      message (mstr[(int) M_DITHER8]) ;
                                      new = dither8to1 (v->image) ;
                                      free_image (v->image);
                                      message(mstr[(int) M_EXPAND8]) ;
                                      v->image = expand1to8 (new) ;
                                      free_image(new);
                                      }
                                 }    
                              free_image(old) ;
                              break ; 
                    default : SPRINTF(str, ostr[(int) O_BADIDEP], d) ;
                              message(str) ;
                              return((XImage *) NULL) ;
                  }
                message(mstr[(int) M_ADDCOLS]) ;
                add_colors(v->image) ;
                ximage = XCreateImage(X->dpy, X->visual, 4, ZPixmap, 0,
                                      (char *) v->image->data, v->image->width,
                                      v->image->height, 
				      pad_value (v->depth), 0);
                ximage->byte_order = MSBFirst ;
                break ;

      case 8  : switch (d)             /* Image depth. */
                  {
                    case 1  : message(mstr[(int) M_EXPAND8]) ;
                              v->image = expand1to8(image) ;
                              break ;
		    case 4  :
                    case 8  : message(mstr[(int) M_COMPRESS]) ;
                              if (v->isgray)
                                {
                                  old      = compress(image) ;
                                  message(mstr[(int) M_GRAY]) ;
                                  v->image = gray(old) ;
                                  free_image(old) ;
                                }
                              else v->image = compress(image) ;
                              break ;
                    case 24 :
                    case 32 : message(mstr[(int) M_DITHER24]) ;
                              old = dither24to8(image) ;
                              message(mstr[(int) M_COMPRESS]) ;
                              if (v->isgray)
                                {
                                  new      = compress(old) ;
                                  message(mstr[(int) M_GRAY]) ;
                                  v->image = gray(new) ;
                                  free_image(new) ;
                                }
                              else v->image = compress(old) ;
                              free_image(old) ;
                              break ;
                    default : SPRINTF(str, ostr[(int) O_BADIDEP], d) ;
                              message(str) ;
                              return((XImage *) NULL) ; 
                  }
                message(mstr[(int) M_ADDCOLS]) ;
                add_colors(v->image) ;
                ximage = XCreateImage(X->dpy, X->visual, 8, ZPixmap, 0,
                                      (char *) v->image->data, v->image->width,
                                      v->image->height, 
				      pad_value (v->depth), 0) ;
                ximage->byte_order = MSBFirst ;
                break ;

      case 24 : switch (d)             /* Image depth. */
                  { 
                    case 1  : message(mstr[(int) M_EXPAND8]) ;
                              new = expand1to8(image) ;
                              message(mstr[(int) M_EXPAND24]) ;
                              v->image = expand8to24(new) ;
                              free_image(new) ;
                              break ;
		    case 4  :
                    case 8  : message(mstr[(int) M_EXPAND24]) ;
                              v->image = expand8to24(image) ;
                              break ; 
                    case 24 :
                              newdata = (unsigned char *)
                                        ck_zmalloc((size_t) image->width *
                                                            image->height * 4);
			      dptr = newdata;
			      optr = image->data;
			      for (i = 0; i < image->height ; i++) {
                                  for (j = 0; j < image->width ; j++) {
                                      *dptr++ = 0;
                                      for (k = 0 ; k < 3 ; k++)
                                          *dptr++ = *optr++;
                                      }
                                  for (j = 0; j < (image->bytes_per_line
                                                 - (image->width * 3)) ; j++)
                                      optr++;
                                  }

                              ck_zfree((caddr_t) image->data,
                                       image->used_malloc) ;
			      image->data = newdata;
			      image->bytes_per_line = image->width * 4;
			      image->depth = 32; 
                    case 32 : v->image = copy_image (image) ;
			      v->image->depth = 24;
                              break ; 
                    default : SPRINTF(str, ostr[(int) O_BADIDEP], d) ;
                              message(str) ; 
                              return((XImage *) NULL) ;  
                  } 
                ximage = XCreateImage(X->dpy, X->visual, 24, ZPixmap, 0,
                                      (char *) v->image->data, v->image->width,
                                      v->image->height, 32, 0) ;
                ximage->byte_order = MSBFirst ;
                break ;

      default : SPRINTF(str, ostr[(int) O_BADSDEP], v->depth) ;
                message(str) ;
                return((XImage *) NULL) ;
    }
  return(ximage) ;
}


int
get_area(dtype, width, height)
enum dump_type dtype ;
int *width, *height ;
{
  int button_down = 0 ;
  int grasp, x, xoff, y, yoff ;
  Event event ;
  XID child, xid ;
  XWindowAttributes attr ;
  extern Xv_opaque xv_default_display ;
  Inputmask inmask ;
  int src_x, src_y, dst_x, dst_y, max_area, new_area;
  Window src_xid, dst_xid;

  switch (dtype)
    {
      case D_RECT   : XFlush((Display *) xv_default_display) ;
                      win_setinputcodebit(&inmask, MS_LEFT) ;
                      win_setinputcodebit(&inmask, MS_MIDDLE) ;
                      win_setinputcodebit(&inmask, MS_RIGHT) ;
                      win_setinputcodebit(&inmask, LOC_DRAG) ;
                      win_setinputcodebit(&inmask, LOC_MOVEWHILEBUTDOWN) ;

                      make_fullscreen(D_RECT) ;

                      while (xv_input_readevent(X->frames[(int) F_MAIN],
                                          &event, TRUE, TRUE, &inmask) != -1)
                        {
			  if ((event_id(&event) != MS_LEFT) &&
			      (event_id(&event) != MS_MIDDLE) &&
			      (event_id(&event) != MS_RIGHT))
			     continue;

                          if (event_id(&event) != MS_LEFT ||
                              win_inputnegevent(&event))
                            {
                              if (get_int_val(PI_HIDEWIN))
                                frame_show(F_ALL, TRUE) ;
                              free_fullscreen() ;
                              message(mstr[(int) M_CANCEL]) ;
                              return(0) ;
                            }
                          break ;
                        }

                      win_translate_xy(X->frames[(int) F_MAIN],
                                       X->frames[(int) F_ROOT],
                                       0, 0, &xoff, &yoff) ;

                      x = event_x(&event) + xoff ;
                      y = event_y(&event) + yoff ;

                      rect_construct(&X->rect[(int) R_BOX], x, y, 0, 0) ;

                      drawbox(&X->rect[(int) R_BOX]) ;
                      button_down = event_id(&event) ;
                      grasp = SE ;

                      while (xv_input_readevent(X->frames[(int) F_MAIN],
                                         &event, TRUE, TRUE, &inmask) != -1)
                        {
   
/* Make the event locations relative to the root frame. */
 
                          win_translate_xy(X->frames[(int) F_MAIN],
                                           X->frames[(int) F_ROOT],
                                           0, 0, &xoff, &yoff) ;

                          x = event_x(&event) + xoff ;
                          y = event_y(&event) + yoff ;

                          if (event_id(&event) == LOC_MOVEWHILEBUTDOWN &&
                              button_down)
                            dofeedback(x, y, &X->rect[(int) R_BOX], &grasp) ;
                          else if (event_id(&event) == button_down)
                            {
                              if (win_inputnegevent(&event)) button_down = 0 ;
                            }
                          else if (event_id(&event) == MS_LEFT)
                            {
                              if (!button_down &&
                                  event_id(&event) == MS_LEFT &&
                                  event_is_down(&event))
                                {
                                  button_down = event_id(&event) ;
                                  grasp = computegrasp(&X->rect[(int) R_BOX],
                                                       x, y) ;
                                  dofeedback(x, y, &X->rect[(int) R_BOX],
                                                   &grasp) ;
                                }
                            }
                          else if (event_id(&event) == MS_RIGHT)
                            {
                              if (get_int_val(PI_HIDEWIN))
                                frame_show(F_ALL, TRUE) ;
                              drawbox(&X->rect[(int) R_BOX]) ;
                              free_fullscreen() ;

                              if (get_int_val(PI_HIDEWIN))
                                frame_show(F_ALL, TRUE) ;
                              message(mstr[(int) M_CANCEL]) ;
                              return(0) ;
                            }
                          else if (event_id(&event) == MS_MIDDLE) break ;
                        }
                      drawbox(&X->rect[(int) R_BOX]) ;
                      *width  = X->rect[(int) R_BOX].r_width ;
                      *height = X->rect[(int) R_BOX].r_height ;
                      break ;

      case D_SCREEN : rect_construct(&X->rect[(int) R_BOX], 0, 0,
                                     X->rect[(int) R_ROOT].r_width,
                                     X->rect[(int) R_ROOT].r_height) ;
                      *width  = X->rect[(int) R_ROOT].r_width ;
                      *height = X->rect[(int) R_ROOT].r_height ;
                      break ;

      case D_WINDOW : XFlush((Display *) xv_default_display) ;
                      win_setinputcodebit(&inmask, MS_LEFT) ;
                      win_setinputcodebit(&inmask, MS_MIDDLE) ;
                      win_setinputcodebit(&inmask, MS_RIGHT) ;
                      win_setinputcodebit(&inmask, LOC_MOVE) ;
 
                      make_fullscreen(D_WINDOW) ;
 
                      while (xv_input_readevent(X->frames[(int) F_MAIN],
                                           &event, TRUE, TRUE, &inmask) != -1)
                        {
                               if (event_id(&event) == MS_LEFT) break ;
                          else if (event_id(&event) == MS_RIGHT ||
                                   event_id(&event) == MS_MIDDLE)
                            {
                              free_fullscreen() ;
                              message(mstr[(int) M_SCANCEL]) ;
                              return(0) ;
                            }
                        }
 
/*
                      win_translate_xy(X->frames[(int) F_MAIN],
                                       X->frames[(int) F_ROOT],
                                       event_x(&event), event_y(&event),
                                       &x, &y) ;

                      xid = win_pointer_under(X->frames[(int) F_ROOT], x, y) ;
*/

		      src_x = event_x (&event);
		      src_y = event_y (&event);
		      src_xid = xv_get (X->frames[(int) F_MAIN], XV_XID);
		      dst_xid = X->rxid;
		      max_area = 0;
		      xid = NULL;
		      for (;;) {
			  XTranslateCoordinates (X->dpy, src_xid, dst_xid,
						 src_x, src_y, &dst_x, &dst_y,
						 &child);
			  if (child == NULL)
			     break;

			  XGetWindowAttributes (X->dpy, child, &attr);
			  new_area = attr.height * attr.width;
			  if (new_area > max_area) {
			     max_area = new_area;
			     xid = child;
			     }
			  src_x = dst_x;
			  src_y = dst_y;
			  src_xid = dst_xid;
			  dst_xid = child;
			  }

                      if (xid == NULL) return(-1) ;

                      if (get_int_val(PI_HIDEWIN))     
                        {                              
                          message(mstr[(int) M_BEBACK]) ;
                          frame_show(F_ALL, FALSE) ;  
                        }                              

/*  Lock screen during copy. */        

                      XGetWindowAttributes((Display *) xv_default_display,
                                           xid, &attr) ;

/* Translate from parent coordinate space to screen coordinate space. */

                      XTranslateCoordinates((Display *) xv_default_display,
                                            xid, X->rxid, 0, 0,
                                            &x, &y, &child) ;

/* If window is partially off the screen, get what's on the screen */

                      if (x < 0)                       
                        {                              
                          attr.width += x ;       
                          x = 0 ;                      
                        }                              

                      if (y < 0)                       
                        {                              
                          attr.height += y ;      
                          y = 0 ;                      
                        }                              

                      if (attr.width > X->rect[(int) R_ROOT].r_width - x)
                        attr.width = X->rect[(int) R_ROOT].r_width - x ;

                      if (attr.height > X->rect[(int) R_ROOT].r_height - y)
                        attr.height = X->rect[(int) R_ROOT].r_height - y ;

                      X->rect[(int) R_BOX].r_left = x ;
                      X->rect[(int) R_BOX].r_top = y ;
                      X->rect[(int) R_BOX].r_width = attr.width ;
                      X->rect[(int) R_BOX].r_height = attr.height ;
                      *width  = attr.width ;
                      *height = attr.height ;
    }

  if ((*width == 0) || (*height == 0)) {
     free_fullscreen() ;
     message(mstr[(int) M_NOSNAP]) ;
     return(0) ;
     }

  return(1) ;
}


char *
get_char_val(pitype)  /* Get panel item string value. */
enum pi_type pitype ;
{
  return((char *) xv_get(X->pitems[(int) pitype], PANEL_VALUE)) ;
}


int
get_int_val(pitype)   /* Get panel item integer value. */
enum pi_type pitype ;
{
  return((int) xv_get(X->pitems[(int) pitype], PANEL_VALUE)) ;
}


char *MSGFILE = "SUNW_DESKSET_SNAPSHOT_MSG";
char *LBLFILE = "SUNW_DESKSET_SNAPSHOT_LABEL";

void
init_graphics(argc, argv)
int *argc ;
char *argv[] ;
{
  char bind_home[MAXPATHLEN];

  X = (XVars) LINT_CAST(malloc(sizeof(XObject))) ;
  X->frames[(int) F_PRINT]    = NULL ;
#ifdef FILECHOOSER
  X->frames[(int) F_LOAD] = NULL ;
  X->frames[(int) F_SAVE] = NULL ;
#else
  X->frames[(int) F_LOADSAVE] = NULL ;
#endif
  X->frames[(int) F_VIEW]     = NULL ;
  X->pixmap = NULL ;
  X->ximage = NULL ;
  X->server = xv_init(XV_INIT_ARGC_PTR_ARGV, argc, argv,
		      XV_USE_LOCALE, TRUE,
		      XV_X_ERROR_PROC, xerror_interpose,
		      0) ;
  ds_expand_pathname("$OPENWINHOME/lib/locale", bind_home);
  bindtextdomain(MSGFILE, bind_home);
  bindtextdomain(LBLFILE, bind_home);
}

#ifdef FILECHOOSER
/*ARGSUSED*/
static int
load_callback (fc, path, file, client_data)
    File_chooser   fc;
    char          *path;
    char          *file;
    Xv_opaque      client_data;
{


      if (check_overwrite(ISLOAD) == FALSE) return ;
/*
      if (!verify_paths(FALSE, PI_LSDIR, PI_LSFILE, path)) return;
*/
      strcpy (v->path, path);
      if (loadfile (path, R_FILE) == TRUE)
         v->ls_status = ISLOAD;

/*
 * Unshow popup if unpinned.
 */
      if (!(int) xv_get ((xv_get ((xv_get (fc, XV_OWNER)), XV_OWNER)),
                           FRAME_CMD_PUSHPIN_IN))
           xv_set (fc, WIN_SHOW, FALSE, NULL);

}

/*ARGSUSED*/
static int
save_callback (fc, path, stats)
    File_chooser   fc;
    char          *path;
    struct stat   *stats;
{

      if (X->ximage == NULL)
        {
          window_bell (X->frames[(int) F_MAIN]); 
          message(mstr[(int) M_NOSIMAGE]) ;
          return ;
        }
/*
      if (!verify_paths(TRUE, PI_LSDIR, PI_LSFILE, path)) return ;
*/
      strcpy (v->path, path);
      savefile(path, R_FILE, 0) ;
/*
 * Unshow popup if unpinned.
 */
      if (!(int) xv_get ((xv_get ((xv_get (fc, XV_OWNER)), XV_OWNER)),
                           FRAME_CMD_PUSHPIN_IN))
           xv_set (fc, WIN_SHOW, FALSE, NULL);

}



#else

/*ARGSUSED*/
static void
load_save_snap(item, event)
Panel_item item ;
Event *event ;
{
  char *blabel ;

  blabel = (char *) xv_get(X->pitems[(int) PI_LSBUT], PANEL_LABEL_STRING) ;
  if (!strcmp(blabel, ostr[(int) O_SAVE]))
    {
      if (X->ximage == NULL)
        {
          message(mstr[(int) M_NOSIMAGE]) ;
          return ;
        }
      if (!verify_paths(TRUE, PI_LSDIR, PI_LSFILE, v->path)) return ;
      savefile(v->path, R_FILE, 0) ;
    }
  else
    {
      if (check_overwrite(ISLOAD) == FALSE) return ;
      if (!verify_paths(FALSE, PI_LSDIR, PI_LSFILE, v->path)) return ;
      if (loadfile(v->path, R_FILE)  == TRUE)
         v->ls_status = ISLOAD ;
    }
}
#endif

/*ARGSUSED*/
static void
load_snap_proc(item, event)
Panel_item item ;
Event *event ;
{
  static int  position_popup = 1;

#ifdef FILECHOOSER
  make_open_filechooser();
#else
  make_ls_panel() ;     /* Create load/save snapshot panel plus items. */
#endif

#ifdef FILECHOOSER
  if (xv_get (X->frames[(int) F_LOAD], XV_SHOW) == FALSE) {
    if (position_popup) 
        ds_position_popup (X->frames[(int) F_MAIN],
			   X->frames[(int) F_LOAD], DS_POPUP_LOR);
  }
/*
 * Re-read the directory then show the filechooser.
 */ 
  xv_set (X->frames[(int) F_LOAD], FILE_CHOOSER_UPDATE, NULL);
  frame_show (F_LOAD, TRUE);
#else
  XV_SET(X->frames[(int) F_LOADSAVE], FRAME_LABEL, Pstrs[(int) L_LABEL], 0) ;
  XV_SET(X->pitems[(int) PI_LSBUT], PANEL_LABEL_STRING, ostr[(int) O_LOAD], 0) ; 
  if (!xv_get(X->frames[(int) F_LOADSAVE], XV_SHOW))
    {
      if (position_popup) 
         ds_position_popup(X->frames[(int) F_MAIN],
                           X->frames[(int) F_LOADSAVE], DS_POPUP_BELOW) ;
      frame_show(F_LOADSAVE, TRUE) ;
    }
#endif

  position_popup = 0;
}


int
loadfile(filename, rtype)
char *filename ;
enum rasio_type rtype ;
{
  image_t *image ;
  unsigned char *mem_data = v->rbuf;
  struct stat stat_buf;

  if (rtype == R_FILE) {
     if (stat (filename, &stat_buf) == NULL) {
        if (S_ISDIR (stat_buf.st_mode)) {
           message(mstr[(int) M_ISDIR]) ;
           return (FALSE);
           }
        if (!S_ISREG (stat_buf.st_mode)) {
           message(mstr[(int) M_ISNOTREG]) ;
           return (FALSE);
           }
        if (access (filename, R_OK)) {
           message(mstr[(int) M_NOACCESS]) ;
           return (FALSE);
           }
        }
     }

  if ((image = rast_load(filename, rtype)) == NULL) {
    v->rbuf = mem_data;
    if ((image = gif_load(filename, rtype)) == NULL) {
      v->rbuf = mem_data;		/* Reset it for the hell of it */
      message(mstr[(int) M_UNRECOG]) ;
      return (FALSE);
      }
    }

  v->rbuf = mem_data;
  if (X->ximage != NULL) {
     if (v->image != NULL) {
        if ((unsigned char *) X->ximage->data == v->image->data) {
	   ck_zfree((char *) v->image->data, v->image->used_malloc);
           v->image->data = (unsigned char *) NULL;
	   X->ximage->data = (char *) NULL;
	   }
	else {
	   ck_zfree((char *) X->ximage->data, FALSE);
	   X->ximage->data = (char *) NULL;
           }
	}
     else {
	ck_zfree((char *) X->ximage->data, FALSE);
	X->ximage->data = (char *) NULL;
	}
     XDestroyImage(X->ximage) ;
     }

  X->ximage = gen_image(image) ;

  X->rect[(int) R_BOX].r_left   = 0 ;
  X->rect[(int) R_BOX].r_top    = 0 ;
  X->rect[(int) R_BOX].r_width  = v->image->width ;
  X->rect[(int) R_BOX].r_height = v->image->height ;

  message(mstr[(int) M_LOADSUCS]) ;

  v->havename = TRUE ;
  if (rtype == R_DATA)
     v->havename = FALSE ;
  else
     set_namestripe ();

  free_image(image) ;

/*
 * This function should only get called if imagetool isn't there.
 */

  if (v->ttid != (char *) NULL)
     viewfile ();
  else if ((X->frames[(int) F_VIEW] != NULL) &&
      	   (xv_get(X->frames[(int) F_VIEW], XV_SHOW) == TRUE)) 
     old_viewfile() ;

/*
 * File was loaded successfully... let user drag it out now.
 */

  xv_set (X->pitems [(int) PI_DRAG], PANEL_DROP_FULL, TRUE, NULL);
  return (TRUE);
}


void
make_cursors()    /* Build the cursors we need. */
{
  Server_image null_pr, confirm_pr, select_pr ;

  confirm_pr = xv_create(XV_NULL,           SERVER_IMAGE,
                         XV_WIDTH,          16,
                         XV_HEIGHT,         16,
                         SERVER_IMAGE_BITS, confirm_data,
                         0) ;
  X->cursors[(int) C_CONFIRM] = xv_create(XV_NULL,      CURSOR,
                                       CURSOR_IMAGE, confirm_pr,
                                       CURSOR_XHOT,  4,
                                       CURSOR_YHOT,  4,
                                       CURSOR_OP,    PIX_SRC ^ PIX_DST,
                                       0) ;

  select_pr = xv_create(XV_NULL,           SERVER_IMAGE,
                        XV_WIDTH,          16,
                        XV_HEIGHT,         16,
                        SERVER_IMAGE_BITS, select_data,
                        0) ;
  X->cursors[(int) C_SELECT] = xv_create(XV_NULL,      CURSOR,
                                      CURSOR_IMAGE, select_pr,
                                      CURSOR_XHOT,  15,
                                      CURSOR_YHOT,  15,
                                      CURSOR_OP,    PIX_SRC ^ PIX_DST,
                                      0) ;

  null_pr = xv_create(XV_NULL,           SERVER_IMAGE,
                      XV_WIDTH,          16,
                      XV_HEIGHT,         16,
                      SERVER_IMAGE_BITS, blank_data,
                      0) ;
  X->cursors[(int) C_NULL] = xv_create(XV_NULL,      CURSOR,
                                    CURSOR_IMAGE, null_pr,
                                    CURSOR_XHOT,  0,
                                    CURSOR_YHOT,  0,
                                    CURSOR_OP,    PIX_SRC | PIX_DST,
                                    0) ;
}

int
quit_callback (ttmsg, key, status, silent, force, msgid)
    Tt_message   ttmsg;
    void        *key;
    int          status;
    int          silent;
    int          force;
    char        *msgid;
{
    return (0);
}

Notify_value
quit_snapshot (client, status)
Notify_client	client;
Destroy_status	status;
{
    
    if (status == DESTROY_CHECKING)
       if (check_overwrite (ISQUIT) == FALSE)
       	  return (notify_veto_destroy (client));

    if (v->ttid != (char *) NULL) {
       dstt_quit ( quit_callback, v->tool_name, v->ttid, TRUE,
		   TRUE, (char *) NULL);
       free (v->ttid);
       v->ttid = (char *) NULL;
       }

    return (notify_next_destroy_func (client, status));
}

int
iconify_callback (ttmsg, key, status, iconify, msgid, buffID)
    Tt_message   ttmsg;
    void        *key;
    int          status;
    int          iconify;
    char        *msgid;
    char	*buffID;
{
    return (0);
}

Notify_value
frame_proc (frame, event, arg, type)
Frame		 	 frame;
Event			*event;
Notify_arg	 	 arg;
Notify_event_type	 type;
{
    Notify_value	 value;
    int			 closed_current;
    int			 closed_initial = xv_get (frame, FRAME_CLOSED);

    value = notify_next_event_func (frame, (Notify_event) event, arg, type);
    
    closed_current = xv_get (frame, FRAME_CLOSED);
    if (closed_current != closed_initial) 
       dstt_set_iconified (iconify_callback, v->tool_name, v->ttid,
			   closed_current, (char *) NULL, (char *) NULL); 

    return (value);
}

void
make_frame()    /* Create main snapshot frame. */
{
  Rect *r ;
  XVisualInfo *vinfo, vtemp ;
  int nitems ;

  X->frames[(int) F_MAIN] = xv_create(XV_NULL,           FRAME,
                              WIN_ERROR_MSG,     ostr[(int) O_NOWIN],
                              XV_LABEL,          ostr[(int) O_FLABEL],
                              FRAME_SHOW_FOOTER, TRUE,
                              FRAME_SHOW_LABEL,  TRUE,
                              FRAME_NO_CONFIRM,  TRUE,
                              FRAME_ICON,        X->frame_icon,
#ifdef OW_I18N
			      WIN_USE_IM,	 FALSE,
#endif
                              0) ;
  if (X->frames[(int) F_MAIN] == NULL)
    {
      FPRINTF(stderr, ostr[(int) O_NOAPP]) ;
      exit(1) ;
    }

/* Get the base frame for later fullscreen drawing. */
 
  X->frames[(int) F_ROOT] = xv_get(X->frames[(int) F_MAIN], XV_ROOT) ;
  r = (Rect *) xv_get(X->frames[(int) F_ROOT], WIN_RECT) ;
  X->rect[(int) R_ROOT] = *r ;
  v->depth = (int) xv_get(X->frames[(int) F_MAIN], WIN_DEPTH) ;
  X->rxid = (Drawable) xv_get(X->frames[(int) F_ROOT], XV_XID) ;

  X->dpy         = (Display *) xv_get(X->frames[(int) F_ROOT], XV_DISPLAY) ;
  X->screen      = DefaultScreen(X->dpy) ;

/* 
 * Can't use DefaultVisual anymore.. must get the Visual from the base
 * frame.
 */

  X->visual      = (Visual *) xv_get (X->frames [(int) F_MAIN], XV_VISUAL);

  if (v->isgray == FALSE)
    {
      vtemp.visualid = XVisualIDFromVisual(X->visual) ;
      vinfo          = XGetVisualInfo(X->dpy, VisualIDMask, &vtemp, &nitems) ;
      if (vinfo->class == GrayScale || vinfo->class == StaticGray)
        v->isgray = TRUE ;
    }

  X->root    = RootWindow(X->dpy, X->screen) ;

/*
 * Can't use BlackPixel, WhitePixel anymore.. must get these values from
 * the base frame.
 */

  X->foregnd = xv_get (xv_get (X->frames [(int) F_MAIN], WIN_CMS),
			   CMS_FOREGROUND_PIXEL);
  X->backgnd = xv_get (xv_get (X->frames [(int) F_MAIN], WIN_CMS),
			   CMS_BACKGROUND_PIXEL);

  X->gc_mask = GCFunction | GCForeground | GCSubwindowMode ;

/*
 * Can't use DisplayPlanes anymore.. get the value from the base frame
 * (ie. the depth).
 */

  X->gc_val.foreground = ~((~0L) << v->depth );
  X->gc_val.function = GXxor ;
  X->gc_val.subwindow_mode = IncludeInferiors ;
  X->rootgc = XCreateGC(X->dpy, X->root, X->gc_mask, &X->gc_val) ;

/*
 * Set up the destroy function, so we can check if the user needs to
 * save an image before exiting.
 */

  notify_interpose_destroy_func (X->frames[(int) F_MAIN], quit_snapshot);

/*
 * Interpose on the frame events so can catch open and close events.
 */

  notify_interpose_event_func (X->frames[(int) F_MAIN], frame_proc, 
			       NOTIFY_SAFE);

/**  XSynchronize(X->dpy, TRUE) ; **/
}


void
make_fullscreen(dtype)
enum dump_type dtype ;
{
  switch (dtype)
    {
      case D_RECT   : X->fs = xv_create(X->frames[(int) F_MAIN], FULLSCREEN,
                                        WIN_CONSUME_EVENTS,
                                          WIN_MOUSE_BUTTONS, LOC_DRAG,
                                          LOC_MOVEWHILEBUTDOWN,
                                          0,
                                        FULLSCREEN_GRAB_SERVER,   TRUE,
                                        FULLSCREEN_GRAB_POINTER,  TRUE,
                                        FULLSCREEN_GRAB_KEYBOARD, TRUE,
                                        WIN_CURSOR, X->cursors[(int) C_SELECT],
                                        0) ;
                      break ;
      case D_SCREEN : X->fs = xv_create(XV_NULL,                  FULLSCREEN,
                                        FULLSCREEN_GRAB_SERVER,   TRUE,
                                        FULLSCREEN_GRAB_POINTER,  FALSE,
                                        FULLSCREEN_GRAB_KEYBOARD, FALSE,
                                        0) ;
                      break ;
      case D_WINDOW : X->fs = xv_create(X->frames[(int) F_MAIN], FULLSCREEN,
                                        WIN_CONSUME_EVENTS,
                                          WIN_MOUSE_BUTTONS, LOC_MOVE, 0,
                                        0) ;
    }
}


void
make_icon()     /* Create snapshot icon (plus mask). */
{
  Server_image server_icon_image, server_icon_mask_image ;

  server_icon_image = xv_create(XV_NULL,            SERVER_IMAGE,
                                SERVER_IMAGE_BITS,  icon_data,
                                SERVER_IMAGE_DEPTH, 1,
                                XV_WIDTH,           64,
                                XV_HEIGHT,          64,
                                0) ;

  server_icon_mask_image = xv_create(XV_NULL,            SERVER_IMAGE,
                                     SERVER_IMAGE_BITS,  icon_mask_image,
                                     SERVER_IMAGE_DEPTH, 1,
                                     XV_WIDTH,           64,
                                     XV_HEIGHT,          64,
                                     0) ;

  X->frame_icon = xv_create(XV_NULL,         ICON,
                         ICON_IMAGE,      server_icon_image,
                         ICON_MASK_IMAGE, server_icon_mask_image,
                         XV_LABEL,        ostr[(int) O_FLABEL],
                         WIN_RETAINED,    TRUE,
			 ICON_TRANSPARENT, TRUE,
                         0) ;
}


void
make_images()        /* Create drag-n-drop server images. */
{
  X->sv[(int) SV_DRAG_PTR] = xv_create(XV_NULL,            SERVER_IMAGE,
                                  SERVER_IMAGE_BITS,  source_drag_image,
                                  SERVER_IMAGE_DEPTH, 1,
                                  XV_WIDTH,           64,
                                  XV_HEIGHT,          64,
                                  0) ;

  X->sv[(int) SV_DROP_PTR] = xv_create(XV_NULL,            SERVER_IMAGE,
                                  SERVER_IMAGE_BITS,  source_drop_image,
                                  SERVER_IMAGE_DEPTH, 1,
                                  XV_WIDTH,           64,
                                  XV_HEIGHT,          64,
                                  0) ;
}

#ifdef FILECHOOSER
static void
make_open_filechooser()
{

  if (X->frames[(int) F_LOAD] == NULL)

          X->frames[(int) F_LOAD] = xv_create (X->frames[(int) F_MAIN], 
			   FILE_CHOOSER_OPEN_DIALOG,
			   XV_LABEL, Pstrs[(int) L_LABEL],
			   FILE_CHOOSER_NOTIFY_FUNC, load_callback,  
			   FILE_CHOOSER_DIRECTORY, v->directory_name,
                           XV_HELP_DATA,             hstr[(int) H_LSDIR],
#ifdef OW_I18N
			   WIN_USE_IM,		     TRUE,
#endif
			   NULL);


}

static void
make_save_filechooser()
{

  if (X->frames[(int) F_SAVE] == NULL)

          X->frames[(int) F_SAVE] = xv_create (X->frames[(int) F_MAIN], 
			   FILE_CHOOSER_SAVE_DIALOG,
			   XV_LABEL, Pstrs[(int) S_LABEL],
			   FILE_CHOOSER_NOTIFY_FUNC, save_callback,  
			   FILE_CHOOSER_DOC_NAME,    v->default_file,
			   FILE_CHOOSER_DIRECTORY,   v->directory_name,
			   FILE_CHOOSER_NO_CONFIRM,  v->noconfirm,
                           XV_HELP_DATA,             hstr[(int) H_LSDIR],
#ifdef OW_I18N
			   WIN_USE_IM,		     TRUE,
#endif
			   NULL);


}

#else
static void
make_ls_panel()
{
  Xv_font item_font;
  Font_string_dims dims;
  int longest;
  int gap;

  if (X->frames[(int) F_LOADSAVE] != NULL) return ;

  X->frames[(int) F_LOADSAVE] = xv_create(X->frames[(int) F_MAIN], FRAME_CMD,
#ifdef OW_I18N
							WIN_USE_IM, TRUE,
#endif
                                          FRAME_LABEL, Pstrs[(int) P_LABEL],
                                          0) ;

  X->panel[(int) P_LOADSAVE] = xv_get(X->frames[(int) F_LOADSAVE],
                                      FRAME_CMD_PANEL, 0) ;

  X->pitems[(int) PI_LSDIR] = xv_create(X->panel[(int) P_LOADSAVE], PANEL_TEXT,
                      PANEL_VALUE_Y,              P_ROW(P_LOADSAVE, LSR_DIR),
                      PANEL_LABEL_STRING,         mpstrs[(int) PI_LSDIR],
                      PANEL_LABEL_BOLD,           TRUE,
                      PANEL_VALUE,                v->directory_name,
                      PANEL_VALUE_DISPLAY_LENGTH, M_LSTEXTLEN,
                      PANEL_NOTIFY_PROC,          dir_proc,
                      XV_HELP_DATA,               hstr[(int) H_LSDIR],
                      0) ;

  item_font = (Xv_font) xv_get (X->pitems [(int) PI_LSDIR], PANEL_LABEL_FONT);
  gap = xv_get (X->panel [(int) P_LOADSAVE], PANEL_ITEM_X_GAP);

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_LSDIR], &dims);
  longest = dims.width;

  X->pitems[(int) PI_LSFILE] = xv_create(X->panel[(int) P_LOADSAVE], PANEL_TEXT,
                       PANEL_VALUE_Y,          P_ROW(P_LOADSAVE, LSR_FILE),
                       PANEL_LABEL_STRING,     mpstrs[(int) PI_LSFILE],
                       PANEL_LABEL_BOLD,           TRUE,
                       PANEL_VALUE,                v->file_name,
                       PANEL_VALUE_DISPLAY_LENGTH, M_LSTEXTLEN,
                       XV_HELP_DATA,               hstr[(int) H_LSFILE],
                       0) ;

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_LSFILE], &dims);
  if (dims.width > longest)
     longest = dims.width;

  xv_set (X->pitems[(int) PI_LSDIR], PANEL_VALUE_X,
		longest + ((MARGIN_OFFSET + 1) * gap), NULL);

  xv_set (X->pitems[(int) PI_LSFILE], PANEL_VALUE_X,
		longest + ((MARGIN_OFFSET + 1) * gap), NULL);

  X->pitems[(int) PI_LSBUT] = xv_create(X->panel[(int)P_LOADSAVE], PANEL_BUTTON,
                                PANEL_ITEM_Y, P_ROW(P_MAIN, LSR_BUTTON),
                                PANEL_LABEL_STRING, mpstrs[(int) PI_LSBUT],
                                PANEL_NOTIFY_PROC,  load_save_snap,
                                XV_HELP_DATA,       hstr[(int) H_LSBUTTON],
                                0) ;

  window_fit(X->panel[(int) P_LOADSAVE]) ;
  ds_center_items(X->panel[(int) P_LOADSAVE], LSR_BUTTON,
                  X->pitems[(int) PI_LSBUT], NULL);
  window_fit(X->frames[(int) F_LOADSAVE]) ;

}
#endif

void
make_main_panel()    /* Create main snapshot panel and items. */
{
  Menu print_menu ;
  Xv_font item_font;
  Font_string_dims dims;
  Rect *rect;
  int value_x;
  int longest;
  int gap;

  X->panel[(int) P_MAIN] = xv_create(X->frames[(int) F_MAIN], PANEL, 
					PANEL_ACCEPT_KEYSTROKE,	TRUE,
					0);

  gap = xv_get (X->panel[(int) P_MAIN], PANEL_ITEM_X_GAP);

  load_button = xv_create (X->panel[(int) P_MAIN], PANEL_BUTTON,
            PANEL_ITEM_X,           P_COL(P_MAIN, MC_LOAD),
            PANEL_ITEM_Y,           P_ROW(P_MAIN, MR_LSSPD),
            PANEL_LABEL_STRING,     Pstrs[(int) LOAD_SNAP],
            PANEL_NOTIFY_PROC,      load_snap_proc,
            XV_HELP_DATA,           hstr[(int) H_LOAD],
            0) ;

  save_button = xv_create (X->panel[(int) P_MAIN], PANEL_BUTTON,
            PANEL_ITEM_Y,           P_ROW(P_MAIN, MR_LSSPD),
            PANEL_NOTIFY_PROC,      save_snap_proc,
            PANEL_LABEL_STRING,     Pstrs[(int) SAVE_SNAP],
            XV_HELP_DATA,           hstr[(int) H_SAVE],
            0) ;

  print_menu = (Menu) create_print_pulldown() ;
  print_button = xv_create (X->panel[(int) P_MAIN], PANEL_BUTTON,
            PANEL_ITEM_Y,           P_ROW(P_MAIN, MR_LSSPD),
            PANEL_ITEM_MENU,        print_menu,
            PANEL_LABEL_STRING,     Pstrs[(int) PRINT_SNAP],
            XV_HELP_DATA,           hstr[(int) H_PRINT],
            0) ;

  make_print_panel() ;   /* Create print snapshot panel plus items. */

  X->pitems[(int) PI_STYPE] = xv_create(X->panel[(int) P_MAIN], PANEL_CHOICE,
                                PANEL_VALUE_Y,      P_ROW(P_MAIN, MR_STYPE),
                                PANEL_LABEL_STRING, mpstrs[(int) PI_STYPE],
                                PANEL_CHOICE_STRINGS,
                                  Pstrs[(int) ST_WINDOW],
                                  Pstrs[(int) ST_REGION],
                                  Pstrs[(int) ST_SCREEN],
                                  0,
                                XV_HELP_DATA,      hstr[(int) H_STYPE],
                                0) ;

  item_font = (Xv_font) xv_get (X->pitems[(int) PI_STYPE], PANEL_LABEL_FONT);

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_STYPE], &dims);
  longest = dims.width;

  X->pitems[(int) PI_DELAY] = xv_create(X->panel[(int) P_MAIN], PANEL_CHOICE,
                                PANEL_VALUE_Y,      P_ROW(P_MAIN, MR_DELAY),
                                PANEL_LABEL_STRING, mpstrs[(int) PI_DELAY],
                                PANEL_CHOICE_STRINGS,
                                  Pstrs[(int) DELAY0],
                                  Pstrs[(int) DELAY2],
                                  Pstrs[(int) DELAY4],
                                  Pstrs[(int) DELAY8],
                                  Pstrs[(int) DELAY16],
                                  0,
                                PANEL_NOTIFY_PROC, delay_choice,
                                XV_HELP_DATA,      hstr[(int) H_DELAY],
                                0) ;

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_DELAY], &dims);
  if (dims.width > longest)
     longest = dims.width;

  xv_set (X->pitems[(int) PI_STYPE], PANEL_VALUE_X,
		longest + ((MARGIN_OFFSET + 1) * gap), NULL);

  xv_set (X->pitems[(int) PI_DELAY], PANEL_VALUE_X,
		longest + ((MARGIN_OFFSET + 1) * gap), NULL);

  rect = (Rect *) xv_get (X->pitems[(int) PI_DELAY], PANEL_ITEM_RECT);
  value_x = xv_get (X->pitems[(int) PI_DELAY], PANEL_VALUE_X);

  XV_CREATE(X->panel[(int) P_MAIN], PANEL_MESSAGE,
	    PANEL_LABEL_X,	    rect->r_left + rect->r_width + gap,
            PANEL_LAYOUT,           PANEL_HORIZONTAL,
            PANEL_LABEL_STRING,     Pstrs[(int) TIMER],
            XV_HELP_DATA,           hstr[(int) H_DELAY],
            0) ;

  X->pitems[(int) PI_BELL] = xv_create(X->panel[(int) P_MAIN], PANEL_CHECK_BOX,
			       PANEL_VALUE_X, value_x,
                               PANEL_VALUE_Y, P_ROW(P_MAIN, MR_BELL),
                               PANEL_CHOICE_STRINGS,
                                 mpstrs[(int) PI_BELL],
                                 0,
                               PANEL_VALUE,    0,
                               PANEL_INACTIVE, (v->snapshot_delay == 0),
                               XV_HELP_DATA,   hstr[(int) H_BELL],
                               0) ;
   
  X->pitems[(int) PI_HIDEWIN] =
                            xv_create(X->panel[(int) P_MAIN], PANEL_CHECK_BOX,
			      PANEL_VALUE_X,	  value_x,
                              PANEL_NOTIFY_PROC,  set_hide_choice,
                              PANEL_VALUE_Y,      P_ROW(P_MAIN, MR_HIDE),
                              PANEL_CHOICE_STRINGS,
                                mpstrs[(int) PI_HIDEWIN],
                                0,
                              PANEL_VALUE,        0,
                              XV_HELP_DATA,       hstr[(int) H_HIDEWIN],
                              0) ;

  X->pitems[(int) PI_SNAP] = xv_create(X->panel[(int) P_MAIN], PANEL_BUTTON,
	    PANEL_ITEM_X,	    P_COL(P_MAIN, 0),
            PANEL_ITEM_Y,           P_ROW(P_MAIN, MR_SNAP_VIEW),
            PANEL_LABEL_STRING,     Pstrs[(int) DO_SNAP],
            PANEL_NOTIFY_PROC,      do_snap_proc,
            XV_HELP_DATA,           hstr[(int) H_SNAP],
            0) ;

  X->pitems[(int) PI_VIEW] = xv_create(X->panel[(int) P_MAIN], PANEL_BUTTON,
            PANEL_ITEM_Y,           P_ROW(P_MAIN, MR_SNAP_VIEW),
            PANEL_LABEL_STRING,     Pstrs[(int) VIEW_SNAP],
            PANEL_NOTIFY_PROC,      view_snap_proc,
            XV_HELP_DATA,           hstr[(int) H_VIEW],
            0) ;

#ifdef KYBDACC
  xv_set (X->frames [(int) F_MAIN], FRAME_MENU_ADD, print_menu, NULL);
#endif

  window_fit(X->panel[(int) P_MAIN]) ;
  ds_center_items(X->panel[(int) P_MAIN], MR_SNAP_VIEW,
		  X->pitems[(int) PI_SNAP], X->pitems[(int) PI_VIEW], NULL);
}


static void
make_print_panel()   /* Create snapshot popup print window plus items. */
{
  char *printer_name ;
  Xv_font item_font;
  Font_string_dims dims;
  Rect *rect;
  int longest;
  int gap;
  int item_width;
  char home_dir [MAXPATHLEN];

  if (X->frames[(int) F_PRINT] != NULL) return ;

  X->frames[(int) F_PRINT] = xv_create(X->frames[(int) F_MAIN], FRAME_CMD,
#ifdef OW_I18N
					WIN_USE_IM, TRUE,
#endif
                                       FRAME_LABEL, Pstrs[(int) P_LABEL],
                                       0) ;

  X->panel[(int) P_PRINT] = xv_get(X->frames[(int) F_PRINT], FRAME_CMD_PANEL) ;

  gap = xv_get (X->panel[(int) P_PRINT], PANEL_ITEM_X_GAP);

  X->pitems[(int) PI_DEST] = xv_create(X->panel[(int) P_PRINT], PANEL_CHOICE,
                               PANEL_NOTIFY_PROC,    snap_destination_proc,
                               PANEL_VALUE_Y, P_ROW(P_PRINT, PR_DEST),
                               PANEL_LABEL_STRING,   mpstrs[(int) PI_DEST],
			       PANEL_LABEL_BOLD,     TRUE,
                               PANEL_CHOICE_STRINGS,
                                 Pstrs[(int) DEST1],
                                 Pstrs[(int) DEST2],
                                 0,
                               XV_HELP_DATA,         hstr[(int) H_DEST],
                               0) ;

  item_font = (Xv_font) xv_get (X->pitems[(int) PI_DEST], PANEL_LABEL_FONT);

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_DEST], &dims);
  longest = dims.width;

  X->pitems[(int) PI_PNAME] = xv_create(X->panel[(int) P_PRINT], PANEL_TEXT,
                                PANEL_VALUE_Y, P_ROW(P_PRINT, PR_PNAME_DIR),
                                PANEL_LABEL_STRING, mpstrs[(int) PI_PNAME],
                                PANEL_LABEL_BOLD,           TRUE,
                                PANEL_VALUE_STORED_LENGTH,  80,
                                PANEL_VALUE_DISPLAY_LENGTH, 16,
                                XV_HELP_DATA, hstr[(int) H_PNAME],
                                0) ;

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_PNAME], &dims);
  if (dims.width > longest)
     longest = dims.width;

#ifdef SVR4

  if ((printer_name = getenv("LPDEST")) != NULL)
    XV_SET(X->pitems[(int) PI_PNAME], PANEL_VALUE, printer_name, 0) ;
  else {
    FILE *fp;
    char buf[256];
    fp = popen ("lpstat -d", "r");
    if (fp) {
       fread (buf, MAXPATHLEN, 1, fp);
       if (strchr (buf, ':') != NULL) {
	  printer_name = (char *) strtok (buf, ":");
	  printer_name = (char *) strtok ((char *) NULL, "\n");
	  XV_SET(X->pitems[(int) PI_PNAME], PANEL_VALUE, printer_name, 0) ;
	  }
       pclose (fp);
       }
    } 

#else

  if ((printer_name = getenv("PRINTER")) != NULL)
    XV_SET(X->pitems[(int) PI_PNAME], PANEL_VALUE, printer_name, 0) ;
  else
    XV_SET(X->pitems[(int) PI_PNAME], PANEL_VALUE, Pstrs[(int) DEF_PRINTER],
									0) ;
#endif /* SVR4 */

  expand_path ("~", home_dir);
  X->pitems[(int) PI_PDIR] = xv_create(X->panel[(int) P_PRINT], PANEL_TEXT,
                               PANEL_VALUE_Y, P_ROW(P_PRINT, PR_PNAME_DIR),
                               PANEL_LABEL_STRING, mpstrs[(int) PI_PDIR],
                               PANEL_LABEL_BOLD,           TRUE,
                               PANEL_VALUE_STORED_LENGTH,  80,
                               PANEL_VALUE_DISPLAY_LENGTH, 16,
                               PANEL_VALUE,                home_dir,
                               PANEL_SHOW_ITEM, FALSE,
                               XV_HELP_DATA,    hstr[(int) H_PFILE],
                               0) ;
 
  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_PDIR], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_PFILE] = xv_create(X->panel[(int) P_PRINT],  PANEL_TEXT,
                                PANEL_VALUE_Y, P_ROW(P_PRINT, PR_PFILE),
                                PANEL_LABEL_STRING, mpstrs[(int) PI_PFILE],
                                PANEL_LABEL_BOLD,           TRUE,
                                PANEL_VALUE_STORED_LENGTH,  80,
                                PANEL_VALUE_DISPLAY_LENGTH, 16,
                                PANEL_VALUE,                "",
                                PANEL_SHOW_ITEM, FALSE,
                                XV_HELP_DATA,    hstr[(int) H_PFILE],
                                0) ;
 
  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_PFILE], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_PORIENT] = xv_create(X->panel[(int) P_PRINT], PANEL_CHOICE,
                          PANEL_VALUE_Y,        P_ROW(P_PRINT, PR_PORIENT),
                          PANEL_LABEL_STRING,   mpstrs[(int) PI_PORIENT],
                          PANEL_LABEL_BOLD,     TRUE,
                          PANEL_CHOICE_STRINGS,
                            Pstrs[(int) ORIENTU],
                            Pstrs[(int) ORIENTS],
                            0,
                          XV_HELP_DATA,         hstr[(int) H_ORIENT],
                          0) ;

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_PORIENT], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_POSITION] = xv_create(X->panel[(int) P_PRINT], PANEL_CHOICE,
                          PANEL_VALUE_Y, P_ROW(P_PRINT, PR_POS),
                          PANEL_LABEL_STRING, mpstrs[(int) PI_POSITION],
                          PANEL_LABEL_BOLD,           TRUE,
                          PANEL_NOTIFY_PROC,    specify_position_proc,
                          PANEL_CHOICE_STRINGS,
                            Pstrs[(int) CENTER],
                            Pstrs[(int) SPECIFY],
                            0,
                          PANEL_VALUE,                0,
                          XV_HELP_DATA,               hstr[(int) H_POS],
			  0);

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_POSITION], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_LEFTPOSV] = xv_create(X->panel[(int) P_PRINT], PANEL_TEXT,
			  PANEL_VALUE_X, P_COL(P_PRINT, PR_PORIENT),
                          PANEL_VALUE_Y, P_ROW(P_PRINT, PR_SPEC),
                          PANEL_LABEL_BOLD,           TRUE,
                          PANEL_VALUE_STORED_LENGTH,  80,
                          PANEL_VALUE_DISPLAY_LENGTH, 5,
                          PANEL_VALUE,                Pstrs[(int) DEF_LEFTP],
                          PANEL_INACTIVE,             TRUE,
                          XV_HELP_DATA,               hstr[(int) H_POS],
                          0) ;
 
  X->pitems[(int) PI_LEFTPOSM] = xv_create(X->panel[(int) P_PRINT], PANEL_MESSAGE,
            PANEL_LABEL_STRING,      Pstrs[(int) PLPOS],
            PANEL_INACTIVE,             TRUE,
            XV_HELP_DATA,            hstr[(int) H_POS],
            0) ;
 
  X->pitems[(int) PI_BOTPOSV] = xv_create(X->panel[(int) P_PRINT], PANEL_TEXT,
                                  PANEL_VALUE_STORED_LENGTH,  80,
                                  PANEL_VALUE_DISPLAY_LENGTH, 5,
                                  PANEL_VALUE,    Pstrs[(int) DEF_BOTP],
				  PANEL_INACTIVE,             TRUE,
                                  XV_HELP_DATA,   hstr[(int) H_POS],
                                  0) ;
 
  X->pitems[(int) PI_BOTPOSM] = xv_create(X->panel[(int) P_PRINT], PANEL_MESSAGE,
            PANEL_LABEL_STRING,      Pstrs[(int) PBPOS],
            PANEL_INACTIVE,             TRUE,
            XV_HELP_DATA,            hstr[(int) H_POS],
            0) ;
 
  X->pitems[(int) PI_SCALE] = xv_create(X->panel[(int) P_PRINT], PANEL_CHOICE,
                                PANEL_VALUE_Y, P_ROW(P_PRINT, PR_SCALE),
                                PANEL_LABEL_BOLD,     TRUE,
                                PANEL_LABEL_STRING,   mpstrs[(int) PI_SCALE],
                                PANEL_NOTIFY_PROC,    snap_scaleto_proc,
                                PANEL_CHOICE_STRINGS,
                                  Pstrs[(int) SCALE1],
                                  Pstrs[(int) SCALE2],
                                  Pstrs[(int) SCALE3],
                                  Pstrs[(int) SCALE4],
                                  0,
                                XV_HELP_DATA,         hstr[(int) H_APPEAR],
                                0) ;
                                 
  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_SCALE], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_WIDTHV] = xv_create(X->panel[(int) P_PRINT], PANEL_TEXT,
                        PANEL_VALUE_Y,  P_ROW(P_PRINT, PR_W_H),
                        PANEL_LABEL_STRING,         mpstrs[(int) PI_WIDTHV],
                        PANEL_LABEL_BOLD,           TRUE,
                        PANEL_VALUE_STORED_LENGTH,  80,
                        PANEL_VALUE_DISPLAY_LENGTH, 3,
                        PANEL_VALUE,                Pstrs[(int) DEF_WIDTH],
                        PANEL_INACTIVE,             TRUE,
                        XV_HELP_DATA,               hstr[(int) H_APPEAR],
                        0) ; 
                                 
  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_WIDTHV], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_DBITS] = xv_create(X->panel[(int) P_PRINT], PANEL_CHOICE,
                                PANEL_LABEL_STRING,   mpstrs[(int) PI_DBITS],
                                PANEL_LABEL_BOLD,     TRUE,
                                PANEL_VALUE_Y, P_ROW(P_PRINT, PR_DBITS),
                                PANEL_CHOICE_STRINGS,
                                  Pstrs[(int) DBITS1],
                                  Pstrs[(int) DBITS2],
                                  0,
                                XV_HELP_DATA,         hstr[(int) H_DOUBLE],
                                0) ;

  xv_get (item_font, FONT_STRING_DIMS, mpstrs[(int) PI_DBITS], &dims);
  if (dims.width > longest)
     longest = dims.width;

  X->pitems[(int) PI_PRBUT] = xv_create(X->panel[(int) P_PRINT], PANEL_BUTTON,
            PANEL_ITEM_Y,            P_ROW(P_PRINT, PR_PRINT),
            PANEL_LABEL_STRING,      Pstrs[(int) PBUTTON],
            PANEL_NOTIFY_PROC,       print_proc,
            XV_HELP_DATA,            hstr[(int) H_PBUTTON],
            0) ;

  xv_set (X->pitems[(int) PI_DEST], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_PNAME], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_PDIR], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_PFILE], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_PORIENT], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_POSITION], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_SCALE], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);
  xv_set (X->pitems[(int) PI_WIDTHV], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);

  rect = (Rect *) xv_get (X->pitems[(int) PI_WIDTHV], PANEL_ITEM_RECT);

  X->pitems[(int) PI_WIDTHT] = xv_create(X->panel[(int) P_PRINT], PANEL_MESSAGE,
				 PANEL_LABEL_X,	      rect->r_left + 
						      rect->r_width + gap,
                                 PANEL_LABEL_Y,	      P_ROW(P_PRINT, PR_W_H),
                                 PANEL_LABEL_STRING,  mpstrs[(int) PI_WIDTHT],
                                 PANEL_INACTIVE,      TRUE,
                                 XV_HELP_DATA,        hstr[(int) H_APPEAR],
                                 0) ; 
                                 
  rect = (Rect *) xv_get (X->pitems[(int) PI_WIDTHT], PANEL_ITEM_RECT);

  X->pitems[(int) PI_HEIGHTV] = xv_create(X->panel[(int) P_PRINT], PANEL_TEXT,
			 PANEL_LABEL_X,  rect->r_left + rect->r_width + gap,
                         PANEL_LABEL_Y,  P_ROW(P_PRINT, PR_W_H),
                         PANEL_LABEL_STRING,         mpstrs[(int) PI_HEIGHTV],
                         PANEL_LABEL_BOLD,           TRUE,
                         PANEL_VALUE_STORED_LENGTH,  80,
                         PANEL_VALUE_DISPLAY_LENGTH, 3,
                         PANEL_VALUE,                Pstrs[(int) DEF_HEIGHT],
                         PANEL_INACTIVE,             TRUE,
                         XV_HELP_DATA,               hstr[(int) H_APPEAR],
                         0) ;

  rect = (Rect *) xv_get (X->pitems[(int) PI_HEIGHTV], PANEL_ITEM_RECT);

  X->pitems[(int) PI_HEIGHTT] = xv_create(X->panel[(int) P_PRINT],PANEL_MESSAGE,
				  PANEL_LABEL_X,      rect->r_left +
						      rect->r_width + gap,
                                  PANEL_LABEL_Y,      P_ROW(P_PRINT, PR_W_H),
                                  PANEL_LABEL_STRING, mpstrs[(int) PI_HEIGHTT],
                                  PANEL_INACTIVE,     TRUE,
                                  XV_HELP_DATA,       hstr[(int) H_APPEAR],
                                  0) ;

  xv_set (X->pitems[(int) PI_DBITS], PANEL_VALUE_X,
		((MARGIN_OFFSET + 1) * gap) + longest, NULL);

  rect = (Rect *) xv_get (X->pitems[(int) PI_DBITS], PANEL_ITEM_RECT);

  X->pitems[(int) PI_MONO] = xv_create(X->panel[(int) P_PRINT], PANEL_CHECK_BOX,
			       PANEL_VALUE_X,  rect->r_left + rect->r_width
							    + gap,
			       PANEL_VALUE_Y,  P_ROW(P_PRINT, PR_DBITS),
                               PANEL_CHOICE_STRINGS,
                                 Pstrs[(int) MONO],
                                 0,
                               PANEL_VALUE,    1,
                               XV_HELP_DATA,   hstr[(int) H_MONO],
                               0) ;

  window_fit(X->panel[(int) P_PRINT]) ;
  ds_center_items(X->panel[(int) P_PRINT], PR_PRINT, 
		  X->pitems[(int) PI_PRBUT], NULL);
  window_fit(X->frames[(int) F_PRINT]) ;
}


void
message(message)
char *message ;
{
  XV_SET(X->frames[(int) F_MAIN], FRAME_LEFT_FOOTER, message, 0) ;
  XSync(X->dpy, FALSE) ;
}


/*ARGSUSED*/
static void
print_options(item, event)
Panel_item item ;
Event *event ;
{
  static int position_popup = 1;
/*
  make_print_panel() ;    Create print snapshot panel plus items. 
*/
  if (!xv_get(X->frames[(int) F_PRINT], XV_SHOW))
    {
      if (position_popup)
         ds_position_popup(X->frames[(int) F_MAIN],
                           X->frames[(int) F_PRINT], DS_POPUP_LOR) ;
      frame_show(F_PRINT, TRUE) ;
    }
  position_popup = 0;
}


/*ARGSUSED*/
static void
print_proc(item, event)
Panel_item item ;
Event *event ;
{
  print_snapfile() ;
}


void
save_cmdline(argc, argv)
int argc ;
char *argv[] ;
{
  XV_SET(X->frames[(int) F_MAIN], FRAME_WM_COMMAND_ARGC_ARGV, argc, argv, 0) ;
}


/*ARGSUSED*/
static void
save_snap_proc(item, event)
Panel_item item ;
Event *event ;
{
  static int position_popup = 1;

#ifdef FILECHOOSER
  make_save_filechooser();
#else
  make_ls_panel() ;     /* Create load/save snapshot panel plus items. */
#endif

#ifdef FILECHOOSER
  if (xv_get (X->frames[(int) F_SAVE], XV_SHOW) == FALSE) {
    if (position_popup)
        ds_position_popup (X->frames[(int) F_MAIN],
			   X->frames[(int) F_SAVE], DS_POPUP_LOR);
  }
/*
 * Re-read the directory then show the filechooser.
 */ 
  xv_set (X->frames[(int) F_SAVE], FILE_CHOOSER_UPDATE, NULL);
  frame_show (F_SAVE, TRUE);
#else
  XV_SET(X->frames[(int) F_LOADSAVE], FRAME_LABEL, Pstrs[(int) S_LABEL], 0) ;
  XV_SET(X->pitems[(int) PI_LSBUT], PANEL_LABEL_STRING, ostr[(int) O_SAVE], 0) ;
  if (!xv_get(X->frames[(int) F_LOADSAVE], XV_SHOW))
    {
      if (position_popup)
         ds_position_popup(X->frames[(int) F_MAIN],
                           X->frames[(int) F_LOADSAVE], DS_POPUP_BELOW) ;
      frame_show(F_LOADSAVE, TRUE) ;
    }
#endif

  position_popup = 0;
}


/*ARGSUSED*/
static void
set_hide_choice(item, value, event)
Panel_item item ;
int value ;
Event *event ;
{
  v->hide = value ;

  if (v->hide)
    {
      delay_choice(X->pitems[(int) PI_DELAY], 0, NULL) ;
      window_bell (X->frames[(int) F_MAIN]);
      message(mstr[(int) M_ADELAY]) ;
    }
}


int
set_image_colormap(image)
image_t *image ;
{
  Colormap cmap ;
  Display *display ;
  XColor xcolors[CMAP_SIZE] ;
  int i ;
  int cmap_size = CMAP_SIZE;

  if (v->depth == 4)
     cmap_size = 16;
  else if (v->depth == 8)
     cmap_size = X->visual->map_entries;

  display = (Display *) xv_get(X->frames[(int) F_ROOT], XV_DISPLAY) ;

/*
  cmap = xv_get (xv_get (X->canvas, WIN_CMS), CMS_CMAP_ID);
*/
 
  for (i = 0; i <= cmap_size - 1; i++) xcolors[i].pixel = i ;
  XQueryColors(display, X->cmap, xcolors, cmap_size) ;

  for (i = 0; i < cmap_size; i++)
    {
      image->red[i]   = xcolors[i].red   >> 8 ;
      image->green[i] = xcolors[i].green >> 8 ;
      image->blue[i]  = xcolors[i].blue  >> 8 ;
    }
  return(cmap_size) ;
}


void
set_label(ftype, label)
enum frame_type ftype ;
char *label ;
{
  XV_SET(X->frames[(int) ftype], FRAME_LABEL, label, 0) ;
}


void
set_time_delay(tv)
struct itimerval *tv ;
{
  notify_set_itimer_func((Notify_client) &X->handle, time_delay,
                         ITIMER_REAL, tv, 0) ;
}


void
show_item(pitype, val)     /* Show/hide a panel item. */
enum pi_type pitype ;
int val ;
{
  XV_SET(X->pitems[(int) pitype], XV_SHOW, val, 0) ;
}


/*ARGSUSED*/
static void
snap_destination_proc(item, value, event)
Panel_item  item ;
int value ;
Event *event ;
{
  do_destination(value) ;
}


/*ARGSUSED*/
static void
specify_position_proc(item, value, event)
Panel_item item ;
int value ;
Event *event ;
{
  do_position(value) ;
}


/*ARGSUSED*/
static void
snap_scaleto_proc(item, value, event)
Panel_item item ;
int value ;
Event *event ;
{
  do_scaleto(value) ;
}


void
sound_bell(tval)     /* Ring that bell... */
struct timeval tval ;
{
  win_bell(X->frames[(int) F_MAIN], tval, 0) ;
}


void
start_tool()    /* Setup event handlers and start application. */
{
  window_fit(X->frames[(int) F_MAIN]) ;

  dstt_xview_start_notifier ();
  xv_main_loop(X->frames[(int) F_MAIN]) ;
}


/*  Call-back proc registered with the notifier to provide a time-delayed
 *  screendump.
 */


static Notify_value
time_delay()
{
  do_delay() ;
  return(NOTIFY_DONE) ;
}


int
warn(s)        /* Warn the user */
char *s ;
{
  Event ie ;
  int status ;

  status = notice_prompt(X->frames[(int) F_MAIN], &ie,
                         NOTICE_MESSAGE_STRINGS, s, 0,
                         NOTICE_BUTTON_YES,      ostr[(int) O_CONTINUE],
                         0) ;
  return((status == NOTICE_YES) ? TRUE : FALSE) ;
}


/*ARGSUSED*/
static void
view_snap_proc(item, event)
Panel_item item ;
Event *event ;
{
  if (X->ximage == NULL)
    {
      message(mstr[(int) M_NOVIMAGE]) ;
      return ;
    }

  if (v->no_imagetool == TRUE)
     old_viewfile ();
  else
     viewfile() ;
}


void
setbusy ()
{
    Frame        frame;
    int          n = 1;

    xv_set (X->frames [(int) F_MAIN], FRAME_BUSY, TRUE, NULL);
    while (frame = xv_get (X->frames [(int) F_MAIN], FRAME_NTH_SUBFRAME, n++))
          xv_set (frame, FRAME_BUSY, TRUE, NULL);
}

void
setactive ()
{
    Frame        frame;
    int          n = 1;

    while (frame = xv_get (X->frames[(int) F_MAIN], FRAME_NTH_SUBFRAME, n++))
          xv_set (frame, FRAME_BUSY, FALSE, NULL);
    xv_set (X->frames [(int) F_MAIN], FRAME_BUSY, FALSE, NULL);

}

int
display_callback (ttmsg, key, status, media, datatype, data, size,
                  messageid, title)
Tt_message 	 ttmsg;
void 		*key;
int 		 status;
char 		*media;
Data_t 		 datatype;
void 		*data;
int 		 size;
char 		*messageid;
char 		*title;
{

/*
 * Get the message status. If it's quit or discontinue, then
 * imagetool is going away.
 */

    if (dstt_test_status (ttmsg) == dstt_status_user_request_cancel) {
       free (v->ttid);
       v->ttid = (char *) NULL;
 
/*
 * Turn back on the panel items.
 */

       xv_set (load_button, PANEL_INACTIVE, FALSE, NULL);
       xv_set (save_button, PANEL_INACTIVE, FALSE, NULL);
       xv_set (print_button, PANEL_INACTIVE, FALSE, NULL);
       xv_set (X->pitems [(int) PI_DRAG], PANEL_INACTIVE, FALSE, NULL);
       setactive ();
       return (OK); 
       } 

   
    switch (tt_message_state (ttmsg)) {
       case TT_REJECTED: 
       case TT_FAILED:   
	    message (mstr [(int) M_NOLAUNCH]);
	    v->no_imagetool = TRUE;
	    old_viewfile ();
       	    xv_set (load_button, PANEL_INACTIVE, FALSE, NULL);
       	    xv_set (save_button, PANEL_INACTIVE, FALSE, NULL);
       	    xv_set (print_button, PANEL_INACTIVE, FALSE, NULL);
       	    xv_set (X->pitems [(int) PI_DRAG], PANEL_INACTIVE, FALSE, NULL);
    	    setactive ();
       }

    return (OK);
}

void
viewfile()
{

/*
 * 1: If imagetool not launched:
 *    a:  launch it now.
 *    b:  Gray out load/save/print buttons.
 * 2: Send image data.
 */
 
  setbusy ();
  if (v->havename) {
     if (v->filetype == RASTERFILE) 
        dstt_display (display_callback, v->tool_name, DGET ("Sun_Raster"), path,
                      v->path, strlen (v->path), NULL, NULL);
     else
        dstt_display (display_callback, v->tool_name, DGET ("GIF"), path,
                      v->path, strlen (v->path), NULL, NULL);
     }
  else {
     savefile (v->path, R_DATA, 0);
     dstt_display (display_callback, v->tool_name, DGET ("Sun_Raster"), 
		   contents, v->rbuf, get_raster_len (v->image), NULL, NULL);
     }
 
  if (v->ttid == (char *) NULL)
     message(mstr[(int) M_LAUNCH]) ;

}

void
old_viewfile ()
{
  int h, w ;
  static position_popup = 1;

  create_viewwin() ;            /* Create viewing window canvas. */

/*  It's a little bit anti-social to fill up the whole screen. If the size
 *  of the image would do this, then make the view window slightly smaller.
 *
 *  Also, used to add 5 more to the height (to get around an xview bug
 *  I think), but would cause garbage to be displayed at the bottom of the
 *  image, so removed the extra 5 pixels...
 *    used to be:  h = X->rect[(int) R_BOX].r_height + 5 + 
 */

  h = X->rect[(int) R_BOX].r_height + 
      (int) xv_get(X->frames[(int) F_VIEW], WIN_TOP_MARGIN) +
      (int) xv_get(X->frames[(int) F_VIEW], WIN_BOTTOM_MARGIN) + 
      (2 * WIN_DEFAULT_BORDER_WIDTH) ;
  
  if (h >= DisplayHeight(X->dpy, X->screen))
    h = DisplayHeight(X->dpy, X->screen) - 50 ;
  XV_SET(X->frames[(int) F_VIEW], XV_HEIGHT, h, 0) ;

  w = X->rect[(int) R_BOX].r_width +
      (int) xv_get(X->frames[(int) F_VIEW], WIN_LEFT_MARGIN) +
      (int) xv_get(X->frames[(int) F_VIEW], WIN_RIGHT_MARGIN) + 
      (2 * WIN_DEFAULT_BORDER_WIDTH) ;

  if (w >= DisplayWidth(X->dpy, X->screen))
    w = DisplayWidth(X->dpy, X->screen) - 50 ;
  XV_SET(X->frames[(int) F_VIEW], XV_WIDTH, w, 0) ;

  if ((int) xv_get(X->frames[(int) F_VIEW], XV_SHOW) == FALSE)
    {
      if (position_popup)
         ds_position_popup(X->frames[(int) F_MAIN],
                           X->frames[(int) F_VIEW], DS_POPUP_LOR) ;
      XV_SET(X->frames[(int) F_VIEW], XV_SHOW, TRUE, 0) ; 
    }
  else
    draw_view_canvas(X->canvas, canvas_paint_window(X->canvas),
                     X->dpy, X->vxid, (Xv_xrectlist *) NULL) ;
  position_popup = 0;
}


static void
xerror_interpose(display, error)
Display *display ;
XErrorEvent *error ;
{
  char msg[80] ;

  XGetErrorText(display, error->error_code, msg, 80) ;
  FPRINTF(stderr, DGET("\nX Error (intercepted): %s\n"), msg) ;
  FPRINTF(stderr, DGET("Major Request Code   : %d\n"),   error->request_code) ;
  FPRINTF(stderr, DGET("Minor Request Code   : %d\n"),   error->minor_code) ;
  FPRINTF(stderr, DGET("Resource ID (XID)    : %u\n"),   error->resourceid) ;
  FPRINTF(stderr, DGET("Error Serial Number  : %u\n"),   error->serial) ;
  abort() ;
}
