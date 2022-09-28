#ifdef lint
#ifdef sccs
static char sccsid[]="@(#)tooltalk.c	3.8 08/19/93 Copyright 1987-1990 Sun Microsystems, Inc.";
#endif   
#endif

/*
 *  Copyright (c) 1987-1990 Sun Microsystems, Inc.
 *  All Rights Reserved.
 *
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

/*
 * ToolTalk interface functions for Iconedit.
 */

#include <stdio.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <desktop/tt_c.h>
#include <ds_popup.h>
#include <ds_tooltalk.h>
#include <dstt.h>

#include "tooltalk.h"

#define RETURN_COLOR    3

extern int        tt_running;
extern void       select_color_from_rgb();
extern Colormap   cmap;
extern XColor     current_pen;
extern Display             *dpy;
extern base_window_objects *base_window;

static Notify_value    handle_message();
static void            handle_reply();
static int             waiting_for_launch_reply = FALSE;
static int             handler_started = FALSE;
static char           *tt_open_method;
static char           *tt_handler;
static char 	      *iconedit_msgid;

char     *toolid = NULL;

void 
tool_info (vend, name, ver)
  char **vend;
  char **name;
  char **ver;
{
  extern char *xv_version;
  extern char *ds_vendname();

  *vend = (char *) strdup (ds_vendname());
  *name = "iconedit";
  *ver  = xv_version;
}

int
map_callback (m, key, status, val, msg, buf)
  Tt_message  m;
  void       *key;
  int         status;
  int         val;
  char       *msg;
  char       *buf;
{

  return (0);
}
 
int
edit_callback (ttmsg, key, status, prop, data, size, iconname, iconsize,
               iconmask, masksize, msgid, title)
  Tt_message  ttmsg;
  void       *key;
  int         status;
  char       *prop;
  void       *data;
  int         size;
  void       *iconname;
  int         iconsize;
  void       *iconmask;
  int         masksize;
  char       *msgid;
  char       *title;
{
  int             r, g, b;

#ifdef DEBUG
  printf ("iconedit edit_callback: got tt status: %d\n", status);
#endif

  switch (tt_message_state (ttmsg)) {
  case TT_HANDLED:
    if (sscanf (data, "#%04x%04x%04x", &r, &g, &b) == 3) {

#ifdef DEBUG
printf ("rgbs returned to iconedit = (%d %d %d)\n", r, g, b);
#endif
    select_color_from_rgb (r, g, b);
    }
    if (toolid) {
      free (toolid);
      toolid = NULL;
    }
/*
 * Resend again if user did not cancel.
 */
    if (dstt_test_status (ttmsg) != dstt_status_user_request_cancel)
      iconedit_tt_get_color (NULL, NULL);
    break;
  case TT_FAILED:
#ifdef DEBUG
    printf ("tt msg TT_FAILED\n");
#endif
    handler_started = FALSE;
    xv_set (base_window->window, FRAME_BUSY, FALSE, NULL);
    break;
  case TT_REJECTED:
#ifdef DEBUG
    printf ("tt msg TT_FAILED\n");
#endif
    handler_started = FALSE;
    xv_set (base_window->window, FRAME_BUSY, FALSE, NULL);
    break;
  default:
    break;
  }

  return (0);
}

void
status_handler (ttmsg, status, vendor, toolname, version, msgid, domain)
    Tt_message  ttmsg;
    char       *status;
    char       *vendor;
    char       *toolname;
    char       *version;
    char       *msgid;
    char       *domain;
{
 
    toolid = strdup (tt_message_sender (ttmsg));
    /*xv_set (base_window->window, FRAME_BUSY, FALSE, NULL); */
    handler_started = TRUE;
 
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
 
   handler_started = FALSE;
   return (0);
}


/*
 * Connect with Tooltalk.
 * This can be called after xv_init() because
 * the DISPLAY environment variable is not being set here.
 */
void
iconedit_tt_init( argc, argv )
    int   argc;
    char **argv;
{
  /*
   * Connect to the ToolTalk server and set flag.
   * Gray out color buttons if ToolTalk not active.
   */
#ifdef OLD_TOOLTALK
  if ( ds_tooltalk_init( NULL, argc, argv ) != 0 ) {
    tt_running = FALSE;
    fputs( ( char * )gettext( "Warning: ToolTalk not initialized in Iconedit!\n" ) , stderr );
  }   
  else
    tt_running = TRUE;
#endif

  tt_running = TRUE;

  /*
   * Get the ToolTalk open method for the ColorChooser.
   */
#ifdef NEVER
  entry = ce_get_entry( ip->tns, 1, "colorchooser" );
  if ( entry == NULL )
    tt_open_method = ce_get_attribute( ip->tns, entry, ip->type_open_tt );
  else
    printf( gettext( "Iconedit: could not get open method for colorchooser\n" ) );
#endif
    
}

void
iconedit_tt_init_complete()
{
#ifdef OLD_TOOLTALK
  /*
   * Register a callback when a reply to the  "get_color"  message comes in.
   */
  ds_tooltalk_set_callback( base_window->window, handle_message );
#endif

    dstt_xview_desktop_callback (base_window->window, NULL);
    dstt_xview_desktop_register (base_window->window, NULL);
    dstt_notice_callback ((char *)base_window->window, STATUS, 
		          status_handler, NULL);
    dstt_notice_register ((char *)base_window->window, STATUS, TRUE, NULL);
    dstt_xview_start_notifier ();

    iconedit_msgid = dstt_messageid();

}

static void
request_color()
{
  Tt_message       request_msg;
  Xv_singlecolor   fg, bg;
  Tt_status        tt_status;
  XColor           current_color;

  /*
   * Send a DS_TT_NON_STD_MSG with appropriate data.
   */
  request_msg = ds_tooltalk_create_message( tt_handler, DS_TT_NO_STD_MSG, "get_color" );
  if ( request_msg == NULL )
    printf( (char *)gettext( "Iconedit: Couldn't create request message.\n" ) );
   
  /*
   * Requesting three new RGBs.
   */
  tt_message_arg_add( request_msg, TT_IN,   "string" , 
		     (void *) gettext("Choosing Color for Iconedit")); /* 0 */

  current_color.pixel = current_pen.pixel;
  XQueryColor( dpy, cmap, &current_color );
  fg.red   = current_color.red >> 8;
  fg.green = current_color.green >> 8;
  fg.blue  = current_color.blue >> 8;
  bg.red = 0;  bg.green = 0; bg.blue = 0;

  tt_message_arg_add( request_msg, TT_IN, "string" , ( void * )NULL );  /* 1 */
  tt_message_arg_add( request_msg, TT_IN, "string" , ( void * )NULL );  /* 2 */
  tt_message_barg_add( request_msg, TT_INOUT, "Xv_singlecolor",         /* 3 */
                       ( unsigned char * )&fg, sizeof( Xv_singlecolor ) );
  tt_message_barg_add( request_msg, TT_INOUT, "Xv_singlecolor" ,        /* 4 */
                       ( unsigned char * )&bg, sizeof( Xv_singlecolor ) );
  /*
   * Send out the request.
   */
  if ( tt_status = tt_message_send( request_msg ) != TT_OK ) 
    printf((char *)gettext( "Iconedit: Couldn't sent message.\n") );


}

static void
request_launch()
{
  Tt_message      request_msg;
  Tt_status       tt_status;
  int             ivalue;
  char           *value;

  /*
   * Get the handler of this message.
   */
#ifdef FIX_LATER
  tt_handler = ds_tooltalk_get_handler( open_method );
#else
  tt_handler = ds_tooltalk_get_handler( "colorchooser" );
#endif

  /*
   * Send a LAUNCH message to start the colorchooser.
   */
  request_msg = ds_tooltalk_create_message( tt_handler, DS_TT_LAUNCH_MSG, NULL );
  if ( request_msg == NULL ) 
    printf( (char *)gettext( "Iconedit: Couldn't create request message.\n" ) );
  /*
   * Set the DISPLAY environment variable and locale.
   */
  value = ( char * )xv_get( base_window->window, XV_LC_BASIC_LOCALE );
  tt_message_arg_add( request_msg, TT_IN, "string", value );

  ivalue = ( int )xv_get( base_window->window, WIN_DEPTH );
  tt_message_iarg_add( request_msg, TT_IN, "int", ivalue );

  ivalue = ( int )xv_get( base_window->window, XV_VISUAL_CLASS );
  tt_message_iarg_add( request_msg, TT_IN, "int", ivalue );

  waiting_for_launch_reply = TRUE;
  /*
   * Send out the request.
   */
  if ( tt_status = tt_message_send( request_msg ) != TT_OK ) 
    printf( (char *)gettext( "Iconedit: Couldn't sent message.\n") );

}

/*
 * Create and send a message to request a color from the Color Editor.
 */
void
iconedit_tt_get_color( item, event )
    Panel_item  item;
    Event      *event;
{
  Xv_singlecolor   fg;
  Tt_status        tt_status;
  XColor           current_color;
  char             hex_color[28];

  /*
   * Check in case tooltalk is not running.  The color 
   * button should be inactivated anyway.
   */
  if ( !tt_running ) 
    return;

#ifdef OLD_TOOLTALK
  /*
   * First, find out if we've started the handling application (Colorchooser).
   * If not, start it first.
   */
  if ( handler_started == FALSE )
    request_launch();
  else 
    request_color();
#endif

  current_color.pixel = current_pen.pixel;
  XQueryColor( dpy, cmap, &current_color );
  fg.red   = current_color.red >> 8;
  fg.green = current_color.green >> 8;
  fg.blue  = current_color.blue >> 8;
  hex_color[0] = NULL;
  sprintf (hex_color, "#%04x%04x%04x#%04x%04x%04x", fg.red, fg.green, fg.blue, 0, 0, 0 );
#ifdef DEBUG
printf("hex_color is %s len =%d\n", hex_color, strlen (hex_color));
#endif
  /*xv_set (base_window->window, FRAME_BUSY, TRUE, NULL); */
  dstt_prop_edit (edit_callback, "iconedit", "Color_Set", hex_color,
		  strlen (hex_color), NULL, 0, NULL, 0, 
		  iconedit_msgid,
		  (char *)gettext ("Choosing Color for Iconedit"));
}

void
iconedit_open_frame()
{
    Tt_message  move_msg;
    Tt_status   status;
    Rect        frame_rect;

    /*
     * Return if the handler is not started.
     */
    if ( !handler_started )
      return;

    dstt_set_mapped ((Mapped_CB*)map_callback, "iconedit", toolid, TRUE, NULL, NULL);
#ifdef OLD_TOOLTALK
    /*
     * MOVE the handler back to its original coordinates.
     */
    frame_get_rect( base_window->window, &frame_rect );
    move_msg = ds_tooltalk_create_message( tt_handler, DS_TT_MOVE_MSG, NULL );
    
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_left );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_top );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_width );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_height );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void *)DS_POPUP_LOR );
    
    status = tt_message_send( move_msg );
    if ( status != TT_OK )
      printf( (char *)gettext( "Iconedit: Could not send move message: \n" ) );
#endif
    
}

void
iconedit_close_frame()
{
    Rect        frame_rect;
    int         width, height;
    Tt_message  move_msg;
    Tt_status   status;

    /*
     * Return if handler is not started.
     */
    if ( !handler_started )
      return;

    dstt_set_mapped ((Mapped_CB*)map_callback, "iconedit", toolid, FALSE, NULL, NULL);
#ifdef OLD_TOOLTALK
    /*
     * Move the handler off the screen.
     * Save the original coordinates.
     */
    frame_get_rect( base_window->window, &frame_rect );
    ds_get_screen_size( base_window->window, &width, &height );
    frame_rect.r_top = height + frame_rect.r_height + 10;
    frame_rect.r_left = width + frame_rect.r_width + 10;

    move_msg = ds_tooltalk_create_message( tt_handler, DS_TT_MOVE_MSG, NULL );
    
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_left );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_top );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_width );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_height );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void *)DS_POPUP_AOB );
    
    status = tt_message_send( move_msg );
    if ( status != TT_OK )
      printf( (char *)gettext( "Iconedit: Could not send move message: \n" ) );
#endif
        
}

/*
 * Receive incoming Tooltalk message and dispatch them 
 * to the appropriate routines.
 */
static Notify_value
handle_message( client, fd )
     Notify_client  client;
     int            fd;
{
  Tt_message  reply_msg;

  /*
   * There is a message awaiting from Tooltalk.
   */
  if (( reply_msg = tt_message_receive() ) != TT_OK ) 
    dispatch_message( reply_msg );
    
}


/*
 * This callback is called when a reply to a  "get_color"  message
 * comes in.  It passes the data to the application's receive-data 
 * callback.
 */
static void
handle_reply( reply_msg )
     Tt_message reply_msg;
{
  Xv_singlecolor *color; 
  int            mark, nbytes;

  tt_message_arg_bval( reply_msg, RETURN_COLOR, ( unsigned char ** )&color, &nbytes );  
  /*
   * Do something with the returned color. - Sean
   * change_icon_color( base_window.window, color, FALSE );
   */
#ifdef DEBUG
  printf( "Iconedit: Returned color is red=%d, green=%d, blue=%d\n" , color->red, color->green, color->blue );
#endif

  select_color_from_rgb( color->red, color->green, color->blue);
}


/*
 *
 */
static void
iconedit_send_move_msg( frame )
     Frame  frame;
{
  Rect        frame_rect;
  Tt_message  move_msg;
  Tt_status   status;

#ifdef FIX_LATER
  tt_handler = ds_tooltalk_get_handler( open_method );
#else
  tt_handler = ds_tooltalk_get_handler( "colorchooser" );
#endif

  frame_get_rect( frame, &frame_rect );
  move_msg = ds_tooltalk_create_message( tt_handler, DS_TT_MOVE_MSG, NULL );
  if ( move_msg == NULL )
    printf ((char *)gettext( "Iconedit: Couldn't create move message.\n" ) );

  tt_message_arg_add( move_msg, TT_IN, "int", (void *)frame_rect.r_left );
  tt_message_arg_add( move_msg, TT_IN, "int", (void *)frame_rect.r_top );
  tt_message_arg_add( move_msg, TT_IN, "int", (void *)frame_rect.r_width );
  tt_message_arg_add( move_msg, TT_IN, "int", (void *)frame_rect.r_height );
  tt_message_arg_add( move_msg, TT_IN, "int", (void *)DS_POPUP_LOR );

  if ( status = tt_message_send( move_msg ) != TT_OK ) 
    printf( (char *)gettext( "Iconedit: Could not send move message: \n" ) );


}

/*
 * 
 */
static void
iconedit_send_expose_msg()
{
  Tt_message  expose_msg;
  Tt_status   status;

#ifdef FIX_LATER
  tt_handler = ds_tooltalk_get_handler( open_method );
#else
  tt_handler = ds_tooltalk_get_handler( "colorchooser" );
#endif

  if ( handler_started ) {
    expose_msg = ds_tooltalk_create_message( tt_handler, DS_TT_EXPOSE_MSG, NULL );
    if ( expose_msg == NULL ) {
      printf( (char *)gettext( "Iconedit: Couldn't create expose message.\n" ) );
      return;
    }

    if ( status = tt_message_send( expose_msg ) != TT_OK ) 
      printf( (char *)gettext( "Iconedit: Could not send expose message: \n" ) );

  }

}
/*
 * 
 */
static void
iconedit_send_quit_msg()
{
  Tt_message  quit_msg;
  Tt_status   status;

#ifdef FIX_LATER
  tt_handler = ds_tooltalk_get_handler( open_method );
#else
  tt_handler = ds_tooltalk_get_handler( "colorchooser" );
#endif

  if ( handler_started ) {
    quit_msg = ds_tooltalk_create_message( tt_handler, DS_TT_QUIT_MSG, NULL );
    if ( quit_msg == NULL ) {
      printf ((char *)gettext( "Iconedit: Couldn't create quit message.\n" ) );
      return;
    }

    if ( status = tt_message_send( quit_msg ) != TT_OK ) 
      printf( (char *)gettext( "Iconedit: Could not send quit message: \n" ) );

  }

}

/*
 * Shut down ToolTalk.
 * Called after xv_main_loop().
 */
void
iconedit_tt_quit()
{
  /*
   * Send a quit message to handling
   * application and quit.
   */
#ifdef OLD_TOOLTALK
  if ( tt_running ) {
    iconedit_send_quit_msg();
    ds_tooltalk_quit();
    tt_running = FALSE;
  }
#endif

  dstt_quit (quit_callback, "iconedit", toolid, TRUE, FALSE, NULL);
  if (toolid) {
    free (toolid);
    toolid = NULL;
  }

}

/*
 *
 */
static 
dispatch_message( message )
     Tt_message  message;
{
  Tt_state        state;
  ds_tt_msg_info  info;

  /*
   * Determine the state of the message.
   */
  state = tt_message_state( message );
  /*
   * Get information about this message.
   */
  info = ds_tooltalk_received_message( message );

  switch( state ) {
    case TT_HANDLED:
      switch ( info.ds_tt_msg_type ) {
        /*
         * We've been notified that the Colorchooser has been launched.
         * Clear the left footer.
         * Send a move message so the colorchooser is positioned to the correct spot.
         */
        case DS_TT_LAUNCH_MSG:
          if ( waiting_for_launch_reply == TRUE ) {
            iconedit_send_move_msg( base_window->window  );
            handler_started = TRUE;
            waiting_for_launch_reply = FALSE;
          }
          break;
        /* 
         * We've been notified that the ColorChooser is positioned correctly.  
         * Send an expose message then
         * send a request message to request a color.
         */
       case DS_TT_MOVE_MSG:
          iconedit_send_expose_msg();
          xv_set( base_window->window, FRAME_LEFT_FOOTER, "", NULL );
          iconedit_tt_get_color( base_window->palette_bt, ( Event * )NULL );
          break;

        case DS_TT_NO_STD_MSG:
          handle_reply( message );
          iconedit_tt_get_color();
          break;

        default:
          break;
      }
      break;

    case TT_SENT:
       switch ( info.ds_tt_msg_type ) {
         /*
          * The Color Chooser is departing. Reset the flag.
          */
         case DS_TT_DEPARTING_MSG:
           handler_started = FALSE;
           break;
         default:
           break;
       }
    case TT_STARTED:
      /*
       * We sent a launch and have been notified that ToolTalk is
       * starting up the colorchooser.  Set the footer message.
       */
      if ( info.ds_tt_msg_type == DS_TT_LAUNCH_MSG ) 
        xv_set( base_window->window, FRAME_LEFT_FOOTER, 
	       gettext("Starting Color Chooser..."), NULL );
      break;

    case TT_REJECTED:
      handler_started = FALSE;
      printf( (char *)gettext("Iconedit: Help! message rejected\n ") );
      break;

    case TT_FAILED:
      handler_started = FALSE;
#ifdef NEVER
      xv_set( base_window->window, FRAME_LEFT_FOOTER, 
	     gettext("Color Chooser Failed"), NULL );
#else
      xv_set( base_window->window, FRAME_LEFT_FOOTER, "", NULL );
#endif

      break;

    default:
#ifdef DEBUG
      printf( "Iconedit: default - %d\n" , ( int )tt_message_state( message ) );
#endif
      break;
  }

}



