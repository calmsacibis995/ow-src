#ifndef lint
        static char sccsid[] = "@(#)isvars.c	1.3\t06/11/93 Copyr 1988 Sun Micro";
#endif
/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * isvars.c
 *
 * Description:
 *	NetISAM gloabl variables
 *
 */

#include "isam_impl.h"

int iserrno;
int isreclen;
long isrecnum;

char   isstat1, isstat2, isstat3, isstat4;

static struct keydesc _nokey;
struct keydesc *nokey = &_nokey;

/* 
 * isdupl is used internally to indicate that some index contains
 * a duplicate value. Used only to set up isstat2 Cobol variable.
 */

int isdupl;				     /* 1 if duplicate found */

