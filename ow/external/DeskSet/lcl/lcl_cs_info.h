/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_cs_info.h 1.2	96/06/27 SMI"

char    *_lct_get_charsetname_from_mimename(LCTd lctd, char *mimename);
char    *_lct_get_charsetname_from_v3name(LCTd lctd, char *v3name);
char    *_lct_get_mime_charsetname(LCTd lctd, char *name);
char    *_lct_get_v3_charsetname(LCTd lctd, char *name);
