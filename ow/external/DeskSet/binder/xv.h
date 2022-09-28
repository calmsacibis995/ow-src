/*
 *  @(#)xv.h	1.6 12/15/93
 *
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

#ifndef	binder_HEADER
#define	binder_HEADER

#include <xview/cms.h>
#include <desktop/ce.h>
#include <desktop/ce_err.h>

#define MAX_ICON_HEIGHT      32
#define MAX_ICON_WIDTH       32
#define MGET(s)     (char *)gettext(s)
#define MSGFILE   "SUNW_DESKSET_BINDER"

#define MARGIN		      5
#define LIST_ENTRY_HEIGHT   MAX_ICON_HEIGHT + MARGIN
#define DB_SOURCE_KEY       888


/*
 * binder_ui.h - User interface object and function declarations.
 */

extern Attr_attribute	INSTANCE;
extern Server_image     fg_chip, bg_chip;

typedef enum {
        USER = 0,
        SYS  = 1,
        NET  = 2
} Scope;

typedef enum {
        ALL      = 0,
        SHARED   = 1,
        PERSONAL = 2
} View;
 
typedef struct {
        int             frame_label_set;    /* Frame label set on cmd line */
        char           *frame_label;        /* Frame label                 */
        int             icon_label_set;     /* Icon label set on cmd line  */
        char           *icon_label;         /* Icon label                  */
        int             geometry_set;       /* Geometry set on cmd line    */
        char           *icon_font;        /* Frame label                 */
} XviewInfo;

extern XviewInfo       *xview_info;
extern XviewInfo       *init_xview ();
 
typedef struct {

       /* Base Frame Objects */

     	Xv_opaque	frame;	           /* Binder Frame */
     	Xv_opaque       panel;	           /* Binder Panel */
        Xv_opaque       save_button;       /* Save Button */
        Xv_opaque       view_button;       /* View Button */
        Xv_opaque       undo_button;       /* Edit Button */
        Xv_opaque       props_button;      /* Props Button */
        Xv_opaque       tns_list;          /* Type Namespace List */
        Xv_opaque       new_button;        /* New Button */
        Xv_opaque       dup_button;        /* Dup Button */
        Xv_opaque       delete_button;     /* Delete Button */
        Xv_opaque       properties;        /* Pointer to Props struct */
 
       /* Misc */      

        int             color; 	           /* Boolean color/monochrome */
	Cms             cms;               /* Colormap Segment */
        int             applying_changes;
        char	       *current_ce_db;     /* Current CE Database open */
        int             binder_modified;
        Scope           scope;
        View            view;
        int             tt_running;        /* Boolean ToolTalk initialized */

       /* CE Types */

        CE_NAMESPACE    fns;
        CE_NAMESPACE    tns;
        CE_ATTRIBUTE    type_icon;
        CE_ATTRIBUTE    type_icon_mask;
	CE_ATTRIBUTE    type_open;
        CE_ATTRIBUTE    type_open_tt;
	CE_ATTRIBUTE    type_fgcolor;
	CE_ATTRIBUTE    type_bgcolor;
	CE_ATTRIBUTE    type_print;
        CE_ATTRIBUTE    type_name;
	CE_ATTRIBUTE    fns_type;
	CE_ATTRIBUTE    fns_filename;
        CE_ATTRIBUTE    fns_magic_offset;
        CE_ATTRIBUTE    fns_magic_match;
        CE_ATTRIBUTE    fns_magic_type;
        CE_ATTRIBUTE    fns_magic_mask;

} Binder;

extern Binder   	*binder_objects_initialize();

#endif
