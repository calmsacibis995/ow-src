#pragma ident	"@(#)BaseWindow.c	302.2	97/03/26 lib/libXol SMI"	/* basewindow:src/BaseWindow.c 1.20 */

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *************************************************************************
 *
 * Description:
 *	Source code for the OPEN LOOK (Tm - AT&T) baseWindowShell widget.
 *	Due to changes in the Vendor shell for release 2.1, this widget
 *	is now obsolete, but is supported for compatibility reasons.
 *
 ******************************file*header********************************
 */

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/BaseWindoP.h>
#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

					/* class procedures		*/

					/* action procedures		*/

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */


/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/* There are no Translations or Action Tables for the baseWindow widget */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

BaseWindowShellClassRec
baseWindowShellClassRec = {
  {						/* core class */
	(WidgetClass) &applicationShellClassRec,/* superclass		*/
	"BaseWindowShell",			/* class_name		*/
	sizeof(BaseWindowShellRec),		/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	NULL,					/* num_actions		*/
	NULL,					/* resources		*/
	0,					/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	FALSE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	FALSE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	NULL,					/* destroy		*/
	XtInheritResize,			/* resize		*/
	XtInheritExpose,			/* expose		*/
	NULL,					/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	XtInheritAcceptFocus,			/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL,					/* tm_table		*/
	XtInheritQueryGeometry			/* query_geometry	*/
  },
  {						/* composite class */
	XtInheritGeometryManager,		/* geometry_manager	*/
	XtInheritChangeManaged,			/* change_managed	*/
	XtInheritInsertChild,			/* insert_child		*/
	XtInheritDeleteChild,			/* delete_child		*/
	NULL					/* extension         	*/
  },
  {						/* shell class */
	NULL					/* extension		*/
  },
  {						/* WMShell class */
	NULL					/* extension		*/
  },
  {						/* VendorShell class */
	NULL					/* extension		*/
  },
  {						/* TopLevelShell class */
	NULL					/* extension		*/
  },
  {						/* ApplicationShell class */
	NULL					/* extension		*/
  },
  {						/* BaseWindowShell class */
	NULL					/* extension		*/
  }
}; 

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass baseWindowShellWidgetClass = (WidgetClass) &baseWindowShellClassRec;
