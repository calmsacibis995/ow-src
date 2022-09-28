#ifndef lint
#ifdef sccs
static char sccsid[]="@(#)binder_ce.c	3.8 02/08/95 Copyright 1987-1990 Sun Microsystems, Inc.";
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/param.h>

#include <X11/Xos.h>
#include <xview/xview.h>
#include <xview/alert.h>
#include <xview/cms.h>
#include <xview/panel.h>
#include <ds_listbx.h>
#include <ds_colors.h>

#include "binder.h"
#include "binder_ce.h"
#include "xv.h"
#include "props.h"

extern Binder    *binder;
extern Fns_entry  fentries[ MAX_FNS_ENTRIES ];
extern Tns_entry  tentries[ MAX_TNS_ENTRIES ];
extern int	  nfns;
extern int	  ntns;
extern Tns_entry  def_entry;

char          **db_names;
char          **db_paths;
int 		num_db;

/*
 * Initialize the Classing Engine.
 */
void
b_init_ce( b )
    Binder *b;
{
    int  error;
  
    error = ce_begin( NULL );
    if ( error ) {
      fprintf( stderr,  MGET( "Error Initializing Classing Engine Database - Error no: %d.\n" ), error );
      exit( error );
    }

    /*
     * Store the CE databases that were loaded
     * and finish creating menus.
     */
    ( void )ce_get_dbs( &num_db, ( char *** )&db_names, ( char *** )&db_paths );

    /*
     * Fix later - hard coding positions in db_names.
     */
    switch ( b->scope ) {
    case USER:
      b->current_ce_db = db_names[0];
      break;
    case SYS:
      b->current_ce_db = db_names[1];
      break;
    case NET:
      b->current_ce_db = db_names[2];
      break;
    default:
      break;
    }

    /*
     * 
     */
    error = ce_test_ok_to_write( b->current_ce_db, CE_FLAG_TEST_PERMISSIONS );
    if ( error ) {
      fprintf( stderr, MGET( "Do not have permission to write to %s database - exiting.\n" ), b->current_ce_db );
      fprintf( stderr, MGET( "Error no - %d\n" ), error );
      exit( error );
    }

    /*
     * Fix this later, hard-coded to user database.
     */
    error = ce_start_write( b->current_ce_db );
    if ( error ) {
      fprintf( stderr, MGET( "Do not have permission to write to %s database - exiting.\n"), b->current_ce_db );
      fprintf( stderr, MGET( "Error no - %d\n" ), error );
      exit( error );
    }

}

/*
 * Check if the two file entries have equal contents.
 */
static int
equal_contents( f1, f2 )
    Fns_entry  *f1, *f2;
{

    if ( f1->data_offset && f2->data_offset ) {
      if ( strcmp( f1->data_offset, f2->data_offset ) != 0 )
        return( FALSE );
    }
    else
      return( FALSE );
    
    if ( f1->data_value && f2->data_value ) {
      if ( strcmp( f1->data_value, f2->data_value ) != 0 )
	return( FALSE );
    }
    else
      return( FALSE );
    
    if ( f1->data_type && f2->data_type ) {
      if ( strcmp( f1->data_type, f2->data_type ) != 0 )
	return( FALSE );
    }
    else
      return( FALSE );
    
    if ( f1->data_mask && f2->data_mask ) {
      if ( strcmp( f1->data_mask, f2->data_mask ) != 0 )
	return( FALSE );
    }
    else if ( !f2->data_mask && !f2->data_mask )
      return( TRUE );
    else
      return( FALSE );
    
    return( TRUE );

}

/*
 * Callback function for 'binder_read_tns'.
 */
int
find_entry_to_delete( fns, entry, f )
     CE_NAMESPACE     fns;
     CE_ENTRY         entry;
     Fns_entry       *f;
{
    char *filename;
    char *type_name;
    char *content;

    type_name = ( char * )ce_get_attribute( fns, entry, binder->fns_type );
    filename = ( char * )ce_get_attribute( fns, entry, binder->fns_filename );
    content = ( char * )ce_get_attribute( fns, entry, binder->fns_magic_match );

    if ( strcmp( type_name, f->type_name ) == 0 ) { 
     
      if ( ( filename ) && ( f->filename ) && strcmp( filename, f->filename ) == 0 ) {
        f->entry = entry;
        return( 1 ); /* Stop searching */
      }
      else if ( ( content ) && ( f->data_value ) && strcmp( content, f->data_value ) == 0 ) {
        f->entry = entry;
        return( 1 ); 
      }
    }
    f->entry = NULL;
    return( NULL ); /* Keep searching */

}
/*
 * Callback function for 'binder_read_tns'.
 */
static int
read_default_entry( tns, entry, ip )
     CE_NAMESPACE     tns;
     CE_ENTRY         entry;
     Binder          *ip;
{
    char *type_name;

    type_name = ( char * )ce_get_attribute( tns, entry, ip->type_name );

    if ( !type_name ) return( 0 );

    if ( strcmp( type_name, "default" ) == 0 ) {
      def_entry.icon_file      = ( char * )ce_get_attribute( tns, entry, ip->type_icon );
      def_entry.icon_mask_file = ( char * )ce_get_attribute( tns, entry, ip->type_icon_mask );
      def_entry.open_method    = ( char * )ce_get_attribute( tns, entry, ip->type_open );
      def_entry.print_method   = ( char * )ce_get_attribute( tns, entry, ip->type_print );
      def_entry.fg_color       = ( char * )ce_get_attribute( tns, entry, ip->type_fgcolor );
      def_entry.bg_color       = ( char * )ce_get_attribute( tns, entry, ip->type_bgcolor );
      return( 1 );  /* Stop searching */
    }
    return( NULL ); /* Keep searching */

}

/*
 * Determine if this entry is viewed depending
 * on scope and current view.
 */
int
display_entry( b, t )
    Binder    *b;
    Tns_entry *t;
{
    int  ans = FALSE;

    switch ( b->scope ) {
    case USER:
      if ( b->view == ALL ) 
        ans = TRUE;         
      else if ( b->view == SHARED ) {
        if ( ( strcmp( t->db_name, "system" ) == 0 ) || 
             ( strcmp( t->db_name, "network" )  == 0 ) )
          ans = TRUE;
      }
      else if ( b->view == PERSONAL ) 
        ans = ( strcmp( t->db_name, "user" ) == 0 );
      break;
    case SYS:
      if ( b->view == ALL )
        ans = TRUE; 
      else if ( b->view == SHARED )
        ans = ( strcmp( t->db_name, "network" ) == 0 );
      else if ( b->view == PERSONAL ) 
        ans = ( strcmp( t->db_name, "system" ) == 0 );
      break;
    case NET:
      ans = TRUE;
      break;
    default:
      break;
    }

    return( ans );
}

/*
 * Callback function for 'binder_read_tns'.
 */
int
read_tns_entry( tns, entry, b )
     CE_NAMESPACE   tns;
     CE_ENTRY       entry;
     Binder        *b;
{
    Tns_entry       t;
    char           *name, *path,
                    iconname[ MAXPATHLEN ],
                    errmsg[ MAXPATHLEN ];
    int		    red, green, blue;
    int             using_default = FALSE;
    int             i;
    int             found;
    Xv_Singlecolor  scolor;

    /*
     * Return non-zero value to stop.
     */
    if ( ntns >= MAX_TNS_ENTRIES ) {
      fprintf( stderr, MGET( "Maximum Type Namespace entries reached\n" ) );
      return ( 1 );
    }

    /* 
     * Place file entry in binder array.
     */
    t.entry = entry;

    /*
     * Store the database from which this entry came from.
     */
    ( void )ce_get_entry_db_info( tns, entry, &name, &path );
    t.db_name = name;

    t.type_name      = ( char * )ce_get_attribute( tns, entry, b->type_name );
    if ( !t.type_name ) 
      return( NULL );
    t.icon_file      = ( char * )ce_get_attribute( tns, entry, b->type_icon );
    t.icon_mask_file = ( char * )ce_get_attribute( tns, entry, b->type_icon_mask );
    t.open_method    = ( char * )ce_get_attribute( tns, entry, b->type_open );
    t.print_method   = ( char * )ce_get_attribute( tns, entry, b->type_print );
    t.fg_color       = ( char * )ce_get_attribute( tns, entry, b->type_fgcolor );
    t.bg_color       = ( char * )ce_get_attribute( tns, entry, b->type_bgcolor );
    
    t.icon_stipple   = NULL;
    t.icon_mask      = NULL;

    /* 
     * Read in the icon files.
     */
    if ( t.icon_file ) {
      ds_expand_pathname( t.icon_file, iconname );
      t.icon_stipple =  ( Server_image )icon_load_svrim( iconname, errmsg );
    }
    /*
     * Use the default icon if no icon specified of
     * icon was not found.
     */
    if ( !t.icon_stipple && def_entry.icon_file ) {
      ds_expand_pathname( def_entry.icon_file, iconname );
      t.icon_stipple = ( Server_image )icon_load_svrim( iconname, errmsg );
      if ( t.icon_stipple ) {
	t.icon_file = def_entry.icon_file;
	using_default = TRUE;
      }
    }

    /*
     * No icon, no entry -- return.
     */
    if ( !t.icon_stipple ) 
      return( NULL );
    
    /* 
     * If we used the default icon, then use the default icon mask 
     * if there is one.
     */
    if ( using_default && ( def_entry.icon_mask_file != NULL ) ) {
      t.icon_mask_file = def_entry.icon_mask_file;  
      ds_expand_pathname( t.icon_mask_file, iconname );
      t.icon_mask = ( Server_image )icon_load_svrim( iconname, errmsg );
    }
    else if ( t.icon_mask_file ) {
      ds_expand_pathname( t.icon_mask_file, iconname );
      t.icon_mask = ( Server_image )icon_load_svrim( iconname, errmsg );
    }
    
    /*
     * Set the Foreground/Background Color Indicies 
     */
    if ( b->color ) {

      if ( t.fg_color ) {
	if ( sscanf( t.fg_color, "%d %d %d", &red, &green, &blue ) == 3 ) {
	  scolor.red   = ( u_char )red;
	  scolor.green = ( u_char )green;
	  scolor.blue  = ( u_char )blue;
	  t.fg_index  = ds_cms_index( b->cms, &scolor );
	}  
      }
      else if ( using_default && def_entry.fg_color ) {
	if ( sscanf( def_entry.fg_color, "%d %d %d", &red, &green, &blue ) == 3 ) {
	  scolor.red   = ( u_char )red;
	  scolor.green = ( u_char )green;
	  scolor.blue  = ( u_char )blue;
	  t.fg_color   = def_entry.fg_color;
	  t.fg_index   = ds_cms_index( b->cms, &scolor );
	}  
      }
      else {
	scolor.red     = 0;
	scolor.green   = 0;
	scolor.blue    = 0;
	t.fg_index     = ds_cms_index( b->cms, &scolor );
      }
      
      if ( t.bg_color ) {  
	if ( sscanf( t.bg_color, "%d %d %d", &red, &green, &blue ) == 3 ) {
	  scolor.red   = ( u_char )red;
          scolor.green = ( u_char )green;
	  scolor.blue  = ( u_char )blue;
	  t.bg_index   = ds_cms_index( b->cms, &scolor );
	}
      }
      else if ( using_default && def_entry.bg_color ) {
	if ( sscanf( def_entry.bg_color, "%d %d %d", &red, &green, &blue ) == 3 ) {
	  scolor.red   = ( u_char )red;
	  scolor.green = ( u_char )green;
	  scolor.blue  = ( u_char )blue;
	  t.bg_color   = def_entry.bg_color;
	  t.bg_index   = ds_cms_index( b->cms, &scolor );
	}  
      }
      else {
	scolor.red     = 255;
	scolor.green   = 255;
	scolor.blue    = 255;
	t.bg_index     = ds_cms_index( b->cms, &scolor );
      }


    } /* end if color */

    /*
     * The order the data bases are read in is user, system, network
     * so add only if not already in list.
     */
    if ( !b->applying_changes ) {
      found = FALSE;
      for ( i = 0; i < ntns; i++ ) {
        if ( strcmp( t.type_name, tentries[i].type_name ) == 0 ) {
	  found = TRUE;
	  break;
        }
      }
      if ( !found && display_entry( b, &t ) ) {
        tentries[ntns] = t;  
        ntns++;
      }
    }

   /*
    * If applying changes, replace with old entry.
    */
    else {
      for ( i = 0; i < ntns; i++ ) {
	if ( tentries[i].entry == entry ) {
          free( tentries[i].type_name );
	  free( tentries[i].icon_file );
	  free( tentries[i].icon_mask_file );
	  free( tentries[i].open_method );
	  free( tentries[i].print_method );
	  free( tentries[i].fg_color );
	  free( tentries[i].bg_color );
#ifdef NEVER
	  free( ( char * )tentries[i].icon_stipple );
	  free( ( char * )tentries[i].icon_mask );
	  free( ( char * )tentries[i].icon_image );
#endif
	  tentries[i] = t;
	  break;
	}
      }
      b->applying_changes = FALSE;
    }

    return( NULL );

}

/*
 * Callback function for 'binder_read_fns'.
 */
static int
read_fns_entry( fns, entry, ip )
     CE_NAMESPACE    fns;
     CE_ENTRY        entry;
     Binder         *ip;
{
    Fns_entry        f;
    char            *name, *path;
    int              i = -1;
    int              found;
    
    /*
     * Return non-zero value to stop.
     */
    if ( nfns >= MAX_FNS_ENTRIES ) {
      fprintf( stderr, MGET( "Maximum File Namespace entries reached.\n" ) );
      return( 1 );
    }

    /* 
     * Store the entry handle to the fns.
     */
    f.entry = entry;
    
    /*
     * Place the database name from which this entry came from.
     */
    ( void )ce_get_entry_db_info( fns, entry, &name, &path );
    f.db_name = name;
  
    /*
     * Store the rest of the attributes.
     */
    f.type_name   = ( char * )ce_get_attribute( fns, entry, ip->fns_type );
    if ( !f.type_name )
      return( NULL );
    f.filename    = ( char * )ce_get_attribute( fns, entry, ip->fns_filename );
    f.data_offset = ( char * )ce_get_attribute( fns, entry, ip->fns_magic_offset );
    f.data_value  = ( char * )ce_get_attribute( fns, entry, ip->fns_magic_match );
    f.data_type   = ( char * )ce_get_attribute( fns, entry, ip->fns_magic_type );
    f.data_mask   = ( char * )ce_get_attribute( fns, entry, ip->fns_magic_mask );
 
    /*
     * The order the data bases are read in is user, system, network
     * so add only if not already in list.
     */
    found = FALSE;
    for ( i = 0; i < nfns; i++ ) {
      if ( f.filename ) {     /* f is a filename entry */
	if ( fentries[i].filename && ( strcmp( f.filename, fentries[i].filename ) == 0 ) ) {
	  found = TRUE;
	  break;
	}
      }
      else {          /* f is a content entry */
	if ( equal_contents( &f, &fentries[i] ) ) {
	  found = TRUE;
	  break;
	}
      }
    }
    if ( !found ) {
      fentries[nfns] = f; 
      nfns++;
    }
    
    return( NULL );
 
}

/*
 * Read in Namespace Entries.
 */
void
b_init_namespaces( b )
    Binder *b;
{

    b->fns = ce_get_namespace_id( "Files" );
    if ( !b->fns ) 
      fprintf( stderr,  MGET("Cannot find File Namespace\n")  );
    
    b->tns = ce_get_namespace_id( "Types" );
    if ( !b->tns )
      fprintf( stderr,  MGET("Cannot find Type Namespace\n")  );
    
    b->fns_type         = ce_get_attribute_id( b->fns, "FNS_TYPE" );
    b->fns_filename     = ce_get_attribute_id( b->fns, "FNS_FILENAME" );
    b->fns_magic_offset = ce_get_attribute_id( b->fns, "FNS_MAGIC_OFFSET" ); 
    b->fns_magic_match  = ce_get_attribute_id( b->fns, "FNS_MAGIC_MATCH" ); 
    b->fns_magic_type   = ce_get_attribute_id( b->fns, "FNS_MAGIC_TYPE" );
    b->fns_magic_mask   = ce_get_attribute_id( b->fns, "FNS_MAGIC_MASK" );
    
    b->type_icon      = ce_get_attribute_id( b->tns, "TYPE_ICON" );
    b->type_icon_mask = ce_get_attribute_id( b->tns, "TYPE_ICON_MASK" );
    b->type_open      = ce_get_attribute_id( b->tns, "TYPE_OPEN" );
    b->type_open_tt   = ce_get_attribute_id( b->tns, "TYPE_OPEN_TT" );
    b->type_print     = ce_get_attribute_id( b->tns, "TYPE_PRINT" );
    b->type_name      = ce_get_attribute_id( b->tns, "TYPE_NAME" );
    b->type_fgcolor   = ce_get_attribute_id( b->tns, "TYPE_FGCOLOR" );
    b->type_bgcolor   = ce_get_attribute_id( b->tns, "TYPE_BGCOLOR" );

    /*
     * Store the default entry in case we need it when building the 
     * Type Namespace entries.
     */
    ce_map_through_entries( b->tns, ( CE_ENTRY_MAP_FUNCTION )read_default_entry, b );
    /*
     * Store the Type Namespace and File Namespace entries.
     */
    nfns = 0;
    ntns = 0;   
    ce_map_through_entries( b->tns, ( CE_ENTRY_MAP_FUNCTION )read_tns_entry, b );
    ce_map_through_entries( b->fns, ( CE_ENTRY_MAP_FUNCTION )read_fns_entry, b );

}

/*
 * Insert the appropriate attributes
 * into the file namespace.
 */
static int
add_fns_attributes( frame, fns, f )
    Frame          frame;
    CE_NAMESPACE   fns;
    Fns_entry     *f;
{
    int  error; 

    error = ce_add_attribute( fns, &f->entry, "FNS_TYPE", "refto-Types" ,
			      f->type_name, strlen( f->type_name ) );
    if ( error ) {
      display_ce_error( frame, error );
      return( 0 );
    }
    
    if ( f->data_value == NULL ) {
      error = ce_add_attribute( fns, &f->entry, "FNS_FILENAME", "str" ,
				f->filename, strlen( f->filename ) );
      if ( error ) {
	display_ce_error( frame, error );
	return( 0 );
      }
    }
    else {
      if ( f->data_offset != NULL ) {
	error = ce_add_attribute( fns, &f->entry, "FNS_MAGIC_OFFSET", "str" ,
				  f->data_offset, strlen( f->data_offset ) );
	if ( error ) {
	  display_ce_error( frame, error );
	  return( 0 );
	}
      }
      
      if ( f->data_value != NULL ) {
	error = ce_add_attribute( fns, &f->entry, "FNS_MAGIC_MATCH", "str" ,
				  f->data_value, strlen( f->data_value ) );
	if ( error ) {
	  display_ce_error( frame, error );
	  return( 0 );
	}
      }

      if ( f->data_type != NULL ) {
	error = ce_add_attribute( fns, &f->entry, "FNS_MAGIC_TYPE", "str" ,
				  f->data_type, strlen( f->data_type ) );
	if ( error ) {
	  display_ce_error( frame, error );
	  return( 0 );
	}
      }
      
      if ( f->data_mask != NULL ) {
	error = ce_add_attribute( fns, &f->entry, "FNS_MAGIC_MASK", "str" ,
				  f->data_mask, strlen( f->data_mask ) );
	if ( error ) {
	  display_ce_error( frame, error );
	  return( 0 );
	}
	
      }
      
    }

    return( 1 );

}

/*
 * Insert the appropriate attributes 
 * into the Type Namespace.
 */
static int
add_tns_attributes( frame, tns, t )
     Frame          frame;
     CE_NAMESPACE   tns;
     Tns_entry     *t;
{
    int       error;

    error = ce_add_attribute( tns, &t->entry, "TYPE_NAME", "type_id",
			      strdup( t->type_name ), strlen( t->type_name ) );
    if ( error ) {
      display_ce_error( frame, error );
      return( 0 );
    }

    if ( t->open_method != NULL ) {
      error = ce_add_attribute( tns, &t->entry, "TYPE_OPEN", "call", 
			        strdup( t->open_method ), strlen( t->open_method ) );
      if ( error ) {
	display_ce_error( frame, error );
        return( 0 );
      }
    }
    
    if ( t->icon_file != NULL ) {
      error = ce_add_attribute( tns, &t->entry, "TYPE_ICON", "icon-file", 
			        strdup( t->icon_file ), strlen( t->icon_file ) );
      if ( error ) {
	display_ce_error( frame, error );
	return( 0 );
      }
    }
    
    if ( t->icon_mask_file != NULL ) { 
      error = ce_add_attribute( tns, &t->entry, "TYPE_ICON_MASK", "icon-file", 
			        strdup( t->icon_mask_file ), strlen( t->icon_mask_file ) );
      if ( error ) {
	display_ce_error( frame, error );
	return( 0 );
      }
    } 
    else {
      error = ce_add_attribute( tns, &t->entry, "TYPE_ICON_MASK", "icon-file", 
			        strdup( "" ), 0 );
      if ( error ) {
	display_ce_error( frame, error );
	return( 0 );
      }
    }

    if ( t->bg_color != NULL) {
      error = ce_add_attribute( tns, &t->entry, "TYPE_BGCOLOR", "color",
				strdup( t->bg_color ), strlen( t->bg_color ) );
      if ( error ) {
	display_ce_error( frame, error );
	return( 0 );
      }
    }

    if ( t->fg_color != NULL ) {
      error = ce_add_attribute( tns, &t->entry, "TYPE_FGCOLOR", "color",
				strdup( t->fg_color ), strlen( t->fg_color ) );
      if ( error ) {
	display_ce_error( frame, error );
	return( 0 );
      }
    }

    if ( t->print_method != NULL ) {
      error = ce_add_attribute( tns, &t->entry, "TYPE_PRINT", "string",
				strdup( t->print_method ), strlen( t->print_method ) );
      if ( error ) {
	display_ce_error( frame, error );
        return( 0 );
      }
    }

    return( 1 );
  
}

/*
 * Add the fns entry to the CE.
 */
int
add_fns_entry( b, f )
    Binder     *b;
    Fns_entry  *f;
{
    Props   *p = ( Props * )b->properties;
    int      error, success;

    f->entry = NULL;
    error = ce_alloc_entry( b->fns, &f->entry ); 

    if ( error == CE_ERR_NAMESPACE_DOES_NOT_EXIST ) {
      error = ce_clone_namespace( &b->fns );
      if ( error ) {
        display_ce_error( b->frame, error );
        return( 0 );
      }
      error = ce_alloc_entry( b->fns, &f->entry ); 
      if ( error ) {
        display_ce_error( b->frame, error );
        return( 0 );
      }
    }
    else if ( error != 0 ) {
      display_ce_error( b->frame, error ); 
      return( 0 );
    }

    success = add_fns_attributes( p->frame, b->fns, f );
    if ( !success ) return( 0 );

    error = ce_add_entry( b->fns, f->entry );
    if ( error ) {
      display_ce_error( b->frame, error );
      return( 0 );
    }

    return( 1 );
}

/*
 * Add the TNS entry to the CE.
 */
int
add_tns_entry( b, t )
    Binder     *b;
    Tns_entry  *t;
{
    CE_ENTRY  entry;
    Props    *p = ( Props * )b->properties;
    int       error, success;

    t->entry = NULL;
    error = ce_alloc_entry( b->tns, &t->entry ); 

    if ( error == CE_ERR_NAMESPACE_DOES_NOT_EXIST ) {
      error = ce_clone_namespace( &b->tns );
      if ( error ) {
        display_ce_error( b->frame, error );
        return( 0 );
      }
      error = ce_alloc_entry( b->tns, &t->entry ); 
      if ( error ) {
        display_ce_error( b->frame, error );
        return( 0 );
      }
    }
    else if ( error != 0 ) {
      display_ce_error( b->frame, error ); 
      return( 0 );
    }

    error = ce_add_entry( b->tns, t->entry );
    if ( error ) {
      display_ce_error( b->frame, error );
      return( 0 );
    }

   /*
    * First try to get the entry to see if it exists at a higher level.
    */
    entry = ce_get_entry( b->tns, 1, t->type_name );
    if ( entry ) {
      t->entry = entry;
      success = modify_tns_entry( b, t );
    }
    else
      success = add_tns_attributes( p->frame, b->tns, t );

    if ( !success ) return( 0 );


    return( 1 );
}


int
modify_fns_attribute( b, f, name, type, value, attr )
    Binder       *b;
    Fns_entry    *f;
    char         *name;
    char         *type;
    char         *value;
    CE_ATTRIBUTE  attr;
{
    int   error;
    char *attr_value;

    if ( value != NULL ) {
      error = ce_add_attribute( b->fns, &f->entry, name, type, strdup( value ), strlen( value ) );
      if ( error == CE_ERR_ATTRIBUTE_EXISTS )
	error = ce_modify_attribute( b->fns, &f->entry, name, type, strdup( value ), strlen( value ) );
    }
    else {
      attr_value = ce_get_attribute( b->fns, f->entry, attr );
      if ( attr_value )
        error = ce_remove_attribute( b->fns, f->entry, attr );
    }
    
    if ( error ) { 
      display_ce_error( b->frame, error );
      return( 0 );
    }

    return ( 1 );

}

static int
modify_tns_attribute( b, t, name, type, value, attr )
    Binder       *b;
    Tns_entry    *t;
    char         *name;
    char         *type;
    char         *value;
    CE_ATTRIBUTE  attr;
{
    int    error = 0;
    char  *attr_value;

    if ( value != NULL ) {
      error = ce_add_attribute( b->tns, &t->entry, name, type, strdup( value ), strlen( value ) );
      if ( error == CE_ERR_ATTRIBUTE_EXISTS )
	error = ce_modify_attribute( b->tns, &t->entry, name, type, strdup( value ), strlen( value ) );
    }

    else if ( strcmp( name, "TYPE_ICON_MASK" ) == 0 ) {
      error = ce_add_attribute( b->tns, &t->entry, name, type, strdup(""), 0 );
      if ( error == CE_ERR_ATTRIBUTE_EXISTS )
	error = ce_modify_attribute( b->tns, &t->entry, name, type, strdup(""), 0 );
    }

    else {
      attr_value = ce_get_attribute( b->tns, t->entry, attr );
      if ( attr_value ) 
        error = ce_remove_attribute( b->tns, t->entry, attr );
    }
    
    if ( error ) { 
      display_ce_error( b->frame, error );
#ifdef DEBUG
      fprintf( stderr, "name = %s, value = %s\n", name, value );
#endif
      return( 0 );
    }

    return ( 1 );

}

/*
 * Insert the appropriate attributes
 * into the file namespace.
 * Returns 1 on success,
 *         0 on failure.
 */
int
modify_fns_entry( b, f )
    Binder      *b;
    Fns_entry   *f;
{
    int   success;

    /* Type Name */
    success = modify_fns_attribute( b, f, "FNS_TYPE", "refto-Types", f->type_name, b->type_name );
    if ( !success )
      return( 0 );

    /* Filename */ 
    success = modify_fns_attribute( b, f, "FNS_FILENAME", "str", f->filename, b->fns_filename );
    if ( !success )
      return( 0 );

    /* FNS Magic Match  */
    success = modify_fns_attribute( b, f, "FNS_MAGIC_MATCH", "str", f->data_value, b->fns_magic_match );
    if ( !success )
      return( 0 );

    /* FNS Magic Offset */
    success = modify_fns_attribute( b, f, "FNS_MAGIC_OFFSET", "str", f->data_offset, b->fns_magic_offset );
    if ( !success )
      return( 0 );

    /* FNS Magic Type */
    success = modify_fns_attribute( b, f, "FNS_MAGIC_TYPE", "str", f->data_type, b->fns_magic_type );
    if ( !success )
      return( 0 );

    /* FNS Magic Mask */
    success = modify_fns_attribute( b, f, "FNS_MAGIC_MASK", "str", f->data_mask, b->fns_magic_mask );
    if ( !success )
      return( 0 );

    return( 1 );

}

/*
 * Insert the appropriate attributes
 * into the file namespace.
 * Returns 1 on success,
 *         0 on failure.
 */
int
modify_tns_entry( b, t )
    Binder      *b;
    Tns_entry   *t;
{
    extern int current_selection();
    int   success;
    int   tns_row;
    char *value;

    /*
     * Modify this only if the type name was changed, otherwise use chaining.
     */
    tns_row = current_selection( b->tns_list );
    if (tns_row != EMPTY) {
      value = ( char * )xv_get( b->tns_list, PANEL_LIST_STRING, tns_row );
      if ( strcmp( t->type_name, value ) != 0 ) {
        success = modify_tns_attribute( b, t, "TYPE_NAME", "type-id", t->type_name, b->type_name );
        if ( !success )
          return( 0 );
      }
    }

    success = modify_tns_attribute( b, t, "TYPE_OPEN", "call", t->open_method, b->type_open );
    if ( !success )
      return( 0 );
    success = modify_tns_attribute( b, t, "TYPE_ICON", "icon-file", t->icon_file, b->type_icon );
    if ( !success )
      return( 0 );
    success = modify_tns_attribute( b, t, "TYPE_ICON_MASK", "icon-file", t->icon_mask_file, b->type_icon_mask );
    if ( !success )
      return( 0 );
    success = modify_tns_attribute( b, t, "TYPE_BGCOLOR", "color", t->bg_color, b->type_bgcolor );
    if ( !success )
      return( 0 );
    success = modify_tns_attribute( b, t, "TYPE_FGCOLOR", "color", t->fg_color, b->type_fgcolor );
    if ( !success )
      return( 0 );
    success = modify_tns_attribute( b, t, "TYPE_PRINT", "string", t->print_method, b->type_print );
    if ( !success )
      return( 0 );

    return( 1 );

}


/*
 * 1 read-only
 * 0 read-write 
 */
int
read_only_entry( db_name )
    char *db_name;
{

    if ( ( db_name ) && ( ce_test_ok_to_write( db_name, CE_FLAG_TEST_PERMISSIONS ) == 0 ) )
      return( 0 );    /* Read-Write */
    else
      return( 1 );    /* Read-Only */

}

/*
 * Display a warning message about the Classing Engine
 */
/*ARGSUSED*/
void
display_ce_error( frame, error )
    Frame  frame;
    int    error;
{

    switch ( error ) {
    case CE_ERR_DB_LOCKED:
    case CE_ERR_WRITE_IN_PROGRESS:
      fprintf( stderr, MGET("Cannot update database file.\nDatabase is locked.\n") );
      break;
    case CE_ERR_NO_PERMISSION_TO_WRITE:
      fprintf( stderr, MGET("Permission to overwrite database has been denied.\n") );
      break;
    case CE_ERR_INTERNAL_ERROR:
      fprintf( stderr, MGET("Error in updating database.\n") );
      break;
    case CE_ERR_WRITE_NOT_STARTED:
      fprintf( stderr, MGET("ce_start_write call not made before writing\n") );
      break;
    default:
      fprintf( stderr, MGET("Error in Classing Engine Database\n") );
      break;
    }
}
