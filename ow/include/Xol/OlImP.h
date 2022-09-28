#ifndef _XOL_IM_P_H
#define _XOL_IM_P_H

#pragma       ident   "@(#)OlImP.h 1.2     92/04/22 SMI"        /* OLIT */

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

#include <Xol/OpenLook.h>

/*
 * One of these is created for each VendorShell (in its Initialize) in the DisplayShell
 * for it.
 */
typedef struct _ImVSInfo {
    Widget          	        vw;
    Boolean		        default_im_open_attempted;
    struct _InputMethodRec *    default_im_id;
    struct _InputContextRec *   ic_list; /* all ics created with ims in this vendor shell */
    Cardinal                    num_ics;
    struct _ImVSInfo * next;
} ImVSInfo, *ImVSInfoList;

/* 
 * External function declarations
 */

#endif	/* _XOL_IM_P_H */
