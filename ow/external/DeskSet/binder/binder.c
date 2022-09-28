#ifndef lint
#ifdef sccs
static char sccsid[]="@(#)binder.c	3.28 06/10/96 Copyright 1987-1990 Sun Microsystems, Inc.";
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

#include <stdio.h>
#include <sys/param.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <xview/xview.h>
#include <xview/alert.h>
#include <xview/cms.h>
#include <xview/panel.h>
#include <xview/svrimage.h>
#include <xview/scrollbar.h>

#include "ds_listbx.h"
#include "ds_colors.h"
#include "binder.h"
#include "binder_tt.h"
#include "xv.h"
#include "binder_ce.h"
#include "props.h"

#define R_ONLY_ENTRY          MGET( "Read-Only Entry" )
#define RW_ENTRY              "" 
#define CONTENT_ENTRY  MGET ("Content")

#define UNNAMED                 MGET ("unnamed")
 
/* I18N_STRING -
   sys = short for system;
         as in user, system, network databases
                ($HOME, /etc, $OPENWINHOME)
         Used when creating a new entry, e.g. "unnamed_sys_1".
         Not absolutely necessary to translate.

 */                                             
#define SYSTEM                  MGET ("sys")
 
/* I18N_STRING -
   net = short for network;
         as in user, system, network databases
                ($HOME, /etc, $OPENWINHOME)
         Used when creating a new entry, e.g. "unnamed_net_1".
         Not absolutely necessary to translate.
 */
#define NETWORK                 MGET ("net")
 
typedef enum {
        NEW,
        DUP,
        DEL
} Operation;

Binder         *binder;
Display	       *dpy;

XviewInfo      *xview_info;
Tns_entry       tentries[ MAX_TNS_ENTRIES ];
Fns_entry       fentries[ MAX_FNS_ENTRIES ];
int		nfns;
int	        ntns;
Tns_entry       def_entry;
Tns_entry       undo_entry;
Tns_entry       reset_entry;
Operation       undo_op;
Server_image    temp_image;
int             curr_row;
int             prev_row;
int             entry_created;

extern char    *ds_relname();
extern char    *ds_hostname();

static GC	  gc = NULL;                /* Graphics Context */
static GC	  bg_gc = NULL;             /* Graphics Context for masks */
static XGCValues  gcvals;                   /* GC Values */
static Drawable	  drawable;	            /* XID of frame */
static char       unnamed[80];

/*
 * Instance XV_KEY_DATA key.  An instance is a set of related
 * user interface objects.  A pointer to an object's instance
 * is stored under this key in every object.  This must be a
 * global variable.
 */
Attr_attribute  INSTANCE;


/*
 * XErrorHandler
 */
static int 
error_handler( dpy, event )
    Display       *dpy;
    XErrorEvent   *event;
{
    char buffer[BUFSIZE];
    char mesg[BUFSIZE];
    char number[32];
    char *mtype = "XlibMessage";
    FILE *fp = stdout;

    XGetErrorText( dpy, event->error_code, buffer, BUFSIZE );
    XGetErrorDatabaseText( dpy, mtype,  "XError" ,  "X Error (intercepted)" , 
	  		   mesg, BUFSIZE );
    ( void )fprintf( fp,  "%s:  %s\n  " , mesg, buffer );
    XGetErrorDatabaseText( dpy, mtype,  "MajorCode" ,  "Request Major code %d" , 
			   mesg, BUFSIZE );
    ( void )fprintf( fp, mesg, event->request_code );
    sprintf( number, "%d", event->request_code );
    XGetErrorDatabaseText( dpy,  "XRequest" , number, "", buffer, BUFSIZE ); 
    ( void )fprintf(fp,  " (%s)" , buffer );
    fputs( "\n  " , fp );
    XGetErrorDatabaseText( dpy, mtype,  "MinorCode" ,  "Request Minor code" ,
			   mesg, BUFSIZE );
    ( void )fprintf( fp, mesg, event->minor_code );
    fputs( "\n  " , fp );
    XGetErrorDatabaseText( dpy, mtype,  "ResourceID" ,  "ResourceID 0x%x" ,
			    mesg, BUFSIZE );
    ( void )fprintf(fp, mesg, event->resourceid );
    fputs( "\n  " , fp );
    XGetErrorDatabaseText( dpy, mtype,  "ErrorSerial" ,  "Error Serial #%d" , 
			    mesg, BUFSIZE );
    ( void )fprintf( fp, mesg, event->serial );
    fputs( "\n  " , fp );
    XGetErrorDatabaseText( dpy, mtype,  "CurrentSerial" ,  "Current Serial #%d" ,
			   mesg, BUFSIZE );
    ( void )fprintf( fp, mesg, NextRequest (dpy) );
    fputs(  "\n" , fp );

}


/*
 *
 */
static void
b_init_scope( argc, argv, b )
    int      argc;
    char   **argv;
    Binder  *b;
{
    int   i;
    Menu  view_menu;
    extern char *xv_version;

    b->scope = USER;  /* Default */

    for ( i = 1; i < argc; ++i ) {
      if ( ( strcmp( argv[i], "-user" ) == 0 ) || 
	   ( strcmp( argv[i], "-User" ) == 0 ) ) 
        b->scope = USER;

      else if ( ( strcmp( argv[i], "-system" ) == 0 ) || 
  	        ( strcmp( argv[i], "-System" ) == 0 ) ) {
        b->scope = SYS;
        view_menu = ( Menu )xv_get( b->view_button, PANEL_ITEM_MENU );
        xv_set( xv_get( view_menu, MENU_NTH_ITEM, 3 ), MENU_STRING, MGET( "Local Entries" ), NULL );
      }
      else if ( ( strcmp( argv[i], "-network" ) == 0 ) ||
		( strcmp( argv[i], "-Network" ) == 0 ) ) {
        b->scope = NET;
        xv_set( b->view_button, PANEL_INACTIVE, TRUE, NULL );
      }
      else if ( strcmp( argv[i], "-v" ) == 0 ) {
        fprintf( stderr, "binder version %s running on %s\n", ds_relname(), xv_version );
        exit( 0 );
      }
      else {
        fprintf( stderr, MGET( "Usage: binder [ -user | -system | -network ] [-v] [ generic-tool-arguments ]\n" ) );
        exit( 0 );
      }
    }

}

/*
 * Initialize some binder objects to initial state of binder.
 */
static void
b_init_state( b )
    Binder  *b;
{
    Props      *p = ( Props * )b->properties;
    Menu        view_menu;

    dpy      = ( Display * )xv_get( b->frame, XV_DISPLAY );
    drawable = ( Drawable )xv_get( b->frame, XV_XID );
    
    /*
     * Grey out color buttons if not 8-bit depth.
     */
    b->color = ( ( int )xv_get( b->frame, WIN_DEPTH ) >= 8 );      /* Color or Monochrome */
    if ( !b->color ) {
      xv_set( p->fg_color_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->bg_color_button, PANEL_INACTIVE, TRUE, NULL );
    }
    b->view = ALL;
    b->applying_changes = FALSE;
    p->icon_props_changed = FALSE;
    p->binding_props_changed = FALSE;
    /*
     * Nothing to undo yet.
     */
    xv_set( b->undo_button, PANEL_INACTIVE, TRUE, NULL );
    view_menu = ( Menu )xv_get( b->view_button, PANEL_ITEM_MENU );
#ifdef NEVER
    xv_set( xv_get( view_menu, MENU_NTH_ITEM, 2 ), MENU_INACTIVE, TRUE, NULL );
    xv_set( xv_get( view_menu, MENU_NTH_ITEM, 3 ), MENU_INACTIVE, TRUE, NULL );
#endif
}


/*
 * Create title bar name - edited/not edited
 */
char *
title_bar_name( b, edited )
    Binder  *b;
    int      edited;
{
    char  label[128];
    char  *relname;
    char  *hostname;
    char  *program;

    program = MGET( "Binder" );
    relname  = ds_relname();
    hostname = ds_hostname( xv_get( b->frame, XV_DISPLAY ) );
    
    if ( edited ) 
      sprintf( label, "%s %s%s%s", program, relname, hostname, MGET( " - edited" ) );
    else 
      sprintf( label, "%s %s%s", program, relname, hostname );
 
    return( label );   
}

/*
 * Change the color chips on the panel to the 
 * fg/bg colors of the currently selected item.
 */
void
change_color_chips( b )
    Binder *b;
{
    Props           *p = ( Props * )b->properties;
    int		     i, height, width;
    unsigned long    valuemask;
    Pixmap	     fg_pixmap, bg_pixmap;
  
    if ( !b->color ) 
      return;

    i = current_selection( b->tns_list );
    if ( i == - 1 ) 
      return;
  
    fg_pixmap = ( Pixmap )xv_get( fg_chip, SERVER_IMAGE_PIXMAP );
    bg_pixmap = ( Pixmap )xv_get( bg_chip, SERVER_IMAGE_PIXMAP );
    height = ( int )xv_get( fg_chip, XV_HEIGHT );
    width  = ( int )xv_get( fg_chip, XV_WIDTH );
    /*
     * Draw the foreground color onto the pixmap
     * of the color chip and set this as the new image.
     */
    gcvals.foreground = ds_x_index( tentries[i].fg_index );
    gcvals.fill_style = FillSolid;
    gcvals.clip_mask = None;
    valuemask = GCForeground | GCFillStyle | GCClipMask;
    
    XChangeGC( dpy, gc, valuemask, &gcvals );
    XFillRectangle( dpy, fg_pixmap, gc, 0, 0, width, height );
    xv_set( fg_chip, SERVER_IMAGE_PIXMAP, fg_pixmap, NULL );
    xv_set( p->fg_color_chip, PANEL_LABEL_IMAGE, fg_chip, NULL );
    /*
     * Draw the foreground color onto the pixmap
     * of the color chip and set this as the new image.
     * Use a bg color if specified else just use window frame color.
     */
    gcvals.foreground = ds_x_index( tentries[i].bg_index );
    
    XChangeGC( dpy, gc, valuemask, &gcvals );
    XFillRectangle( dpy, bg_pixmap, gc, 0, 0, width, height );
    
    xv_set( bg_chip, SERVER_IMAGE_PIXMAP, bg_pixmap, NULL );
    xv_set( p->bg_color_chip, PANEL_LABEL_IMAGE, bg_chip, NULL );
    
}


/*  
 * Change the icon color to the new color.
 * If 'background' is set, change the background 
 * color otherwise change the foreground color.
 */
void
refresh_icon_color( b, scolor, background )
    Binder         *b;
    Xv_singlecolor *scolor;
    int             background;
{
    Props         *p = ( Props * )b->properties;
    int		   i, height, width;
    int            fg_index, bg_index;
    int            red, green, blue;
    Pixmap	   pixmap;
    unsigned long  valuemask;
    char           buf[ 64 ];
    Pixmap	   fg_pixmap, bg_pixmap;
    Xv_singlecolor sc;
    char          *value;
    
    if ( !b->color )
      return;

    i = current_selection( b->tns_list );
    if ( i == - 1 )
      return;
    
    if ( background ) {
      value = ( char * )xv_get( p->fg_color_item, PANEL_VALUE );
      if ( *value == NULL ) {
        sc.red = 0; sc.green = 0; sc.blue = 0;
        fg_index = ds_cms_index( b->cms, &sc ); 
      }
      else {
       	( void )sscanf( value, "%d %d %d", &red, &green, &blue );
        sc.red = ( u_char )red; sc.green = ( u_char )green; sc.blue = ( u_char )blue;
        fg_index = ds_cms_index( b->cms, &sc );
      }
      bg_index = ds_cms_index( b->cms, scolor );
    }
    else {
      fg_index = ds_cms_index( b->cms, scolor );
      value = ( char * )xv_get( p->bg_color_item, PANEL_VALUE );
      if ( *value == NULL ) {
        sc.red = 0; sc.green = 0; sc.blue = 0;
        bg_index = ds_cms_index( b->cms, &sc ); 
      }
      else {
       	sscanf( value, "%d %d %d", &red, &green, &blue );
        sc.red = ( u_char )red; sc.green = ( u_char )green; sc.blue = ( u_char )blue;
        bg_index = ds_cms_index( b->cms, &sc );
      }
    }
    
    sprintf( buf, "%d %d %d", scolor->red, scolor->green, scolor->blue );
    if ( background )
        xv_set( p->bg_color_item, PANEL_VALUE, buf, NULL );
    else 
        xv_set( p->fg_color_item, PANEL_VALUE, buf, NULL );

    /* 
     * Set the the GC values.
     */
    gcvals.background = ds_x_index( bg_index );
    gcvals.foreground = ds_x_index( fg_index );
    gcvals.stipple = ( Pixmap )xv_get( tentries[ i ].icon_stipple, SERVER_IMAGE_PIXMAP );
    gcvals.fill_style = FillOpaqueStippled;
    
    height = ( int )xv_get( tentries[ i ].icon_stipple, XV_HEIGHT );
    width  = ( int )xv_get( tentries[ i ].icon_stipple, XV_WIDTH );
    if ( temp_image )
      xv_destroy_safe( temp_image );

    temp_image = 
      ( Server_image )xv_create( NULL, SERVER_IMAGE,
				SERVER_IMAGE_DEPTH,  ( int )xv_get( b->frame, WIN_DEPTH ),
				XV_WIDTH,  	    width,
				XV_HEIGHT, 	    height,
				NULL );
    pixmap = ( Pixmap )xv_get( temp_image, SERVER_IMAGE_PIXMAP );
    
    /*
     * Fill the masked part with the window color or
     * garbage will be displayed when using icon masks.
     */
    if ( b->color && tentries[ i ].icon_mask ) {
      gcvals.clip_mask = ( Pixmap )xv_get( tentries[i].icon_mask, SERVER_IMAGE_PIXMAP );
      XFillRectangle( dpy, pixmap, bg_gc, 0, 0, width, height );
    }
    else
      gcvals.clip_mask = None;
    
    valuemask = GCForeground | GCBackground | GCStipple | GCClipMask | GCFillStyle;
    XChangeGC( dpy, gc, valuemask, &gcvals );
    XFillRectangle( dpy, pixmap, gc, 0, 0, width, height );
    xv_set( temp_image, SERVER_IMAGE_PIXMAP, pixmap,  NULL );
    xv_set( p->icon_preview, PANEL_LABEL_IMAGE, temp_image, NULL ); 
    
    fg_pixmap = ( Pixmap )xv_get( fg_chip, SERVER_IMAGE_PIXMAP );
    bg_pixmap = ( Pixmap )xv_get( bg_chip, SERVER_IMAGE_PIXMAP );
    height = ( int )xv_get( fg_chip, XV_HEIGHT );
    width  = ( int )xv_get( fg_chip, XV_WIDTH );
    /*
     * Draw the foreground color onto the pixmap
     * of the color chip and set this as the new image.
     */
    gcvals.foreground = ds_x_index( fg_index );
    gcvals.fill_style = FillSolid;
    gcvals.clip_mask = None;
    valuemask = GCForeground | GCFillStyle | GCClipMask;
    
    XChangeGC( dpy, gc, valuemask, &gcvals );
    XFillRectangle( dpy, fg_pixmap, gc, 0, 0, width, height );
    xv_set( fg_chip, SERVER_IMAGE_PIXMAP, fg_pixmap, NULL );
    xv_set( p->fg_color_chip, PANEL_LABEL_IMAGE, fg_chip, NULL );
    /*
     * Draw the foreground color onto the pixmap
     * of the color chip and set this as the new image.
     * Use a bg color if specified else just use window frame color.
     */
    gcvals.foreground = ds_x_index( bg_index );
    
    XChangeGC( dpy, gc, valuemask, &gcvals );
    XFillRectangle( dpy, bg_pixmap, gc, 0, 0, width, height );
    
    xv_set( bg_chip, SERVER_IMAGE_PIXMAP, bg_pixmap, NULL );
    xv_set( p->bg_color_chip, PANEL_LABEL_IMAGE, bg_chip, NULL );
 
    p->icon_props_changed = TRUE;
}

/*
 * Create the cms for the binder.
 */
static void
b_init_colors( b )
    Binder *b;
{
    Props *p = ( Props * )b->properties;
    /*
     * Create these for the drag/drop, but deactivate them if monochrome.
     */
#ifdef OLD
    b->fg_color_chip = binder_fg_color_chip_create( ip, b->panel );
    b->bg_color_chip = binder_bg_color_chip_create( ip, b->panel );
#endif
 
    if ( b->color ) {
      b->cms = ds_cms_create( b->frame );
      if ( !b->cms ) {
	fputs( MGET("Error creating Colormap Segment in binder - starting in monochrome.\n") , stderr );
	b->color = FALSE;
        xv_set( p->fg_color_button, PANEL_INACTIVE, TRUE, NULL );
        xv_set( p->bg_color_button, PANEL_INACTIVE, TRUE, NULL );
        return;
      }
      xv_set( b->cms, CMS_NAME, "binder_cms" , NULL );
      (void) ds_set_colormap (b->frame, b->cms,
	(unsigned long) DS_NULL_CMS_INDEX, (unsigned long) DS_NULL_CMS_INDEX);
    }
    else {
      xv_set( p->fg_color_chip, XV_SHOW, FALSE, NULL );
      xv_set( p->bg_color_chip, XV_SHOW, FALSE, NULL );
    }

}

/*
 * Add an entry to the scrolling list with the correct colors.
 */
void
create_tns_entry( b, t, i )
    Binder     *b;
    Tns_entry  *t;
    int         i;
{ 
    unsigned long   valuemask;
    Pixmap          pixmap;
    int	            width, height;
    
    /*
     * Set up the GC values.
     */
    if ( b->color ) {
      gcvals.foreground = ds_x_index( t->fg_index );
      gcvals.background = ds_x_index( t->bg_index );
    } 
    else {     /* Monochrome */
      gcvals.foreground = xv_get( xv_get( b->frame, WIN_CMS ), CMS_FOREGROUND_PIXEL );
      gcvals.background = xv_get( xv_get( b->frame, WIN_CMS ), CMS_BACKGROUND_PIXEL );
    }
    
    gcvals.stipple = ( Pixmap )xv_get( t->icon_stipple, SERVER_IMAGE_PIXMAP ); 
    width  = ( int )xv_get( t->icon_stipple, XV_WIDTH );
    height = ( int )xv_get( t->icon_stipple, XV_HEIGHT );
    
    if ( width > MAX_ICON_HEIGHT || height > MAX_ICON_HEIGHT ) {
      fprintf( stderr, MGET( "Binder: Cannot create Type Entry: %s - Icon too Large.\n" ), t->icon_file );
      if ( t->icon_stipple )
	xv_destroy_safe( t->icon_stipple );
      if ( t->icon_mask )
        xv_destroy_safe( t->icon_mask );
      return;
    }
    /*
     * Create the actual server image.
     */
    t->icon_image = 
      ( Server_image )xv_create( NULL, SERVER_IMAGE,
				SERVER_IMAGE_DEPTH,  ( int )xv_get( b->frame, WIN_DEPTH ),
				XV_WIDTH,  	    width,
				XV_HEIGHT, 	    height,
				NULL );
    pixmap = ( Pixmap )xv_get( t->icon_image, SERVER_IMAGE_PIXMAP );
    
    /*
     * Set the clip mask, and fill the masked part with the window 
     * color or garbage will be displayed on the panel when using 
     * icon masks.
     */
    if ( b->color && t->icon_mask ) {
      gcvals.clip_mask = ( Pixmap )xv_get( t->icon_mask, SERVER_IMAGE_PIXMAP );
      XFillRectangle( dpy, pixmap, bg_gc, 0, 0, width, height );
    }
    else
      gcvals.clip_mask = None;
    
    gcvals.fill_style = FillOpaqueStippled;
    valuemask = GCForeground | GCBackground | GCStipple | GCClipMask | GCFillStyle;
    XChangeGC( dpy, gc, valuemask, &gcvals );
    XFillRectangle( dpy, pixmap, gc, 0, 0, width, height );
    
    list_add_entry( b->tns_list, t->type_name, t->icon_image, NULL, i, TRUE );
    if (t->icon_mask != NULL) {
      width = xv_get (t->icon_mask, XV_WIDTH);
      height = xv_get (t->icon_mask, XV_HEIGHT);
  
      if (width <= LIST_ENTRY_HEIGHT && height <= LIST_ENTRY_HEIGHT)
        xv_set (b->tns_list, PANEL_LIST_MASK_GLYPH, i, t->icon_mask, NULL);
    }

}


static int
sort_tns_entries( t1, t2 )
    const void *t1, *t2;
{
    Tns_entry *tmp1, *tmp2;

    tmp1 = (Tns_entry *)t1;
    tmp2 = (Tns_entry *)t2;

    /*
     * Entries sorted by type name and then data base name.
     */
    if ( strcmp( tmp1->type_name, tmp2->type_name ) == 0 )
      return( strcmp( tmp1->db_name, tmp2->db_name ) );
    else
      return( strcmp( tmp1->type_name, tmp2->type_name ) );

}

static void
delete_tns_list( b )
    Binder *b;
{
    int  i, nrows;

    /*
     * Delete current tns list
     */
    list_flush( b->tns_list );
    
    /*
     * Free up the server images.
     */
    nrows = ( int )xv_get( b->tns_list, PANEL_LIST_NROWS );
    for ( i = 0; i < nrows; i++ ) {
      xv_destroy_safe( tentries[i].icon_stipple );
      if ( tentries[i].icon_mask )
	xv_destroy_safe( tentries[i].icon_mask );
      xv_destroy_safe( tentries[i].icon_image );
    }

}

/*
 * Create the Type Namespace scrolling list.
 */
static void
create_tns_list( b )
    Binder *b;
{
    int            i;
    unsigned long  valuemask;

    /*  
     * Create a GC for background colors if color monitor.
     */
    if ( ( b->color )  && ( !bg_gc ) ) {
      valuemask = GCForeground;
      gcvals.foreground = ds_x_index( WIN_INDEX );
      bg_gc = XCreateGC( dpy, drawable, valuemask, &gcvals );
    }
    /*
     * Create a GC for drawing icons.
     */
    if ( !gc ) {
      valuemask = GCForeground | GCFillStyle;
      gcvals.foreground = xv_get( xv_get( b->frame, WIN_CMS ), CMS_FOREGROUND_PIXEL );
      gcvals.fill_style = FillOpaqueStippled;
      gc = XCreateGC( dpy, drawable, valuemask, &gcvals );
    }
    
    /*
     * Sort the list and create the entries on the scroll list.
     */
    qsort( ( char * )tentries, ntns, sizeof( Tns_entry ), sort_tns_entries );
    
    for ( i = 0; i < ntns; i++ )
      create_tns_entry( b, &tentries[i], i );
    
}

/*
 * Determine which item off scroll list is selected, if any.
 */
int
current_selection( list )
     Panel_item  list;
{
  register int i, nrows = xv_get( list, PANEL_LIST_NROWS );

  for ( i = 0; i < nrows; i++ )
    if ( list_item_selected( list, i ) )
      return( i );

  return( EMPTY );
}


int
data_type( type )
    char *type;
{

    if ( type == NULL )
      return( 3 );
    else if ( strcmp ( type, "byte" ) == 0 )
      return( 0 );
    else if ( strcmp( type, "short" ) == 0 )
      return( 1 );
    else if ( strcmp( type, "long" ) == 0 ) 
      return( 2 );
    else if ( strcmp( type, "string" ) == 0 ) 
      return( 3 );
    else
      return( 3 );
}

char *
string_data_type( value )
    int value;
{
    char *type;

    switch ( value ) {
    case BYTE_TAG:
      type = "byte";
      break;
    case SHORT_TAG:
      type = "short";
      break;
    case LONG_TAG:
      type = "long";
      break;
    case STRING_TAG:
      type = "string";
      break;
    default:
      type = "string";
      break;
    }

    return ( type );    

}

static void
undisplay_tns_data( b )
    Binder  *b;
{
    Props  *p = ( Props * )b->properties;

    if ( xv_get( p->frame, XV_SHOW ) ) {
      switch ( p->current_view ) {
      case ICON_PROPS:
        xv_set( p->icon_item, PANEL_VALUE, "", NULL );
        xv_set( p->image_file_item, PANEL_VALUE, "", NULL );
        xv_set( p->mask_file_item, PANEL_VALUE, "", NULL );
        xv_set( p->fg_color_item, PANEL_VALUE, "", NULL );
        xv_set( p->bg_color_item, PANEL_VALUE, "", NULL );
        xv_set( p->open_item, PANEL_VALUE, "", NULL );
        xv_set( p->print_item, PANEL_VALUE, "", NULL );
        if ( !b->color ) {
          xv_set( p->fg_color_button, PANEL_INACTIVE, TRUE, NULL );
          xv_set( p->bg_color_button, PANEL_INACTIVE, TRUE, NULL );
        }
        break;
      case BINDING_PROPS:
        list_flush( p->fns_list );
        undisplay_fns_data( b );
        xv_set( p->new_button, PANEL_INACTIVE, TRUE, NULL );
        xv_set( p->pattern_item, PANEL_INACTIVE, TRUE, NULL );
        break;
      case PROGRAM_PROPS:
        break;
      default:
        break;
      }
    }

}

/*ARGSUSED*/
static Notify_value
cancel_handler( client, sig )
    Notify_client client;
    int           sig;
{
    Binder *b = ( Binder * )xv_get( client, XV_KEY_DATA, INSTANCE );

    xv_set( b->tns_list, PANEL_LIST_SELECT, curr_row, FALSE, NULL );
    xv_set( b->tns_list, PANEL_LIST_SELECT, prev_row, TRUE, NULL );
    return( NOTIFY_DONE );

}

/*
 * Resize  
 */
static Notify_value
frame_resize( frame, event, arg, type )
    Frame              frame;
    Event             *event;
    Notify_arg         arg;
    Notify_event_type  type;
{
    Binder      *b = ( Binder * )xv_get( frame, XV_KEY_DATA, INSTANCE );
    int          button_row = 0;
    int          row_height, nrows;
    int          max_y, max_list_y;
    int          fwidth, fheight;
    static int   old_fwidth = 0, old_fheight = 0;

    /*
     * Check the dimensions to see if it really changed size.
     * Return if no size change.
     */    
    fwidth = xv_get( frame, XV_WIDTH );
    fheight = xv_get( frame, XV_HEIGHT );
    if ( ( fwidth == old_fwidth ) && ( fheight == old_fheight ) )
      return( notify_next_event_func( frame, ( Notify_event )event, arg, type ) );

    old_fwidth = fwidth;  
    old_fheight = fheight;  

    /*
     * Set the new panel sizes.
     */
    xv_set( b->panel, XV_WIDTH, xv_get( frame, XV_WIDTH ),
                      XV_HEIGHT, xv_get( frame, XV_HEIGHT),
	              NULL );
    /*
     * 5 for offset, 20 for scrollbar.
     */
    xv_set( b->tns_list, PANEL_LIST_WIDTH, xv_get( b->panel, XV_WIDTH ) - 5 - 20, NULL );

    row_height = xv_get( b->panel, WIN_ROW_HEIGHT ) + xv_get( b->panel, PANEL_ITEM_Y_GAP );

    max_y = xv_get( b->panel, XV_HEIGHT ) - xv_get( b->panel, PANEL_ITEM_Y_GAP ) -
            xv_get( b->new_button, XV_HEIGHT );
    button_row = max_y / row_height;

    if ( button_row != 0 )
		xv_set(b->new_button, XV_Y, button_row * row_height, NULL);

    max_y = xv_get( b->new_button, XV_Y ) - xv_get( b->panel, PANEL_ITEM_Y_GAP );
    max_list_y = max_y - xv_get( b->tns_list, XV_Y );

    nrows = max_list_y / xv_get( b->tns_list, PANEL_LIST_ROW_HEIGHT );

    xv_set( b->tns_list, PANEL_LIST_DISPLAY_ROWS, nrows - 1, NULL );

    ds_center_items( b->panel, button_row, b->new_button, 
		b->dup_button,
		b->delete_button, NULL );

    return( notify_next_event_func( frame, ( Notify_event )event, arg, type ) );
}

/*ARGSUSED*/
static Notify_value
frame_interpose_proc( frame, event, arg, type )
    Frame              frame;
    Event             *event;
    Notify_arg         arg;
    Notify_event_type  type;
{
    Notify_value  value;
    Binder       *b = ( Binder * )xv_get( frame, XV_KEY_DATA, INSTANCE );

    switch( event_action( event ) ) {
      case WIN_RESIZE:
        value = frame_resize( frame, event, arg, type );
        break;
      case ACTION_PROPS:
        show_icon_props( ( Menu )xv_get( b->props_button, PANEL_ITEM_MENU ), NULL );
        value = notify_next_event_func( frame, ( Notify_event )event, arg, type );
        break;
      case ACTION_OPEN:
        b_open_frame( b );
        value = notify_next_event_func( frame, ( Notify_event )event, arg, type );
        break;
      case ACTION_CLOSE:
        b_close_frame( b );
        value = notify_next_event_func( frame, ( Notify_event )event, arg, type );
        break;
      default:
        value = notify_next_event_func( frame, ( Notify_event )event, arg, type );
        break;
    }
    return ( value );
}

/*ARGSUSED*/
static Notify_value
panel_interpose_proc( panel, event, arg, type )
    Panel              panel;
    Event             *event;
    Notify_arg         arg;
    Notify_event_type  type;
{
    Notify_value  value;
    Binder       *b = ( Binder * )xv_get( panel, XV_KEY_DATA, INSTANCE );
    extern void   undo_button();

    switch( event_action( event ) ) {
      case ACTION_PROPS:
        if ( event_is_down( event ) )
          show_icon_props( ( Menu )xv_get( b->props_button, PANEL_ITEM_MENU ), NULL );
        break;
      case ACTION_UNDO:
	if (event_is_down (event) &&
	    xv_get (b->undo_button, PANEL_INACTIVE) == FALSE)
	  undo_button (b->undo_button, NULL);
	break;
      default:
        break;
    }

    return( notify_next_event_func( panel, ( Notify_event )event, arg, type ) );

}

static void
save_reset_entry( t, reset )
    Tns_entry *t, *reset;
{
/*
 * Free old reset values.
 */
    if (reset->type_name != NULL)
       free(reset->type_name);
    if (reset->icon_file != NULL)
       free(reset->icon_file);
    if (reset->icon_mask_file != NULL)
       free(reset->icon_mask_file);
    if (reset->fg_color != NULL)
       free(reset->fg_color);
    if (reset->bg_color != NULL)
       free(reset->bg_color);
    if (reset->open_method != NULL)
       free(reset->open_method);
    if (reset->print_method != NULL)
       free(reset->print_method);
/*
 * Store old values.
 */
    if (t->type_name != NULL)
       reset->type_name = strdup( t->type_name);
    else
       reset->type_name = NULL;
    if (t->icon_file != NULL)
       reset->icon_file = strdup( t->icon_file );
    else
       reset->icon_file = NULL;
    if (t->icon_mask_file != NULL)
       reset->icon_mask_file = strdup( t->icon_mask_file );
    else
       reset->icon_mask_file = NULL;
    if (t->fg_color != NULL)
       reset->fg_color = strdup( t->fg_color );
    else
       reset->fg_color = NULL;
    if (t->bg_color != NULL)
       reset->bg_color = strdup( t->bg_color );
    else
      reset->bg_color = NULL;
    if (t->open_method != NULL)
       reset->open_method = strdup( t->open_method );
    else
       reset->open_method = NULL;
    if (t->print_method != NULL)
       reset->print_method = strdup( t->print_method );
    else
       reset->print_method = NULL;
    if (t->icon_image != NULL)
       reset->icon_image = t->icon_image;
   
}

/*
 * Notify callback function for 'tns_list'.
 */
/*ARGSUSED*/
void
tns_list_notify( item, string, client_data, op, event, row )
    Panel_item       item;
    char            *string;
    caddr_t          client_data;
    Panel_list_op    op;
    Event           *event;
    int              row;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     notice_value;
    static int cancel_next_select = FALSE;

    printf("");
    p = ( Props * )b->properties;
      
    switch ( op ) {
    case PANEL_LIST_OP_SELECT:

      if ( row < 0 && row >= ntns ) { 
	xv_set( b->tns_list, FRAME_RIGHT_FOOTER, "", NULL );
	return;
      }
      xv_set( b->frame, FRAME_LEFT_FOOTER, "", NULL );
      xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );

      /*
       * When cancelling next select, deselect current row,
       * and select prev row.
       */
      if ( cancel_next_select ) {
        curr_row = row;
        kill( getpid(), SIGUSR1 );
        cancel_next_select = FALSE;
        return;
      }

      /*
       * Tns entry selected, so activate items on props menu.
       */
      xv_set( b->props_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( b->dup_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, FALSE, NULL );

      if ( read_only_entry( tentries[row].db_name ) ) {
	xv_set( b->frame, FRAME_RIGHT_FOOTER, R_ONLY_ENTRY, NULL );
	xv_set( b->delete_button, PANEL_INACTIVE, TRUE, NULL );
      }
      else {
	xv_set( b->frame, FRAME_RIGHT_FOOTER, RW_ENTRY, NULL );
	xv_set( b->delete_button, PANEL_INACTIVE, FALSE, NULL );
      }
      
      /*
       * If the props frame is up, then display new data.
       */
      if ( xv_get( p->frame, XV_SHOW ) ) {
	switch( p->current_view ) { 
	case ICON_PROPS:
	  show_binding_items( b, FALSE );
	  show_program_items( b, FALSE );
	  show_icon_items( b, TRUE );
	  break;
	case BINDING_PROPS:
	  show_program_items( b, FALSE );
	  show_icon_items( b, FALSE );
	  show_binding_items( b, TRUE );
          xv_set( p->new_button, PANEL_INACTIVE, FALSE, NULL );
	  break;
	case PROGRAM_PROPS:
	  show_icon_items( b, FALSE );
	  show_binding_items( b, FALSE );
	  show_program_items( b, TRUE );
	  break;
	default:
	  break;
	}
      }
     /*
      * Save the icon items in case of a reset.
      */
      save_reset_entry( &tentries[row], &reset_entry );
      break;

    case PANEL_LIST_OP_DESELECT:
      /*
       * Put notice up if props change for this icon.
       */      
      cancel_next_select = FALSE;
      if ( p->icon_props_changed || p->binding_props_changed ) {

        notice_value = notice_prompt( p->frame, NULL,
				     NOTICE_MESSAGE_STRINGS,
				     MGET("You have made changes that\nhave not been applied.\nYou can:"),
				     NULL,
				     NOTICE_BUTTON_YES, MGET( "Reset and Continue" ),
				     NOTICE_BUTTON_NO,  MGET( "Cancel" ),
				     NULL );
	switch( notice_value ) {
#ifdef NEVER
        case NOTICE_YES:    /* Apply and continue */
	  xv_set( b->tns_list, PANEL_LIST_SELECT, TRUE, row, NULL );
          apply_button( p->icon_apply_button, NULL);
	  xv_set( b->tns_list, PANEL_LIST_SELECT, FALSE, row, NULL);
          break;
#endif
        case NOTICE_YES:     /* Reset and Continue really means Continue */
#if 0
	  reset_image = FALSE;
          reset_button( p->icon_apply_button, NULL );
	  reset_image = TRUE;
#endif
    	  p->icon_props_changed = p->binding_props_changed = FALSE;
  	  break;
        case NOTICE_NO:  /* Cancel and go back */
          prev_row = row;
	  cancel_next_select = TRUE;
	  return;
	default:
	  return;
	}
      }

      xv_set( b->props_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->dup_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->delete_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->frame, FRAME_RIGHT_FOOTER, "", NULL );
      if ( xv_get( p->frame, XV_SHOW ) )
	undisplay_tns_data( b );
      break;

    default:
      break;

    }

}


/*
 *
 */
static char *
unique_tns_name( b )
    Binder *b;
{
    int   i;
    int   count = 1;
    int   found = FALSE;
    
    while ( !found ) {
      switch ( b->scope ) {
      case USER:
	sprintf( unnamed, "%s_%d", UNNAMED, count);
        break;
      case SYS:
        sprintf( unnamed, "%s_%s_%d", UNNAMED, SYSTEM, count );
        break;
      case NET:
        sprintf( unnamed, "%s_%s_%d", UNNAMED, NETWORK, count );
        break;
      default:
        break;
      }
      for ( i = 0; i < ntns; i++ ) {
        if ( strcmp( unnamed, tentries[i].type_name ) == 0 ) 
          break;
      }
      if ( i == ntns ) 
        found = TRUE;
      count++;
    }
    return( unnamed );

}

void
setbusy ()
{
    Frame	 frame;
    int		 n = 1;

    xv_set (binder->frame, FRAME_BUSY, TRUE, NULL);
    while (frame = xv_get (binder->frame, FRAME_NTH_SUBFRAME, n++))
	  xv_set (frame, FRAME_BUSY, TRUE, NULL);
}

void
setactive ()
{
    Frame	 frame;
    int		 n = 1;

    while (frame = xv_get (binder->frame, FRAME_NTH_SUBFRAME, n++))
	  xv_set (frame, FRAME_BUSY, FALSE, NULL);
    xv_set (binder->frame, FRAME_BUSY, FALSE, NULL);

}

/*
 * Notify callback function for `save_button'.
 */
/*ARGSUSED*/
void     
save_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     tns_row;
    int     error;
    char   *type_name;
    Scrollbar  sb;
    int        nrows, drows, greatest, view_start;

    p = ( Props * )b->properties;

    /*
     * Remove last entry deleted.
     */
    if ( undo_entry.entry != NULL && undo_op == DEL ) {
      error = ce_remove_entry( b->tns, undo_entry.entry );
      if ( error ) 
	display_ce_error( b->frame, error );
        /* Fix later, free up icon images */
    }
    undo_entry.entry = NULL;

    xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Saving Changes" ), NULL );

    /*
     * Force unapplied changes.
     */
    if ( p->icon_props_changed || p->binding_props_changed ) {
      apply_button( p->icon_apply_button, NULL );
      b->binder_modified = TRUE;
    }
    if (!b->binder_modified)
	return;

    setbusy();

    /*
     * Flush to disk.
     */
    error = ce_commit_write( NULL );  
    if ( error ) {
      display_ce_error( b->frame, error );
      setactive();
      return;
    }

    /*
     * Save the type name of the currently selected entry
     * so that we can set this same entry to be selected after saves.
     */
    tns_row = current_selection( b->tns_list );
    if ( tns_row != -1 )
      type_name = strdup( tentries[tns_row].type_name );

    /*
     * Reinitialize CE Pointers.
     */
    ce_end();
    b_init_ce( b );
    b_init_namespaces( b );

    xv_set( b->tns_list, XV_SHOW, FALSE, NULL );
    delete_tns_list( b ); 
    create_tns_list( b );

    /*
     * Find the type name in the list and selected this entry.
     */
    if ( tns_row != -1 ) {
      for ( tns_row = 0; tns_row < ntns; tns_row++ )
        if ( strcmp( type_name, tentries[tns_row].type_name ) == 0 )
          break;
      
      xv_set( b->tns_list, PANEL_LIST_SELECT, tns_row, TRUE, NULL );
      free( type_name );
    }
    xv_set( b->tns_list, XV_SHOW, TRUE, NULL );
    
    /*
     * Fill in data on the props if showing.
     */
    if ( xv_get( p->frame, XV_SHOW ) && ( tns_row != EMPTY ) ) {
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		       ( Event * )NULL, tns_row );
      switch ( p->current_view ) {
      case ICON_PROPS:
	show_icon_items( b, TRUE );
	break;
      case BINDING_PROPS:
	show_binding_items( b, TRUE );
	break;
      case PROGRAM_PROPS:
	show_program_items( b, TRUE );
	break;
	default:
	break;
      }
    }
    /*
     * Changed title bar back to NOT editted.
     */
    if ( b->binder_modified ) {
      b->binder_modified = FALSE;
      xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
    }
    
    /*
     * Nothing to undo.
     */
    xv_set( b->undo_button, PANEL_INACTIVE, TRUE, NULL );

    xv_set( b->frame, FRAME_LEFT_FOOTER, "", NULL);
    setactive();

    view_start = current_selection(b->tns_list);
    if ( view_start != EMPTY ) {
      sb = (Scrollbar)xv_get(b->tns_list, PANEL_LIST_SCROLLBAR);
      drows = xv_get(b->tns_list, PANEL_LIST_DISPLAY_ROWS);
      nrows = xv_get(b->tns_list, PANEL_LIST_NROWS);
      greatest = nrows - drows;
      if (view_start > greatest)
        view_start = greatest;
      xv_set( sb, SCROLLBAR_VIEW_START, view_start, NULL );
    }
}

/*
 * Notify callback function for `View - All'.
 */
/*ARGSUSED*/
void     
view_all( menu, item )
    Menu        menu;
    Menu_item   item;
{ 
    Binder *b = ( Binder * )xv_get( menu, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     value;
    int     error; 

    p = ( Props * )b->properties;
    /*
     * Return if already in this view.
     */
    if ( b->view == ALL )
      return;
    
    xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Changing Views" ), NULL);
    setbusy();

    if ( b->binder_modified || p->icon_props_changed || p->binding_props_changed ) {
      value = notice_prompt( b->frame, 0,
			 ALERT_MESSAGE_STRINGS,
  			   MGET( "You must save your changes\nbefore changing views.\nYou can:"),
			   NULL,
			 ALERT_BUTTON_YES,  MGET( "Save" ),
			 ALERT_BUTTON_NO,   MGET( "Continue" ),
			 ALERT_BUTTON,      MGET( "Cancel" ), 2,
			 NULL );
      switch ( value ) {
      case NOTICE_YES:
        /*
         * Remove last entry deleted.
         */
        if ( undo_entry.entry != NULL && undo_op == DEL ) {
          error = ce_remove_entry( b->tns, undo_entry.entry );
          if ( error ) 
	    display_ce_error( b->frame, error );
        }
        undo_entry.entry = NULL;
        /* 
	 * Apply props if changed, save CE to disk, then quit 
 	 */
        if ( p->icon_props_changed || p->binding_props_changed )
          apply_button( p->icon_apply_button, NULL );
	error = ce_commit_write( NULL );  
	if ( error ) 
	  display_ce_error( b->frame, error );
	ce_end();  
        b_init_ce( b );
	b->binder_modified = FALSE;
        xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
        break;
      case NOTICE_NO:
        /* Undo all edits, and go to next view */
	ce_abort_write( NULL );
        ce_end();
        b_init_ce( b );
        b->binder_modified = FALSE;
        p->icon_props_changed = FALSE;
        p->binding_props_changed = FALSE;
        break;
      default:
        setactive();
        return;
      }
    }
 
    b->view = ALL;
    xv_set( b->new_button, PANEL_INACTIVE, FALSE, NULL );

    /*
     * Reinitialize CE Pointers.
     */
    b_init_namespaces( b );

    xv_set( b->tns_list, XV_SHOW, FALSE, NULL );
    delete_tns_list( b ); 
    create_tns_list( b );
    xv_set( b->tns_list, PANEL_LIST_TITLE, MGET( "Binder Entries" ),
	                 XV_SHOW, TRUE, NULL );
    if ( ntns > 0 ) {
      xv_set( b->tns_list, PANEL_LIST_SELECT, 0, TRUE, NULL );
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		       ( Event * )NULL, 0 );
    }
    else {
      xv_set( b->props_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, TRUE, NULL );
    }
     
    xv_set( b->frame, FRAME_LEFT_FOOTER, "", NULL);
    setactive();

}
     
/*
 * Notify callback function for `View - Shared'.
 */
/*ARGSUSED*/
void     
view_shared( menu, item )
    Menu        menu;
    Menu_item   item;
{
    Binder *b = ( Binder * )xv_get( menu, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     value;
    int     error; 

    p = ( Props * )b->properties;
    /*
     * Return if already in this view.
     */
    if ( b->view == SHARED )
      return;
    
    xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Changing Views" ), NULL);
    setbusy(); 

    if ( b->binder_modified || p->icon_props_changed || p->binding_props_changed ) {
      value = notice_prompt( b->frame, 0,
			 ALERT_MESSAGE_STRINGS,
  			   MGET( "You must save your changes\nbefore changing views.\nYou can:"),
			   NULL,
			 ALERT_BUTTON_YES,  MGET( "Save" ),
			 ALERT_BUTTON_NO,   MGET( "Continue" ),
			 ALERT_BUTTON,      MGET( "Cancel" ), 2,
			 NULL );
      switch ( value ) {
      case NOTICE_YES:
        /*
         * Remove last entry deleted.
         */
        if ( undo_entry.entry != NULL && undo_op == DEL ) {
          error = ce_remove_entry( b->tns, undo_entry.entry );
          if ( error ) 
	    display_ce_error( b->frame, error );
        }
        undo_entry.entry = NULL;
        /* 
 	 * Apply props if changed, save CE to disk, then quit 
	 */
        if ( p->icon_props_changed || p->binding_props_changed )
          apply_button( p->icon_apply_button, NULL );
	error = ce_commit_write( NULL );  
	if ( error ) 
	  display_ce_error( b->frame, error );
	ce_end();  
        b_init_ce( b );
	b->binder_modified = FALSE;
        xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
        break;
      case NOTICE_NO:
        /* Undo all edits, and go to next view */
	ce_abort_write( NULL );
        ce_end();
        b_init_ce( b );
        b->binder_modified = FALSE;
        p->icon_props_changed = FALSE;
        p->binding_props_changed = FALSE;
        xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
        break;
      default:
        setactive();
        return;
      }
    }
 
    b->view = SHARED;
    xv_set( b->new_button, PANEL_INACTIVE, TRUE, NULL );

    /*
     * Reinitialize CE Pointers.
     */
    b_init_namespaces( b );

    xv_set( b->tns_list, XV_SHOW, FALSE, NULL );
    delete_tns_list( b ); 
    create_tns_list( b );
    xv_set( b->tns_list, PANEL_LIST_TITLE, MGET( "Binder Entries - Shared" ),
	                 XV_SHOW, TRUE, NULL );
    if ( ntns > 0 ) {
      xv_set( b->tns_list, PANEL_LIST_SELECT, 0, TRUE, NULL );
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		       ( Event * )NULL, 0 );
    }
    else {
      xv_set( b->props_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, TRUE, NULL );
    }
     
    xv_set( b->frame, FRAME_LEFT_FOOTER, "", NULL);
    setactive();

}
     
/*
 * Notify callback function for `View - Personal'.
 */
/*ARGSUSED*/
void     
view_personal( menu, item )
    Menu        menu;
    Menu_item   item;
{
    Binder *b = ( Binder * )xv_get( menu, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     value;
    int     error; 

    p = ( Props * )b->properties;
    /*
     * Return if already in this view.
     */
    if ( b->view == PERSONAL )
      return;
    
    xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Changing Views" ), NULL);
    setbusy();

    if ( b->binder_modified || p->icon_props_changed || p->binding_props_changed ) {
      value = notice_prompt( b->frame, 0,
			 ALERT_MESSAGE_STRINGS,
  			   MGET( "You must save your changes\nbefore changing views.\nYou can:"),
			   NULL,
			 ALERT_BUTTON_YES,  MGET( "Save" ),
			 ALERT_BUTTON_NO,   MGET( "Continue" ),
			 ALERT_BUTTON,      MGET( "Cancel" ), 2,
			 NULL );
      switch ( value ) {
      case NOTICE_YES:
        /*
         * Remove last entry deleted.
         */
        if ( undo_entry.entry != NULL && undo_op == DEL ) {
          error = ce_remove_entry( b->tns, undo_entry.entry );
          if ( error ) 
	    display_ce_error( b->frame, error );
        }
        undo_entry.entry = NULL;
        /* 
	 * Apply props if changed, save CE to disk, then quit 
	 */
        if ( p->icon_props_changed || p->binding_props_changed )
          apply_button( p->icon_apply_button, NULL );
	error = ce_commit_write( NULL );  
	if ( error ) 
	  display_ce_error( b->frame, error );
	ce_end();  
        b_init_ce( b );
	b->binder_modified = FALSE;
        xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
        break;
      case NOTICE_NO:
        /* Undo all edits, and go to next view */
	ce_abort_write( NULL );
        ce_end();
        b_init_ce( b );
        b->binder_modified = FALSE;
        p->icon_props_changed = FALSE;
        p->binding_props_changed = FALSE;
        break;
      default:
        setactive();
        return;
      }
    }
 
    b->view = PERSONAL;
    xv_set( b->new_button, PANEL_INACTIVE, FALSE, NULL );

    /*
     * Reinitialize CE Pointers.
     */
    b_init_namespaces( b );

    xv_set( b->tns_list, XV_SHOW, FALSE, NULL );
    delete_tns_list( b ); 
    create_tns_list( b );
    xv_set( b->tns_list, PANEL_LIST_TITLE, MGET( "Binder Entries - Personal" ),
	                 XV_SHOW, TRUE, NULL );
    if ( ntns > 0 ) {
      xv_set( b->tns_list, PANEL_LIST_SELECT, 0, TRUE, NULL );
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		       ( Event * )NULL, 0 );
    }
    else {
      xv_set( b->props_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, TRUE, NULL );
    }
     
    xv_set( b->frame, FRAME_LEFT_FOOTER, "", NULL);
    setactive();

}

/*
 * Notify callback function for `Undo Edit'.
 */
/*ARGSUSED*/
void     
undo_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder    *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props     *p;
    int        error;
    int        i, tns_row;

    p = ( Props * )b->properties;
    /*
     * Check is there if something to undo.
     */
    if ( undo_entry.entry == NULL ) {
      window_bell( b->frame );
      xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Nothing to Undo" ), NULL );
      return;
    }

    switch ( undo_op ) {
    case NEW:
    case DUP:
      /*
       * Delete the last entry just added for an undo.
       * Nothing is selected now, so inactivate buttons/menus.
       */
      for ( tns_row = 0; tns_row < ntns; tns_row++ ) {
        if ( tentries[tns_row].entry == undo_entry.entry )
          break;
      }
      xv_set( b->tns_list, PANEL_LIST_DELETE, tns_row, NULL );

      error = ce_remove_entry( b->tns, undo_entry.entry );
      if ( error ) 
	display_ce_error( b->frame, error );
      /* Fix later, free up icon images */
      undo_entry.entry = NULL;

      p->icon_props_changed = FALSE;
      p->binding_props_changed = FALSE;

      xv_set( b->props_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->dup_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->delete_button, PANEL_INACTIVE, TRUE, NULL );
      if ( xv_get( p->frame, XV_SHOW ) )
	undisplay_tns_data( b ); 

      ntns--;
      for ( i = tns_row; i < ntns; i++ ) 
	tentries[i] = tentries[i+1];
      break;

    case DEL:
      /*
       * Restore last_entry deleted and
       * display icon properties.
       */
      memcpy( ( char * )&tentries[ntns], ( char * )&undo_entry, sizeof( Tns_entry ) );
      list_add_entry( b->tns_list, undo_entry.type_name, undo_entry.icon_image, 
		     NULL, ntns, TRUE ); 
      xv_set (b->tns_list, PANEL_LIST_MASK_GLYPH, ntns, undo_entry.icon_mask, NULL);
      xv_set( b->tns_list, PANEL_LIST_SELECT, ntns, TRUE, NULL ); 
      xv_set( b->props_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set (b->dup_button, PANEL_INACTIVE, FALSE, NULL);
      xv_set (p->category_item, PANEL_INACTIVE, FALSE, NULL);
      if ( read_only_entry( tentries[ntns].db_name ) ) {
	xv_set( b->frame, FRAME_RIGHT_FOOTER, R_ONLY_ENTRY, NULL );
	xv_set( b->delete_button, PANEL_INACTIVE, TRUE, NULL );
      }
      else {
	xv_set( b->frame, FRAME_RIGHT_FOOTER, RW_ENTRY, NULL );
	xv_set( b->delete_button, PANEL_INACTIVE, FALSE, NULL );
      }
      
     /*
      *  Comment out for #1065552 Fix.
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		      ( Event * )NULL, ntns ); 
      */
      ntns++;
      /* fix later - will put up notice for unsaved applies. */
      if ( xv_get( p->frame, XV_SHOW ) )
        show_icon_props( ( Menu )xv_get( b->props_button, PANEL_ITEM_MENU ), NULL );
      undo_entry.entry = NULL;
      break;

    default:
      break;
    }
    /*
     * Disable the undo item.
     */
    xv_set( b->undo_button, PANEL_INACTIVE, TRUE, NULL );
    undo_entry.entry = NULL;

}



/*
 * Notify callback function for `new_button'.
 */
/*ARGSUSED*/
void
new_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder    *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props     *p;
    int        success;
    Tns_entry  t;
    int        status;

    p = ( Props * )b->properties;

    /*
     * Default new entry to the default Type entry.
     */
    t = def_entry;
    t.entry = NULL;
    t.db_name = strdup( b->current_ce_db );
    t.type_name = strdup( unique_tns_name( b ) );

    success = add_tns_entry( b, &t );
    if ( !success ) {
      if ( t.type_name ) free( t.type_name );
      if ( t.db_name )   free( t.db_name );
      return;
    }

    /*
     * Read the entry just added back from the CE into memoryy.
     * Create the new entry into the Type namespace list.
     */ 
    status = read_tns_entry( b->tns, t.entry, b );
    if ( status != 0 )
      return;
    create_tns_entry( b, &tentries[ntns-1], ntns-1 );
    xv_set( b->tns_list, PANEL_LIST_SELECT, ntns-1, TRUE, NULL );
    tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
        	     ( Event * )NULL, ntns-1 );
    /*
     * Save the undo entry just added, so we can delete it again.
     * for the UNDO operation.
     */
    undo_entry = tentries[ntns-1];
    undo_op = NEW;

    /*
     * Show the new icon properties.
     */
    entry_created = TRUE;
    show_icon_props( ( Menu )xv_get( b->props_button, PANEL_ITEM_MENU ), NULL );
    entry_created = FALSE;
    xv_set( p->icon_item, PANEL_TEXT_SELECT_LINE, NULL );

    /*
     * Enable the undo item.
     */
    xv_set( b->undo_button, PANEL_INACTIVE, FALSE, NULL );

    if ( !b->binder_modified ) {
      b->binder_modified = TRUE;
      xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
    }

}

/*
 * Notify callback function for `dup_button'.
 */
/*ARGSUSED*/
void
dup_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder    *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props     *p;
    int        tns_row;
    int        success;
    int        status;
    Tns_entry  t;

    p = ( Props * )b->properties;

    tns_row = current_selection( b->tns_list );

    if ( tns_row == -1 ) {
      window_bell( b->frame );
      xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
    }
    else {
      t = tentries[tns_row];
      t.entry = NULL;
      t.db_name = strdup( b->current_ce_db );
      t.type_name = strdup( unique_tns_name( b ) );      

      success = add_tns_entry( b, &t );
      if ( !success ) {
        if ( t.type_name ) free( t.type_name );
        if ( t.db_name )   free( t.db_name );
        return;
      }

      /*
       * Read the entry just added back from the CE into memoryy.
       * Create the new entry into the Type namespace list.
       */ 
      status = read_tns_entry( b->tns, t.entry, b );
      if ( status != 0 )
        return;
      if ( !display_entry( b, &t ) ) {
        window_bell( b->frame );
        switch ( b->scope ) {
        case USER:
          xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Duplicated to Personal" ), NULL );
          break;
        case SYS:
          xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Duplicated to System" ), NULL );
          break;
        default:
          break;
        }
        return;
      }
    
      create_tns_entry( b, &tentries[ntns-1], ntns-1 );
      xv_set( b->tns_list, PANEL_LIST_SELECT, ntns-1, TRUE, NULL );
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		       ( Event * )NULL, ntns-1 );
      /*
       * Show the new icon properties.
       */
      entry_created = TRUE;
      show_icon_props( ( Menu )xv_get( b->props_button, PANEL_ITEM_MENU ), NULL );
      entry_created = FALSE;
      xv_set( p->icon_item, PANEL_TEXT_SELECT_LINE, NULL );
    }
      
    /*
     * Save the undo entry just added, so we can delete it again.
     * for the UNDO operation.
     */
    undo_entry = tentries[ntns-1];
    undo_op = DUP;

    /*
     * Enable the undo item.
     */
    xv_set( b->undo_button, PANEL_INACTIVE, FALSE, NULL );
    undo_op = DUP;

    if ( !b->binder_modified ) {
      b->binder_modified = TRUE;
      xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
    }

}

/*
 * Notify callback function for `delete_button'.
 */
/*ARGSUSED*/
void
delete_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      i;
    int      error;
    int      tns_row;

    p = ( Props * )b->properties;

   /*
    * Delete from CE database.
    * Delete the fns entry from the panel list.
    * Delete from filelist.
    */
    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      window_bell( b->frame );
      xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
    }
    else {
      /*
       * Remove the previously deleted entry, if there is one.
       */
      if ( undo_entry.entry != NULL && undo_op == DEL ) { 
        error = ce_remove_entry( b->tns, undo_entry.entry );
	if ( error ) {
	  display_ce_error( b->frame, error );
          /* Fix later - free up images */
	  return;
	}
      } 
      /*
       * Set undo_entry to next one to be deleted.
       */
      undo_entry = tentries[tns_row];
      p->icon_props_changed = FALSE;
      p->binding_props_changed = FALSE;
      /*
       * Nothing is selected now, so inactivate buttons/menus.
       */
      xv_set( b->tns_list, PANEL_LIST_DELETE, tns_row, NULL );
      xv_set( b->props_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->category_item, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->dup_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( b->delete_button, PANEL_INACTIVE, TRUE, NULL );
      if ( xv_get( p->frame, XV_SHOW ) )
	undisplay_tns_data( b ); 

      ntns--;
      for ( i = tns_row; i < ntns; i++ ) 
	tentries[i] = tentries[i+1];
    }
    
    /*
     * Enable the undo item.
     */
    xv_set( b->undo_button, PANEL_INACTIVE, FALSE, NULL );
    undo_op = DEL;

    if ( !b->binder_modified ) {
      b->binder_modified = TRUE;
      xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
    }

}


/*
 * Destroy callback for binder.
 */
static Notify_value
binder_quit( frame, status )
    Frame          frame;
    Destroy_status status;
{
    Binder *b = ( Binder * )xv_get( frame, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     value;
    int     error;

    p = ( Props * )b->properties;
    if ( ( b->binder_modified || p->icon_props_changed || p->binding_props_changed ) && 
         ( status == DESTROY_CHECKING ) )  {

      value = notice_prompt( b->frame, 0,
			 ALERT_MESSAGE_STRINGS,
  			   MGET( "You have not saved your changes to\nthe Binder database.  If you quit now,\nyour changes will be lost.  You can:"),
			   NULL,
			 ALERT_BUTTON_YES,  MGET( "Save and Quit" ),
			 ALERT_BUTTON_NO,   MGET( "Quit" ),
			 ALERT_BUTTON,      MGET( "Cancel" ), 2,
			 NULL );
      switch ( value ) {
      case NOTICE_YES:
        /* Apply props if changed, save CE to disk, then quit */
        xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Saving Changes" ), NULL );
	setbusy();
        if ( p->icon_props_changed || p->binding_props_changed )
          apply_button( p->icon_apply_button, NULL );
	error = ce_commit_write( NULL );  
	if ( error ) 
	  display_ce_error( b->frame, error );
	ce_end(); 
	setactive();
        break;
      case NOTICE_NO:
        /* Undo all edits, and quit */
	ce_abort_write( NULL );
        break;
      default:
        /* Cancel and return to binder */
	( void )notify_veto_destroy( ( Notify_client )frame );
	return( NOTIFY_DONE );
      }

    }
    else if ( status == DESTROY_CLEANUP )
      return( notify_next_destroy_func( frame, status ) );
    else
      return( NOTIFY_DONE );

}
 
/*
 * Initialize the binder 
 */
static void
b_init_program( argc, argv, b )
    int      argc;
    char   **argv;
    Binder  *b;
{

    /*
     * Parse command line args and determine if
     * a database was specified.
     */
    b_init_scope( argc, argv, b );

    /*
     * Initialize the Classing Engine
     */
    b_init_ce( b );

    /*
     * Initialize current state of binder.
     */
    b_init_state( b );

    /*
     * Create a cms before binder entries read.
     */
    b_init_colors( b );
    
    /*
     * Read in the File and Type NS from the CE
     * after initializing colors.
     */
    b_init_namespaces( b );

    /*
     * Set up items on the scrolling list after binder entries read 
     * and after colors are set in cms.
     */
    create_tns_list( b );
    if ( ntns >= 1 ) {
      xv_set( b->tns_list, PANEL_LIST_SELECT, 0, TRUE, NULL ); 
      tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		       ( Event * )NULL, 0 ); 
    }
      
}

void
checknextarg (next_arg, cmd_option)
char 	*next_arg;
char 	*cmd_option;
{
    if (next_arg != (char *) NULL) 
	  return;

    fprintf (stderr, MGET("binder: Need value for command line option %s\n"), 
				cmd_option);
    exit (0);

}

/*
 * Parse the command line args before being
 * passed to xv_init().
 */
void
check_for_args (argc, argv)
    int		argc;
    char       *argv [];
{
    int		i;

    for (i = 0 ; i < argc; i++) {
		
/*
 * Check if the icon label is set on
 * the command line.
 */
	if (strcmp (argv[i], MGET ("-WL")) == 0 ||
	    strcmp (argv[i], MGET ("-icon_label")) == 0) {

           if (strcmp (argv[i], MGET ("-WL")) == 0)
  	     checknextarg (argv [i+1], MGET ("-WL"));
           else if (strcmp (argv[i], MGET ("-icon_label")) == 0)
  	     checknextarg (argv [i+1], MGET ("-icon_label"));
	   xview_info->icon_label = strdup(argv[++i]);
	   xview_info->icon_label_set = TRUE;
	   } 
/*
 * Check if the frame label is set on
 * the command line.
 */
        else if (strcmp (argv[i], MGET ("-title")) == 0 ||
		 strcmp (argv[i], MGET ("-Wl")) == 0 ||
		 strcmp (argv[i], MGET ("-label")) == 0) {

	   if (strcmp (argv[i], MGET ("-title")) == 0)
  	     checknextarg (argv [i+1], MGET ("-title"));
	   else if (strcmp (argv[i], MGET ("-Wl")) == 0)
  	     checknextarg (argv [i+1], MGET ("-Wl"));
	   else if (strcmp (argv[i], MGET ("-label")) == 0)
  	     checknextarg (argv [i+1], MGET ("-label"));

	   xview_info->frame_label = strdup(argv[++i]);
	   xview_info->frame_label_set = TRUE;
	   }
/*
 * Check if the geometry is set on
 * the command line.
 */
        else if (strcmp (argv[i], MGET ("-geometry")) == 0 ||
		 strcmp (argv[i], MGET ("-size")) == 0 ||
		 strcmp (argv[i], MGET ("-Ws")) == 0) {

	   if (strcmp (argv[i], MGET ("-geometry")) == 0)
  	     checknextarg (argv [i+1], MGET ("-geometry"));
	   else if (strcmp (argv[i], MGET ("-Ws")) == 0)
  	     checknextarg (argv [i+1], MGET ("-Ws"));
	   else if (strcmp (argv[i], MGET ("-size")) == 0)
  	     checknextarg (argv [i+1], MGET ("-size"));

	   xview_info->geometry_set = TRUE;
	   }
/* 
 * Check if icon font was set.
*/
        else if (strcmp (argv[i], MGET ("-icon_font")) == 0 ||
		 strcmp (argv[i], MGET ("-WT")) == 0)
	   	xview_info->icon_font = strdup(argv[++i]);
    }

}


main( argc, argv )
    int  argc;
    char **argv;
{
    Binder  *b;
    Props   *p;
    char     bind_home[MAXPATHLEN];
    extern void tool_info();
#ifdef TOOLTALK
#ifdef OLD_TOOLTALK
    b_init_tt( argc, argv );
#else
    dstt_check_startup (tool_info, &argc, &argv);
#endif
#endif
    /*
     * Initialize xview defaults.
     */
    xview_info = init_xview();

/*
 * Need to check and see if we were started with several different
 * command line options before we do the Xview initialization (such
 * as -label, -icon_label).
 */

    check_for_args (argc, argv);

    /*
     * Connect to the server and set up an X error handler.
     */
    xv_init( XV_INIT_ARGC_PTR_ARGV, &argc, argv,
	    XV_USE_LOCALE, TRUE, 
	    XV_X_ERROR_PROC, error_handler,
	    0);
    
    ds_expand_pathname( "$OPENWINHOME/lib/locale" , bind_home);
    bindtextdomain(MSGFILE, bind_home);
    textdomain(MSGFILE);
    
    /*
     * Initialize user interface components.
     */
    binder = b = binder_objects_initialize( NULL );
    xv_set( b->frame, FRAME_WM_COMMAND_ARGC_ARGV, ( argc-1 ), &( argv[1] ), NULL );
    p = ( Props * )b->properties;
    /*
     * Interpose on the panel for selecting.
     */
    notify_interpose_destroy_func( b->frame, binder_quit );
    
    /*
     * Connect to the ToolTalk server.
     */
#ifndef TOOLTALK
    xv_set( p->fg_color_button, PANEL_INACTIVE, TRUE, XV_NULL );
    xv_set( p->bg_color_button, PANEL_INACTIVE, TRUE, XV_NULL );
#endif
    /*
     * Initialize the Binder elements into the UI components.
     */
    b_init_program( argc, argv, b );
    
#ifdef TOOLTALK
    b_init_tt_complete( b );
#endif
   
    /* 
     *Initialize drag/drop processing for tool.
     */
    init_dragdrop();
    
    ( void )notify_set_signal_func( b->panel, cancel_handler, SIGUSR1, NOTIFY_SYNC );
    ( void )notify_interpose_event_func( b->frame, frame_interpose_proc, NOTIFY_SAFE );
    ( void )notify_interpose_event_func( b->panel, panel_interpose_proc, NOTIFY_SAFE );
    /*
     * Turn control over to XView.
     */
    xv_main_loop( b->frame );
    
#ifdef TOOLTALK
    b_quit_tt();
#endif
    ce_end();
    exit( 0 );
    
}
