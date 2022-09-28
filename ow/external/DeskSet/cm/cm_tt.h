/*
 * static  char sccsid[] = "@(#)cm_tt.h 1.4 93/01/20 Copyr 1991 Sun Microsystems, Inc.";
 *
 *  ToolTalk interface functions.
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


/* get compose window from mailtool */
extern int cm_tt_compose(/* char *msghdr; Xv_opaque cframe; Xv_opaque bframe */);

/* notify ae that display setting is changed */
extern void cm_tt_update_props(/* char *filename */);

