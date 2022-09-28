/* @(#)mle.h	3.1 - 92/04/03 */


/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*	Copyright (c) 1989, 1990, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

/* mle.h -- control the multilange extensions in mail */

extern char *textdomain();
extern char *gettext();

#ifndef OW_I18N
#define FONT_COLUMN_WIDTH	FONT_DEFAULT_CHAR_WIDTH
#endif
