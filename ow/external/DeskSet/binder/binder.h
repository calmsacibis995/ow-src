/*
 *  @(#)binder.h	3.8 02/02/93
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

#ifndef _binder_h
#define _binder_h

#include <desktop/ce.h>

#define MAX_FNS_ENTRIES         1024
#define MAX_TNS_ENTRIES         1024
#define WIN_INDEX	 	  0
#define EMPTY           	 -1
#define BUFSIZE             	255

typedef struct {
    char       *type_name;	/* Type Name Key */
    char       *filename;   	/* File NS Filename for Pattern entries */
    char       *data_value;	/* Data value for Content entries */
    char       *data_offset;	/* Data offset for Content entries */
    char       *data_type;	/* Data type for Content entries */
    char       *data_mask;	/* Mask value for Content entries */
    char       *db_name;	/* Database Name */
    CE_ENTRY   entry;		/* Handle to File NS */
} Fns_entry;
 
typedef struct {
    char          *db_name;
    char          *type_name;
    char          *icon_file;
    char          *icon_mask_file;
    char          *open_method;
    char          *print_method;
    char          *fg_color;
    char          *bg_color;
    CE_ENTRY       entry;

    Server_image   icon_stipple; /* Icon Stipple Image */
    Server_image   icon_mask;    /* Icon Mask Image */
    Server_image   icon_image;   /* Created stippled-masked-clipped icon image */
    unsigned long  fg_index;
    unsigned long  bg_index;
} Tns_entry;
 

#endif /* !_binder_h */
