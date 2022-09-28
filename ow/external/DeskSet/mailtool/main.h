/* Copyright 1992, Sun Microsystems Inc */


#ifndef mailtool_main_h
#define mailtool_main_h

#pragma ident "@(#)main.h	1.2 92/12/18 SMI"

#include <sys/types.h>
#include <unistd.h>


void mt_parse_tool_args(int argc, char **argv);
void mt_done(int i);
void mt_cleanup_tmpfiles(void);
gid_t mt_getegid(void);


#endif /* mailtool_main_h */

