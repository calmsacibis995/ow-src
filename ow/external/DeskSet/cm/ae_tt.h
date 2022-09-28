/*
 * static  char sccsid[] = "@(#)ae_tt.h 1.3 92/11/16 Copyr 1991 Sun Microsystems, Inc.";
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

#ifndef ae_tt_h
#define ae_tt_h

#include <desktop/tt_c.h>
#include <dstt.h>

extern int tt_flag;

extern void get_version(char **, char **, char **);
extern status_t handle_display(Tt_message, char *, Data_t, void *, int,
				char *, char *);
extern status_t handle_edit(Tt_message, char *, Data_t, void *, int,
				char *, char *);
extern status_t handle_modified(Tt_message, char *, char *);
extern status_t handle_quit(Tt_message, int, int, char *);
extern void ae_tt_send_appt(char *, int);
extern void ae_dstt_start(Xv_opaque, int);
extern int ae_dstt_done();

#endif

