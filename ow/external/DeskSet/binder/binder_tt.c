#ifndef lint
#ifdef sccs
static char sccsid[]="@(#)binder_tt.c	3.1 03 Apr 1992 Copyright 1987-1990 Sun Microsystems, Inc.";
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
 * binder_tt.c - ToolTalk interface functions for the binder.
 */
#ifdef TOOLTALK

#include <stdio.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <desktop/tt_c.h>
#include <ds_popup.h>
#include <ds_tooltalk.h>
#include <dstt.h>

#include "binder.h"
#include "xv.h"
#include "props.h"

#define RETURN_FG_COLOR    3
#define RETURN_BG_COLOR    4

extern Binder         *binder;
extern Tns_entry       tentries[ MAX_TNS_ENTRIES ];
extern int  	       current_selection();
extern void 	       refresh_icon_color();

static Notify_value    handle_message();
static void            handle_reply();
static int             background;
static int             waiting_for_launch_reply = FALSE;
static int             handler_started = FALSE;
static char           *tt_handler = NULL;
static char           *tt_open_method = NULL;
static char            tt_started;
static char           *binder_msgid;


char 		      *toolid = NULL;

void 
tool_info (vend, name, ver)
  char **vend;
  char **name;
  char **ver;
{
  extern char *xv_version;
  extern char *ds_vendname();

  *vend = (char *) strdup (ds_vendname());
  *name = "binder";
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
  int 	      status;
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
  Xv_singlecolor  color; 
  extern void     b_get_color();

  switch (tt_message_state (ttmsg)) {
  case TT_HANDLED:
    if (sscanf (data, "#%04x%04x%04x", &r, &g, &b) == 3) {

#ifdef DEBUG
printf ("rgbs returned to binder = (%d %d %d)\n", r, g, b);
#endif
    color.red = r;
    color.green = g;
    color.blue = b;

    if ( background )
      refresh_icon_color( binder, &color, TRUE );
    else
      refresh_icon_color( binder, &color, FALSE );
     
    }

    if (toolid) {
      free (toolid);
      toolid = NULL;
    }
/*
 * Resend again if user did not cancel.
 */
    if (dstt_test_status (ttmsg) != dstt_status_user_request_cancel)
      b_get_color(NULL, NULL); 
    break;

  case TT_FAILED:
#ifdef DEBUG
    fprintf (stderr, "tt msg TT_FAILED\n");
#endif
    handler_started = FALSE;
    /*setactive(); */
    break;
  case TT_REJECTED:
#ifdef DEBUG
    fprintf (stderr, "tt msg TT_FAILED\n");
#endif
    handler_started = FALSE;
    /*setactive(); */
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
    /*setactive(); */
    handler_started = TRUE;

}

int
quit_callback (ttmsg, key, status, silent, force, msgid)
    Tt_message   ttmsg; 
    void        *key;
    int          status;
    int 	 silent;
    int          force;
    char        *msgid;
{
#ifdef DEBUG
    fprintf (stderr, "binder: quit_callback\n");
#endif

   handler_started = FALSE;
   return (0);
} 

/*
 * Connect with Tooltalk.
 * This can be called after xv_init() because
 * the DISPLAY environment variable is not being set here.
 */
void
b_init_tt( argc, argv )
    int     argc;
    char  **argv;
{
#ifdef OLD_TOOLTALK
    /*
     * Connect to the ToolTalk server and set flag.
     * Gray out color buttons if ToolTalk not active.
     */
    if ( ds_tooltalk_init( NULL, argc, argv ) != 0 ) {
      tt_started = FALSE;
      fputs( ( char * )MGET( "Warning: ToolTalk not initialized in Binder!\n" ) , stderr );
    }   
    else 
      tt_started = TRUE;
#endif
  
}

/*ARGSUSED*/
void
b_init_tt_complete( b )
    Binder *b;
{
    Props *p = ( Props * )b->properties;
    CE_ENTRY  entry;

    ds_position_popup (b->frame, p->frame, DS_POPUP_LOR);
    dstt_xview_desktop_callback (p->frame, NULL);
    dstt_xview_desktop_register (p->frame, NULL);
    dstt_notice_callback ((char *)b->frame, STATUS, status_handler, NULL);
    dstt_notice_register ((char *)b->frame, STATUS, TRUE, NULL);
    dstt_xview_start_notifier ();

    binder_msgid = dstt_messageid();

#ifdef OLD_TOOLTALK
    if ( !tt_started ) {
      xv_set( p->fg_color_button, PANEL_INACTIVE, TRUE, XV_NULL );
      xv_set( p->bg_color_button, PANEL_INACTIVE, TRUE, XV_NULL );
      b->tt_running = FALSE;
      return;
    }
    else 
      b->tt_running = TRUE;

    /*
     * Register a callback when a reply to the  "get_color"  message comes in.
     */
    ds_tooltalk_set_callback( p->frame, handle_message );

    /*
     * Get the ToolTalk open method for the ColorChooser.
     * Get the tooltalk handler for the messages.
     */
#ifdef LATER
    entry = ce_get_entry( ip->tns, 1, "colorchooser" );
    if ( entry != NULL ) { 
      tt_open_method = ce_get_attribute( ip->tns, entry, ip->type_open_tt );
      tt_handler     = ds_tooltalk_get_handler( tt_open_method );
    }
    else
      fprintf( stderr, MGET( "Binder: Could not get open method for colorchooser\n" ) );
#else
    tt_handler = ds_tooltalk_get_handler( "colorchooser" );
    if ( !tt_handler )
      fprintf( stderr, MGET( "Binder: Could not get open method for colorchooser\n" ) );
#endif
#endif
    
}

static void
request_launch()
{
    Tt_message   request_msg;
    Tt_status    tt_status;
    int          i;
    int          ivalue;  
    char        *value;

    i = current_selection( binder->tns_list );
    if ( i == - 1 ) {
      window_bell( binder->frame );
      xv_set( binder->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }
    /*
     * Send a LAUNCH message to start the colorchooser.
     */
    request_msg = ds_tooltalk_create_message( tt_handler, DS_TT_LAUNCH_MSG, NULL );
    if ( request_msg == NULL )
      fprintf( stderr, MGET( "Binder: Couldn't create request message.\n" ) );
    /*
     * Set the DISPLAY environment variable and locale.
     */
    value = ( char * )xv_get( binder->frame, XV_LC_BASIC_LOCALE );
    tt_message_arg_add( request_msg, TT_IN, "string", value );

    ivalue = ( int )xv_get( binder->frame, WIN_DEPTH );
    tt_message_iarg_add( request_msg, TT_IN, "int", ivalue );
    
    ivalue = ( int )xv_get( binder->frame, XV_VISUAL_CLASS );
    tt_message_iarg_add( request_msg, TT_IN, "int", ivalue );

    waiting_for_launch_reply = TRUE;
    /*
     * Send out the request.
     */
    if ( ( tt_status = tt_message_send( request_msg ) ) != TT_OK ) 
      fprintf( stderr, MGET( "Couldn't sent message. Status: %d\n" ), tt_status );
}



static void
request_color()
{
    int             i, r = 0, g = 0, blue = 0, r1 = 0, g1 = 0, b1 = 0;
    char            iconname[ MAXPATHLEN ];
    char            iconmask[ MAXPATHLEN ];
    Xv_singlecolor  fg, bg;
    Tt_message      request_msg;
    Tt_status       tt_status;
    char           *fg_item, *bg_item;
    Props          *p = ( Props * )binder->properties;
    
    i = current_selection( binder->tns_list );
    if ( i == -1 ) {
      window_bell( binder->frame );
      xv_set( binder->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }
    
    /*
     * Send a DS_TT_NON_STD_MSG with appropriate data.
     */
    request_msg = ds_tooltalk_create_message( tt_handler, DS_TT_NO_STD_MSG, "get_color" );
    if ( request_msg == NULL )
      fprintf( stderr, MGET( "Binder: Couldn't create request message.\n" ) );
    
    /*
     * Requesting three new RGBs.
     */
    fg_item = ( char * )xv_get( p->fg_color_item, PANEL_VALUE );
    bg_item = ( char * )xv_get( p->bg_color_item, PANEL_VALUE );
    if ( *fg_item != NULL )
      ( void )sscanf( fg_item, "%d %d %d", &r, &g, &blue );
    else 
      r = g = blue = 0;
    
    fg.red = ( u_char )r;   fg.green = ( u_char )g;   fg.blue = ( u_char )blue;
    
    if ( *bg_item != NULL ) {
      ( void )sscanf( bg_item, "%d %d %d", &r1, &g1, &b1 );
    }
    else {
      r1 = 255;
      g1 = 255;
      b1 = 255;
    }
    bg.red = ( u_char )r1;  bg.green = ( u_char )g1;  bg.blue = ( u_char )b1;
    
    if ( background )
      tt_message_arg_add( request_msg, TT_IN,   "string" , 
			 ( void * ) MGET("Choosing Background Color for Binder") );  /* 0 */
    else
      tt_message_arg_add( request_msg, TT_IN,   "string" , 
			 ( void * ) MGET("Choosing Foreground Color for Binder") );  /* 0 */
    
#ifdef DEBUG   
    printf( " Binder: Current fg = %d %d %d\n" , r, g, blue );
    printf( " Binder: Current bg = %d %d %d\n" , r1, g1, b1 );
#endif

    if ( tentries[i].icon_file )
      ds_expand_pathname( tentries[i].icon_file, iconname );
    if ( tentries[i].icon_mask_file )
      ds_expand_pathname( tentries[i].icon_mask_file, iconmask );
    
    tt_message_arg_add( request_msg, TT_IN, "string" , ( void * )iconname );  /* 1 */
    tt_message_arg_add( request_msg, TT_IN, "string" , ( void * )iconmask );  /* 2 */
    tt_message_barg_add( request_msg, TT_INOUT, "Xv_singlecolor",             /* 3 */
			( unsigned char * )&fg, sizeof( Xv_singlecolor ) );
    tt_message_barg_add( request_msg, TT_INOUT, "Xv_singlecolor" ,            /* 4 */
			( unsigned char * )&bg, sizeof( Xv_singlecolor ) );
    /*
     * Send out the request.
     */
    if ( ( tt_status = tt_message_send( request_msg ) ) != TT_OK ) 
      fprintf( stderr, MGET( "Couldn't send message. Status: %d\n" ), tt_status );

}


/*
 * Create and send a message to request a color from the Color Editor.
 */
/*ARGSUSED*/
void
b_get_color( item, event )
    Panel_item item;
    Event      *event;
{
    char *fg_item, *bg_item;
    Props          *p = ( Props * )binder->properties;
    int             r, g, b, r1, g1, b1;
    int		    i;
    char            hex_color[28];
    char            iconname[ MAXPATHLEN ];
    char            iconmask[ MAXPATHLEN ];

    /*
     * Check in case tooltalk is not running.  The color 
     * button should be inactivated anyway.
     */
#ifdef OLD_TOOLTALK
    if ( !binder->tt_running )
      return;
  
    /*
     * First, find out if we've started the handling application (Colorchooser).
     * If not, start it first.
     */
    if ( handler_started == FALSE ) 
      request_launch();
    else
      request_color();
#endif
    /*
     * Requesting three new RGBs.
     */
    fg_item = ( char * )xv_get( p->fg_color_item, PANEL_VALUE );
    bg_item = ( char * )xv_get( p->bg_color_item, PANEL_VALUE );
    if ( *fg_item != NULL )
      ( void )sscanf( fg_item, "%d %d %d", &r, &g, &b );
    else 
      r = g = b = 0;

    if (*bg_item != NULL)
      ( void )sscanf( bg_item, "%d %d %d", &r1, &g1, &b1 );
    else 
      r1 = g1 = b1 = 255;
    
    hex_color[0] = NULL;
    sprintf (hex_color, "#%04x%04x%04x#%04x%04x%04x", r, g, b, r1, g1, b1 );

    i = current_selection( binder->tns_list );
    if ( i == -1 ) {
      window_bell( binder->frame );
      xv_set( binder->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }
    
    if ( tentries[i].icon_file )
      ds_expand_pathname( tentries[i].icon_file, iconname );
    if ( tentries[i].icon_mask_file )
      ds_expand_pathname( tentries[i].icon_mask_file, iconmask );

#ifdef DEBUG
printf("hex_color is %s\n", hex_color);
printf("hex_color len is %d\n", strlen(hex_color));
printf("iconname is %s len is %d\n", iconname, strlen(iconname));
printf("iconmask is %s len is %d\n", iconmask, strlen(iconmask));
#endif
    /*setbusy(); */

    if (background)
      dstt_prop_edit (edit_callback, "binder", "Color_Set", 
	 	      hex_color, strlen (hex_color), 
	              iconname, strlen (iconname),
	              iconmask, strlen (iconmask),
		      binder_msgid,
	              (char *)MGET ("Choosing Background Color for Binder"));
    else
      dstt_prop_edit (edit_callback, "binder", "Color_Set", 
	 	      hex_color, strlen (hex_color), 
	              (void *)iconname, strlen (iconname),
	              (void *)iconmask, strlen (iconmask),
		      binder_msgid,
	              (char *)MGET ("Choosing Foreground Color for Binder"));

}

/*
 * Create and send a message to request a color from the Color Editor.
 */
void
b_fgcolor_tt( item, event )
    Panel_item item;
    Event      *event;
{
    background = FALSE;
    b_get_color( item, event );
}

/*
 * Create and send a message to request a color from the Color Editor.
 */
void
b_bgcolor_tt( item, event )
    Panel_item item;
    Event      *event;
{
    background = TRUE;
    b_get_color( item, event );
}

void
b_open_frame( b )
    Binder  *b;
{
    Tt_message  move_msg;
    Tt_status   status;
    Rect        frame_rect;
    Props      *p = ( Props * )b->properties;

    /*
     * Return if the handler is not started.
     */
    if ( !handler_started )
      return;

    dstt_set_mapped ((Mapped_CB *)map_callback, "binder", toolid, TRUE, NULL, NULL);
#ifdef OLD_TOOLTALK
    /*
     * MOVE the handler back to its original coordinates.
     */
    frame_get_rect( p->frame, &frame_rect );
    move_msg = ds_tooltalk_create_message( tt_handler, DS_TT_MOVE_MSG, NULL );
    
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_left );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_top );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_width );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_height );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void *)DS_POPUP_LOR );
    
    status = tt_message_send( move_msg );
    if ( status != TT_OK )
      fprintf( stderr, MGET( "Binder: Could not send move message: \n" ) );

    if ( p->full_size ) {
      xv_set( p->panels[ICON_PANEL_R], XV_SHOW, TRUE, NULL );
      xv_set( p->icon_plus_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->icon_minus_button, PANEL_INACTIVE, FALSE, NULL );
    }
#endif
    
}


b_close_frame( b )
    Binder *b;
{
    Rect        frame_rect;
    int         width, height;
    Tt_message  move_msg;
    Tt_status   status;
    Props      *p = ( Props * )b->properties;

    /*
     * Return if handler is not started.
     */
    if ( !handler_started )
      return;

    dstt_set_mapped ((Mapped_CB *)map_callback, "binder", toolid, FALSE, NULL, NULL);

#ifdef OLD_TOOLTALK
    /*
     * Move the handler off the screen.
     * Save the original coordinates.
     */
    frame_get_rect( p->frame, &frame_rect );
    ds_get_screen_size( p->frame, &width, &height );
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
      fprintf( stderr, MGET( "Binder: Could not send move message: \n" ) );
#endif
   
    
}

/*
 * Receive incoming Tooltalk message and dispatch them 
 * to the appropriate routines.
 */
/* ARGSUSED */
static Notify_value
handle_message( client, fd )
    Notify_client  client;
    int            fd;
{
    Tt_message reply_msg;

    /*
     * There is a message awaiting from Tooltalk.
     */
    if (( reply_msg = tt_message_receive() ) != TT_OK ) 
      dispatch_message( reply_msg );
    
    return( NOTIFY_DONE );
    
}

/*
 * This callback is called when a reply to a  "get_color"  message
 * comes in.  It passes the data to the application's receive-data 
 * callback.
 */
static void
handle_reply( reply_msg )
    Tt_message  reply_msg;
{
    Xv_singlecolor *color; 
    int             nbytes;

    if ( background ) {
      tt_message_arg_bval( reply_msg, RETURN_BG_COLOR, ( unsigned char ** )&color, &nbytes );
      refresh_icon_color( binder, color, TRUE );
    }
    else {
      tt_message_arg_bval( reply_msg, RETURN_FG_COLOR, ( unsigned char ** )&color, &nbytes );  
      refresh_icon_color( binder, color, FALSE );
    }
    
#ifdef DEBUG
    printf( " Binder: values are red=%d, green=%d, blue=%d\n" , color->red, color->green, color->blue );
    tt_message_destroy( reply_msg );
#endif

}


/*
 *
 */
static void
binder_send_move_msg( frame )
    Frame  frame;
{
    Rect        frame_rect;
    Tt_message  move_msg;
    Tt_status   status;

    frame_get_rect( frame, &frame_rect );
    move_msg = ds_tooltalk_create_message( tt_handler, DS_TT_MOVE_MSG, NULL );
    
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_left );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_top );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_width );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void * )frame_rect.r_height );
    tt_message_arg_add( move_msg, TT_IN, "int", ( void *)DS_POPUP_LOR );
    
    status = tt_message_send( move_msg );
    if ( status != TT_OK )
      fprintf( stderr, MGET( "Binder: Could not send move message: \n" ) );

}



/*
 *
 */
static void
binder_send_expose_msg()
{
    Tt_message  expose_msg;
    Tt_status   status;

    if ( handler_started ) {
      expose_msg = ds_tooltalk_create_message( tt_handler, DS_TT_EXPOSE_MSG, NULL );
      
      status = tt_message_send( expose_msg );
      if ( status != TT_OK ) 
	fprintf( stderr, MGET( "Binder: Could not send expose message: \n" ) );
    }

}

/*
 *
 */
static void
binder_send_quit_msg()
{
    Tt_message  quit_msg;
    Tt_status   status;

    if ( handler_started ) {
      quit_msg = ds_tooltalk_create_message( tt_handler, DS_TT_QUIT_MSG, NULL );
      
      status = tt_message_send( quit_msg );
      if ( status != TT_OK ) 
	fprintf( stderr, MGET( "Binder: Could not send quit message: \n" ) );
    }
    
}

/*
 * Shut down ToolTalk.
 * Called after xv_main_loop().
 */
void
b_quit_tt()
{

  /*
   * Send a quit message to handling application
   * and quit.
   */
#ifdef OLD_TOOLTALK
    if ( binder->tt_running ) {
      binder_send_quit_msg();
      ds_tooltalk_quit();
      binder->tt_running = FALSE;
    }
#endif    
      dstt_quit (quit_callback, "binder", toolid,
	  	 TRUE, FALSE, NULL);
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
    Props          *p = ( Props * )binder->properties;

    /*
     * Determine the state of the message - if it's TT_SENT
     * then extract the values. 
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
         * Send a move message so the colorchooser is positioned to the correct spot.
         */
      case DS_TT_LAUNCH_MSG:
	if ( waiting_for_launch_reply == TRUE ) {
	  binder_send_move_msg( p->frame  );
	  handler_started = TRUE;
	  waiting_for_launch_reply = FALSE;
	}
	break;
	
        /*
         * We been notified that the ColorChooser has been moved.
         * Send an expose message to make it visible.
         * Then send a request message for a color.
         */
      case DS_TT_MOVE_MSG:
	binder_send_expose_msg();
	xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
	if ( background )
	  b_get_color( p->bg_color_button, ( Event * )NULL );
	else
	  b_get_color( p->fg_color_button, ( Event * )NULL );
	break;
        /*
         * We just got a color back from the Color Chooser.
         * Resend another request.
         */
      case DS_TT_NO_STD_MSG:
	handle_reply( message );
	if ( background )
	  b_get_color( p->bg_color_button, ( Event * )NULL );
	else
	  b_get_color( p->fg_color_button, ( Event * )NULL );
	break;
        
      default:
	break;
      }
      break;
      
    case TT_SENT:
      switch ( info.ds_tt_msg_type ) {
        /*
         * The Color Chooser is departing.  Reset the flag.
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
       * starting up the colorchooser.  Set the binder footer message in the binder.
       */
      if ( info.ds_tt_msg_type == DS_TT_LAUNCH_MSG )
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Starting Color Chooser" ), NULL );
      
      break;
      
    case TT_REJECTED:
      handler_started = FALSE;
      fprintf( stderr, MGET( " Binder: Help! message rejected\n " ) );
      break;
      
    case TT_FAILED:
      handler_started = FALSE;
#ifdef FIX
      xv_set( p->frame, FRAME_LEFT_FOOTER, "Color Chooser Failed.", NULL );
#else
      xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
#endif
      break;
      
    default:
#ifdef DEBUG
      printf( " Binder: default - %d\n" , ( int )tt_message_state( message ) );
#endif
      break;
    }

}

#endif
