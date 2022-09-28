#ifndef lint
#ifdef sccs
static char sccsid[]="@(#)props.c	3.21 09/26/94 Copyright 1987-1990 Sun Microsystems, Inc.";
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
#include <sys/stat.h>
#include <unistd.h>

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>

#include "ds_popup.h"
#include "ds_listbx.h"
#include "binder.h"
#include "props.h"
#include "xv.h"
#include "binder_ce.h"

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
 
extern void        change_color_chips();
extern void        create_tns_entry();
extern int         data_type();
extern char       *string_data_type();
extern void        refresh_icon_color();
extern void        tns_list_notify();

extern Fns_entry   fentries[ MAX_FNS_ENTRIES ];
extern Tns_entry   tentries[ MAX_TNS_ENTRIES ];
extern Tns_entry   reset_entry;
extern int	   nfns;
extern int	   ntns;
extern int         entry_created;

static Fns_entry   filelist[ MAX_FNS_ENTRIES ];
static int         nfiles = 0;
static Fns_list    ops[ MAX_FNS_ENTRIES ];
static int         nops = 0;
static Fns_entry   clipboard;
static int         clipboard_empty = TRUE;
static char        unnamed[80];


static void 
display_fns_data( b )
    Binder *b;
{
    Props *p = ( Props * )b->properties;
    int i;
    Fns_entry f;

    i = current_selection( p->fns_list );
    f = filelist[i];

    /*
     * Check if file entry is selected.
     */
    if ( i != -1 ) {
        
      if ( f.filename ) {   /* by Name */
	xv_set( p->pattern_item, PANEL_INACTIVE, FALSE,
	        PANEL_VALUE, f.filename,
	        NULL );
	xv_set( p->identify_item, PANEL_VALUE, 0, NULL );
	xv_set( p->offset_item, PANEL_VALUE, 0, 
	        PANEL_INACTIVE, TRUE, NULL );
	xv_set( p->type_item, PANEL_VALUE, 0,
	        PANEL_INACTIVE, TRUE, NULL );
	xv_set( p->value_item, PANEL_VALUE, "",
	        PANEL_INACTIVE, TRUE, NULL );
	xv_set( p->mask_item, PANEL_VALUE, "",
	        PANEL_INACTIVE, TRUE, NULL );
      }
      else {  /* by Content */
	xv_set( p->pattern_item, PANEL_INACTIVE, TRUE,
	       PANEL_VALUE, "",
	       NULL );
	xv_set( p->identify_item, PANEL_VALUE, BY_CONTENT, NULL );
	
	if ( !f.data_offset )
	  xv_set( p->offset_item, PANEL_VALUE, 0,
		  PANEL_INACTIVE, FALSE, NULL );
	else
	  xv_set( p->offset_item, PANEL_VALUE, atoi( f.data_offset ), 
		  PANEL_INACTIVE, FALSE, NULL );
          
	xv_set( p->type_item, PANEL_VALUE, data_type( f.data_type ), 
	        PANEL_INACTIVE, FALSE, NULL );
	xv_set( p->value_item, PANEL_VALUE, f.data_value, 
	        PANEL_INACTIVE, FALSE, NULL );
	xv_set( p->mask_item, PANEL_VALUE, f.data_mask, 
	        PANEL_INACTIVE, FALSE, NULL );
      }  /* if f.filename */

    }  /* if i < nfiles */

   /*
    * No file entry selected - clear.
    */
    else 
      xv_set( p->pattern_item, PANEL_VALUE, "", NULL );

  
}

void 
undisplay_fns_data( b )
    Binder *b;
{
    Props *p = ( Props * )b->properties;
        
    /*
     * No fns entry selected initially so gray out buttons.
     */
    xv_set( p->cut_button, PANEL_INACTIVE, TRUE, NULL );
    xv_set( p->copy_button, PANEL_INACTIVE, TRUE, NULL );
    xv_set( p->paste_button, PANEL_INACTIVE, clipboard_empty, NULL );
    xv_set( p->delete_button, PANEL_INACTIVE, TRUE, NULL );

    xv_set( p->pattern_item, PANEL_INACTIVE, FALSE,
	    PANEL_VALUE, "",
	    NULL );
    xv_set( p->identify_item, PANEL_VALUE, BY_NAME, NULL );
    xv_set( p->offset_item, PANEL_VALUE, 0, 
	    PANEL_INACTIVE, TRUE, 
	    NULL );
    xv_set( p->type_item, 
	    PANEL_VALUE, BYTE_TAG,
	    PANEL_INACTIVE, TRUE,
	    NULL );
    xv_set( p->value_item, PANEL_VALUE, "",
	    PANEL_INACTIVE, TRUE, 
	    NULL );
    xv_set( p->mask_item, PANEL_VALUE, "",
	    PANEL_INACTIVE, TRUE, 
	    NULL );
 
}

static void
create_fns_list( b )
    Binder     *b;
{
    Props  *p = ( Props * )b->properties;
    int     tns_row;
    int     i;

    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) 
      return;

    xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );
    xv_set( p->cut_button, PANEL_INACTIVE, FALSE, NULL );
    xv_set( p->delete_button, PANEL_INACTIVE, FALSE, NULL );
    /*
     * Allocate pointer only once.
     */
#ifdef NEVER
    if ( s == NULL ) {
printf(" allocating space for s\n" );
      s = ( char ** )malloc( sizeof( char * ) * ( MAX_FNS_ENTRIES + 1 ) );
    }
#endif
    /*
     * Initialize booleans.
     */
    for ( i = 0; i < MAX_FNS_ENTRIES; i++ ) {
      ops[i].add    = FALSE;
      ops[i].delete = FALSE;
    }

    /*
     * Step through all file entries and display those
     * that match the selected tns entry.
     */
    nfiles = 0;

    for ( i = 0; i < nfns; i++ ) {
      if ( strcmp( fentries[i].type_name, tentries[tns_row].type_name ) == 0 ) {
#ifdef NEVER
        if ( fentries[i].filename ) 
          s[nfiles] = fentries[i].filename;
        else
          s[nfiles] = fentries[i].type_name;
#else
        if ( fentries[i].filename ) 
          list_add_entry( p->fns_list, fentries[i].filename, NULL, NULL, nfiles, TRUE );
        else
          list_add_entry( p->fns_list, CONTENT_ENTRY, NULL, NULL, nfiles, TRUE );
#endif
        /* Save fns list entries for this Type entry */
        filelist[nfiles] = fentries[i];
        nfiles++;
      }
    }

#ifdef NEVER
    s[nfiles] = ( char * )NULL; 
    xv_set( p->fns_list, PANEL_LIST_INSERT, 0,
                         PANEL_LIST_STRINGS, s, NULL );
#endif
if ( p->current_view == BINDING_PROPS )
  xv_set( xv_get( p->fns_list, PANEL_LIST_SCROLLBAR ), XV_SHOW, TRUE, NULL );
else
  xv_set( xv_get( p->fns_list, PANEL_LIST_SCROLLBAR ), XV_SHOW, FALSE, NULL );

}

void
show_icon_items( b, show )
    Binder *b;
    int     show;
{
    Props     *p = ( Props * )b->properties;
    int        i;
    Tns_entry  t; 
    
    xv_set( p->panels[ICON_PANEL_L], XV_SHOW, show, NULL );

    if ( !show ) {
      xv_set( p->panels[ICON_PANEL_R], XV_SHOW, FALSE, NULL );
    }

    i = current_selection( b->tns_list );
    if ( i == -1 ) 
      return;

    if ( p->full_size ) {
      xv_set( p->panel, XV_WIDTH, 2 * p->widest, NULL );
      window_fit_width( p->frame );
      xv_set( p->panels[ICON_PANEL_R], XV_SHOW, TRUE, NULL );
      xv_set( p->icon_plus_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->icon_minus_button, PANEL_INACTIVE, FALSE, NULL );
    }
    else {
      xv_set( p->panel, XV_WIDTH, p->widest, NULL );
      xv_set( p->panels[ICON_PANEL_R], XV_SHOW, FALSE, NULL );
      window_fit( p->frame );
      xv_set( p->icon_plus_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( p->icon_minus_button, PANEL_INACTIVE, TRUE, NULL );
    }

    if ( !p->icon_props_changed || entry_created ) {
    
      t = tentries[i];

      if ( t.icon_image )
	xv_set( p->icon_preview, PANEL_LABEL_IMAGE, t.icon_image, NULL ); 
      if ( t.type_name )
	xv_set( p->icon_item,  PANEL_VALUE, t.type_name, NULL );
      else
	xv_set( p->icon_item,  PANEL_VALUE, "", NULL );
      if ( t.icon_file )
	xv_set( p->image_file_item, PANEL_VALUE, t.icon_file, NULL );
      else
	xv_set( p->image_file_item, PANEL_VALUE, "", NULL );
      if ( t.icon_mask_file )
	xv_set( p->mask_file_item, PANEL_VALUE, t.icon_mask_file, NULL );
      else
	xv_set( p->mask_file_item, PANEL_VALUE, "", NULL );
      if ( t.fg_color )
	xv_set( p->fg_color_item, PANEL_VALUE, t.fg_color, NULL );
      else
	xv_set( p->fg_color_item, PANEL_VALUE, "", NULL );
      if ( t.bg_color )
	xv_set( p->bg_color_item, PANEL_VALUE, t.bg_color, NULL );
      else
	xv_set( p->bg_color_item, PANEL_VALUE, "", NULL );
      if ( t.open_method )
	xv_set( p->open_item, PANEL_VALUE, t.open_method, NULL );
      else
	xv_set( p->open_item, PANEL_VALUE, "", NULL );
      if ( t.print_method )
	xv_set( p->print_item, PANEL_VALUE, t.print_method, NULL );
      else
	xv_set( p->print_item, PANEL_VALUE, "", NULL );

#ifdef OLD_TOOLTALK      
      if ( b->color && b->tt_running ) {
#endif
      if (b->color) {
	xv_set( p->fg_color_button, PANEL_INACTIVE, FALSE, NULL );
	xv_set( p->bg_color_button, PANEL_INACTIVE, FALSE, NULL );
      }

      change_color_chips( b );

    }

}

void
show_binding_items( b, show )
    Binder *b;
    int     show;
{
    Props *p = ( Props * )b->properties;

    xv_set( p->panels[BINDING_PANEL_L], XV_SHOW, show, NULL );

    if ( !show ) {
      xv_set( p->panels[BINDING_PANEL_R], XV_SHOW, FALSE, NULL );
      return;
    }

    if ( p->full_size ) {
      xv_set( p->panel, XV_WIDTH, 2 * p->widest, NULL );
      window_fit_width( p->frame );
      xv_set( p->panels[BINDING_PANEL_R], XV_SHOW, TRUE, NULL );
      xv_set( p->binding_plus_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->binding_minus_button, PANEL_INACTIVE, FALSE, NULL );
    }
    else {
      xv_set( p->panel, XV_WIDTH, p->widest, NULL );
      xv_set( p->panels[BINDING_PANEL_R], XV_SHOW, FALSE, NULL );
      window_fit( p->frame );
      xv_set( p->binding_plus_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( p->binding_minus_button, PANEL_INACTIVE, TRUE, NULL );
    }
    /*
     * Search all FNS entries that match t->type_name and
     * display in fns list.
     */
    if ( !p->binding_props_changed ) {
      list_flush( p->fns_list );
      create_fns_list( b );
    }
    undisplay_fns_data( b );    

}

void
show_program_items( b, show )
    Binder *b;
    int     show;
{
    Props  *p = ( Props * )b->properties;

    xv_set( p->panels[PROGRAM_PANEL_L], XV_SHOW, show, NULL );
    if ( !show ) {
      xv_set( p->panels[PROGRAM_PANEL_R], XV_SHOW, FALSE, NULL );
      return;
    }
    
    if ( p->full_size ) {
      xv_set( p->panel, XV_WIDTH, 2 * p->widest, NULL );
      window_fit_width( p->frame );
      xv_set( p->panels[PROGRAM_PANEL_R], XV_SHOW, TRUE, NULL );
#ifdef LATER
      xv_set( p->program_plus_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->program_minus_button, PANEL_INACTIVE, FALSE, NULL );
#endif
    }
    else {
      xv_set( p->panel, XV_WIDTH, p->widest, NULL );
      xv_set( p->panels[PROGRAM_PANEL_R], XV_SHOW, FALSE, NULL );
      window_fit( p->frame );
#ifdef LATER
      xv_set( p->program_plus_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( p->program_minus_button, PANEL_INACTIVE, TRUE, NULL );
#endif
    }

}

/*
 * Notify callback function for `category item'.
 */
/*ARGSUSED*/
void     
props_category_notify( item, value, event )
    Panel_item  item;
    int         value;
    Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     notice_value;
    int     next_category = TRUE;
    int     tns_row;
    int     success;

    p = ( Props * )b->properties;
    xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );
    xv_set( p->cut_button, PANEL_INACTIVE, FALSE, NULL );
    xv_set( p->delete_button, PANEL_INACTIVE, FALSE, NULL );

#ifdef NEVER
    /*
     * Alert user if any property values have changed
     * before switching categories.
     */
    if ( ( p->current_view == ICON_PROPS && p->icon_props_changed ) ||
	 ( p->current_view == BINDING_PROPS && p->binding_props_changed ) ) {
      xv_set( item, PANEL_VALUE, p->current_view, NULL );
      notice_value = notice_prompt( p->panel, event,
		       NOTICE_MESSAGE_STRINGS,
  		         MGET( "You must save your changes\nbefore changing the view. Unsaved\nchanges will be lost if you continue.\nYou can:" ),
		         NULL,
		       NOTICE_BUTTON_YES, MGET( "Save" ),
		       NOTICE_BUTTON_NO,  MGET( "Continue" ),
		       NOTICE_BUTTON,     MGET( "Cancel" ), 2,
		       NULL );

      switch( notice_value ) {
        case NOTICE_YES:
          apply_button( item, event );
          xv_set( item, PANEL_VALUE, value, NULL );
          break;
        case NOTICE_NO:
          reset_button( item, event );
          xv_set( item, PANEL_VALUE, value, NULL );
          break;
        default:
          next_category = FALSE;
	  break;
      }
    }
#endif

    if ( value == BINDING_PROPS ) {

      /*
       * Verify that the icon text items are valid.
       */
      tns_row = current_selection( b->tns_list );
      if ( tns_row == -1 )
        return;
      success = verify_tns_items( b, &tentries[tns_row] );
      if ( !success ) {
        p->current_view = ICON_PROPS;
        xv_set( item, PANEL_VALUE, p->current_view, NULL );
        next_category = FALSE;
      }
      else
	p->current_view = value;
    }
    else
      p->current_view = value;

    if ( next_category ) {
      /*
       * Set current props sheet items.
       */
      switch ( value ) {
      case ICON_PROPS:
 	show_program_items( b, FALSE );
	show_binding_items( b, FALSE );    
	show_icon_items( b, TRUE );    
	break;
      case BINDING_PROPS:
	show_icon_items( b, FALSE );    
	show_program_items( b, FALSE );
	show_binding_items( b, TRUE );    
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


}

/*
 * Notify callback function for `icon props'.
 */
/*ARGSUSED*/
void     
show_icon_props( menu, item )
    Menu        menu;
    Menu_item   item;
{
    Binder  *b = ( Binder * )xv_get( menu, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      tns_row;
    int      next_category = TRUE;
    int      notice_value;

    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      window_bell( b->frame );
      xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }
    p = ( Props * )b->properties;

#ifdef NEVER
    if ( p->binding_props_changed ) {

      xv_set( p->category_item, PANEL_VALUE, p->current_view, NULL );
      notice_value = notice_prompt( p->panel, NULL,
		       NOTICE_MESSAGE_STRINGS,
  		         MGET( "You must save your changes\nbefore changing the view. Unsaved\nchanges will be lost if you continue\nYou can:" ),
		         NULL,
		       NOTICE_BUTTON_YES, MGET( "Save" ),
		       NOTICE_BUTTON_NO,  MGET( "Continue" ),
		       NOTICE_BUTTON,     MGET( "Cancel" ), 2,
		       NULL );

      switch( notice_value ) {
        case NOTICE_YES:
          apply_button( p->icon_apply_button, NULL );
          break;
        case NOTICE_NO:
          reset_button( p->icon_apply_button, NULL );
          break;
        default:
          next_category = FALSE;
	  break;
      }
    }
#endif

    if ( next_category ) {
      /*
       * Set current props sheet items.
       */
      p->current_view = ICON_PROPS;
      xv_set( p->category_item, PANEL_VALUE, p->current_view, NULL );

      show_program_items( b, FALSE );
      show_binding_items( b, FALSE );    
      show_icon_items( b, TRUE );    
      /*
       * Redisplay only if not showing.
       */
      if ( !xv_get( p->frame, XV_SHOW ) ) {
	ds_position_popup( b->frame, p->frame, DS_POPUP_RIGHT );
      }
    }
    xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );
    xv_set( p->frame, XV_SHOW, TRUE, NULL );

}

/*
 * Notify callback function for `icon props'.
 */
/*ARGSUSED*/
void     
show_binding_props( menu, item )
    Menu        menu;
    Menu_item   item;
{
    Binder  *b = ( Binder * )xv_get( menu, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      tns_row;
    int      next_category = TRUE;
    int      notice_value;

    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      window_bell( b->frame );
      xv_set( b->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }

    p = ( Props * )b->properties;
#ifdef NEVER
    if ( p->icon_props_changed ) {

      xv_set( p->category_item, PANEL_VALUE, p->current_view, NULL );
      notice_value = notice_prompt( p->panel, NULL,
		       NOTICE_MESSAGE_STRINGS,
  		         MGET( "You must save your changes\nbefore changing the view. Unsaved\nchanges will be lost if you continue.\nYou can:" ),
		         NULL,
		       NOTICE_BUTTON_YES, MGET( "Save" ),
		       NOTICE_BUTTON_NO,  MGET( "Continue" ),
		       NOTICE_BUTTON,     MGET( "Cancel" ), 2,
		       NULL );

      switch( notice_value ) {
        case NOTICE_YES:
          apply_button( p->icon_apply_button, NULL );
          break;
        case NOTICE_NO:
          reset_button( p->icon_apply_button, NULL );
          break;
        default:
          next_category = FALSE;
	  break;
      }
    }
#endif

    if ( next_category ) {
      /*
       * Set current props sheet items.
       */
      p->current_view = BINDING_PROPS;
      xv_set( p->category_item, PANEL_VALUE, p->current_view, NULL );

      show_icon_items( b, FALSE );    
      show_program_items( b, FALSE );
      show_binding_items( b, TRUE );    
      /*
       * Redisplay only if not showing.
       */
      if ( !xv_get( p->frame, XV_SHOW ) ) {
	ds_position_popup( b->frame, p->frame, DS_POPUP_RIGHT );
      }
    }
    xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );
    xv_set( p->frame, XV_SHOW, TRUE, NULL );

}
      
/*
 * Notify callback function for `program props'.
 */
/*ARGSUSED*/
void     
show_program_props( menu, item )
    Menu        menu;
    Menu_item   item;
{
    Binder  *b = ( Binder * )xv_get( menu, XV_KEY_DATA, INSTANCE );
    Props   *p;

    p = ( Props * )b->properties;

    p->current_view = PROGRAM_PROPS;
    show_program_items( b, TRUE );
    xv_set( p->category_item, PANEL_VALUE, PROGRAM_PROPS, NULL );

    if ( !xv_get( p->frame, XV_SHOW ) ) {
      ds_position_popup( b->frame, p->frame, DS_POPUP_RIGHT );
    }
    xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );
    xv_set( p->frame, XV_SHOW, TRUE, NULL );

}

/*
 * Check if duplicate.
 */
static int
fns_duplicate( f )
    Fns_entry *f;
{
    int i, j;

    for ( i = 0; i < nfiles; i++ ) {
	
      if ( f->filename && filelist[i].filename &&
	  strcmp( filelist[i].filename, f->filename ) == 0  &&
	  strcmp( filelist[i].db_name, f->db_name ) == 0 ) {
	
	for (j = 0; j < nops; j++) {
          if (ops[j].delete == TRUE && ops[j].f.filename &&
              strcmp (ops[j].f.filename, f->filename) == 0 &&
              clipboard.filename &&
              strcmp (clipboard.filename, f->filename) == 0)
            return 0; /*Not Dup*/
        }

	return( 1 );  /* Dup */
      }
      else if ( ( filelist[i].data_value && f->data_value && 
		  strcmp( filelist[i].data_value, f->data_value ) == 0 ) &&
	        ( filelist[i].data_offset && f->data_offset && 
	 	  strcmp( filelist[i].data_offset, f->data_offset ) == 0 ) &&
	        ( filelist[i].data_type && f->data_type && 
		  strcmp( filelist[i].data_type, f->data_type ) == 0 ) &&
	        ( ( filelist[i].data_mask && f->data_mask &&
		    strcmp( filelist[i].data_mask, f->data_mask ) == 0 ) || 
		  ( !filelist[i].data_mask && !f->data_mask ) ) &&
	        ( f->db_name &&
                  strcmp( filelist[i].db_name, f->db_name ) == 0 ) )

	return( 1 );  /* Dup */
    }

  /*
   * Not a Duplicate
   */
  return( 0 ); 

}

/*
 *
 */
static char *
unique_fns_name( b )
    Binder *b;
{
    int   i;
    int   count = 1;
    int   found = FALSE;
    
    while ( !found ) {     
      switch ( b->scope ) {
      case USER:
        sprintf( unnamed, "%s_%d", UNNAMED, count );
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

      for ( i = 0; i < nfiles; i++ ) {
        if ( filelist[i].filename && ( strcmp( unnamed, filelist[i].filename ) == 0 ) ) 
          break;
      }
      if ( i == nfiles ) 
        found = TRUE;

      count++;

    }
 
    return( unnamed );

}

/*
 * Notify callback function for 'fns_list'.
 */
/* ARGSUSED */
void
fns_list_notify( item, string, client_data, op, event, row )
     Panel_item       item;
     char            *string;
     caddr_t          client_data;
     Panel_list_op    op;
     Event           *event;
     int              row;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );

    if ( row < 0 || row >= nfiles ) return;

    p = ( Props * )b->properties;

    if ( op == PANEL_LIST_OP_SELECT ) {
      printf( "" );
      if ( read_only_entry( filelist[row].db_name ) ) {
	xv_set( p->frame, FRAME_RIGHT_FOOTER, R_ONLY_ENTRY, NULL );
	xv_set( p->cut_button, PANEL_INACTIVE, TRUE, NULL );
	xv_set( p->delete_button, PANEL_INACTIVE, TRUE, NULL );
      }
      else {
	xv_set( p->frame, FRAME_RIGHT_FOOTER, RW_ENTRY, NULL );
	xv_set( p->cut_button, PANEL_INACTIVE, FALSE, NULL );
	xv_set( p->delete_button, PANEL_INACTIVE, FALSE, NULL );
      }
      xv_set( p->copy_button, PANEL_INACTIVE, FALSE, NULL );
      display_fns_data( b );
    }
    else if ( op == PANEL_LIST_OP_DESELECT ) {
      xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );
      xv_set( p->cut_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( p->copy_button, PANEL_INACTIVE, TRUE, NULL );
      xv_set( p->delete_button, PANEL_INACTIVE, FALSE, NULL );
      undisplay_fns_data( b );
    }

}

/*
 * Notify callback function for `props_new_button'.
 */
/*ARGSUSED*/
void
props_new_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder    *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props     *p;
    int        tns_row;
    Fns_entry  f;

    p = ( Props * )b->properties;

    f.entry = NULL;
    f.data_value = NULL;
    f.data_offset = NULL;
    f.data_type = NULL;
    f.data_mask = NULL;
    f.db_name = strdup( b->current_ce_db );

    /*
     * Change the type name in the FNS entry 
     * to point to currently selected TNS entry.
     * Default to "by Name" and "unnamed*".
     */
    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }
    f.type_name = strdup( tentries[tns_row].type_name );
    f.filename  = strdup( unique_fns_name( b ) );

    /*
     * Add new FNS entry to File Namespace
     * (if not a duplicate).
     */
    if ( fns_duplicate( &f ) ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Pattern Already in Use" ), NULL );
      if ( f.db_name ) free( f.db_name );
      if ( f.type_name ) free( f.type_name );
      if ( f.filename ) free( f.filename );
      return;
    }

   /*
    * Append to ops and fns list.
    */
    filelist[nfiles] = f;
    nfiles++;
    ops[nops].f = f;
    ops[nops].add = TRUE;
    nops++;

   /*
    * Add to FNS scroll list.
    */
    list_add_entry( p->fns_list, f.filename, NULL, NULL, nfiles-1, TRUE );
    xv_set( p->fns_list, PANEL_LIST_SELECT, nfiles-1, TRUE, NULL );
    fns_list_notify( p->fns_list, f.filename, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		     ( Event * )NULL, nfiles-1 );

    xv_set( p->pattern_item, PANEL_VALUE, f.filename,
	                     PANEL_TEXT_SELECT_LINE, NULL );
    p->binding_props_changed = TRUE;

}

/*
 * Notify callback function for `cut_button'.
 */
/*ARGSUSED*/
void
cut_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      fns_row;
    int      j;

    p = ( Props * )b->properties;

   /*
    * Delete from CE database.
    * Delete the fns entry from the panel list.
    * Save on clipboard.
    * Delete from ops.
    * Delete from fnentries list.
    */
    fns_row = current_selection( p->fns_list );
    if ( fns_row == -1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return;
    }
    else if ( fns_row >= 0 && fns_row <= nfiles ) {
      xv_set( p->fns_list, PANEL_LIST_DELETE, fns_row, NULL );
      clipboard = filelist[fns_row];
      clipboard_empty = FALSE;
      xv_set( p->paste_button, PANEL_INACTIVE, clipboard_empty, NULL );

      ops[nops].delete = TRUE;
      ops[nops].f = clipboard;
      nops++;
/* anuj */
      nfiles--;
      for ( j = fns_row; j < nfiles; j++ )
        filelist[j] = filelist[j+1];

    }
    else
      fprintf( stderr, MGET("cut_button: Invalid FNS entry\n" ) );

    undisplay_fns_data( b );
    p->binding_props_changed = TRUE;

}

/*
 * Notify callback function for `copy_button'.
 */
/*ARGSUSED*/
void
copy_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      i;

    p = ( Props * )b->properties;

    i = current_selection( p->fns_list );
    if ( i == -1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
    }
    else if ( i >= 0 && i < nfiles ) {
      clipboard = filelist[i];
      clipboard_empty = FALSE;
      xv_set( p->paste_button, PANEL_INACTIVE, clipboard_empty, NULL );
    }
    else
      fprintf( stderr, MGET( "copy_button: Invalid FNS entry\n" ) );

}

/*
 * Notify callback function for `paste_button'.
 */
/*ARGSUSED*/
void
paste_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder    *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props     *p;
    int        tns_row;
    Fns_entry  f;

    p = ( Props * )b->properties;

   /*
    * Check if something is on the clipboard.
    */
   if ( clipboard_empty ) {
     window_bell( p->frame );
     xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Clipboard is Empty" ), NULL );
     return;
   }

   /*
    * Make a copy of the clipboard to the
    * end of the File Namespace entries.
    */
    f = clipboard;

   /*
    * Change the type name in the FNS entry 
    * to point to currently selected TNS entry.
    * Fix later strdup()
    */
    tns_row = current_selection( b->tns_list );
    if ( tns_row == - 1 )
      return;
    f.type_name = strdup( tentries[tns_row].type_name );
    f.db_name = strdup( b->current_ce_db );

   /*
    * Add new FNS entry to File Namespace
    * (if not a duplicate).
    */
    if ( fns_duplicate( &f ) ) {
      window_bell( p->frame );
      if ( f.filename )
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Pattern Already in Use" ), NULL );
      else
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Content Already in Use" ), NULL );
      if ( f.type_name ) free( f.type_name );
      if ( f.db_name ) free( f.db_name );
      return;
    }

   /*
    * Add to FNS scroll list.
    */
    if ( f.filename ) 
      list_add_entry( p->fns_list, f.filename, NULL, NULL, nfiles, TRUE );
    else 
      list_add_entry( p->fns_list, CONTENT_ENTRY, NULL, NULL, nfiles, TRUE );

   /*
    * Append to ops and filelist.
    */ 
    filelist[nfiles] = f;
    nfiles++;
    ops[nops].f = f;
    ops[nops].add = TRUE;
    nops++;

    fns_list_notify( p->fns_list, NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT, 
		     ( Event * )NULL, nfiles );

    p->binding_props_changed = TRUE;

}

static int
type_name_exists( b, type_name )
    Binder *b;
    char   *type_name;
{
    int  found = FALSE;
    int  i;
    int  tns_row;

    tns_row = current_selection( b->tns_list );
    if ( tns_row == - 1 )
      return( 0 );

    for ( i = 0; i < ntns; i++ ) { 
     if ( i == tns_row )
       continue;
     if ( strcmp( type_name, tentries[i].type_name ) == 0 ) {
	found = TRUE;
	break;
      }
    }
    return( found );

}

/*
 * Verifies the text entries before applying.
 * Returns 1 success
 *         0 failure
 */
static int
verify_tns_items( b, t )
     Binder    *b;
     Tns_entry *t;
{
    Props *p = ( Props * )b->properties;
    char  *icon, *image, *mask, *app, *print, *fg, *bg;
    char   fullpath[ MAXPATHLEN ], msg[ MAXPATHLEN ];
    int    width, height;
    int    ncolors;
    int    red, green, blue;
    struct stat statbuf;

    icon  = ( char * )xv_get( p->icon_item, PANEL_VALUE );
    image = ( char * )xv_get( p->image_file_item, PANEL_VALUE );
    mask  = ( char * )xv_get( p->mask_file_item, PANEL_VALUE );
    app   = ( char * )xv_get( p->open_item, PANEL_VALUE );
    print = ( char * )xv_get( p->print_item, PANEL_VALUE );
    fg    = ( char * )xv_get( p->fg_color_item, PANEL_VALUE );
    bg    = ( char * )xv_get( p->bg_color_item, PANEL_VALUE );

    /*
     * There must be an icon label
     */
    if ( *icon == NULL ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Enter an Icon Label" ), NULL );
      return( 0 );
    }
    
    /*
     * There must be an icon image file.
     */
    if ( *image == NULL ) {
      window_bell ( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Enter an Image File" ), NULL );
      return( 0 );
    }

    /*
     * The icon file must exist on disk.
     */
    ds_expand_pathname( image, fullpath );
    if ( access( fullpath, F_OK ) != 0 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Image File not Found" ), NULL );
      return( 0 );
    }
    if ( stat( fullpath, &statbuf ) == - 1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Image File" ), NULL );
      return( 0 );
    }
    if ( !( S_ISREG( statbuf.st_mode ) ) ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Image File" ), NULL );
      return( 0 );
    }
     
   /*
    * Ensure the icon file has valid contents.
    */
    t->icon_stipple = ( Server_image )icon_load_svrim( fullpath, msg );
    if ( !t->icon_stipple ) { 
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Bad Image File" ), NULL );
      xv_destroy_safe( t->icon_stipple );
      return( 0 );
    }

    /*
     * Verify the size of the icon.
     */
    width  = ( int )xv_get( t->icon_stipple, XV_WIDTH );
    height = ( int )xv_get( t->icon_stipple, XV_HEIGHT );
    if ( width > MAX_ICON_HEIGHT || height > MAX_ICON_HEIGHT ) { 
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Icon too Large" ), NULL );
      if ( t->icon_stipple ) xv_destroy_safe( t->icon_stipple );
      return( 0 );
    }  

    /*
     * If an icon mask file is specified, it must exist on disk.
     * Then ensure the mask contents are valid.
     */ 
    if ( *mask != NULL ) {
      ds_expand_pathname( mask, fullpath );
      if ( access( fullpath, F_OK ) != 0 ) {    
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Mask File not Found" ), NULL );
        return( 0 );
      }
      if ( stat( fullpath, &statbuf ) == - 1 ) {
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Mask File" ), NULL );
        return( 0 );
      }
      if ( !( S_ISREG( statbuf.st_mode ) ) ) {
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Mask File" ), NULL );
        return( 0 );
      }

      t->icon_mask = ( Server_image )icon_load_svrim( fullpath, msg );
      if ( !t->icon_mask ) {
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Bad Mask File" ), NULL );
	xv_destroy_safe( t->icon_stipple );
        return( 0 );
      }
    }

    if ( *fg != NULL ) {
      ncolors =  sscanf( fg, "%d %d %d", &red, &green, &blue );
      if ( ncolors != 3 ) {
	window_bell( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Foreground Color" ), NULL );
	return( 0 );
      }
    }
    if ( *bg != NULL ) {
      ncolors =  sscanf( bg, "%d %d %d", &red, &green, &blue );
      if ( ncolors != 3 ) {
	window_bell( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Background Color" ), NULL );
	return( 0 );
      }
    }

    t->type_name = strdup( icon );
    
    if ( *image != NULL )
      t->icon_file = strdup( image );
    else
      t->icon_file = NULL;
    if ( *mask != NULL )
      t->icon_mask_file = strdup( mask );
    else
      t->icon_mask_file = NULL;
    if ( *app != NULL )
      t->open_method = strdup( app );
    else
      t->open_method = NULL;
    if ( *print != NULL )
      t->print_method = strdup( print );
    else
      t->print_method = NULL;
    if ( *fg != NULL )
      t->fg_color = strdup( fg );
    else
      t->fg_color = NULL;
    if ( *bg != NULL )
      t->bg_color = strdup( bg );
    else
      t->bg_color = NULL;
    
    return( 1 );
}

/*
 * Apply the Icon Properties.
 * Returns 1 success
 *         0 failure
 */
static int
apply_icon_props( b )
    Binder *b;
{
    Props     *p = ( Props * )b->properties;
    int        tns_row;
    int        success;
    int        status;

    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 )
      return( 0 );

    /*
     * Verify that the icon text items are valid.
     */
    success = verify_tns_items( b, &tentries[tns_row] );
    if ( !success ) 
      return( 0 );

    /*
     * If the current CE database open matches the
     * entry just modified, then do a modify, otherwise
     * add the entry.
     */
    if ( strcmp( tentries[tns_row].db_name, b->current_ce_db ) == 0 )
      success = modify_tns_entry( b, &tentries[tns_row] );
    else 
      success = add_tns_entry( b, &tentries[tns_row] );

    if ( !success ) 
      return( 0 );
    /*
     * Read them back into our memory from the CE.
     * FIX later.  Free up old icon space.
     */
    status = read_tns_entry( b->tns, tentries[tns_row].entry, b ); 
    if ( status != 0 )
      return( 0 );

    /*
     * Delete current tns entry on scroll list and
     * create a new one.
     */
    xv_set( b->tns_list, PANEL_LIST_DELETE, tns_row, NULL ); 
    create_tns_entry( b, &tentries[tns_row], tns_row );
    xv_set( b->tns_list, PANEL_LIST_SELECT, tns_row, TRUE, NULL ); 
    tns_list_notify( b->tns_list, ( char * )NULL, ( caddr_t )NULL, PANEL_LIST_OP_SELECT,
		     ( Event * )NULL, tns_row );

    xv_set( p->icon_preview, PANEL_LABEL_IMAGE, tentries[tns_row].icon_image, NULL ); 
    change_color_chips( b );

    return( 1 );

}


/*
 * Apply the Binding Properties.
 * Returns 1 success
 *         0 failure
 */
static int
apply_binding_props( b )
    Binder *b;
{
    extern    int find_entry_to_delete();
    Props     *p = ( Props * )b->properties;
    char      *tag_value, *tag_mask;
    int        fns_row;
    int        tag_offset;
    int        new_entry;
    int        success;
    int        i, j, k;
    int        error;
    Fns_entry  f, old_f;
    char       buf[10];

    fns_row = current_selection( p->fns_list );

    if ( ( p->current_view == BINDING_PROPS ) && 
         ( fns_row != -1 ) && 
         ( xv_get( p->identify_item, PANEL_VALUE ) == BY_CONTENT ) ) {

      /* There must be a content value.
       */
      tag_value = ( char * )xv_get( p->value_item, PANEL_VALUE );
      if ( *tag_value == NULL ) {
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Enter a Tag Value" ), NULL );
	return( 0 );
      }
      f = filelist[fns_row];
      old_f = f;
      f.filename = NULL;
      f.data_value = strdup( tag_value );
      f.data_type = strdup( string_data_type( xv_get( p->type_item, PANEL_VALUE ) ) );

      tag_offset = ( int )xv_get( p->offset_item, PANEL_VALUE );
      sprintf( buf, "%d", tag_offset );
      f.data_offset = strdup( buf );

      tag_mask = ( char * )xv_get( p->mask_item, PANEL_VALUE );
      if ( *tag_mask == NULL )
        f.data_mask = NULL;
      else
        f.data_mask = strdup( tag_mask );

      /*
       * If the current CE database open matches the
       * entry just modified, then do a modify, otherwise
       * add the entry. Fix this later? Do we want to add from user to system?
       */
      if ( strcmp( filelist[fns_row].db_name, b->current_ce_db ) == 0 ) {
	f.db_name = strdup( filelist[fns_row].db_name );
	f.entry = filelist[fns_row].entry;
	new_entry = FALSE;
      }
      else {
	f.db_name = strdup( b->current_ce_db );
	f.entry = NULL;
	new_entry = TRUE;
      }

      /*
       * Change FNS scroll list.
       */
      if ( new_entry ) {
        list_add_entry( p->fns_list, CONTENT_ENTRY, NULL, NULL, nfiles, TRUE );
	xv_set( p->fns_list, PANEL_LIST_SELECT, nfiles, TRUE, NULL );
      }
      else
        xv_set( p->fns_list, PANEL_LIST_STRING, fns_row, CONTENT_ENTRY, NULL );

      /*
       * Place new/modified entry in the ops and fentries list.
       */ 
      if ( new_entry ) {
        filelist[nfiles] = f;
        nfiles++;
	ops[nops].f = f;
        ops[nops].add = TRUE;
	nops++;
      }
      else { /* change */
	
        filelist[fns_row] = f;
        ops[nops].add = TRUE;
        ops[nops].f = f;
        nops++;
        ops[nops].delete = TRUE;
        ops[nops].f = old_f;
        nops++;
      }

    }

    /*
     * Apply last n operations.
     */
    for ( i = 0; i < nops; i++ ) {
      if ( ops[i].add ) {
        success = add_fns_entry( b, &ops[i].f );
        if ( !success ) 
          fprintf( stderr, "Unsucessful add entry\n" ); 
        fentries[nfns] = ops[i].f;
        nfns++;
      }
      else if ( ops[i].delete ) {
        if ( ops[i].f.entry == NULL ) 
          ce_map_through_entries( b->fns, ( CE_ENTRY_MAP_FUNCTION )find_entry_to_delete, &ops[i].f );
#ifdef NEVER
        ops[i].f.entry = ce_get_entry( b->fns, 1, ops[i].f.filename );
#endif        
        if ( ops[i].f.entry != NULL )
          error = ce_remove_entry( b->fns, ops[i].f.entry );
        if ( error ) {
          display_ce_error( p->frame, error );
          continue;
        }
        k = nfns;
        for ( j = 0; j < nfns; j++ ) {
          if ( fentries[j].entry == ops[i].f.entry ) {
            k = j;
            break;
          }
        }
        nfns--;
        for ( j = k; j < nfns; j++ ) 
          fentries[j] = fentries[j+1];
      }
    }
    nops = 0;
    list_flush( p->fns_list );
    create_fns_list( b );

    return( 1 );

}

/*
 * Notify callback function for `apply_button'.
 */
/*ARGSUSED*/
void
apply_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      success;
    int      tns_row, i;
    char    *old_type_name, *new_type_name;
    extern char *title_bar_name();
	
    char *pattern_val;

    p = ( Props * )b->properties;
    b->applying_changes = TRUE;

/** check if there is some text in pattern textfield, then call 
    pattern_notify  ***/

    pattern_val = xv_get(p->pattern_item, PANEL_VALUE);
    if (( p->current_view == BINDING_PROPS ) && strcmp(pattern_val, ""))
	pattern_notify(item, event);

#ifdef NEVER
    switch( p->current_view ) {
      case ICON_PROPS:
        success = apply_icon_props( b );
        p->icon_props_changed = FALSE;
        break;
      case BINDING_PROPS:
        success = apply_binding_props( b );
        p->binding_props_changed = FALSE;
        break;
      case PROGRAM_PROPS:
        /* success = apply_program_props */
        break;
      default:
        break;
    }
#endif
     
    /*
     * Verify that the icon text items are valid.
     */
    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 )
      return;
    old_type_name = strdup( tentries[tns_row].type_name );
/*    success = verify_tns_items( b, &tentries[tns_row] ); 
    if ( !success ) {
      free( old_type_name );
      return;
    }
*/
    new_type_name = ( char * )xv_get( p->icon_item, PANEL_VALUE ); 

   /* If new_type_name is null, this means the screen fields for
      icon properties  are not updated hence neglect it.
   */
    if( *new_type_name == NULL) {
    	new_type_name = ( char * )tentries[tns_row].type_name;
    }

#ifdef NEVER
    new_type_name = ( char * )tentries[tns_row].type_name;
#endif
    /*
     * If the icon label has changed, change all file
     * entry pointers to this new name.
     */
    if ( strcmp( old_type_name, new_type_name ) != 0 ) {
      for ( i = 0; i < nfns; i++ ) {
        if ( strcmp( fentries[i].type_name, old_type_name ) == 0 ) {
          fentries[i].type_name = strdup( new_type_name );   
          success = modify_fns_attribute( b, &fentries[i], "FNS_TYPE", "refto-Types", fentries[i].type_name, b->type_name );
          if ( !success )
            return;
        }
      }
      for ( i = 0; i < nops; i++ ) {
        if ( strcmp( ops[i].f.type_name, old_type_name ) == 0 )
          ops[i].f.type_name = strdup( new_type_name );   

      }
    }

    /*
     * Clear the props flags for a successful apply.
     * Do the bindings props first, because 
     * apply_icon_props, recreates fns_list and wipes out ops.
     */
    if ( p->binding_props_changed ) {
      success = apply_binding_props( b ); 
      if ( !success ) return;
      p->binding_props_changed = FALSE;
    }    
    if ( p->icon_props_changed ) {
      success = apply_icon_props( b );
      if ( !success ) return;
      p->icon_props_changed = FALSE;
    }

    b->applying_changes = FALSE;

    if ( !b->binder_modified ) {
      b->binder_modified = TRUE;
      xv_set( b->frame, FRAME_LABEL, title_bar_name( b, b->binder_modified ), NULL );
    }

}

void
reset_icon_props( b )
    Binder *b;
{
    	Props     *p = ( Props * )b->properties;
    	int        i;
    	Tns_entry  t;
	
    	i = current_selection( b->tns_list );
    	if (i == EMPTY) return;
	free(tentries[i].type_name);
    	if (reset_entry.type_name != NULL)
      		tentries[i].type_name = strdup(reset_entry.type_name);
    	else
      		tentries[i].type_name = NULL;
	free(tentries[i].icon_file);
    	if (reset_entry.icon_file != NULL)
   	   	tentries[i].icon_file = strdup(reset_entry.icon_file);
    	else
    	  	tentries[i].icon_file = NULL;
	free(tentries[i].icon_mask_file);
    	if (reset_entry.icon_mask_file != NULL)
      		tentries[i].icon_mask_file = strdup(reset_entry.icon_mask_file);
    	else
      		tentries[i].icon_mask_file = NULL;
	free(tentries[i].fg_color);
    	if (reset_entry.fg_color != NULL)
      		tentries[i].fg_color = strdup(reset_entry.fg_color);
    	else
      		tentries[i].fg_color = NULL;
	free(tentries[i].bg_color);
    	if (reset_entry.bg_color != NULL)
      		tentries[i].bg_color = strdup(reset_entry.bg_color);
    	else 
      		tentries[i].bg_color = NULL;
	free(tentries[i].open_method);
    	if (reset_entry.open_method != NULL)
      		tentries[i].open_method = strdup(reset_entry.open_method);
    	else
      		tentries[i].open_method = NULL;
	free(tentries[i].print_method);
    	if (reset_entry.print_method != NULL)
      		tentries[i].print_method = strdup(reset_entry.print_method);
    	else
      		tentries[i].print_method = NULL;
	tentries[i].icon_image = reset_entry.icon_image;
	
    	t = tentries[i];
/*    
    xv_set( p->panels[ICON_PANEL_L], XV_SHOW, TRUE, NULL );
    xv_set( p->panels[ICON_PANEL_R], XV_SHOW, TRUE, NULL );
*/
    if ( t.type_name )
      xv_set( p->icon_item,  PANEL_VALUE, t.type_name, NULL );
    else
      xv_set( p->icon_item,  PANEL_VALUE, "", NULL );
    if ( t.icon_file )
      xv_set( p->image_file_item, PANEL_VALUE, t.icon_file, NULL );
    else
      xv_set( p->image_file_item, PANEL_VALUE, "", NULL );
    if ( t.icon_mask_file )
      xv_set( p->mask_file_item, PANEL_VALUE, t.icon_mask_file, NULL );
    else
      xv_set( p->mask_file_item, PANEL_VALUE, "", NULL );
    if ( t.fg_color )
      xv_set( p->fg_color_item, PANEL_VALUE, t.fg_color, NULL );
    else
      xv_set( p->fg_color_item, PANEL_VALUE, "", NULL );
    if ( t.bg_color )
      xv_set( p->bg_color_item, PANEL_VALUE, t.bg_color, NULL );
    else
      xv_set( p->bg_color_item, PANEL_VALUE, "", NULL );
    if ( t.open_method )
      xv_set( p->open_item, PANEL_VALUE, t.open_method, NULL );
    else
      xv_set( p->open_item, PANEL_VALUE, "", NULL );
    if ( t.print_method )
      xv_set( p->print_item, PANEL_VALUE, t.print_method, NULL );
    else
      xv_set( p->print_item, PANEL_VALUE, "", NULL );
    
    if ( b->color && b->tt_running ) {
      xv_set( p->fg_color_button, PANEL_INACTIVE, FALSE, NULL );
      xv_set( p->bg_color_button, PANEL_INACTIVE, FALSE, NULL );
    }

    change_color_chips( b );
    if ( t.icon_image )
      xv_set( p->icon_preview, PANEL_LABEL_IMAGE, t.icon_image, NULL ); 

}

/*
 *
 */
static void
reset_binding_props( b )
    Binder  *b;
{
    Props *p = ( Props * )b->properties;

    nops = 0;
    list_flush( p->fns_list );
    create_fns_list( b );
    undisplay_fns_data( b );
}

/*
 * Notify callback function for `reset_button'.
 */
/*ARGSUSED*/
void
reset_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;

    p = ( Props * )b->properties;
    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    xv_set( p->frame, FRAME_RIGHT_FOOTER, "", NULL );

    if ( p->icon_props_changed ) {
      p->icon_props_changed = FALSE;
      reset_icon_props( b );
    }
    if ( p->binding_props_changed ) {
      p->binding_props_changed = FALSE;
      reset_binding_props( b );
    }
    if ( p->current_view == BINDING_PROPS ) {
      xv_set( p->panels[BINDING_PANEL_L], XV_SHOW, TRUE, NULL );
      if ( p->full_size )
        xv_set( p->panels[BINDING_PANEL_R], XV_SHOW, TRUE, NULL );
    }
 
}

/*
 * Notify callback function for `delete_button'.
 */
/*ARGSUSED*/
void
props_delete_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      j;
    int      fns_row;

    p = ( Props * )b->properties;

   /*
    * Delete from CE database.
    * Delete the fns entry from the panel list.
    * Delete from ops and fns list.
    */
    fns_row = current_selection( p->fns_list );
    if ( fns_row == -1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, "Nothing Selected", NULL );
    }
    else if ( fns_row >= 0 && fns_row < nfiles ) {
      xv_set( p->fns_list, PANEL_LIST_DELETE, fns_row, NULL );
      ops[nops].delete = TRUE;
      ops[nops].f = filelist[fns_row];
      nops++;

      nfiles--;
      for ( j = fns_row; j < nfiles; j++ ) 
        filelist[j] = filelist[j+1];

    }
    else
      fprintf( stderr, MGET("delete_button: Invalid FNS entry\n" ) );

    undisplay_fns_data( b );
    p->binding_props_changed = TRUE;

}

/*
 * Display or erase the content items on the panel.
 */
static void 
set_tag_items( b, displaymode )
    Binder    *b;
    int        displaymode;
{
    Props  *p = ( Props * )b->properties;

    xv_set( p->offset_item, PANEL_INACTIVE, !displaymode, NULL ); 
    xv_set( p->type_item, PANEL_INACTIVE, !displaymode, NULL ); 
    xv_set( p->value_item, PANEL_INACTIVE, !displaymode, NULL ); 
    xv_set( p->mask_item, PANEL_INACTIVE, !displaymode, NULL ); 
  
}

/*
 * Notify callback function for `choice_item'.
 */
/*ARGSUSED*/
void
identify_notify( item, value, event )
    Panel_item   item;
    int          value;
    Event       *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;
    /*
     * Activate/Inactivate tag items.
     */
    if ( value == BY_CONTENT ) {
      undisplay_fns_data( b );
      xv_set( p->identify_item, PANEL_VALUE, BY_CONTENT, NULL );
      set_tag_items( b, TRUE );
      xv_set( p->pattern_item, PANEL_INACTIVE, TRUE, NULL );
    }
    else if ( value == BY_NAME ) {
      display_fns_data( b );
      xv_set( p->identify_item, PANEL_VALUE, BY_NAME, NULL );
      set_tag_items( b, FALSE );
      xv_set( p->pattern_item, PANEL_INACTIVE, FALSE, NULL );
    }

    p->icon_props_changed = TRUE;

}

/*
 *
 */
/*ARGSUSED*/
void
tag_type_notify( item, value, event )
    Panel_item   item;
    int          value;
    Event       *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;
    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    p->binding_props_changed = TRUE;

}

/*
 * Notify callback function for `plus_button'.
 */
/*ARGSUSED*/
void
plus_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;
    p->full_size = TRUE;

    xv_set( p->panel, XV_WIDTH, 2 * p->widest, NULL );
    window_fit_width( p->frame );

    switch ( p->current_view ) {
      case ICON_PROPS:
        xv_set( p->panels[ICON_PANEL_R], XV_SHOW, TRUE, NULL );
        xv_set( p->icon_plus_button, PANEL_INACTIVE, TRUE, NULL );
        xv_set( p->icon_minus_button, PANEL_INACTIVE, FALSE, NULL );
        break;
      case BINDING_PROPS:
        xv_set( p->panels[BINDING_PANEL_R], XV_SHOW, TRUE, NULL );
        xv_set( p->binding_plus_button, PANEL_INACTIVE, TRUE, NULL );
        xv_set( p->binding_minus_button, PANEL_INACTIVE, FALSE, NULL );
        break;
      case PROGRAM_PROPS:
        xv_set( p->panels[PROGRAM_PANEL_R], XV_SHOW, TRUE, NULL );
        xv_set( p->program_plus_button, PANEL_INACTIVE, TRUE, NULL );
        xv_set( p->program_minus_button, PANEL_INACTIVE, FALSE, NULL );
        break;
      default:
        break;
    }


}

/*
 * Notify callback function for `minus_button'.
 */
/*ARGSUSED*/
void
minus_button( item, event )
    Panel_item 	item;
    Event	*event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;
    p->full_size = FALSE;

    /*
     * This left panels need to be set back to original size because
     * when the base panel was widened these panels were widened also.
     */
    xv_set( p->panels[ICON_PANEL_L], XV_WIDTH, p->widest, NULL );
    xv_set( p->panels[BINDING_PANEL_L], XV_WIDTH, p->widest, NULL );
    xv_set( p->panels[PROGRAM_PANEL_L], XV_WIDTH, p->widest, NULL );

    xv_set( p->panel, XV_WIDTH, p->widest, NULL );
    xv_set( p->panels[PROGRAM_PANEL_R], XV_SHOW, FALSE, NULL );
    xv_set( p->panels[BINDING_PANEL_R], XV_SHOW, FALSE, NULL );
    xv_set( p->panels[ICON_PANEL_R], XV_SHOW, FALSE, NULL );

    switch ( p->current_view ) {
      case ICON_PROPS:
        xv_set( p->icon_plus_button, PANEL_INACTIVE, FALSE, NULL );
        xv_set( p->icon_minus_button, PANEL_INACTIVE, TRUE, NULL );
        break;
      case BINDING_PROPS:
        xv_set( p->binding_plus_button, PANEL_INACTIVE, FALSE, NULL );
        xv_set( p->binding_minus_button, PANEL_INACTIVE, TRUE, NULL );
        break;
      case PROGRAM_PROPS:
        xv_set( p->program_plus_button, PANEL_INACTIVE, FALSE, NULL );
        xv_set( p->program_minus_button, PANEL_INACTIVE, TRUE, NULL );
        break;
      default:
        break;
    }
    window_fit_width( p->frame );

}

/*
 * Notify callback function for
 */
Notify_value
props_done_proc( frame )
    Frame	frame;
{
    Binder *b = ( Binder * )xv_get( frame, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     tns_row;
    int     notice_value;
    char    msg[80];

    p = ( Props * )b->properties;

    if ( !p->icon_props_changed && !p->binding_props_changed ) {
      xv_set( p->frame, XV_SHOW, FALSE, NULL );
      return( NOTIFY_DONE );
    }
    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      xv_set( p->frame, XV_SHOW, FALSE, NULL );
      return( NOTIFY_DONE );
    }
    sprintf( msg, MGET( "changes to the %s icon." ), tentries[tns_row].type_name );
    notice_value = notice_prompt( p->frame, NULL,
		     NOTICE_MESSAGE_STRINGS,
		       MGET( "You have not applied your" ),
                       msg,
                       MGET( "You can:" ),
                       NULL,
		     NOTICE_BUTTON_YES, MGET( "Apply" ),
		     NOTICE_BUTTON_NO,  MGET( "Reset" ),
                     NOTICE_BUTTON,     MGET( "Cancel" ), 2,
		     NULL );
    switch( notice_value ) {
    case NOTICE_YES:
      apply_button( p->icon_apply_button, NULL );
      xv_set( p->frame, XV_SHOW, FALSE, NULL );
      break;
    case NOTICE_NO:
      reset_button( p->icon_apply_button, NULL );
      xv_set( p->frame, FRAME_CMD_PUSHPIN_IN, TRUE,
	     XV_SHOW, TRUE, NULL );
      break;
    default:
      xv_set( p->frame, FRAME_CMD_PUSHPIN_IN, TRUE,
	                XV_SHOW, TRUE, NULL );
      break;
    }
    
    return( NOTIFY_DONE );
}

/*
 * Notify callback function for changing other icon props items.
 */
/*ARGSUSED*/
Panel_setting
icon_props_notify( item, event )
     Panel_item  item;
     Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;
  
    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      break;
    default:
      p->icon_props_changed = TRUE;
      break;
    }

    return( panel_text_notify( item, event ) );

}

/*
 * Notify callback function for changing other binding props items.
 */
/*ARGSUSED*/
Panel_setting
binding_props_notify( item, event )
    Panel_item  item;
    Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;

    p = ( Props * )b->properties;

    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->binding_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      break;
    default:
      p->binding_props_changed = TRUE;
      break;
    }

    return( panel_text_notify( item, event ) );

}


/*
 * Notify callback function for changing other binding props items.
 */
/*ARGSUSED*/
Panel_setting
icon_label_notify( item, event )
    Panel_item  item;
    Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    char   *icon;
    int     notice_value;
    int     tns_row;
    Event   ie;

    p = ( Props * )b->properties;
 
    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      icon = ( char * )xv_get( p->icon_item, PANEL_VALUE );

      /*
       * There must be an icon label.
       */
      if ( *icon == NULL ) {
	window_bell ( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Enter an Icon Label" ), NULL );
	return( PANEL_NONE );
      }
      else if ( type_name_exists( b, icon ) ) {
        window_bell( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Icon Label Already Exists" ), NULL );
	return( PANEL_NONE );
      }
      
      tns_row = current_selection( b->tns_list );
      if ( tns_row == -1 )
	return( panel_text_notify( item, event ) );
      
      /*
       * If this is unnamed, no need to warn them.
       */
      if ( strncmp( tentries[tns_row].type_name, UNNAMED, 7 ) == 0 ) {
	p->icon_props_changed = TRUE;
	return( panel_text_notify( item, event ) );
      }
      
      /*
       * Alert user about changing the type name.
       */
      notice_value = notice_prompt( p->panel, &ie,
		   NOTICE_MESSAGE_STRINGS,
		   MGET( "The Icon Label you are about to change may be\nused to bind this icon to other file types.\nIf you want any of these file types to continue\nto use this icon, you will need to add them." ),
		   NULL,
		   NOTICE_BUTTON_YES, MGET( "Continue" ),
		   NOTICE_BUTTON_NO,  MGET( "Cancel" ),
		   NULL );
      
      switch( notice_value ) {
      case NOTICE_YES:
	p->icon_props_changed = TRUE;
	break;
      case NOTICE_NO:
	tns_row = current_selection( b->tns_list );
	if ( tns_row != -1 )
	  xv_set( p->icon_item, PANEL_VALUE, tentries[tns_row].type_name, NULL );
	break;
      default:
	break;
      }
      break;
 
    default:
      p->icon_props_changed = TRUE;
      break;
    }

    return( panel_text_notify( item, event ) );
}

/*
 * Notify callback function for changing other binding props items.
 */
/*ARGSUSED*/
Panel_setting
application_props_notify( item, event )
    Panel_item  item;
    Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    char   *app;
    int     notice_value;
    int     tns_row;
    Event   ie;

    p = ( Props * )b->properties;
 
    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      app = ( char * )xv_get( p->open_item, PANEL_VALUE );

      tns_row = current_selection( b->tns_list );
      if ( tns_row == -1 )
	return( panel_text_notify( item, event ) );
      
      /*
       * Alert user about changing the open method.
       */
      notice_value = notice_prompt( p->panel, &ie,
		   NOTICE_MESSAGE_STRINGS,
		   MGET( "Applications do not use this field when a\nToolTalk method of launching them is available.\nSee the DeskSet Integration Guide for specifying\nToolTalk launching methods." ),
		   NULL,
		   NOTICE_BUTTON_YES, MGET( "Continue" ),
		   NOTICE_BUTTON_NO,  MGET( "Cancel" ),
		   NULL );
      
      switch( notice_value ) {
      case NOTICE_YES:
	p->icon_props_changed = TRUE;
	break;
      case NOTICE_NO:
	tns_row = current_selection( b->tns_list );
	if ( tns_row != -1 )
	  xv_set( p->open_item, PANEL_VALUE, tentries[tns_row].open_method, NULL );
	break;
      default:
	break;
      }
      break;
 
    default:
      p->icon_props_changed = TRUE;
      break;
    }

    return( panel_text_notify( item, event ) );
}

/*
 * Notify callback function for changing props items.
 */
/*ARGSUSED*/
Panel_setting
image_file_notify( item, event )
     Panel_item  item;
     Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    char   *image;
    char   fullpath[ MAXPATHLEN ], msg[ MAXPATHLEN ];
    Server_image stipple;
    int          width, height;
    struct stat statbuf;

    p = ( Props * )b->properties;

    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      image = ( char * )xv_get( p->image_file_item, PANEL_VALUE );
      /*
       * There must be an icon image file.
       */
      if ( *image == NULL ) {
	window_bell ( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Enter an Image File" ), NULL );
	return( PANEL_NONE );
      }
      
      /*
       * The icon file must exist on disk.
       */
      ds_expand_pathname( image, fullpath );
      if ( access( fullpath, F_OK ) != 0 ) { 
	window_bell ( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Image File not Found" ), NULL );
	return( PANEL_NONE );
      }
      if ( stat( fullpath, &statbuf ) == - 1 ) {
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Image File" ), NULL );
        return( 0 );
      }
      if ( !( S_ISREG( statbuf.st_mode ) ) ) {
        window_bell( p->frame );
        xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Image File" ), NULL );
        return( 0 );
      }
      
      /*
       * Ensure the icon file has valid contents.
       */
      stipple = ( Server_image )icon_load_svrim( fullpath, msg );
      if ( !stipple ) {
	window_bell( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Bad Image File" ), NULL );
	return( PANEL_NONE );
      }
      
      /*
       * Verify the size of the icon.
       */
      width  = ( int )xv_get( stipple, XV_WIDTH );
      height = ( int )xv_get( stipple, XV_HEIGHT );
      if ( width > MAX_ICON_HEIGHT || height > MAX_ICON_HEIGHT ) {
	window_bell( p->frame );
	xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Icon too Large" ), NULL );
	if ( stipple ) 
	  xv_destroy_safe( stipple );
	return( PANEL_NONE );
      }  
      
      if ( stipple ) 
	xv_destroy_safe( stipple );

      break;

    default:
      p->icon_props_changed = TRUE;
      break;
      
    } /* end switch */

    return( panel_text_notify( item, event ) );

}

/*
 * Notify callback function for changing props items.
 */
/*ARGSUSED*/
Panel_setting
mask_file_notify( item, event )
     Panel_item  item;
     Event      *event;
{
    Binder       *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props        *p;
    char         *mask;
    char          fullpath[ MAXPATHLEN ], msg[ MAXPATHLEN ];
    Server_image  icon_mask;
    struct stat   statbuf;
 
    p = ( Props * )b->properties;

    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      mask  = ( char * )xv_get( p->mask_file_item, PANEL_VALUE );
      /*
       * If an icon mask file is specified, it must exist on disk.
       */ 
      if ( *mask != NULL ) {
	ds_expand_pathname( mask, fullpath );
	if ( access( fullpath, F_OK ) != 0 ) {   
	  window_bell( p->frame );
	  xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Mask File not Found" ), NULL );
	  return( PANEL_NONE );
	}
        if ( stat( fullpath, &statbuf ) == - 1 ) {
          window_bell( p->frame );
          xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Mask File" ), NULL );
          return( 0 );
        }
        if ( !( S_ISREG( statbuf.st_mode ) ) ) {
          window_bell( p->frame );
          xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Mask File" ), NULL );
          return( 0 );
        }
	icon_mask = ( Server_image )icon_load_svrim( fullpath, msg );
	if ( !icon_mask ) {
	  window_bell( p->frame );
	  xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Bad Mask File" ), NULL );
	  return( PANEL_NONE );
	}
        if ( icon_mask )
	  xv_destroy_safe( icon_mask );
      }
      
      break;
      
    default:
      p->icon_props_changed = TRUE;
      break;
    }

    return( panel_text_notify( item, event ) );

}

/*
 * Notify callback function for changing props items.
 */
/*ARGSUSED*/
Panel_setting
fg_color_notify( item, event )
    Panel_item  item;
    Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    char   *text;
    int     red, green, blue;
    int     ncolors;
    Xv_singlecolor color;

    p = ( Props * )b->properties;

    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      text = ( char * )xv_get( p->fg_color_item, PANEL_VALUE );

      if ( *text != NULL ) {
	ncolors =  sscanf( text, "%d %d %d", &red, &green, &blue );
	if ( ncolors != 3 ) {
	  window_bell( p->frame );
	  xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Foreground Color" ), NULL );
	  return( PANEL_NONE );
	}
	color.red = ( u_char )red;
	color.green = ( u_char )green;
	color.blue = ( u_char )blue;
      }
      else {
	color.red = ( u_char )0;
	color.green = ( u_char )0;
	color.red = ( u_char )0;
      }
      
      refresh_icon_color( b, &color, FALSE );
      break;

    default:
      p->icon_props_changed = TRUE;
      break;
    }

    return( panel_text_notify( item, event ) );

}


/*
 * Notify callback function for changing props items.
 */
/*ARGSUSED*/
Panel_setting
bg_color_notify( item, event )
     Panel_item  item;
     Event      *event;
{
    Binder *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props  *p;
    int     red, green, blue;
    int     ncolors;
    char   *text;
    Xv_singlecolor color;

    p = ( Props * )b->properties;

    if ( event_is_up( event ) ) {
      switch event_action(event) {
        case ACTION_PROPS:
        case ACTION_UNDO:
        case ACTION_CUT:
        case ACTION_PASTE:
	  p->icon_props_changed = TRUE;
          break;
        default:
	  break;
      }       
      return( panel_text_notify( item, event ) );
    }

    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    switch ( event_action( event ) ) {
    case '\n':
    case '\t':
    case '\r':
      text = ( char * )xv_get( p->bg_color_item, PANEL_VALUE );

      if ( *text != NULL ) {
	ncolors = sscanf( text, "%d %d %d", &red, &green, &blue );
	if ( ncolors != 3 ) {
	  window_bell( p->frame );
	  xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Invalid Background Color" ), NULL );
	  return( PANEL_NONE );
	}
	color.red = ( u_char )red;
	color.green = ( u_char )green;
	color.blue = ( u_char )blue;
      }
      else {
	color.red = 255;
	color.green = 255;
	color.blue = 255;
      }
      
      refresh_icon_color( b, &color, TRUE );
      break;

    default:
      p->icon_props_changed = TRUE;
      break;
    }
      
    return( panel_text_notify( item, event ) );

}

/*
 * Add a new File Entry pattern.
 */
static int
new_pattern( b )
    Binder    *b;
{
    Props     *p = ( Props * )b->properties;
    int        tns_row;
    char      *pattern;
    Fns_entry  f;

    f.entry = NULL;
    f.data_value = NULL;
    f.data_offset = NULL;
    f.data_mask = NULL;
    f.data_type = NULL;
    f.db_name = strdup( b->current_ce_db );
   /*
    * Change the type name in the FNS entry 
    * to point to currently selected TNS entry.
    */
    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return( 0 );
    }

    pattern = ( char * )xv_get( p->pattern_item, PANEL_VALUE );
    if ( pattern != NULL )
      f.filename  = strdup( pattern );
    else {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "No Pattern Specified" ), NULL );
      return( 0 );
    }

    f.type_name = strdup( tentries[tns_row].type_name );

   /*
    * Add new FNS entry to File Namespace
    * (if not a duplicate).
    */
    if ( fns_duplicate( &f ) ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Pattern Already in Use" ), NULL );
      if ( f.db_name ) free( f.db_name );
      if ( f.type_name ) free( f.type_name );
      return( 0 );
    }

   /*
    * Add to FNS scroll list.
    */
    list_add_entry( p->fns_list, f.filename, NULL, NULL, nfiles, TRUE );

   /*
    * Append to ops and fns list.
    */ 
    filelist[nfiles] = f;
    nfiles++;
    ops[nops].f = f;
    ops[nops].add = TRUE;
    nops++;

    return( 1 );

}

/*
 * Change a File Entry pattern.
 */
static int
change_pattern( b )
    Binder    *b;
{
    Props     *p = ( Props * )b->properties;
    int        tns_row;
    int        fns_row;
    char      *pattern;
    Fns_entry  f, old_f;
    int        new_entry;

   /*
    * Change the type name in the FNS entry 
    * to point to currently selected TNS entry.
    */
    tns_row = current_selection( b->tns_list );
    if ( tns_row == -1 ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Nothing Selected" ), NULL );
      return( 0 );
    }

    pattern = ( char * )xv_get( p->pattern_item, PANEL_VALUE );

    if ( pattern == NULL ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "No Pattern Specified" ), NULL );
      return( 0 );
    }

    fns_row = current_selection( p->fns_list );
    old_f = filelist[fns_row];
    f.type_name = strdup( filelist[fns_row].type_name );
    f.filename = strdup( pattern );
    f.db_name  = strdup( filelist[fns_row].db_name );
    f.data_value = NULL;
    f.data_offset = NULL;
    f.data_type = NULL;
    f.data_mask = NULL;
    /*
     * Check for duplicate.
     */
    if ( fns_duplicate( &f ) ) {
      window_bell( p->frame );
      xv_set( p->frame, FRAME_LEFT_FOOTER, MGET( "Pattern Already in Use" ), NULL );
      return( 0 );
    }
    
    /*
     * If the current CE database open matches the
     * entry just modified, then do a modify, otherwise
     * add the entry. Fix this later? Do we want to add from user to system?
     */

    if ( strcmp( filelist[fns_row].db_name, b->current_ce_db ) == 0 ) {
      f.db_name = strdup( filelist[fns_row].db_name );
      f.entry = filelist[fns_row].entry;
      new_entry = FALSE;
    }
    else {
      f.db_name = strdup( b->current_ce_db );
      f.entry = NULL;
      new_entry = TRUE;
    }

   /*
    * Change FNS scroll list.
    */
    if ( new_entry ) {
      list_add_entry( p->fns_list, f.filename, NULL, NULL, nfiles, TRUE );
      xv_set( p->fns_list, PANEL_LIST_SELECT, nfiles, TRUE, NULL );
    }
    else
      xv_set( p->fns_list, PANEL_LIST_STRING, fns_row, f.filename, NULL );

   /*
    * Place new/modified entry in the ops and fentries list.
    */ 
    if ( new_entry ) {
      filelist[nfiles] = f;
      nfiles++;
      ops[nops].f = f;
      ops[nops].add = TRUE;
      nops++;
    }
    else { /* Change - Add/Delete */

      filelist[fns_row] = f;

      ops[nops].f = f;
      ops[nops].add = TRUE;
      nops++;
      ops[nops].f = old_f;
      ops[nops].delete = TRUE;
      nops++;
      
    }

    return( 1 );

}

/*
 * Notify callback function for adding a new pattern.
 */
/*ARGSUSED*/
Notify_value
pattern_notify( item, event )
    Panel_item  item;
    Event      *event;
{
    Binder  *b = ( Binder * )xv_get( item, XV_KEY_DATA, INSTANCE );
    Props   *p;
    int      fns_row;
    int      success;

    p = ( Props * )b->properties;
    xv_set( p->frame, FRAME_LEFT_FOOTER, "", NULL );
    fns_row = current_selection( p->fns_list );
    /*
     * If nothing is selected in File Entries, add this.
     * Otherwise modify this entry.
     */
    if ( fns_row == -1 )
      success = new_pattern( b );
    else 
      success = change_pattern( b ); 

    if ( success )
      p->binding_props_changed = TRUE;

    xv_set(p->pattern_item, PANEL_VALUE, "", NULL);

    return( NOTIFY_DONE );
}
