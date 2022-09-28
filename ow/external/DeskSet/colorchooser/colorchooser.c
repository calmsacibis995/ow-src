#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)colorchooser.c	3.7 12/23/92 Copyright 1990 Sun Microsystems, Inc.";
#endif
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>

#include "colorchooser.h"
#include "color_tt.h"

#include <xview/cms.h>
#include <xview/text.h>
#include <xview/font.h>
#include <ds_popup.h>
#include <dstt.h>

Frame       frame;
Panel       panel;
int         showing_factory;
int         color = TRUE;
int         started_by_tooltalk = FALSE;
int         started_by_ma = FALSE;
Display    *dsp;
Drawable    drawable;


/*ARGSUSED*/
static void
apply_props_proc(panel_item, event)
    Panel_item  panel_item;
    Event      *event;
{
    Xv_singlecolor   scolor;

    /*
     * Apply colors and tell ToolTalk about it.
     */
    apply_colors( &scolor );
    color_tt_reply( &scolor );
    /*
     * Kill the Color Chooser.
     */

     if ( !xv_get( frame, FRAME_CMD_PUSHPIN_IN ) ) {
       xv_destroy_safe( frame );
       exit (0);
     }

}


static void
reset_props_proc(panel_item, event)
    Panel_item  panel_item;
    Event      *event;
{

    if (color)
	reset_colors();
    xv_set(panel_item, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
   
}


add_buttons()
{
#ifdef NEVER
    Font_string_dims apply_size;
    Font        font;
    Font_string_dims reset_size;
    int         width;
    int         height;

    font = xv_get(frame, WIN_FONT);
    xv_get(font, FONT_STRING_DIMS, "Apply", &apply_size);
    xv_get(font, FONT_STRING_DIMS, "Reset", &reset_size);
    width = (int) xv_get(panel, XV_WIDTH);
#endif
    Panel_item apply_button;
    Panel_item reset_button;

    apply_button = xv_create(panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, MGET( "Apply" ),
	      PANEL_NOTIFY_PROC, apply_props_proc,
	      PANEL_NEXT_ROW, -1,
#ifdef NEVER
	      XV_X, (width / 4) - (apply_size.width / 2),
#endif
	      XV_HELP_DATA, "colorchooser:ApplyInfo",
	      NULL);

    reset_button = xv_create(panel, PANEL_BUTTON,
	      PANEL_LABEL_STRING, MGET( "Reset" ),
	      PANEL_NOTIFY_PROC, reset_props_proc,
#ifdef NEVER
	      XV_X, (3 * width) / 4 - (reset_size.width / 2),
#endif
	      XV_HELP_DATA, "colorchooser:ResetInfo",
	      NULL);

    ds_center_items( panel, 7, apply_button, reset_button, NULL );

    window_fit_height( panel );

}

create_panel()
{
    panel = xv_get( frame, FRAME_CMD_PANEL );
    xv_set( panel, XV_X, 0,
	           XV_Y, 0,
	           NULL );

    if (color)
	create_color_panel();

}

Notify_value
frame_unmap_proc(frame, event, arg, type)
    Frame       frame;
    Event      *event;
    Notify_arg  arg;
    Notify_event_type type;
{

    if (event_action(event) == ACTION_CLOSE) {
        exit(0);
    }
    else if (event_action(event) == ACTION_DISMISS) {
      if (started_by_ma) {
         cancel_message();
      }
      exit(0);
    }
    return (notify_next_event_func(frame, (Notify_event)event, arg, type));
}

main(argc, argv)
    int         argc;
    char       *argv[];
{
  char         *s;
  char         *relname, *hostname;
  char         *label;
  int           status;
  int           visual;
  char          bind_home[MAXPATHLEN];
  extern char  *ds_relname();
  extern char  *ds_hostname();
  extern void   tool_info();
    /*
     * Initialize ToolTalk and tell it what the valid types are
     * and the callback to invoke when data is demanded.
     */
    status = color_tt_init( argc, argv );
    if ((!status && started_by_tooltalk ) ||
	(!status && started_by_ma))
      exit( 0 );
    else if (status && started_by_ma)
      dstt_check_startup (tool_info, &argc, &argv);
 
    xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
	    XV_USE_LOCALE, TRUE,
	    NULL);

    ds_expand_pathname( "$OPENWINHOME/lib/locale", bind_home );
    bindtextdomain( MSGFILE, bind_home );
    textdomain( MSGFILE );

/*
 * If tooltalk, then create off screen, position later.
 */
    if (started_by_tooltalk || started_by_ma) 
      frame = xv_create(NULL, FRAME_CMD, 
		      XV_X, -400, 
		      XV_Y, -400,
#ifdef OW_I18N
		      WIN_USE_IM, FALSE,
#endif
		      FRAME_NO_CONFIRM, TRUE,
		      FRAME_CMD_PUSHPIN_IN, FALSE,
		      FRAME_SHOW_FOOTER, TRUE,
		      NULL);
    else
      frame = xv_create(NULL, FRAME_CMD, 
		      XV_X, 0,
		      XV_Y, 0,
#ifdef OW_I18N
		      WIN_USE_IM, FALSE,
#endif
		      FRAME_NO_CONFIRM, TRUE,
		      FRAME_CMD_PUSHPIN_IN, FALSE,
		      FRAME_SHOW_FOOTER, TRUE,
		      NULL);

    notify_interpose_event_func(frame, frame_unmap_proc, NOTIFY_SAFE);

    dsp = (Display *) xv_get(frame, XV_DISPLAY);
    drawable = ( Drawable )xv_get( frame, XV_XID );
    /*
     * Do not start up if monochrome.
     */
    if ( ( int )xv_get( frame, WIN_DEPTH ) < 4 ) {
      fprintf( stderr, MGET( " Colorchooser is not useful on monochrome - exiting.\n" ) );
      exit( 0 );
    }

    relname  = ds_relname();
    hostname = ds_hostname( dsp );
    s = MGET( "Color Chooser" );
    label = ( char * )malloc( strlen( relname ) + strlen( hostname ) +
			strlen( s ) + 3 );
    sprintf( label, "%s %s%s", s, relname, hostname );
    xv_set( frame, FRAME_LABEL, label, NULL );

    visual = xv_get( frame, XV_VISUAL_CLASS );

/* Grayscale and StaticGray should be considered color, too!

    color = ( visual == PseudoColor || visual == StaticColor ||
              visual == TrueColor   || visual == DirectColor );
*/

    create_panel();

    add_buttons();

    xv_set(frame,
           XV_WIDTH,  xv_get( panel, XV_WIDTH ),
           XV_HEIGHT, xv_get( panel, XV_HEIGHT )+19,
           NULL );

    init_dragdrop();

    if (started_by_tooltalk)
      color_complete_tt_init (frame);
    else if (started_by_ma)
      colorchooser_dstt_start (frame);

/*
    if (started_by_tooltalk || started_by_ma)
      notify_start();
*/

    /*window_main_loop(frame); */
     xv_main_loop (frame);

    /*
     * Shut down ToolTalk.
     * Called after xv_main_loop()
     */
     if (started_by_tooltalk)
       color_tt_quit();

}
