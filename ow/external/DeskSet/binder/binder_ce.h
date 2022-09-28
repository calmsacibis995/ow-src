/*
 *  @(#)binder_ce.h	3.1 04/03/92
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

#ifndef _binder_ce_h
#define _binder_ce_h

extern void b_init_ce();
extern void b_init_namespaces();

extern int  add_fns_entry();
extern int  add_tns_entry();
extern int  modify_fns_entry();
extern int  modify_tns_entry();

extern int     read_tns_entry();
extern int     read_only_entry();
extern void    display_ce_error();

#endif /* !_binder_ce_h */
