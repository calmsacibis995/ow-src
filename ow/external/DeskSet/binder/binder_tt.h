/*
 *  "@(#)binder_tt.h	3.1 04/03/92"
 *
 *  binder_tt.h - ToolTalk interface functions.
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


#ifndef _binder_tt_h
#define _binder_tt_h

#ifdef TOOLTALK

extern void b_init_tt();
extern void b_init_tt_complete();
extern void b_fgcolor_tt();
extern void b_bgcolor_tt();
extern void b_open_frame();
extern void b_close_frame();
extern void b_quit_tt();

#endif /* TOOLTALK */

#endif /* !_binder_tt.h */
