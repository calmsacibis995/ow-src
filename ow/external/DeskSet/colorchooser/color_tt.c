#ifndef lint
static char sccsid[] = "@(#)color_tt.c	3.8 12/08/93 Copyright 1990 Sun Microsystems, Inc.";
#endif

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * Tool Talk interface routines for the Color Chooser.
 *
 */

#include <stdio.h>
#include <string.h>
#include <xview/xview.h>
#include <xview/cms.h>
#include <desktop/tt_c.h>

#include "colorchooser.h"
#include "color_tt.h"
#include "ds_tooltalk.h"
#include "ds_popup.h"
#include "dstt.h"

extern int      started_by_tooltalk;
extern int      started_by_ma;
extern Frame 	frame;
extern HSV   	xact_work;
static Tt_message tt_msg;

static Notify_value   handle_message();
static void           color_tt_receive();
static void           color_tt_move();

static int             tt_running = FALSE;   /* ToolTalk flag */
static Tt_message      in_process_msg;       /* Message currently in process */
static char           *orig_sender;
static char           *new_locale;
static int             new_depth, new_visual; 
static int             exposed = FALSE;
static char           *prev_msgid = NULL;

/*
 * ToolTalk Argument Numbers
 */
#define IN_ARG_FOOTER             0
#define IN_ARG_ICONFILE           1
#define IN_ARG_ICONMASKFILE       2
#define INOUT_ARG_FG_COLOR        3
#define INOUT_ARG_BG_COLOR        4

status_t
colorchooser_dstt_display (m, media, type, data, size, msg, title)
      Tt_message  m;
      char       *media;
      Data_t 	  type;
      void 	 *data;
      int 	  size;
      char 	 *msg;
      char       *title;
{
     return (OK);
}

int
geometry_callback (ttmsg, key, status, w, h, x, y)
   Tt_message  ttmsg;
   void	      *key;
   int         status;
   int         w, h, x, y;
{
   Rect   rect;
   
   rect.r_width = w;
   rect.r_height = h;
   rect.r_left = x;
   rect.r_top = y;

   ds_position_popup_rect (&rect, frame, DS_POPUP_RIGHT);

   xv_set (frame, XV_SHOW, TRUE, NULL);

   return (0);
}

int
quit_callback (m, silent, force, msg)
{
/*
  tt_session_quit( tt_default_session() );
  tt_close();
*/
  exit (0);
}

void
cancel_message()
{
  dstt_set_status (tt_msg, dstt_status_user_request_cancel);
  tt_message_fail (tt_msg);
  tt_msg = NULL;
}

status_t
colorchooser_dstt_edit (m, prop, data, size, icon1, size1,
			icon2, size2, msg, title)
	Tt_message  m;
	char       *prop;
	void       *data;
	int         size;
	void       *icon1;
	int         size1;
	void       *icon2;
	int         size2;
	char       *msg;
        char       *title;
{
    int 	    r, g, b, r1, g1, b1;
    RGB	  	    fg_color, bg_color;

/*
 * if (prev msgid == 0)
 *    prev msgid == msg;
 * else if (prev msgid && (msg == prev msgid) then
 *    fail old msg with user cancel.
 * else
 *    fail current message.
 *    return
 */
    if (prev_msgid == 0) 
      prev_msgid = strdup (msg);
    else if (strcmp (msg, prev_msgid) == 0) {
      cancel_message();
    }
    else {
      return (REJECT);
    }
 
    tt_msg = m;

/*
 * Reply back to sender telling it that yes, we have it, but we will
 * take some time getting back to you.
 */
    dstt_status (tt_message_sender (m), (char *)dstt_set_status (0, dstt_status_req_rec),
		 NULL, NULL);
/*
 * Do not allow any other displays or edit while editting.
 */
/*
    dstt_prop_register ("Color_Set",
			  PROP_EDIT,   FALSE,
			  NULL);
*/
    if (title) {
      xv_set (frame, FRAME_LEFT_FOOTER, title, NULL);
      if (strstr (title, MGET ("Background")) != NULL) 
	windowmode = TRUE;
      else
	windowmode = FALSE;
    }

    if (size1 > 0)
      iconname = (char *)icon1; 
    else
      iconname = NULL;

    if (size2 > 0)
      iconmaskname = (char *)icon2; 
    else
      iconmaskname = NULL;

    new_image = TRUE;

    if (sscanf (data, "#%04x%04x%04x#%04x%04x%04x", 
			&r, &g, &b, &r1, &g1, &b1) == 6) {
      fg_color.r = (u_char) r;
      fg_color.g = (u_char) g;
      fg_color.b = (u_char) b;
      bg_color.r = (u_char) r1;
      bg_color.g = (u_char) g1;
      bg_color.b = (u_char) b1;
#ifdef DEBUG
printf ("fgrgb = %d %d %d\n", fg_color.r, fg_color.g, fg_color.b);
printf ("bgrgb = %d %d %d\n", bg_color.r, bg_color.g, bg_color.b);
#endif
      rgb_to_hsv (&fg_color, &xact_work);
      rgb_to_hsv (&bg_color, &xact_win);
      store_custom_colors();
      update_colors();
      backup_colors();
/*
 * Request geometry if not shown yet.
 */
     if (xv_get (frame, XV_SHOW) == FALSE) 
       dstt_get_geometry (geometry_callback, NULL, tt_message_sender (m), 
			  NULL, NULL);

    }
/*
    else
      return (FAIL); 
*/

    return (HOLD);
}

void
colorchooser_dstt_start (frame)
    Xv_opaque  frame;
{
    dstt_xview_desktop_callback (frame, QUIT, quit_callback, NULL);
    dstt_xview_desktop_register (frame, QUIT, TRUE, NULL);
    dstt_prop_callback ("Color_Set",
			  PROP_EDIT, colorchooser_dstt_edit,
			  NULL);
    dstt_prop_register ("Color_Set", PROP_EDIT, TRUE, NULL);
    dstt_xview_start_notifier ();
}


void 
tool_info (vend, name, ver)
  char **vend;
  char **name;
  char **ver;
{
  extern char *xv_version;

  *vend = "Sun Microsystems, Inc.";
  *name = "colorchooser";
  *ver  = xv_version;
}

/*
 * color_tt_init()
 *   Initialize TookTalk.
 *   Determine if the app was started by tooltalk.
 */
int
color_tt_init( argc, argv )
     int   argc;
     char  **argv;
    
{
  int           i;

  for (i = 0; i < argc; i++ ) {
    if ( strcmp( argv[ i ], "-tooltalk" ) == 0 ) {
#ifdef DEBUG
      printf( " Colorchooser: Started by tooltalk!\n" );
#endif
      started_by_tooltalk = TRUE;
    
    }
    else if (strcmp (argv[i], "-message_alliance") == 0) {
#ifdef DEBUG
      printf( " Colorchooser: Started by message_alliance!\n" );
#endif
       started_by_ma = TRUE;
    }
  }
      
  /*
   * This assumes that colorchooser is always started by tooltalk.
   */
  if (started_by_tooltalk) {

    if ( ds_tooltalk_init( "colorchooser", argc, argv ) != 0 ) {
      tt_running = FALSE;
      fputs( MGET( "Warning: ToolTalk not initialized in the Color Chooser!\n" ), stderr );
      return( 0 );
    }
    else
      tt_running = TRUE;

    /* 
     * Set the DISPLAY environment variable - this
     * must be done before xv_init().
     */
    if ( started_by_tooltalk ) {
      handle_message( ( Notify_client)NULL, 0 );
      if ( ( new_locale != ( char * )NULL ) &&
           ( new_depth != -1 ) && ( new_visual != -1 ) )
        ds_tooltalk_set_argv( argc, argv, new_locale, new_depth, new_visual );
    }
  }

  return( 1 );

}

/*
 * Set the callback routine to handle tooltalk messages.
 * This should be called before xv_main_loop().
 */
void
color_complete_tt_init( frame )
     Frame  frame;
{
  /*
   * set callback
   */
  if ( tt_running )
    ds_tooltalk_set_callback( frame, handle_message );

}

/*
 * color_tt_quit()
 *   Shut down ToolTalk.
 *   Called after xv_main_loop()
 */
void
color_tt_quit()
{
  if ( !tt_running )
    return;
  /*
   * Reply with a failed msg if terminating without returning
   * correct values.
   */
  if ( tt_message_state( in_process_msg ) != TT_HANDLED ) {
    tt_message_fail( in_process_msg );
  }
  ( void * )tt_message_destroy( in_process_msg );
  /*
   * Tell all tooltalk senders that we're going away...
   */
  ds_tooltalk_send_departing_message();
  
  /*
   * Shut down session. By the way, if tt_session_quit is not called, server hangs.
   */
  tt_session_quit( tt_default_session() );
  tt_close();

}

static Notify_value
handle_message( client, fd )
     Notify_client client;
     int           fd;
{
  Tt_message      incoming_msg;
  Tt_state        state;
  ds_tt_msg_info  info;
  char           *locale;

  incoming_msg = tt_message_receive();

  if ( incoming_msg == 0 )
    return NOTIFY_DONE;

  if ( !orig_sender ) 
    orig_sender = tt_message_sender( incoming_msg );

  if ( strcmp( orig_sender, tt_message_sender ( incoming_msg ) ) != 0 ) {
    tt_message_reject( incoming_msg );
#ifdef DEBUG
printf( "colorchooser: rejecting message!\n" );
#endif
  }

  state = tt_message_state( incoming_msg );

  switch ( state ) {
    case TT_SENT:    /* Someone sent us a message */
      info = ds_tooltalk_received_message( incoming_msg );
      switch ( info.ds_tt_msg_type ) {
        case DS_TT_LAUNCH_MSG:
          locale  = ( char * )tt_message_arg_val( incoming_msg, 0 );
          new_locale = ( char * )malloc( strlen( locale ) + 1 );
          strcpy( new_locale, locale );

          tt_message_arg_ival( incoming_msg, 1, &new_depth );
          tt_message_arg_ival( incoming_msg, 2, &new_visual );

          tt_message_reply( incoming_msg );
#ifdef DEBUG 
          printf(" Colorchooser: We've been sent a launch message\n" );
          printf(" colorchooser: sender is %s\n", tt_message_sender( incoming_msg ));
#endif
          break;
      
        case DS_TT_MOVE_MSG:
#ifdef DEBUG
          printf(" Colorchooser: We've been sent a move message\n" );
#endif
          color_tt_move( incoming_msg );
          tt_message_reply( incoming_msg );
          break;

        case DS_TT_EXPOSE_MSG:
#ifdef DEBUG
          printf(" Colorchooser: We've been sent an expose message\n" );
#endif 
          /*
           * Notify stop only once.
           */
          if ( started_by_tooltalk && !exposed ) {
            notify_stop();
            exposed = TRUE;
          }
          break;

        case DS_TT_NO_STD_MSG:
#ifdef DEBUG
          printf( " Colorchooser: Non-standard msg received\n" );
#endif
          xv_set( frame, XV_SHOW, TRUE, NULL );
          color_tt_receive( incoming_msg );
          break;

        case DS_TT_QUIT_MSG:
#ifdef DEBUG 
          printf(" Colorchooser: Requestor is quitting - we should too\n" );
#endif
          color_tt_quit();
          exit( 0 );
          break;

        default:
          break;
      }
      break;
    
    default:
      break;
  }

  return ( NOTIFY_DONE );

}

	     
/*
 * This is a simple notifier fd input function to read
 * tt messages whenever the tt filedescriptor has input.
 * It is only needed because the TT XView integration is
 * not yet complete.
 */
static void
color_tt_receive( incoming_msg )
     Tt_message  incoming_msg;
{
  Xv_singlecolor *fg, *bg;
  RGB             fg_color, bg_color;
  int             nbytes;

  /*
   * Check if there really is a message on an active fd.
   */
  if ( incoming_msg == 0 )
    return;
 
  /*
   * Extract the incoming values, and set the return color to
   * the incoming color.
   */
  xv_set( frame, FRAME_LEFT_FOOTER, 
	  ( char * )tt_message_arg_val( incoming_msg, IN_ARG_FOOTER ), NULL );
  /*
   * Set the original message sender who started this up.
   */
  if ( !orig_sender ) {
    orig_sender = ( char * )malloc(  strlen( tt_message_sender( incoming_msg ) ) );
    strcpy( orig_sender, tt_message_sender( incoming_msg ) );
  }

  /*
   * Reject message if requested by someone other than the
   * original requestor.
   */
  if ( strcmp( orig_sender, tt_message_sender( incoming_msg ) ) != 0 ) {
    ( void * )tt_message_reject( incoming_msg );
    ( void * )tt_message_destroy( incoming_msg );
  }
  else {
    in_process_msg = incoming_msg;
    /*
     * Extract the values from the request.
     */
    iconname     = ( char * )tt_message_arg_val( in_process_msg, IN_ARG_ICONFILE );
    iconmaskname = ( char * )tt_message_arg_val( in_process_msg, IN_ARG_ICONMASKFILE );
    ( void * )tt_message_arg_bval( in_process_msg, INOUT_ARG_FG_COLOR, 
	      		           ( unsigned char ** )&fg, &nbytes );
    ( void * )tt_message_arg_bval( in_process_msg, INOUT_ARG_BG_COLOR, 
			           ( unsigned char ** )&bg, &nbytes );


    new_image = TRUE;

    if ( strstr( tt_message_arg_val( in_process_msg, IN_ARG_FOOTER ), MGET("Background")) != NULL ) {
      windowmode = TRUE;
      tt_message_arg_bval_set( in_process_msg, INOUT_ARG_BG_COLOR,
	         	       ( unsigned char * )bg, sizeof( Xv_singlecolor ) );
    }
    else {
      windowmode = FALSE;
      tt_message_arg_bval_set( in_process_msg, INOUT_ARG_FG_COLOR,
		               ( unsigned char * )fg, sizeof( Xv_singlecolor ) );
    }

    fg_color.r = fg->red; 
    fg_color.g = fg->green; 
    fg_color.b = fg->blue;
    bg_color.r = bg->red; 
    bg_color.g = bg->green; 
    bg_color.b = bg->blue;

    rgb_to_hsv( &fg_color, &xact_work );
    rgb_to_hsv( &bg_color, &xact_win );
    store_custom_colors();
    update_colors();
    backup_colors();
  }


}


/*
 * This is a simple notifier fd input function to read
 * tt messages whenever the tt filedescriptor has input.
 * It is only needed because the TT XView integration is
 * not yet complete.
 */
static void
color_tt_move( incoming_msg )
     Tt_message  incoming_msg;
{
  Rect   frame_rect, new_rect;
  int    location, n = 0;

  /*
   * Check if there really is a message on an active fd.
   */
  if ( incoming_msg == 0 )
    return;
 
  frame_rect.r_left   = ( int )tt_message_arg_val( incoming_msg, n++ );
  frame_rect.r_top    = ( int )tt_message_arg_val( incoming_msg, n++ );
  frame_rect.r_width  = ( int )tt_message_arg_val( incoming_msg, n++ );
  frame_rect.r_height = ( int )tt_message_arg_val( incoming_msg, n++ );
  location            = ( int )tt_message_arg_val( incoming_msg, n++ );

  /*
   * Hide the window before the move to get around XView bug 1048933.
   */

  /*
   * DS_POPUP_AOB used in this case from client to move off screen.
   * Use coordinates passed in.
   */
  if ( location == DS_POPUP_AOB ) {
    frame_get_rect( frame, &new_rect );
    new_rect.r_left = frame_rect.r_left;
    new_rect.r_top = frame_rect.r_top;
    frame_set_rect( frame, &new_rect );
  }
  else
    ds_position_popup_rect( &frame_rect, frame, location );

}

/*
 * This is called when the 'Apply' button is pressed 
 * from the Color Chooser sending back the selected color.
 */
void
color_tt_reply( color )
     Xv_singlecolor *color;
{
  char  hex_color[14];

  if (started_by_tooltalk) {
  /*
   * Fill in the arguments into the incoming message.
   */
  if ( windowmode ) {
    tt_message_arg_bval_set( in_process_msg, INOUT_ARG_BG_COLOR,
			     ( unsigned char * )color, sizeof( Xv_singlecolor ) );
  }
  else {
    tt_message_arg_bval_set( in_process_msg, INOUT_ARG_FG_COLOR, 
			     ( unsigned char * )color, sizeof( Xv_singlecolor ) );
  }
  /*
   * Reply to the request.
   */
  tt_message_reply( in_process_msg );

  /*
   * Don't destroy the message yet. 
   * color_tt_quit() checks if the message was handled
   * and returns a message failed if no reply.
   tt_message_destroy( in_process_msg );  
   */
   }
   else if (started_by_ma) {
     hex_color[0] = NULL;
     sprintf (hex_color, "#%04x%04x%04x", color->red, color->green, color->blue);
     if (prev_msgid) {
       free (prev_msgid);
       prev_msgid = NULL;
     }
/*
 * If frame is not pinned, then send user request cancel
 * to NOT send another request message, but don't fail it.
 */
     if (!xv_get (frame, FRAME_CMD_PUSHPIN_IN))
       dstt_set_status (tt_msg, dstt_status_user_request_cancel);
/*
 * Return the data.
 */
     dstt_message_return (tt_msg, hex_color, strlen (hex_color));
     tt_msg = NULL;
   }

}

