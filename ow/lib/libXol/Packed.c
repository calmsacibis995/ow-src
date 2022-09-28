#pragma ident	"@(#)Packed.c	302.5	97/03/26 lib/libXol SMI"	/* olcommon:src/Packed.c 1.6	*/

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


#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Shell.h>

#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define WORKAROUND 1

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlCreatePackedWidget - this routine creates a widget from an
 * OlPackedWidget structure.  Various checks are made to insure the
 * incoming structure has valid parameters.
 *	When a widget is created, its id is placed in the field "widget."
 * The routine returns the address of the new widget.
 *
 * During the creation process, the following should be understood.
 *
 *    * If the field "parent_ptr" is non-NULL, the parent's id will
 *	be taken from the given address.  Now, if the "descendant" key
 *	is not NULL, a GetValues call will be made to get the id
 *	of the descendant that is the actual destination of the widget
 *	to be added.
 *
 *    * If "parent_ptr" is NULL and the "class_ptr" does not evaluate to
 *	a ShellWidget subclass, an OlError is generated.
 *
 *    * If "class_ptr" evaluates to a ShellWidget subclass and "parent_ptr"
 *	is NULL, an application shell widget is created.
 *
 *    * If "class" evaluates to a ShellWidget subclass and "parent_ptr"
 *	is non-NULL, a popup shell widget is created as a child of the
 *	specified child.
 *
 ****************************procedure*header*****************************
 */
Widget
OlCreatePackedWidget(register OlPackedWidget *pw)
{
	register WidgetClass wc;
	register Boolean     shell_widget = False;
	register Widget      parent       = (Widget) NULL;

	GetToken();

	if (!pw->class_ptr || *pw->class_ptr == (WidgetClass) NULL)
		OlError(dgettext(OlMsgsDomain, "OlCreatePackedWidget: NULL class !!"));

				/* Is this to be a shell widget		*/

	for(wc = *pw->class_ptr; wc; wc = wc->core_class.superclass)
		if (wc == shellWidgetClass) {
			shell_widget = True;
			break;
		}

				/* Get the parent of the new widget	*/

	if ( pw->parent_ptr != (Widget *) NULL)
		parent = *pw->parent_ptr;

	if (parent != (Widget) NULL && pw->descendant != (String) NULL) {
		Arg	query_parent[1];
		Widget	real_parent = (Widget) NULL;
	
		XtSetArg(query_parent[0], pw->descendant, &real_parent);
		XtGetValues(parent, query_parent, 1);
		parent = real_parent;
	}

	if (shell_widget) {
		if (parent) 
			pw->widget = XtCreatePopupShell(pw->name,
						*pw->class_ptr,
						parent,
						pw->resources,
						pw->num_resources);
		else
			pw->widget = XtCreateApplicationShell(pw->name,
						*pw->class_ptr,
						pw->resources,
						pw->num_resources);
	}
	else {
		if (parent)
			pw->widget = XtCreateWidget(pw->name,
						*pw->class_ptr,
						parent,
						pw->resources,
						pw->num_resources);
		else
			OlError(dgettext(OlMsgsDomain, "OlCreatePackedWidget: NULL widget parent !!"));
	}

	if (pw->managed)
		XtManageChild(pw->widget);

	ReleaseToken();

	return(pw->widget);
} /* END OF OlCreatePackedWidget() */

/*
 *************************************************************************
 * OlCreatePackedWidgetList - this routine creates many widgets using the
 * array of structures (i.e., of type OlPackedWidget).  The routine
 * makes sure that if the number of structures is more than zero, that
 * the array pointer is non-NULL.  The id of the first created widget
 * is returned.  If no widgets are created, NULL is returned.
 * The procedure OlCreatePackedWidget() is called to do the actual widget
 * creation.
 ****************************procedure*header*****************************
 */
Widget
OlCreatePackedWidgetList(OlPackedWidgetList pw_list, register Cardinal num_pw)
{
	register int                 i;
	register OlPackedWidgetList  pwl = pw_list;
	register Widget              first_widget = (Widget) NULL;

	if (num_pw > 0 && !pwl)
		OlError(dgettext(OlMsgsDomain, "OlCreatePackedWidgetList: NULL list for count > 0 !!"));

	for (i = 0; i < num_pw; ++i, ++pwl)
		if (!first_widget)
			first_widget = OlCreatePackedWidget(pwl);
		else
			(void) OlCreatePackedWidget(pwl);

	return(first_widget);
} /* END OF OlCreatePackedWidgetList() */

