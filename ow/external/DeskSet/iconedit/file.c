#ifdef SVR4
#include <unistd.h>    /* R_OK    4       Test for Read permission */
#else
#include <sys/file.h> /* R_OK    4       Test for Read permission */
#endif /* SVR4 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/svrimage.h>
#include <xview/frame.h>
#include <xview/notice.h>
#include <xview/cms.h>
#include <xview/file_chsr.h>
#include "gdd.h"
#include "ds_popup.h"
#include "base_gui.h"
#include "base.h"
#include "file_gui.h"
#include "xpm.h"
#include "resources.h"

#define OK              0
#define ERROR           -1

#define MAX_UNDOS 8

#define MAX_FOOTER_LENGTH 64

#define COLOR 0
#define MONO 1

/* declare some file choosing thingie... */
extern File_chooser load_dialog;
extern File_chooser save_dialog;
extern Xv_opaque format_item;

extern base_window_objects     *base_window;

extern GC gc, gc_rv, redraw_gc;
extern Colormap cmap;
extern Display *dpy;
extern Window preview_xid, big_bit_xid;
extern Pixmap preview_pixmap, big_pixmap;
extern XImage *edit_image;
extern int screen_depth;
extern int	canvas_dimension;
extern int	positioned;

extern char *gettext();

extern Undo_struct undo_images[MAX_UNDOS];

extern Vars v;

extern int current_undo, undos_allowed, redos_allowed;
extern int Edited;
extern int grid_status;
extern int current_color_state;
extern XColor black, white, current_pen;
Pixmap printing_pixmap, bitplane_pixmap;
extern int boarder_upper, boarder_lower, boarder_left, boarder_right, height_block, width_block;
extern int icon_height, icon_width, preview_boarder_upper, preview_boarder_left;
extern void do_that_base_repaint_thing();
extern void set_select_cliprects();
extern void selection_off();

char    name[MAXPATHLEN];
char    *errmsg;
char    other_error_message[256]; /* This is really brain dead - icon_load_svrim requires it... */
extern char    *sys_errlist[];
FILE    *fd;

extern char * get_dir();
extern char * get_file();
extern int file_load();
extern int write_xbm_format();
extern int write_icon_format();
extern int    write_pixmap_format();

extern void change_stuff_to_mono();
extern void change_stuff_to_color();
extern void change_save_default();
extern int do_that_save_thing();

char            dir[MAXPATHLEN];
char            loaded_file[MAXPATHLEN] = "";
char            *file = NULL;

int             save_format;

/*
 * Set the current directory.
 */
void
set_load_dir(path)
        char           *path;
{ 
    if (load_dialog)
      xv_set(load_dialog, FILE_CHOOSER_DIRECTORY, path, NULL);
}
void
set_save_dir(path)
        char           *path;
{ 
    if (save_dialog)
      xv_set(save_dialog, FILE_CHOOSER_DIRECTORY, path, NULL);
}
char *
get_save_dir()
{ 
    static char bar[MAXPATHLEN];

    if (!save_dialog)
      bar[0] = NULL;
    else
      strcpy(bar, (char *) xv_get(save_dialog, FILE_CHOOSER_DIRECTORY));
    return bar;
}
char *
get_save_file()
{ 
    static char bar[MAXPATHLEN];
    if (!save_dialog)
      bar[0] = NULL;
    else
      strcpy(bar, (char *) xv_get(save_dialog, FILE_CHOOSER_DOC_NAME));
    return bar;
}
char *
get_truncated_file_name()
{
    char *name, *walk;

    if (!load_dialog)
      return(NULL);

    name =  (char *) xv_get(load_dialog, FILE_CHOOSER_DOC_NAME);
    if (!name)
       name = file;
    if (!name)
       name =  (char *) xv_get(save_dialog, FILE_CHOOSER_DOC_NAME);
    
    if ((name) && (*name != ' '))
      {
	  walk = name + strlen(name);
	  do { 
	      walk-- ;
	  }	  while ((*walk != '/') && (name <= walk)) ;	    
	  walk = walk +1;
	  return walk;
      }
    else 
      return NULL;
}
    
void
set_dir(path)
        char           *path;
{
/* do nothing? */
}

/*
 * Set the current file.
 */
void
set_file(name)
        char           *name;
{
    if (save_dialog)
      xv_set(save_dialog, FILE_CHOOSER_DOC_NAME, name, NULL);
    if (load_dialog)
      xv_set(load_dialog, FILE_CHOOSER_DOC_NAME, name, NULL);
}

/*
 * Set the left footer.
 */
void
set_left_footer(string)
     char              *string;
{
/* Do nothing I hope */
}

/*
 * Set the right footer.
 */
void
set_right_footer(string)
     char              *string;
{
/* Do nothing I hope */
}

/*
 * Get the current directory.
 */
char *
get_dir()
{
/* NO OP */
}

/*
 * Get the current file.
 */
char *
get_file()
{
}

/*
 * Expand a path in place.  Returns OK if successful, otherwise sets errno
 * and returns ERROR.
 */
int
file_expand_path(path)
        char           *path;
{
        char            buf[MAXPATHLEN];

        ds_expand_pathname(path, buf);
        strcpy(path, buf);
        return OK;
}

/*
 * initialize the file popup - set the directory to cwd and move
 * the cursor to the file line...
 */

void
file_initialize()
{


  if (!printing_pixmap)
    {
      printing_pixmap = XCreatePixmap(dpy, preview_xid, 128, 128, screen_depth);
    }
}

/*
 * Put up a notice window for the open window.
 */
void
pop_notice(msg)
        char           *msg;
{
        Event           event;

        notice_prompt(base_window->window, &event,
                      NOTICE_MESSAGE_STRINGS, msg, 0,
                      NOTICE_BUTTON_YES,  gettext("Continue") ,
                      0);
}

/*
 * Menu handler for `save_format_menu'.
 */
Menu
save_file(menu, op)
	Menu		menu;
	Menu_generate	op;
{
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return menu;
}

/*
 * Menu handler for `save_format_menu (XView Icon)'.
 */
Menu_item
save_icon(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	file_popup_objects * ip = (file_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		if (write_icon_format() != 0)
			xv_set(xv_get(item, MENU_PARENT), MENU_NOTIFY_STATUS, XV_ERROR, 0);
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

/*
 * Menu handler for `save_format_menu (X Bitmap)'.
 */
Menu_item
save_xbm(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	file_popup_objects * ip = (file_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		if (write_xbm_format() != 0)
			xv_set(xv_get(item, MENU_PARENT), MENU_NOTIFY_STATUS, XV_ERROR, 0);
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

/*
 * Menu handler for `save_format_menu (Color X Pixmap)'.
 */
Menu_item
save_c_xpm(item, op)
	Menu_item	item;
	Menu_generate	op;
{
  
  file_popup_objects * ip = (file_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
  
  switch (op) {
  case MENU_DISPLAY:
    break;
    
  case MENU_DISPLAY_DONE:
    break;
    
  case MENU_NOTIFY:
    if (write_pixmap_format() != 0) 
		xv_set(xv_get(item, MENU_PARENT), MENU_NOTIFY_STATUS, XV_ERROR, 0);
    break;
    
  case MENU_NOTIFY_DONE:
    break;
  }
  return item;
}

/*
 * Menu handler for `save_format_menu (Mono X Pixmap)'.
 */
Menu_item
save_m_xpm(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	file_popup_objects * ip = (file_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		if (write_pixmap_format() != 0)
			xv_set(xv_get(item, MENU_PARENT), MENU_NOTIFY_STATUS, XV_ERROR, 0);
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}


/* Do nothing I hope... */


/*
 * Event callback function for `load_op_steeing'.
 */
void
set_load_op(item, event)
	Panel_item	item;
	Event		*event;
{
/* set the detault save format */
	panel_default_handle_event(item, event);
}

int 
file_dialog_load(fc, path, exists)
        File_chooser  fc;
        char          *path;
        int           exists;
{
    file = path;
    return(file_load());
}
int 
file_dialog_save(fc, path, exists)
        File_chooser  fc;
        char          *path;
        int           exists;
{
    file = path;
    save_format = xv_get(format_item, PANEL_VALUE) + 1;
    return(do_that_save_thing());
}
/*
 * Notify callback function for `load_bt'.
 */
void
load_file_item(item, event)
	Panel_item	item;
	Event		*event;
{
	file_popup_objects	*ip = (file_popup_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	
	if (file_load())
		xv_set(item, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
}

int
write_pixmap_to_file(fullpath)

char	*fullpath;

{
        int             error_return;
        char            notice_buf[MAXPATHLEN];
        Event           event;
        struct stat     statbuf;

        strcpy(loaded_file, fullpath);

  	if (printing_pixmap)
	  XFreePixmap(dpy, printing_pixmap);

	printing_pixmap = XCreatePixmap(dpy, preview_xid, icon_width, 
					icon_height, screen_depth);

	XCopyArea(dpy, preview_pixmap, printing_pixmap, gc, 
		  preview_boarder_left, preview_boarder_upper, 
		  icon_width, icon_height, 0, 0);

	other_error_message[0] = NULL;

        if (error_return = XWritePixmapFile(dpy, cmap, fullpath, 
				      printing_pixmap,
				      icon_width, icon_height, 
				      NULL, 1, NULL))
	  {
	    switch (error_return) {
	        case PixmapOpenFailed:
		      sprintf(notice_buf,  gettext("Could not open '%s' for writing.") , fullpath);
	              pop_notice(notice_buf);
		      break;
	        case PixmapFileInvalid:
		      sprintf(notice_buf,  gettext("Could not open '%s/%s' for writing.") , name, file);
	              pop_notice(notice_buf);
		      break;
	        case PixmapNoMemory:
		      sprintf(notice_buf,  gettext("Did not have enough memory to write out '%s/%s'.") , name, file);
	              pop_notice(notice_buf);
		      break;
	    }
	    return error_return;
	  }
	Edited = undo_images[current_undo%MAX_UNDOS].edited = 0;
	set_iconedit_footer();
	return(NULL);
}

/*
 * Save the current file.  This function assumes that the user has
 * already set the directory and file name.
 */
int
write_pixmap_format()
{
        char            *errmsg;
	int             error_return;
        char            notice_buf[MAXPATHLEN];
	char            fullpath[MAXPATHLEN];

	file = get_save_file();
	
        if (!file || !*file)
	  {
	    pop_notice( gettext("No file specified"));
	    return (TRUE);
	  }
	
	if(get_save_dir())
	  strcpy(dir, get_save_dir());
	else
	  {
	    getcwd(dir, MAXPATHLEN);
	    set_dir(dir);
	  }

        strcpy(name, dir);
	strcpy(fullpath, dir);
	if ( name[ strlen(name) -1 ] == '/')
	  strcat(fullpath, file);
	else
	  {
	    strcat(fullpath, "/");
	    strcat(fullpath, file);
	  }

	if (error_return = write_pixmap_to_file(fullpath))
		return(error_return);
	
        /*
         * Store was successful.
         */
 
        return (FALSE);
}

void
load_file_misc()
{
  reset_those_pesky_clip_masks();
  set_select_cliprects();
  initialize_grid_lines();
  grid_proc(grid_status);
  selection_off();
  do_that_base_repaint_thing(undo_images[(current_undo)%MAX_UNDOS].image);
  set_iconedit_label();
}

file_load_from_name(file, fullpath)

char	*file;
char	*fullpath;

{
  char            notice_buf[MAXPATHLEN];

  unsigned int height, width;
  unsigned long *pixels_return;
  unsigned int npixles_return;
  ColorSymbol *colorsymbols;
  unsigned int numsymbols;
  Server_image temp_server_image;
  GC bitmap_gc;
  XColor one, two;
   
  unsigned int theight, twidth, border_width, depth;
  int x, y;
  Window root;
  Status status;
  Menu color_menu, edit_menu;
  int error_return;
  int i, items;
  FILE *fp;
  int     answer;
  char MONO_XPM;

  if (Edited)
  {
  	answer = notice_prompt(base_window->window, NULL,
                                NOTICE_MESSAGE_STRINGS,
                                gettext("Your image has been edited.  You may continue,\ndiscarding your edits, or you may cancel your load.\nIf you continue your load, your old image may be restored using Undo."),
                                0,
                                NOTICE_BUTTON_YES, gettext("Cancel"),
                                NOTICE_BUTTON_NO, gettext("Continue"),
                                0);

                if (answer == NOTICE_YES)
                {
                        return(NULL);
                }

  }

  /* discard any load connections thru dragdrop transfer */

  discard_load();

  bitmap_gc = XCreateGC(dpy, big_bit_xid, 0, 0);
  bitplane_pixmap = XCreatePixmap(dpy, preview_xid, 
				  icon_width, icon_height, 1);

  xv_set(base_window->window, FRAME_BUSY, TRUE, 0);

  /* Trying pixmap */
  if (!(error_return = XReadPixmapFile(dpy, 
				       xv_get( base_window->window, XV_VISUAL),
				       preview_pixmap, cmap, file, 
				       screen_depth, &printing_pixmap, 
				       &width, &height,
				       &pixels_return, &npixles_return,
				       colorsymbols, &numsymbols, NULL)))
    {
      if (npixles_return == 2)
	{
/*
	  XWritePixmapFile(dpy, cmap, "/tmp/sean.debug2", 
			 printing_pixmap,
			 width, height,
			 NULL, 1, NULL);
*/
	  one.pixel = pixels_return[0];
	  two.pixel = pixels_return[1];
	  one.flags = DoRed | DoGreen | DoBlue;
	  two.flags = DoRed | DoGreen | DoBlue;

	  XQueryColor(dpy, cmap, &one);
	  XQueryColor(dpy, cmap, &two);

	  if (((one.red == 0) && (two.red == 65535) 
	       && (one.blue == 0) && (two.blue == 65535) 
	       && (one.green == 0) && (two.green== 65535))
	      || ((one.red == 65535) && (two.red == 0) 
	       && (one.blue == 65535) && (two.blue == 0) 
	       && (one.green == 65535) && (two.green== 0)))
	    MONO_XPM = 1;
	  else
	    MONO_XPM = 0;
	} else {
	  MONO_XPM = 0;
	}
      
      
      if ( (screen_depth == 1) && !(MONO_XPM))
	{
	  sprintf(notice_buf,  
		  gettext("'%s' cannot be viewed on a single plane display."),
		  fullpath);
	  pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	  return(TRUE);
	}

      icon_height = height;
      icon_width = width;
      
      if ((width > 128) || (height > 128))
	{
	  sprintf(notice_buf,  
		  gettext("'%s' is too big for iconedit to read."),
		  fullpath);
	  pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	  return(TRUE);
	}
      XFillRectangle( dpy, preview_pixmap, gc_rv, 0 , 0, 138, 138);
      XFillRectangle( dpy, big_pixmap, gc_rv, 0, 0, 
		     canvas_dimension, canvas_dimension); 
      XFillRectangle( dpy, preview_xid, gc_rv, 0 , 0, 138, 138);
      XFillRectangle( dpy, big_bit_xid, gc_rv, 0, 0, 
		     canvas_dimension, canvas_dimension); 
      
      if (MONO_XPM) {
	save_format = 4;
	v->format = 4;
      } else {
	save_format = 3;
	v->format = 3;
      }
      set_the_size_info();	  
      change_save_default(3);

      XCopyArea(dpy, printing_pixmap, preview_pixmap, gc, 
		0, 0, icon_width, icon_height, 
		preview_boarder_left, preview_boarder_upper);  
      
      undo_images[(current_undo+1)%MAX_UNDOS].image = 
	XGetImage(dpy, preview_pixmap, 
		  preview_boarder_left, preview_boarder_upper, 
		  icon_width, icon_height, -1, ZPixmap); 
      
      undo_images[current_undo%MAX_UNDOS].state = current_color_state;
      undo_images[current_undo%MAX_UNDOS].height = icon_height;
      undo_images[current_undo%MAX_UNDOS].width = icon_width;
      current_undo++;
      
      if (undos_allowed < (MAX_UNDOS -1))
	undos_allowed++;
      
      Edited = 0;
      undo_images[current_undo%MAX_UNDOS].edited = 0;

      redos_allowed = 0;
      
      if (undos_allowed == 1)
	{
	  edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	  xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		 MENU_INACTIVE, FALSE, NULL);
	}
	  
      set_iconedit_footer();
      
      if (MONO_XPM) {
	undo_images[current_undo%MAX_UNDOS].state = MONO;
	current_color_state = MONO;
	if (undo_images[(current_undo-1)%MAX_UNDOS].state == COLOR)
	  {
	    change_stuff_to_mono();
	  }
      } else {
	undo_images[current_undo%MAX_UNDOS].state = COLOR;
	current_color_state = COLOR;
	if (undo_images[(current_undo-1)%MAX_UNDOS].state == MONO)
	  {
	    change_stuff_to_color();
	  }
      }	  

      /* Deals with clipmasks, cliprects, grid lines, selections */
      /* Also repaints the base canvas */

      load_file_misc();

      XCopyArea(dpy, preview_pixmap, preview_xid, gc, 0, 0, 138, 138, 0, 0); 

      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
      strcpy(loaded_file, fullpath);
      return(FALSE);
    }
  else
    {
      if (error_return != PixmapFileInvalid)
	switch ( error_return ) {
	case PixmapOpenFailed:
	  sprintf(notice_buf,  
		  gettext("Open failed on '%s'."),
		  fullpath);
          pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
          return(TRUE);
          break;
        case PixmapNoMemory:
	  sprintf(notice_buf,  
		  gettext("Not enough memory to open '%s'."),
		  fullpath);
          pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
          return(TRUE);
          break;
        case PixmapParseColorFailed:
	  sprintf(notice_buf,  
		  gettext("Color parsing failed on '%s'."),
		  fullpath);
          pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
          return(TRUE);
          break;
        case PixmapAllocColorFailed:
	  sprintf(notice_buf,  
		  gettext("Color allocation failed on '%s'."),
		  fullpath);
          pop_notice(notice_buf);
	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
          return(TRUE);
          break;
	}
    }

  /* End of reading pixmap stuff - now trying for a sun icon */

  other_error_message[0] = NULL;
  temp_server_image = 
    (Server_image) icon_load_svrim(fullpath, other_error_message);
 
  if (!other_error_message[0])
    {
      status = XGetGeometry( dpy, 
			    (Pixmap)xv_get(temp_server_image, XV_XID), 
			    &root, &x, &y, &width, &height, 
			    &border_width, &depth);
      
      if (status == 1) 
	{
	  icon_height = height;
	  icon_width = width;
	  set_the_size_info();	  
	  XFillRectangle( dpy, preview_pixmap, gc_rv, 0 , 0, 138, 138);
	  XFillRectangle( dpy, big_pixmap, gc_rv, 0, 0, 
			 canvas_dimension, canvas_dimension); 
	  XFillRectangle( dpy, preview_xid, gc_rv, 0 , 0, 138, 138);
	  XFillRectangle( dpy, big_bit_xid, gc_rv, 0, 0, 
			 canvas_dimension, canvas_dimension); 
	  XSetClipOrigin( dpy, bitmap_gc, 
			 preview_boarder_left, preview_boarder_upper);
	  XSetClipMask( dpy, bitmap_gc, 
		       (Pixmap)xv_get(temp_server_image, XV_XID));
	  XSetForeground(dpy, bitmap_gc, black.pixel);
	  XSetBackground(dpy, bitmap_gc, white.pixel);	  
	  XFillRectangle(dpy, preview_pixmap, bitmap_gc, 0, 0, 138, 138);  
	  undo_images[(current_undo+1)%MAX_UNDOS].image = 
	    XGetImage(dpy, preview_pixmap, 
		      preview_boarder_left, preview_boarder_upper, 
		      icon_width, icon_height, -1, ZPixmap); 
	  undo_images[current_undo%MAX_UNDOS].state = current_color_state;
	  undo_images[current_undo%MAX_UNDOS].height = icon_height;
	  undo_images[current_undo%MAX_UNDOS].width = icon_width;

	  current_undo++;
	  if (undos_allowed < (MAX_UNDOS -1))
	    undos_allowed++;
	  
	  Edited = 0;
	  undo_images[current_undo%MAX_UNDOS].edited = 0;
	  redos_allowed = 0;
	  
	  if (undos_allowed == 1)
	    {
	      edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	      xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		     MENU_INACTIVE, FALSE, NULL);
	    }
	  
	  set_iconedit_footer();
	  undo_images[current_undo%MAX_UNDOS].state = MONO;
	  current_color_state = MONO;	      
	  save_format = 1;
	  v->format = 1;
	  change_save_default(1);
	  if (undo_images[(current_undo-1)%MAX_UNDOS].state == COLOR)
	    {
	      change_stuff_to_mono();
	    }
	  
	  
	  load_file_misc();

	  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
          strcpy(loaded_file, fullpath);
	  return(FALSE);
	}
    }   

  status = XReadBitmapFile( dpy, preview_pixmap, fullpath, 
			   &width, &height, &bitplane_pixmap, 0, 0);
  
  if (status != BitmapSuccess)
    {
      if (status == BitmapNoMemory)
	sprintf(notice_buf,  
		gettext("There was not enough memory to read in '%s'"),
		fullpath);
      else
	if (status == BitmapOpenFailed)
	  sprintf(notice_buf,  
		  gettext("'%s' could not be opened") , fullpath); 
	else
	  sprintf(notice_buf,  
		  gettext("'%s' was not in a format that iconedit can read."), 
		  fullpath);
      pop_notice(notice_buf);
      xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
      return(TRUE);
    }
  else
    if ((width > 128) || (height > 128))
      {
	sprintf(notice_buf,  
		gettext("'%s' is too big for iconedit to read."),
		fullpath);
	pop_notice(notice_buf);
	xv_set(base_window->window, FRAME_BUSY, FALSE, 0);
	return(TRUE);
	
      }
    else
      {
	icon_height = height;
	icon_width = width;
	set_the_size_info();	  
	save_format = 2;
	v->format = 2;
	change_save_default(2);
	XFillRectangle( dpy, preview_pixmap, gc_rv, 0 , 0, 138, 138);
	XFillRectangle( dpy, big_pixmap, gc_rv, 0, 0, 
		       canvas_dimension, canvas_dimension); 
	XFillRectangle( dpy, preview_xid, gc_rv, 0 , 0, 138, 138);
	XFillRectangle( dpy, big_bit_xid, gc_rv, 0, 0, 
		       canvas_dimension, canvas_dimension); 
	XSetClipOrigin( dpy, bitmap_gc, 
		       preview_boarder_left, preview_boarder_upper);
	XSetClipMask( dpy, bitmap_gc, bitplane_pixmap);
	XSetForeground(dpy, bitmap_gc, black.pixel);
	XSetBackground(dpy, bitmap_gc, white.pixel);	  
	XFillRectangle(dpy, preview_pixmap, bitmap_gc, 0, 0, 138, 138);
	undo_images[(current_undo+1)%MAX_UNDOS].image = 
	  XGetImage(dpy, preview_pixmap, 
		    preview_boarder_left, preview_boarder_upper, 
		    icon_width, icon_height, -1, ZPixmap); 
	undo_images[current_undo%MAX_UNDOS].state = 
	  current_color_state;
	undo_images[current_undo%MAX_UNDOS].height = icon_height;
	undo_images[current_undo%MAX_UNDOS].width = icon_width;
	current_undo++;
	if (undos_allowed < (MAX_UNDOS -1))
	  undos_allowed++;
	
	Edited = 0;
	undo_images[current_undo%MAX_UNDOS].edited = 0;
	redos_allowed = 0;
	
	if (undos_allowed == 1)
	  {
	    edit_menu  = xv_get(base_window->edit_bt, PANEL_ITEM_MENU);
	    xv_set(xv_get(edit_menu, MENU_NTH_ITEM, 2), 
		   MENU_INACTIVE, FALSE, NULL);
	  }
	
	set_iconedit_footer();
	
	undo_images[current_undo%MAX_UNDOS].state = MONO;
	current_color_state = MONO;	      
	
	if (undo_images[(current_undo-1)%MAX_UNDOS].state == COLOR)
	  {
	    change_stuff_to_mono();
	  }
	
	load_file_misc();
	
	XCopyArea(dpy, preview_pixmap, preview_xid, 
		  bitmap_gc, 0, 0, 138, 138, 0, 0); 
	XFreePixmap(dpy, bitplane_pixmap);
      }
  xv_set(base_window->window, FRAME_BUSY, FALSE, 0);

  strcpy(loaded_file, fullpath);
  return(FALSE);

}

file_load()
{
  char            *errmsg;
  int             ret;
  char            notice_buf[MAXPATHLEN];
  char            fullpath[MAXPATHLEN];
  Event           event;
  struct stat     statbuf;
  int		  error;

  if (!file || !*file)
    {
      pop_notice( gettext("No file specified"));
      return(TRUE);
    }

  strcpy(fullpath, file);

  if (stat(fullpath, &statbuf) == ERROR) {
    errmsg = sys_errlist[errno];
    pop_notice(errmsg);
    return (TRUE);
  }

  if (!(S_ISREG(statbuf.st_mode))) {
    sprintf(notice_buf,  gettext("'%s' is not a normal file") , fullpath);
    pop_notice(notice_buf);
    return (TRUE);
    }
      
  if (access(fullpath, R_OK))
    {
      sprintf(notice_buf,  gettext("Permission denied:  %s:") , file);
      pop_notice(notice_buf);
      return (TRUE);
    }
  if (!(error = file_load_from_name(file, fullpath)))
  {
	if (strcmp((char *) xv_get(load_dialog, FILE_CHOOSER_DIRECTORY), (char *) xv_get(save_dialog, FILE_CHOOSER_DIRECTORY)))
        {
		xv_set(save_dialog, FILE_CHOOSER_DIRECTORY, xv_get(load_dialog, FILE_CHOOSER_DIRECTORY), NULL);
		xv_set(save_dialog, FILE_CHOOSER_UPDATE, NULL);
        }

	xv_set(save_dialog, FILE_CHOOSER_DIRECTORY, xv_get(load_dialog, FILE_CHOOSER_DIRECTORY), NULL);
	xv_set(save_dialog, FILE_CHOOSER_DOC_NAME, get_truncated_file_name(), NULL);
  }
  return(error);
}
   


/*
 * Repaint callback function for `big_bit_canvas' after a new 
 * image has been loaded in.
 */
void
do_that_base_repaint_thing(image)
     XImage *image;
{

  long pixel, temp_pixel, even_more_temp_pixel;
  int x, y, corner_x, corner_y;

  temp_pixel = current_pen.pixel;

  for (x = 0; x < icon_width; x++)
    {
      corner_x = (x * width_block) + boarder_left +1;
      for (y = 0; y < icon_height; y++)
	{
	  corner_y = (y * height_block) + boarder_upper +1;
	  pixel = XGetPixel(image, x, y);
	  current_pen.pixel = pixel;
	  XSetForeground(dpy, redraw_gc, current_pen.pixel);
	  XFillRectangle(dpy, big_bit_xid, redraw_gc, corner_x, corner_y, width_block-1, height_block-1);	    
	  XFillRectangle(dpy, big_pixmap, redraw_gc, corner_x, corner_y, width_block-1, height_block-1);	    
	  XDrawPoint(dpy, preview_pixmap, redraw_gc, x+preview_boarder_left, y+preview_boarder_upper);
	  XDrawPoint(dpy, preview_xid, redraw_gc, x+preview_boarder_left, y+preview_boarder_upper);
	}
    }
  XSetForeground(dpy, redraw_gc, black.pixel);
  XDrawRectangle(dpy, big_bit_xid, redraw_gc, boarder_left -1, boarder_upper -1, canvas_dimension - ( boarder_left + boarder_right) +1, canvas_dimension - ( boarder_upper + boarder_lower) +1);
  XDrawRectangle(dpy, big_pixmap, redraw_gc, boarder_left -1, boarder_upper -1, canvas_dimension - ( boarder_left + boarder_right) +1, canvas_dimension - ( boarder_upper + boarder_lower) +1);
  current_pen.pixel = temp_pixel;
}


write_icon_format()
{
  char            *errmsg;
  int             ret;
  char            notice_buf[MAXPATHLEN];
  char            fullpath[MAXPATHLEN];
  Event           event;
  struct stat     statbuf;
  int i, x, y, padded_width;
  short unsigned int w;
  
  register FILE		*fd;
  
	file = get_save_file();

  if (!file || !*file)
	  {
	    pop_notice( gettext("No file specified"));
	    return (TRUE);
	  }

	if(get_save_dir())
	  strcpy(dir, get_save_dir());
	else
    {
      getcwd(dir, MAXPATHLEN);
      set_dir(dir);
    }

  strcpy(name, dir);
  
  strcpy(fullpath, dir);
  if ( name[ strlen(name) -1 ] == '/')
    strcat(fullpath, file);
  else
    {
      strcat(fullpath, "/");
      strcat(fullpath, file);
    }

  strcpy(loaded_file, fullpath);
  
  if (  (fd = fopen( fullpath, "w")) != NULL ) 
    {
      edit_image = XGetImage(dpy, preview_pixmap, preview_boarder_left, preview_boarder_upper, icon_width, icon_height, -1, ZPixmap); 
      padded_width = ((( icon_width - 1 ) / 16 ) + 1 ) * 16;
      fprintf(fd, "/* Format_version=1, Width=%d, Height=%d, Depth=1, Valid_bits_per_item=16\n */\n", padded_width, icon_height);
      for (y = 0; y < icon_height; y++)
	{
	for (x = 0; x < icon_width; x = x+16)
	  {
	    w = 0;
	    for (i = 0; i < 16; i++)
	      {
		w = w << 1;
		if (white.pixel != XGetPixel(edit_image, x +i, y)) 
		    w = w | 0001;
	      }
	    fprintf(fd,"\t0x%04X,", w);
	  }
	fprintf(fd, "\n");
      }
      fclose(fd);
      Edited = undo_images[current_undo%MAX_UNDOS].edited = 0;
      set_iconedit_footer();
      return(FALSE);
    }
  else
    {
      sprintf(notice_buf,  gettext("Permission denied:  %s:") , file);
      pop_notice(notice_buf);
      return(TRUE);
    }  
}


write_xbm_to_file(fullpath)

char	*fullpath;

{
  char          	notice_buf[MAXPATHLEN];
  int 			i, x, y, status;
  Event           	event;
  struct stat     	statbuf;
  long 			temp_pixel;
  GC 			write_gc;
  unsigned long 	value_mask;
  XGCValues 		values;
  Window 		root;
  unsigned int 		win_border, win_depth, win_width, win_height; 
  int			win_x, win_y;
  unsigned long         wpixel, bpixel;

  Cms             xview_cms;
  unsigned long   *temp_colors;

  xview_cms = (Cms)xv_create(NULL, CMS,
			     CMS_SIZE, 2,
			     CMS_NAMED_COLORS, "white", "black", NULL,
			     NULL);
  
  temp_colors = (unsigned long *)xv_get(xview_cms, CMS_INDEX_TABLE);
  
  wpixel = temp_colors[0];
  bpixel = temp_colors[1];
  
  
  bitplane_pixmap = XCreatePixmap(dpy, preview_xid, icon_width, icon_height, 1);

  status = XGetGeometry(dpy, bitplane_pixmap, &root, &win_x, &win_y, &win_width, &win_height, &win_border, &win_depth);
  
  write_gc = XCreateGC(dpy, bitplane_pixmap, 0, 0);

  value_mask = 0xFFFF;

  status = XGetGCValues(dpy, write_gc, value_mask, &values);
  
  XSetForeground(dpy, write_gc, wpixel );
  XSetBackground(dpy, write_gc, bpixel );

  XFillRectangle(dpy, bitplane_pixmap, write_gc, 0, 0, icon_width, icon_height);    

  XSetForeground(dpy, write_gc, bpixel );
  XSetBackground(dpy, write_gc, wpixel );

  edit_image = XGetImage(dpy, preview_pixmap, preview_boarder_left, preview_boarder_upper, icon_width, icon_height, -1, ZPixmap); 

  for (y = 0; y < icon_height; y++)
    for (x = 0; x < icon_width; x++)
      {
	if (white.pixel != XGetPixel(edit_image, x, y))  
	  {
	    XDrawPoint(dpy, bitplane_pixmap, write_gc, x, y);
	  }
      }

  status = XWriteBitmapFile(dpy, fullpath, bitplane_pixmap, icon_width, icon_height, -1, -1);
  if (status == BitmapOpenFailed)
    {
      sprintf(notice_buf,  gettext("Bitmap open failed for '%s'") , fullpath);
      pop_notice(notice_buf);    
      return(status);
    }
  else
  if (status == BitmapNoMemory)
    {
      pop_notice( gettext("Bitmap is too large for available memory") );
      return(status, TRUE);
    }
  else
  {
  XFreeGC(dpy, write_gc);
  XFreePixmap(dpy, bitplane_pixmap);
  }

  Edited = undo_images[current_undo%MAX_UNDOS].edited = 0;
  set_iconedit_footer();
  return(FALSE);
}

write_xbm_format()
{
  char            *errmsg;
  int             ret;
  char            fullpath[MAXPATHLEN];
  
  register FILE		*fd;
  
	file = get_save_file();

  if (!file || !*file)
    {
      pop_notice( gettext("No file specified") );
      return (TRUE);
    }
  
	if(get_save_dir())
	  strcpy(dir, get_save_dir());
  else
    {
      getcwd(dir, MAXPATHLEN);
      set_dir(dir);
    }

  
  strcpy(name, dir);
  
  strcpy(fullpath, dir);
  if ( name[ strlen(name) -1 ] == '/')
    strcat(fullpath, file);
  else
    {
      strcat(fullpath, "/");
      strcat(fullpath, file);
    }
 
  strcpy(loaded_file, fullpath);

  return(write_xbm_to_file(fullpath)); 
}

int
do_that_save_thing()
{

  if (save_thru_load())
    return 0;

  if (!save_dialog)
    save_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_SAVEAS);
  if (!load_dialog)
    load_dialog = file_dialog_initialize(base_window->window, FILE_CHOOSER_OPEN);

	file = get_save_file();
  if (!file || !*file)
    {
      file_initialize();   
      if (!positioned)
        {
	    ds_position_popup(base_window->window, save_dialog, 
			      DS_POPUP_LOR);
	    positioned = TRUE;
        }
      xv_set(save_dialog, XV_SHOW, TRUE, 0);
/* Make save the default and bring up the popup */
      return 1;
    }

/* set save to be the default action and bring up the filechooser thingie */

      file_initialize();   
  if ((current_color_state == COLOR) && (save_format != 3))
    pop_notice( gettext("Cannot save color image in monochrome format.\nConvert to monochrome or choose a color format."));
  else
    {
      if (save_format == 1)
	write_icon_format();    
      else if (save_format == 2)
	write_xbm_format();
      else if ((save_format == 3) || (save_format == 4))
	write_pixmap_format();	

      /* tell the file choosers they need to update if necessary */

      xv_set(save_dialog, FILE_CHOOSER_UPDATE, NULL);

      if (!strcmp( (char *) xv_get(save_dialog, FILE_CHOOSER_DIRECTORY),  (char *) xv_get(load_dialog, FILE_CHOOSER_DIRECTORY)))
        xv_set(load_dialog, FILE_CHOOSER_UPDATE, NULL);
      }
  return 0;
}


Menu_item
props_icon(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		v->format = 1;
		save_format = 1;
		change_save_default(1);
		write_resources();
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

Menu_item
props_xbm(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		v->format = 2;
		save_format = 2;
		change_save_default(2);
		write_resources();
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

Menu_item
props_c_xpm(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		v->format = 3;
		save_format = 3;
		change_save_default(3);
		write_resources();
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}

Menu_item
props_m_xpm(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	switch (op) {
	case MENU_DISPLAY:
		break;

	case MENU_DISPLAY_DONE:
		break;

	case MENU_NOTIFY:
		v->format = 4;
		save_format = 4;
		change_save_default(4);
		write_resources();
		break;

	case MENU_NOTIFY_DONE:
		break;
	}
	return item;
}


void 
change_save_default(value)
int value;
{
Menu format_menu;
Menu props_menu;

/* Change the default format on the filechooser */
props_menu = xv_get(base_window->props_bt, PANEL_ITEM_MENU);
format_menu = xv_get(xv_get(props_menu, MENU_NTH_ITEM, 1), 
		     MENU_PULLRIGHT);
xv_set(format_menu, MENU_DEFAULT_ITEM, 
       xv_get(format_menu, MENU_NTH_ITEM, value), NULL);
}
