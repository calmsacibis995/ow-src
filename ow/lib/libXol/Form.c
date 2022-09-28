#pragma ident	"@(#)Form.c	302.11	97/03/26 lib/libXol SMI"	/* form:src/Form.c 1.26	*/
/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*************************************<+>*************************************
 *****************************************************************************
 **
 **   File:        Form.c
 **
 **   Description: Contains code for the X Widget's Form manager.
 **
 *****************************************************************************
 **   
 **   Copyright (c) 1988 by Hewlett-Packard Company
 **   
 *****************************************************************************
 *************************************<+>*************************************/


#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <values.h>
#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/keysymdef.h>
  
#include <Xol/FormP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlI18nP.h>
#include <Xol/SuperCaret.h>


#define Strdup(S) strcpy(XtMalloc((unsigned)_OlStrlen(S) + 1), S)
#define MAGIC_NUM	1000

/*  Constraint resource list for Form  */

static XtResource constraintResources[] = 
{
   {
      XtNxRefName, XtCXRefName, XtRString, sizeof(XtPointer),
      XtOffset(FormConstraints, x_ref_name), XtRString, (XtPointer) NULL
   },

   {
      XtNxRefWidget, XtCXRefWidget, XtRPointer, sizeof(XtPointer),
      XtOffset(FormConstraints, x_ref_widget), XtRPointer, NULL
   },

   {
      XtNxOffset, XtCXOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, x_offset), XtRString, "0"
   },

   {
      XtNxAddWidth, XtCXAddWidth, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_add_width), XtRString, "False"
   },

   {
      XtNxVaryOffset, XtCXVaryOffset, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_vary_offset), XtRString, "False"
   },

   {
      XtNxResizable, XtCXResizable, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_resizable), XtRString, "False"
   },

   {
      XtNxAttachRight, XtCXAttachRight, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, x_attach_right), XtRString, "False"
   },

   {
      XtNxAttachOffset, XtCXAttachOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, x_attach_offset), XtRString, "0"
   },

   {
      XtNyRefName, XtCYRefName, XtRString, sizeof(XtPointer),
      XtOffset(FormConstraints, y_ref_name), XtRString, (XtPointer) NULL
   },

   {
      XtNyRefWidget, XtCYRefWidget, XtRPointer, sizeof(XtPointer),
      XtOffset(FormConstraints, y_ref_widget), XtRPointer, NULL
   },

   {
      XtNyOffset, XtCYOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, y_offset), XtRString, "0"
   },

   {
      XtNyAddHeight, XtCYAddHeight, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_add_height), XtRString, "False"
   },

   {
      XtNyVaryOffset, XtCYVaryOffset, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_vary_offset), XtRString, "False"
   },

   {
      XtNyResizable, XtCYResizable, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_resizable), XtRString, "False"
   },

   {
      XtNyAttachBottom, XtCYAttachBottom, XtRBoolean, sizeof(Boolean),
      XtOffset(FormConstraints, y_attach_bottom), XtRString, "False"
   },

   {
      XtNyAttachOffset, XtCYAttachOffset, XtRDimension, sizeof(Dimension),
      XtOffset(FormConstraints, y_attach_offset), XtRString, "0"
   }
};


/*  Static routine definitions  */

static void     Initialize(FormWidget request, FormWidget new);
static void     Realize(FormWidget fw, XtValueMask *valueMask, XSetWindowAttributes *attributes);
static void     Resize(FormWidget fw);
static void     Destroy(FormWidget fw);
static Boolean  SetValues(FormWidget current, FormWidget request, FormWidget new);

static void     ChangeManaged(FormWidget fw);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);

static void     ConstraintInitialize(Widget request, Widget new);
static void     ConstraintDestroy(Widget w);
static Boolean  ConstraintSetValues(Widget current, Widget request, Widget new);

static void     GetRefWidget(Widget *widget, char **name, Widget w);
static Widget   XwFindWidget(FormWidget w, char *name);
static FormRef *XwGetFormRef(Widget this, Widget ref, Dimension offset, Boolean add, Boolean vary, Boolean resizable, Boolean attach, Dimension attach_offset, Position loc, Dimension size);
static Widget   XwFindValidRef(Widget refWidget, int refType, FormRef *formRef);
static FormRef *XwRefTreeSearch(Widget w, FormRef *formRef);
static FormRef *XwParentRefTreeSearch(Widget w, FormRef *wFormRef, FormRef *parentFormRef);
static void     XwMakeRefs(Widget w);
static void     XwDestroyRefs(Widget w);
static void     XwProcessRefs(FormWidget fw, Boolean formResizable);
static void     XwAddRef(FormRef *refParent, FormRef *ref);
static void     XwRemoveRef(FormRef *refParent, FormRef *ref);
static void     XwFindDepthAndCount(FormRef *node, int nodeLevel);
static void     XwInitProcessList(FormProcess **processList, FormRef *node, int nodeLevel);
static void     XwConstrainList(FormProcess **processList, int leaves, int depth, Dimension *formSize, Boolean varySize, int orient);
static void     XwFreeConstraintList(FormProcess **processList, int leaves);
static void	ResortTree(FormRef *, Widget , FormRef *, FormRef *, int );
static Boolean FixYRefWidget(FormRef *root, Widget dead_widget);
static Boolean FixXRefWidget(FormRef *root, Widget dead_widget);

/*  Static global variable definitions  */

static int depth, leaves, arrayIndex;


/*  The Form class record */

FormClassRec formClassRec =
{
   {
    (WidgetClass) &(managerClassRec), /* superclass		  */	
      "Form",                           /* class_name	         */	
      sizeof(FormRec),                /* widget_size	         */	
      NULL,                             /* class_initialize      */    
      NULL,                             /* class_part_initialize */    
      FALSE,                            /* class_inited          */	
      (XtInitProc)Initialize,          /* initialize	         */	
      NULL,                             /* initialize_hook       */
      (XtRealizeProc)Realize,          /* realize	         */	
      NULL,	                        /* actions               */	
      0,                                /* num_actions	         */	
      NULL,	                        /* resources	         */	
      0,				/* num_resources         */	
      NULLQUARK,                        /* xrm_class	         */	
      TRUE,                             /* compress_motion       */	
      TRUE,                             /* compress_exposure     */	
      TRUE,                             /* compress_enterleave   */
      FALSE,                            /* visible_interest      */	
      (XtWidgetProc)Destroy,           /* destroy               */	
      (XtWidgetProc)Resize,            /* resize                */
      NULL,                             /* expose                */	
      (XtSetValuesFunc)SetValues,      /* set_values	         */	
      NULL,                             /* set_values_hook       */
      XtInheritSetValuesAlmost,         /* set_values_almost     */
      NULL,                             /* get_values_hook       */
      NULL,                             /* accept_focus	         */	
      XtVersion,                        /* version               */
      NULL,                             /* callback private      */
      XtInheritTranslations,            /* tm_table              */
      NULL,                             /* query_geometry        */
   },

   {                                         /*  composite class           */
      (XtGeometryHandler) GeometryManager,   /*  geometry_manager          */
      (XtWidgetProc) ChangeManaged,          /*  change_managed            */
      XtInheritInsertChild,		     /* insert_child	           */
      XtInheritDeleteChild,                  /*  delete_child (inherited)  */
      NULL,				     /*  extension    	           */
   },

   {                                      /*  constraint class            */
      constraintResources,		  /*  constraint resource set     */
      XtNumber(constraintResources),      /*  num_resources               */
      sizeof(FormConstraintRec),	  /*  size of the constraint data */
      (XtInitProc) ConstraintInitialize,  /*  contraint initilize proc    */
      (XtWidgetProc) ConstraintDestroy,   /*  contraint destroy proc      */
      (XtSetValuesFunc) ConstraintSetValues,  /*  contraint set values proc */
      NULL,				  /*  extension                   */
   },

   {					/* manager class	*/
      NULL,				/* highlight_handler   	*/
      NULL,				/* reserved		*/
      NULL,				/* reserved		*/
      NULL,				/* traversal_handler	*/
      NULL,				/* activate		*/
      NULL,				/* event_procs		*/
      0,				/* num_event_procs	*/
      NULL,				/* register_focus	*/
      NULL,				/* reserved		*/
      OlVersion,			/* version		*/
      NULL,				/* extension		*/
      {NULL,0 },			/* dyn data		*/
      NULL,				/* transparent_proc     */
      NULL,				/* query_sc_locn_proc   */
   },

   {              			/*  form class  */
      0					/*  mumble      */               
   }	
};


WidgetClass formWidgetClass = (WidgetClass)&formClassRec;


/************************************************************************
 *
 *  Initialize
 *     The main widget instance initialization routine.
 *
 ************************************************************************/

static void Initialize (FormWidget request, FormWidget new)
{

   /*  Initialize the tree fields to NULL  */

   new -> form.width_tree = 
      XwGetFormRef (	(Widget)new, (Widget)NULL, (Dimension)0, 
			False, False, True, False, 
			(Dimension)0, (Position)0, (Dimension)0);   
   new -> form.height_tree =
      XwGetFormRef (	(Widget)new, (Widget)NULL, (Dimension)0, 
			False, False, True, False, 
			(Dimension)0, (Position)0, (Dimension)0);   

   new->form.save_border = -1;

   /*  Set up a geometry for the widget if it is currently 0.  */

   if (request -> core.width == (Dimension)0)
      new -> core.width += (Dimension)200;
   if (request -> core.height == (Dimension)0)
      new -> core.height += (Dimension)200;
}


/************************************************************************
 *
 *  ConstraintInitialize
 *     The main widget instance constraint initialization routine.
 *
 ************************************************************************/

static void ConstraintInitialize (Widget request, Widget new)
{
   FormConstraintRec * constraintRec;


   constraintRec = (FormConstraintRec *) new -> core.constraints;


   /*  Initialize the contraint widget sizes for later processing  */

   /* Sanity check introduced */
   if ((short) constraintRec->x_offset < 0) {
   	OlWarning(dgettext(OlMsgsDomain,
		"Form: Child has invalid Xoffset."));
	constraintRec->x_offset = 0;
   }

   if ((short) constraintRec->x_attach_offset < 0) {
   	OlWarning(dgettext(OlMsgsDomain,
		"Form: Child has invalid XAttachOffset."));
   	constraintRec->x_attach_offset = 0;
   }

   if ((short) constraintRec->y_offset < 0) {
   	OlWarning(dgettext(OlMsgsDomain,
		"Form: Child has invalid Yoffset."));
   	constraintRec->y_offset = 0;
   }

   if ((short) constraintRec->y_attach_offset < 0) {
   	OlWarning(dgettext(OlMsgsDomain,
		"Form: Child has invalid YAttachOffset."));
   	constraintRec->y_attach_offset = 0;
   }

   constraintRec -> set_x = (Position)0;
   constraintRec -> set_y = (Position)0;
   constraintRec -> set_width = (Dimension)0;
   constraintRec -> set_height = (Dimension)0;

   constraintRec -> x = new -> core.x;
   constraintRec -> y = new -> core.y;
   constraintRec -> width = new -> core.width;
   constraintRec -> height = new -> core.height;

   constraintRec -> managed = False;

   constraintRec -> width_when_unmanaged = (Dimension)0;
   constraintRec -> height_when_unmanaged = (Dimension)0;

   /*
    * The refWidget and refName resources will be resolved at the last
    * possible moment, to allow forward referencing of yet-to-be-created
    * peers.
    */
   if (constraintRec -> x_ref_name)
      constraintRec -> x_ref_name = Strdup(constraintRec -> x_ref_name);
   if (constraintRec -> y_ref_name)
      constraintRec -> y_ref_name = Strdup(constraintRec -> y_ref_name);
}


/************************************************************************
 *
 *  GetRefWidget
 *     Get and verify the reference widget given.
 *     NOTE: it is up to the caller to free these names.
 *
 ************************************************************************/

static void 
GetRefWidget(Widget *widget, char **name, Widget w)
{
	if (*widget != NULL) {
		if (*name != NULL) {
			if (XrmStringToQuark(*name) != (*widget)->core.xrm_name) {
				char *buf;

				if (buf = malloc(512)) {
					snprintf(buf, 512, dgettext(OlMsgsDomain,
			"Form \"%s\": refWidget (%2$s) and refName (%3$s) don't match."),
						w->core.parent->core.name,
						XtName(*widget),
						*name);
					OlWarning(buf);
					XtFree(*name);
					*name = Strdup(XtName(*widget));
					free(buf);
				}
			}
		} else
			*name = Strdup(XtName(*widget));

		if ((*widget) != w->core.parent &&
			(*widget)->core.parent != w->core.parent) {
			char *buf;

			if (buf = malloc(512)) {
				snprintf(buf, 512, dgettext(OlMsgsDomain,
			"Form \"%1$s\": refWidget \"%2$s\" is neither a child of the form"),
					w->core.parent->core.name,
					*name);
				OlWarning(buf);
				OlWarning(dgettext(OlMsgsDomain,
					"      nor the form itself."));
				XtFree(*name);
				*name = Strdup(w->core.parent->core.name);
				*widget = w->core.parent;
				free(buf);
			}
		}
	} else if (*name != NULL) {
		if ((*widget = XwFindWidget((FormWidget)w->core.parent, *name))
			== (Widget) NULL) {
			char *buf;

			if (buf = malloc(512)) {
				snprintf(buf, 512, dgettext(OlMsgsDomain,
				"Form \"%1$s\": refName \"%2$s\" is not a known widget."),
					w->core.parent->core.name,
					*name);
				OlWarning(buf);

				XtFree(*name);
				*name = Strdup(w->core.parent->core.name);
				*widget = w->core.parent;
				free(buf);
			}
		}
	} else {
		*name = Strdup(w->core.parent->core.name);
		*widget = w->core.parent;
	}
}


/************************************************************************
 *
 *  XwFindWidget
 *
 ************************************************************************/

static Widget 
XwFindWidget(FormWidget w, char *name)
{
	int             i;
	Widget         *list;
	int             count;

	/*
	 * Compare quarks, not strings. First, it should prove a little faster,
	 * second, it works for gadget children (the string version of the name
	 * isn't in core for gadgets).
	 */
	XrmQuark        xrm_name = XrmStringToQuark(name);

	if (xrm_name == w->core.xrm_name)
		return ((Widget) w);

	list = w->composite.children;
	count = w->composite.num_children;

	for (i = 0; i < count; i++) {
		if (xrm_name == (*list)->core.xrm_name)
			return (*list);
		list++;
	}
	return (NULL);
}


/************************************************************************
 *
 *  Realize
 *	Create the widget window and create the gc's.
 *
 ************************************************************************/

static void 
Realize(FormWidget fw, XtValueMask *valueMask, XSetWindowAttributes *attributes)
{
	Mask            newValueMask = *valueMask;

	XtCreateWindow((Widget)fw, InputOutput, (Visual *)CopyFromParent,
		newValueMask, attributes);

	XwProcessRefs(fw, False);
}


/************************************************************************
 *
 *  Resize
 *
 ************************************************************************/
		
static void
Resize(FormWidget fw)
{

	if (XtIsRealized((Widget)fw))
		XwProcessRefs(fw, False);
}


/************************************************************************
 *
 *  Destroy
 *	Deallocate the head structures of the reference trees.
 *	The rest of the tree has already been deallocated.
 *
 ************************************************************************/

static void 
Destroy(FormWidget fw)
{

	if (fw->form.width_tree)
	    XtFree((char *)fw->form.width_tree);
	if (fw->form.height_tree)
	    XtFree((char *)fw->form.height_tree);
}


/************************************************************************
 *
 *  ConstraintDestroy
 *	Deallocate the allocated referenence names.
 *
 ************************************************************************/

static void 
ConstraintDestroy(Widget w)
{
	FormWidget fw = (FormWidget) XtParent(w);
	FormConstraintRec *constraint;

	constraint = (FormConstraintRec *) w->core.constraints;

	if (!fw->core.being_destroyed)
	{
	    (void) FixXRefWidget(fw->form.width_tree, w);
	    (void) FixYRefWidget(fw->form.height_tree, w);
	}

	if (constraint->x_ref_name != NULL)
		XtFree(constraint->x_ref_name);
	if (constraint->y_ref_name != NULL)
		XtFree(constraint->y_ref_name);
}


/************************************************************************
 *
 *  SetValues
 *	Currently nothing needs to be done.  The XtSetValues call
 *	handles geometry requests and form does not define any
 *	new resources.
 *
 ************************************************************************/

static Boolean
SetValues(FormWidget current, FormWidget request, FormWidget new)
{
	return (False);
}


/************************************************************************
 *
 *  ConstraintSetValues
 *	Process changes in the constraint set of a widget.
 *
 ************************************************************************/

static Boolean ConstraintSetValues (Widget current, Widget request, Widget new)
{
   FormConstraintRec * curConstraint;
   FormConstraintRec * newConstraint;
   FormConstraintRec * tempConstraint;


   curConstraint = (FormConstraintRec *) current -> core.constraints;
   newConstraint = (FormConstraintRec *) new -> core.constraints;


   /*  Check the geometrys to see if new's contraint record  */
   /*  saved geometry data needs to be updated.              */

   if (XtIsRealized (current))
   {
      if (new -> core.x != current -> core.x)
         newConstraint -> set_x = new -> core.x;
      if (new -> core.y != current -> core.y)
         newConstraint -> set_y = new -> core.y;
      if (new -> core.width != current -> core.width)
         newConstraint -> set_width = new -> core.width;
      if (new -> core.height != current -> core.height)
         newConstraint -> set_height = new -> core.height;
   }


   /*  If the reference widget or name has changed, set the  */
   /*  opposing member to NULL in order to get the proper    */
   /*  referencing.  For names, the string space will be     */
   /*  deallocated out of current later.                     */
   /*  However, if both reference widget and reference name  */
   /*  have changed, leave them alone so that mismatches can */
   /*  be caught.                                            */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget
    && newConstraint -> x_ref_name != curConstraint -> x_ref_name)
      ;
   else if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget)
      newConstraint -> x_ref_name = NULL;
   else if (newConstraint -> x_ref_name != curConstraint -> x_ref_name)
      newConstraint -> x_ref_widget = NULL;

   if (newConstraint -> x_ref_name != curConstraint -> x_ref_name) {
      XtFree (curConstraint -> x_ref_name);
      if (newConstraint -> x_ref_name)
	newConstraint -> x_ref_name = Strdup(newConstraint -> x_ref_name);
   }

   if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget
    && newConstraint -> y_ref_name != curConstraint -> y_ref_name)
      ;
   else if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget)
      newConstraint -> y_ref_name = NULL;
   else if (newConstraint -> y_ref_name != curConstraint -> y_ref_name)
      newConstraint -> y_ref_widget = NULL;

   if (newConstraint -> y_ref_name != curConstraint -> y_ref_name) {
      XtFree (curConstraint -> y_ref_name);
      if (newConstraint -> y_ref_name)
	newConstraint -> y_ref_name = Strdup(newConstraint -> y_ref_name);
   }



   /*  See if any constraint data for the widget has changed.  */
   /*  (note that after the above checks it is sufficient to   */
   /*  compare just the reference widgets, below).             */
   /*  Is so, remove the old reference tree elements from the  */
   /*  forms constraint processing trees and build and insert  */
   /*  new reference tree elements.                            */
   /*                                                          */
   /*  Once this is finished, reprocess the constraint trees.  */

   if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget       ||
       newConstraint -> y_ref_widget != curConstraint -> y_ref_widget       ||

       newConstraint -> x_offset != curConstraint -> x_offset               ||
       newConstraint -> y_offset != curConstraint -> y_offset               ||

       newConstraint -> x_vary_offset != curConstraint -> x_vary_offset     ||
       newConstraint -> y_vary_offset != curConstraint -> y_vary_offset     ||

       newConstraint -> x_resizable != curConstraint -> x_resizable         ||
       newConstraint -> y_resizable != curConstraint -> y_resizable         ||

       newConstraint -> x_add_width != curConstraint -> x_add_width         ||
       newConstraint -> y_add_height != curConstraint -> y_add_height       ||

       newConstraint -> x_attach_right != curConstraint -> x_attach_right   ||
       newConstraint -> y_attach_bottom != curConstraint -> y_attach_bottom ||

       newConstraint -> x_attach_offset != curConstraint -> x_attach_offset ||
       newConstraint -> y_attach_offset != curConstraint -> y_attach_offset)
   {
      if (XtIsRealized (current) && current -> core.managed)
      {
	 if (newConstraint -> x_ref_widget != curConstraint -> x_ref_widget)
		GetRefWidget(&newConstraint -> x_ref_widget,
			     &newConstraint -> x_ref_name,
			     new);
	 if (newConstraint -> y_ref_widget != curConstraint -> y_ref_widget)
		GetRefWidget(&newConstraint -> y_ref_widget,
			     &newConstraint -> y_ref_name,
			     new);

         XwDestroyRefs (current->core.self); /*KSK*/

         tempConstraint = (FormConstraintRec *) current -> core.constraints;
         current -> core.constraints = new -> core.constraints;
	 XwMakeRefs (new->core.self); /*METH*/
         current -> core.constraints = (XtPointer) tempConstraint;
      }

/* 
 * Don't need this. We don't need to re-layout the children at this point 
 * but just indicate that the border_width has changed so that the Intrinsics
 * will automatically call the GM for us to do the layout. Else this will be 
 * duplicated
 */
/*
      if (XtIsRealized (current)) XwProcessRefs (new -> core.parent, True);
*/
		if (XtIsRealized (current)) {
			FormWidget fw = (FormWidget) XtParent(new);
			fw->form.save_border = new->core.border_width;
			new->core.border_width += MAGIC_NUM;
		}	

   }

   return False;
}


/************************************************************************
 *
 *  GeometryManager
 *      Always accept the childs new size, set the childs constraint
 *      record size to the new size and process the constraints.
 *
 ************************************************************************/

static XtGeometryResult 
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	FormWidget      fw = (FormWidget) w->core.parent;
	FormConstraintRec *constraint;
	FormRef        *xRef;
	FormRef        *yRef;
	Dimension       newBorder = w->core.border_width;
	Boolean         moveFlag = False;
	Boolean         resizeFlag = False;

	Boolean         fromSetValues = False;	/* will be set to True, if GM
						 * is called from SetValues */

	constraint = (FormConstraintRec *) w->core.constraints;

	if (request->request_mode & CWX)
		constraint->set_x = request->x;

	if (request->request_mode & CWY)
		constraint->set_y = request->y;

	if (request->request_mode & CWWidth) {
		constraint->set_width = request->width;
	}
	if (request->request_mode & CWHeight)
		constraint->set_height = request->height;

	if (request->request_mode & CWBorderWidth) {	/* could be fake/real */
		newBorder = request->border_width;
		if (fw->form.save_border != -1) {	/* From SetValues */
			fromSetValues = True;
			w->core.border_width = fw->form.save_border;	/* Restore org value */
			newBorder = fw->form.save_border;
			request->border_width -= MAGIC_NUM;	/* Not really necessary
								 * ,but incase later, we
								 * decide to use
								 * 'request' */
			fw->form.save_border = -1;	/* reset */
		}
	}
	/* If the x or the width has changed, find the horizontal  */
	/* reference tree structure for this widget and update it  */

	xRef = yRef = (FormRef *) NULL;

	if ((request->request_mode & CWWidth) || (request->request_mode & CWX)) {
		if ((xRef = XwRefTreeSearch(w, fw->form.width_tree)) != (FormRef *) NULL) {
			if (request->request_mode & CWX)
				xRef->set_loc = request->x;
			if (request->request_mode & CWWidth)
				xRef->set_size = request->width;
		}
	}
	/* If the y or the height has changed, find the vertical   */
	/* reference tree structure for this widget and update it  */

	if ((request->request_mode & CWHeight) || (request->request_mode & CWY)) {
		if ((yRef = XwRefTreeSearch(w, fw->form.height_tree)) != (FormRef *) NULL) {
			if (request->request_mode & CWY)
				yRef->set_loc = request->y;
			if (request->request_mode & CWHeight)
				yRef->set_size = request->height;
		}
	}
	/* Process the constraints if either of the ref structs have changed */

	if (xRef != NULL || yRef != NULL || fromSetValues) {
		_OlDnDSetDisableDSClipping(w, True);
		if ((request->request_mode & CWX) || (request->request_mode & CWY))
			moveFlag = True;
		if ((request->request_mode & CWWidth) ||
			(request->request_mode & CWHeight))
			resizeFlag = True;

		if (moveFlag && resizeFlag)
			OlConfigureWidget(w, constraint->set_x, constraint->set_y,
				constraint->set_width, constraint->set_height,
				newBorder);
		else if (resizeFlag) {
			OlResizeWidget(w, constraint->set_width, constraint->set_height,
				newBorder);
		} else if (moveFlag)
			OlMoveWidget(w, constraint->set_x, constraint->set_y);


		XwProcessRefs((FormWidget)w->core.parent, True);
		_OlDnDSetDisableDSClipping(w, False);
		OlDnDWidgetConfiguredInHier(w);
	}
	/* See if an almost condition should be returned  */

	if (((request->request_mode & CWX) && w->core.x != request->x) ||
		((request->request_mode & CWY) && w->core.y != request->y) ||
		((request->request_mode & CWWidth) &&
			w->core.width != request->width) ||
		((request->request_mode & CWHeight) &&
			w->core.height != request->height)) {
		reply->request_mode = request->request_mode;

		if (request->request_mode & CWX)
			reply->x = w->core.x;
		if (request->request_mode & CWY)
			reply->y = w->core.y;
		if (request->request_mode & CWWidth)
			reply->width = w->core.width;
		if (request->request_mode & CWHeight)
			reply->height = w->core.height;
		if (request->request_mode & CWBorderWidth)
			reply->border_width = request->border_width;
		if (request->request_mode & CWSibling)
			reply->sibling = request->sibling;
		if (request->request_mode & CWStackMode)
			reply->stack_mode = request->stack_mode;

		return (XtGeometryAlmost);
	}
	return (XtGeometryDone);
}


/************************************************************************
 *
 *  ChangeManaged
 *
 ************************************************************************/

static void 
ChangeManaged(FormWidget fw)
{
	Widget          child;
	FormConstraintRec *constraint;
	int             i;


	/* If the widget is being managed, build up the reference     */
	/* structures for it, adjust any references, and process the  */
	/* reference set.  If unmanaged, remove its reference.        */

	/*
	 * Resolve refWidget and refName values, if they have not yet been
	 * resolved.
	 */
	for (i = 0; i < fw->composite.num_children; i++) {
		child = fw->composite.children[i];
		constraint = (FormConstraintRec *) child->core.constraints;
		GetRefWidget(&constraint->x_ref_widget, &constraint->x_ref_name, child);
		GetRefWidget(&constraint->y_ref_widget, &constraint->y_ref_name, child);
	}

	for (i = 0; i < fw->composite.num_children; i++) {
		child = fw->composite.children[i];
		constraint = (FormConstraintRec *) child->core.constraints;

		if (constraint->set_width == 0) {
			constraint->set_x = child->core.x;
			constraint->set_y = child->core.y;
			constraint->set_width = child->core.width;
			constraint->set_height = child->core.height;
		}
		if (child->core.managed != constraint->managed) {
			if (child->core.managed) {
				if (constraint->width_when_unmanaged != child->core.width)
					constraint->set_width = child->core.width;
				if (constraint->height_when_unmanaged != child->core.height)
					constraint->set_height = child->core.height;
				XwMakeRefs(child);
			} else {
				constraint->width_when_unmanaged = child->core.width;
				constraint->height_when_unmanaged = child->core.height;
				XwDestroyRefs(child);
			}
			constraint->managed = child->core.managed;
		}
	}

	XwProcessRefs(fw, True);
}


/************************************************************************
 *
 *  XwMakeRefs
 *	Build up and insert into the forms reference trees the reference
 *      structures needed for the widget w.
 *
 ************************************************************************/

static void 
XwMakeRefs(Widget w)
{
	Widget          xRefWidget;
	Widget          yRefWidget;
	FormWidget      formWidget;
	FormConstraintRec *constraint;
	FormRef        *xRefParent;
	FormRef        *yRefParent;
	FormRef        *xRef;
	FormRef        *yRef;
	FormRef        *checkRef;
	register int    i;


	formWidget = (FormWidget) w->core.parent;
	constraint = (FormConstraintRec *) w->core.constraints;


	/* The "true" reference widget may be unmanaged, so  */
	/* we need to back up through the reference set      */
	/* perhaps all the way to Form.                      */

	xRefWidget = XwFindValidRef(constraint->x_ref_widget, OL_HORIZONTAL,
		formWidget->form.width_tree);
	yRefWidget = XwFindValidRef(constraint->y_ref_widget, OL_VERTICAL,
		formWidget->form.height_tree);


	/* Search the referencing trees for the referencing widgets  */
	/* The constraint reference struct will be added as a child  */
	/* of this struct.                                           */

	if (xRefWidget != NULL)
		xRefParent = XwRefTreeSearch(xRefWidget, formWidget->form.width_tree);

	if (yRefWidget != NULL)
		yRefParent = XwRefTreeSearch(yRefWidget, formWidget->form.height_tree);


	/* Allocate, initialize, and insert the reference structures  */

	if (xRefWidget != NULL) {
		xRef = XwGetFormRef(w, xRefWidget, constraint->x_offset,
			constraint->x_add_width, constraint->x_vary_offset,
			constraint->x_resizable, constraint->x_attach_right,
			constraint->x_attach_offset,
			constraint->set_x, constraint->set_width);
		XwAddRef(xRefParent, xRef);
	}
	if (yRefWidget != NULL) {
		yRef = XwGetFormRef(w, yRefWidget, constraint->y_offset,
			constraint->y_add_height, constraint->y_vary_offset,
			constraint->y_resizable, constraint->y_attach_bottom,
			constraint->y_attach_offset,
			constraint->set_y, constraint->set_height);
		XwAddRef(yRefParent, yRef);
	}
	/* Search through the parents reference set to get any child  */
	/* references which need to be made child references of the   */
	/* widget just added.                                         */

	if (xRefWidget != NULL) {
		FormRef *root = formWidget->form.width_tree;
		ResortTree(root, w, xRef, root, OL_HORIZONTAL);
	}
	if (yRefWidget != NULL) {
		FormRef *root = formWidget->form.height_tree;
		ResortTree(root, w, yRef, root, OL_VERTICAL);
	}
}

static void
ResortTree(FormRef *root, Widget w, FormRef *w_ref, FormRef *form_ref, int refType)
{
	int i;
	FormConstraintRec *constraint;

	for (i = 0; i < root->ref_to_count; i++) {
		constraint = (FormConstraintRec *)
				root->ref_to[i]->c_this->core.constraints;

	    if ((refType == OL_HORIZONTAL && 
	          XwFindValidRef(constraint->x_ref_widget,OL_HORIZONTAL,form_ref) == w) ||
		(refType == OL_VERTICAL && 
		  XwFindValidRef(constraint->y_ref_widget,OL_VERTICAL,form_ref) == w)) {

			/* Move this subtree to w_ref */
			FormRef *temp = root->ref_to[i];

			/* Fix up the pointer back to the parent */
			temp->ref = w_ref->c_this;
			XwRemoveRef(root, root->ref_to[i]);
			XwAddRef(w_ref, temp);
	    } else {
			/* Recurse */
			ResortTree(root->ref_to[i], w, w_ref, form_ref, refType);
	    }
	}
}


/************************************************************************
 *
 *  XwDestroyRefs
 *	Remove and deallocate the reference structures for the widget w.
 *
 ************************************************************************/

static void 
XwDestroyRefs(Widget w)
{
	Widget          xRefWidget;
	Widget          yRefWidget;
	FormWidget      formWidget;
	FormRef        *xRefParent;
	FormRef        *yRefParent;
	FormRef        *xRef;
	FormRef        *yRef;
	FormRef        *tempRef;
	int		i;

	formWidget = (FormWidget)w->core.parent;


	/* Search through the reference trees to see if the widget  */
	/* is within the tree.                                      */

	xRefWidget = w;
	yRefWidget = w;

	xRefParent =
		XwParentRefTreeSearch(xRefWidget, formWidget->form.width_tree,
		formWidget->form.width_tree);
	yRefParent =
		XwParentRefTreeSearch(yRefWidget, formWidget->form.height_tree,
		formWidget->form.height_tree);


	/* 
	 * For both the width and height references, if the ref parent was
	 * not null, find the reference to be removed within the parents
	 * list, remove this reference.  Then, for any references attached
	 * to the one just removed, reparent them to the parent reference.
	 */
	if (xRefParent != NULL) {
		for (i = 0; i < xRefParent->ref_to_count; i++) {
			if (xRefParent->ref_to[i]->c_this == xRefWidget) {
				xRef = xRefParent->ref_to[i];
				break;
			}
		}

		XwRemoveRef(xRefParent, xRefParent->ref_to[i]);

		while (xRef->ref_to_count) {
			tempRef = xRef->ref_to[0];
			tempRef->ref = xRefParent->c_this;
			XwRemoveRef(xRef, tempRef);
			XwAddRef(xRefParent, tempRef);
		}

		XtFree((char*)xRef);
	}
	if (yRefParent != NULL) {
		for (i = 0; i < yRefParent->ref_to_count; i++) {
			if (yRefParent->ref_to[i]->c_this == yRefWidget) {
				yRef = yRefParent->ref_to[i];
				break;
			}
		}

		XwRemoveRef(yRefParent, yRef);

		while (yRef->ref_to_count) {
			tempRef = yRef->ref_to[0];
			tempRef->ref = yRefParent->c_this;
			XwRemoveRef(yRef, tempRef);
			XwAddRef(yRefParent, tempRef);
		}

		XtFree((char*)yRef);
	}
}


/************************************************************************
 *
 *  XwGetFormRef
 *	Allocate and initialize a form constraint referencing structure.
 *
 ************************************************************************/

static FormRef *
XwGetFormRef(Widget this, Widget ref, Dimension offset, Boolean add, Boolean vary, Boolean resizable, Boolean attach, Dimension attach_offset, Position loc, Dimension size)
{
	FormRef        *formRef;

	formRef = (FormRef *)XtMalloc(sizeof (FormRef));
	formRef->c_this = this;
	formRef->ref = ref;
	formRef->offset = offset;
	formRef->add = add;
	formRef->vary = vary;
	formRef->resizable = resizable;
	formRef->attach = attach;
	formRef->attach_offset = attach_offset;

	formRef->set_loc = loc;
	formRef->set_size = size;

	formRef->ref_to = NULL;
	formRef->ref_to_count = 0;

	return (formRef);
}


/************************************************************************
 *
 *  XwFindValidRef
 *	Given an initial reference widget to be used as a constraint,
 *	find a valid (managed) reference widget.  This is done by
 *	backtracking through the widget references listed in the
 *	constraint records.  If no valid constraint is found, "form"
 *	is returned indicating that this reference should be stuck
 *	immediately under the form reference structure.
 *
 ************************************************************************/

static Widget 
XwFindValidRef(Widget refWidget, int refType, FormRef *formRef)
{
	FormConstraintRec *constraint;

	if (refWidget == NULL)
		return (NULL);

	while (1) {
		if (XwRefTreeSearch(refWidget, formRef) != NULL)
			return (refWidget);

		constraint = (FormConstraintRec *) refWidget->core.constraints;

		if (refType == OL_HORIZONTAL)
			refWidget = constraint->x_ref_widget;
		else
			refWidget = constraint->y_ref_widget;

		if (refWidget == NULL)
			return (refWidget->core.parent);
	}
}


/************************************************************************
 *
 *  XwRefTreeSearch
 *	Search the reference tree until the widget listed is found.
 *
 ************************************************************************/

static FormRef *
XwRefTreeSearch(Widget w, FormRef *formRef)
{
	int             i;
	FormRef        *tempRef;


	if (formRef == NULL)
		return NULL;
	if (formRef->c_this == w)
		return (formRef);

	for (i = 0; i < formRef->ref_to_count; i++) {
		tempRef = XwRefTreeSearch(w, formRef->ref_to[i]);
		if (tempRef != (FormRef *) NULL)
			return (tempRef);
	}

	return (NULL);
}



/************************************************************************
 *
 *  XwParentRefTreeSearch
 *	Search the reference tree until the parent reference of the 
 *      widget listed is found.
 *
 ************************************************************************/

static FormRef *
XwParentRefTreeSearch(Widget w, FormRef *wFormRef, FormRef *parentFormRef)
{
	int		i;
	FormRef        *tempRef;

	if (parentFormRef == NULL)
		return (NULL);
	if (wFormRef->c_this == w)
		return (parentFormRef);

	for (i = 0; i < wFormRef->ref_to_count; i++) {
		tempRef =
			XwParentRefTreeSearch(w, wFormRef->ref_to[i], wFormRef);
		if (tempRef != NULL)
			return (tempRef);
	}

	return (NULL);
}


/************************************************************************
 *
 *  XwAddRef
 *	Add a reference structure into a parent reference structure.
 *
 ************************************************************************/
      
static void
XwAddRef(FormRef *refParent, FormRef *ref)
{

	refParent->ref_to = (FormRef **) XtRealloc((char *)refParent->ref_to,
		sizeof (FormRef *)*(refParent->ref_to_count + 1));

	refParent->ref_to[refParent->ref_to_count] = ref;
	refParent->ref_to_count += 1;
}


/************************************************************************
 *
 *  XwRemoveRef
 *	Remove a reference structure from a parent reference structure.
 *
 ************************************************************************/
      
static void 
XwRemoveRef(FormRef *refParent, FormRef *ref)
{
	int    i,
	                j;

	for (i = 0; i < refParent->ref_to_count; i++) {
		if (refParent->ref_to[i] == ref) {
			for (j = i; j < refParent->ref_to_count - 1; j++)
				refParent->ref_to[j] = refParent->ref_to[j + 1];
			break;
		}
	}

	if (refParent->ref_to_count > 1) {
		refParent->ref_to =
			(FormRef **) XtRealloc((char *) refParent->ref_to,
			sizeof (FormRef *) * (refParent->ref_to_count - 1));

	} else {
		XtFree((char *) refParent->ref_to);
		refParent->ref_to = NULL;
	}

	refParent->ref_to_count -= 1;
}


/************************************************************************
 *
 *  XwProcessRefs
 *	Traverse throught the form's reference trees, calculate new
 *      child sizes and locations based on the constraints and adjust
 *      the children as is calculated.  The resizable flag indicates
 *      whether the form can be resized or not.
 *
 ************************************************************************/

static void 
XwProcessRefs(FormWidget fw, Boolean formResizable)
{
	int             configured = 0;
	Dimension       formWidth,
	                formHeight;
	int		i,
	                j;

	int             horDepth,
	                horLeaves;
	int             vertDepth,
	                vertLeaves;
	FormProcess   **horProcessList;
	FormProcess   **vertProcessList;

	XtGeometryResult geometryReturn;
	Dimension       replyW,
	                replyH;

	FormConstraintRec *constraintRec;
	Widget          child;
	Boolean         moveFlag,
	                resizeFlag;


	/* Initialize the form width and height variables  */

	/*
	 * if (fw -> manager.layout == XwIGNORE) formResizable = False;
	 */

	if (formResizable)
		formWidth = formHeight = (Dimension) 0;
	else {
		formWidth = fw->core.width;
		formHeight = fw->core.height;
	}


	/* Traverse the reference trees to find the depth and leaf node count  */

	leaves = 0;
	depth = 0;
	XwFindDepthAndCount(fw->form.width_tree, 1);
	horDepth = depth;
	horLeaves = leaves;

	leaves = 0;
	depth = 0;
	XwFindDepthAndCount(fw->form.height_tree, 1);
	vertDepth = depth;
	vertLeaves = leaves;

	if (horDepth == 0 && vertDepth == 0)
		return;


	/* Allocate and initialize the constraint array processing structures  */

	horProcessList =
		(FormProcess **) XtMalloc(sizeof (FormProcess **) * horLeaves);
	for (i = 0; i < horLeaves; i++) {
		horProcessList[i] =
			(FormProcess *) XtMalloc(sizeof (FormProcess) * horDepth);

		for (j = 0; j < horDepth; j++)
			horProcessList[i][j].ref = NULL;
	}


	vertProcessList =
		(FormProcess **) XtMalloc(sizeof (FormProcess **) * vertLeaves);
	for (i = 0; i < vertLeaves; i++) {
		vertProcessList[i] =
			(FormProcess *) XtMalloc(sizeof (FormProcess) * vertDepth);

		for (j = 0; j < vertDepth; j++)
			vertProcessList[i][j].ref = NULL;
	}


	/* Initialize the process array placing each node of the tree into    */
	/* the array such that it is listed only once and its first children  */
	/* listed directly next within the array.                             */

	arrayIndex = 0;
	XwInitProcessList(horProcessList, fw->form.width_tree, 0);
	arrayIndex = 0;
	XwInitProcessList(vertProcessList, fw->form.height_tree, 0);


	/* Process each array such that each row of the arrays contain  */
	/* their required sizes and locations to match the constraints  */

	XwConstrainList(horProcessList, horLeaves,
		horDepth, &formWidth, formResizable, OL_HORIZONTAL);
	XwConstrainList(vertProcessList, vertLeaves,
		vertDepth, &formHeight, formResizable, OL_VERTICAL);


	/* If the form is resizable and the form width or height returned  */
	/* is different from the current form width or height, then make   */
	/* a geometry request to get the new form size.  If almost is      */
	/* returned, use these sizes and reprocess the constrain lists     */

	if (formResizable &&
		(formWidth != fw->core.width || formHeight != fw->core.height)) {
		if (formWidth == (Dimension) 0)
			formWidth = (Dimension) 1;

		if (formHeight == (Dimension) 0)
			formHeight = (Dimension) 1;

		geometryReturn =
			XtMakeResizeRequest((Widget) fw, formWidth, formHeight, &replyW, &replyH);

		if (geometryReturn == XtGeometryAlmost) {
			formWidth = replyW;
			formHeight = replyH;

			XtMakeResizeRequest((Widget) fw, formWidth, formHeight, NULL, NULL);
			configured++;

			XwConstrainList(horProcessList, horLeaves,
				horDepth, &formWidth, False, OL_HORIZONTAL);
			XwConstrainList(vertProcessList, vertLeaves,
				vertDepth, &formHeight, False, OL_VERTICAL);
		} else if (geometryReturn == XtGeometryNo) {
			formWidth = fw->core.width;
			formHeight = fw->core.height;

			XwConstrainList(horProcessList, horLeaves,
				horDepth, &formWidth, False, OL_HORIZONTAL);
			XwConstrainList(vertProcessList, vertLeaves,
				vertDepth, &formHeight, False, OL_VERTICAL);
		} else
			configured++;
	}
	/* Process the forms child list to compare the widget sizes and  */
	/* locations with the widgets current values and if changed,     */
	/* reposition, resize, or reconfigure the child.                 */

	_OlDnDSetDisableDSClipping((Widget) fw, True);
	for (i = 0; i < fw->composite.num_children; i++) {
		child = (Widget) fw->composite.children[i];

		if (child->core.managed) {
			constraintRec = (FormConstraintRec *) child->core.constraints;

			moveFlag = resizeFlag = False;

			if (constraintRec->x != child->core.x ||
				constraintRec->y != child->core.y)
				moveFlag = True;

			if (constraintRec->width != child->core.width ||
				constraintRec->height != child->core.height)
				resizeFlag = True;

			if (moveFlag && resizeFlag)
				OlConfigureWidget(child, constraintRec->x, constraintRec->y,
					constraintRec->width, constraintRec->height,
					child->core.border_width);
			else if (moveFlag)
				OlMoveWidget(child, constraintRec->x, constraintRec->y);
			else if (resizeFlag)
				OlResizeWidget(child, constraintRec->width,
					constraintRec->height, child->core.border_width);
			if (moveFlag || resizeFlag)
				configured++;
		}
	}
	_OlDnDSetDisableDSClipping((Widget) fw, False);
	if (configured)
		OlDnDWidgetConfiguredInHier((Widget) fw);

	XwFreeConstraintList(horProcessList, horLeaves);
	XwFreeConstraintList(vertProcessList, vertLeaves);
}


/************************************************************************
 *
 *  XwFreeConstraintList
 *	Free an allocated constraint list.
 *
 ************************************************************************/

static void 
XwFreeConstraintList(FormProcess **processList, int leaves)
{
	int	i;

	/* Free each array attached to the list then free the list  */
	for (i = 0; i < leaves; i++)
		XtFree((char *) processList[i]);

	XtFree((char *)processList);
}


/************************************************************************
 *
 *  XwFindDepthAndCount
 *	Search a constraint reference tree and find the maximum depth
 *      of the tree and the number of leaves in the tree.
 *
 ************************************************************************/

static void 
XwFindDepthAndCount(FormRef *node, int nodeLevel)
{
	int             i;

	if (node->ref_to == NULL)
		leaves++;
	else {
		nodeLevel++;
		if (nodeLevel > depth)
			depth = nodeLevel;
		for (i = 0; i < node->ref_to_count; i++)
			XwFindDepthAndCount(node->ref_to[i], nodeLevel);
	}
}


/************************************************************************
 *
 *  XwInitProcessList
 *	Search a constraint reference tree and find place the ref node
 *      pointers into the list.
 *
 ************************************************************************/

static void 
XwInitProcessList(FormProcess **processList, FormRef *node, int nodeLevel)
{
	int             i;

	processList[arrayIndex][nodeLevel].ref = node;

	if (node->ref_to == NULL) {
		processList[arrayIndex][nodeLevel].leaf = True;
		arrayIndex++;
	} else {
		processList[arrayIndex][nodeLevel].leaf = False;
		nodeLevel++;
		for (i = 0; i < node->ref_to_count; i++)
			XwInitProcessList(processList, node->ref_to[i], nodeLevel);
	}
}


/************************************************************************
 *
 *  XwConstrainList
 *	Process each array such that each row of the arrays contain
 *	their required sizes and locations to match the constraints
 *
 ************************************************************************/

static void 
XwConstrainList(FormProcess **processList, int leaves, int depth, Dimension *formSize, Boolean varySize, int orient)
{
	int             i,
	                j;
	FormRef        *ref;
	FormConstraintRec *constraint;
	FormConstraintRec *parentConstraint;
	Dimension       heldSize = (Dimension) 0;
	Dimension       sizeDif;
	Dimension       vary,
	                resize;
	int             varyCount,
	                resizeCount;
	Dimension       varyAmount,
	                resizeAmount;
	Dimension       constantSubtract;
	Dimension       addAmount,
	                subtractAmount;
	Dimension       size = (Dimension) MAXSHORT,
	                separation = (Dimension) MAXSHORT;
	Dimension       Dim0 = (Dimension) 0,
	                Dim1 = (Dimension) 1;
	Position        Pos0 = (Position) 0;
	FormProcess    *rptr;		/* temporary pointer for current [i][j] */


	for (i = 0; i < leaves; i++) {	/* Process all array lines  */
		processList[i][0].size = Dim0;
		processList[i][0].loc = Pos0;


		for (j = 1; j < depth; j++) {	/* Process array line */
			rptr = &processList[i][j];
			ref = rptr->ref;

			if (ref != NULL) {
				rptr->size = ref->set_size;

				if (ref->ref == ref->c_this->core.parent) {
					if (ref->offset != Dim0)
						rptr->loc = (Position) ref->offset;
					else
						rptr->loc = ref->set_loc;
				} else {
					rptr->loc =
						processList[i][j - 1].loc + (Position) ref->offset;
					if (ref->add)
						rptr->loc += (Position) (

							(int) (processList[i][j - 1].size) +
							((int) (rptr->ref->c_this->core.border_width) * 2));
				}

			} else {
				rptr->ref = processList[i - 1][j].ref;
				rptr->loc = processList[i - 1][j].loc;
				rptr->size = processList[i - 1][j].size;
				rptr->leaf = processList[i - 1][j].leaf;
			}

			if (rptr->leaf) {
				if (processList[i][0].size < (Dimension)
					((int) (rptr->size)
						+
						((int) (rptr->ref->c_this->core.border_width) * 2)
						+
						(int) (rptr->loc) + (int) (ref->attach_offset))) {
					processList[i][0].size = (Dimension)
						((int)(rptr->size)
						+
						((int)(rptr->ref->c_this->core.border_width) * 2)
						+
						(int)(rptr->loc) + (int) (ref->attach_offset));
				}
				if (rptr->leaf && processList[i][0].size > heldSize)
					heldSize = processList[i][0].size;

				break;
			}
		}
	}


	/* Each array line has now been processed to optimal size.  Reprocess  */
	/* each line to constrain it to formSize if not varySize or to         */
	/* heldSize if varySize.                                               */

	if (varySize)
		*formSize = heldSize;



	for (i = 0; i < leaves; i++) {

		/*
		 * For each array line if the 0th size (calculated form size
		 * needed
		 */

		/*
		 * for this array line is less than the form size then increase
		 * the
		 */
		/* seperations between widgets whose constaints allow it.            */

		if (processList[i][0].size < *formSize) {
			sizeDif = *formSize - processList[i][0].size;

			varyCount = 0;
			if (processList[i][1].leaf != True) {
				for (j = 2; j < depth; j++) {	/* Can't vary the first
								 * spacing  */
					if (processList[i][j].ref->vary)
						varyCount++;
					if (processList[i][j].leaf)
						break;
				}
			}
			addAmount = Dim0;
			if (varyCount == 0)
				varyAmount = Dim0;
			else
				varyAmount = (Dimension) (((int) sizeDif) / varyCount);

			j = 1;

			while (j < depth) {
				if (j > 1 && processList[i][j].ref->vary)
					addAmount += varyAmount;
				processList[i][j].loc += (Position) addAmount;

				if (processList[i][j].leaf)
					break;

				j++;
			}

			if (j > 1) {

				rptr = &processList[i][j];

				if (rptr->ref->vary && rptr->ref->attach)
					rptr->loc = (Position) (*formSize - rptr->size -
						rptr->ref->attach_offset);
				else if (rptr->ref->vary == False &&
						rptr->ref->resizable &&
					rptr->ref->attach) {
					rptr->size = *formSize
						- ((Dimension) rptr->loc
						+ (Dimension) ((int) (rptr->ref->c_this->core.border_width) * 2)
						+ rptr->ref->attach_offset);
				}

				/*
				 * If last child cannot be resized or Varied,
				 * but needs to be attached to the right , then
				 * lets try resizing any of the other children
				 * ...  	-JMK
				 */
				else if (rptr->ref->attach) {
					int             index;
					int             diff =
					(Position) (*formSize - rptr->size - rptr->ref->attach_offset) -
					(Position) rptr->loc;

					index = j;

#ifdef	DEBUG
#undef	NDEBUG
					assert(diff >= 0);	/* Should NEVER happen ! */
#endif	/* DEBUG */

					j--;
					while (j) {
						rptr = &processList[i][j];
						if (rptr->ref->resizable) {
							while (index > j) {
								processList[i][index].loc += diff;
								index--;
							}
							rptr->size += diff;
							break;
						} else {
							j--;
						}
					}
				}
			} /* end of if(j>1) */ 
			else {
				rptr = &processList[i][j];
				if (rptr->ref->vary == False &&
					rptr->ref->resizable &&
					rptr->ref->attach) {
					rptr->loc = (Position) rptr->ref->offset;
					rptr->size = *formSize
						- ((Dimension) rptr->loc
						+ (Dimension) ((int) (rptr->ref->c_this->core.border_width) * 2)
						+ rptr->ref->attach_offset);

				} else if (rptr->ref->vary &&
					rptr->ref->attach) {
					rptr->loc = (Position) (*formSize
						- (rptr->size
							+ (Dimension) ((int) (rptr->ref->c_this->core.border_width) * 2)
							+ rptr->ref->attach_offset));
				} else if (rptr->ref->vary &&
					rptr->ref->attach == False) {
					rptr->loc = (Position) rptr->ref->offset;
				}
			}	/* end of !if(j>1) */
		}

		/*
		 * If the form size has gotten smaller, process the vary
		 * constraints
		 */

		/*
		 * until the needed size is correct or all seperations are 1
		 * pixel.
		 */
		/* If separations go to 1, then process the resizable widgets        */
		/* until the needed size is correct or the sizes have gone to 1      */
		/* pixel.  If the size is still not correct punt, cannot find a      */
		/* usable size so clip it.                                           */

		if (processList[i][0].size > *formSize) {
			sizeDif = processList[i][0].size - *formSize;

			varyAmount = Dim0;
			varyCount = 0;

			j = 0;
			do {
				j++;

				if (j > 1 && processList[i][j].ref->vary &&
					processList[i][j].ref->offset) {
					varyAmount += processList[i][j].ref->offset;
					varyCount++;
				}
			}
			while (processList[i][j].leaf == False);


			resizeAmount = Dim0;
			resizeCount = 0;
			for (j = 1; j < depth; j++) {
				if (processList[i][j].ref->resizable && processList[i][j].size
					> Dim1) {
					if (processList[i][j].leaf || processList[i][j + 1].ref->add) {
						resizeCount++;
						resizeAmount +=
							(Dimension) ((int) (processList[i][j].size) - 1);
					}
				}
				if (processList[i][j].leaf)
					break;
			}


			/* Do we have enough varience to match the constraints?  */

			if (((Dimension) (varyAmount + resizeAmount)) > sizeDif) {

				/* first process out the vary amount  */

				if (varyCount) {
					do {
						subtractAmount = Dim0;

						for (j = 1; j < depth; j++) {
							if (j > 1 && processList[i][j].ref->vary) {
								vary = (Dimension)
									(processList[i][j].loc - processList[i][j - 1].loc);

								if (processList[i][j].ref->add)
									vary = (Dimension) (

										(int) (vary - processList[i][j - 1].size)
										- (1
											+ (int)
											(processList[i][j - 1].ref->c_this->core.border_width) * 2));
							} else
								vary = Dim0;

							if (vary > Dim0)
								subtractAmount++;

							if (subtractAmount) {
								processList[i][j].loc -= (Position) subtractAmount;
								sizeDif--;
								/*
								 * This problem is similar to the one down below on line #1884 in that sizeDif
								 * can overflow(since unsigned) within this for loop. So we check ahead and
								 * break.
								 */

								if (sizeDif == Dim0)
									break;
							}
							if (processList[i][j].leaf)
								break;
						}
					}
					while (subtractAmount != Dim0 && sizeDif > Dim0);
				}

				/*
				 * now process resize constraints if further
				 * constraint
				 */
				/* processing is necessary.                             */

				if (sizeDif) {
					if (resizeAmount > sizeDif)
						resizeAmount = sizeDif;
					if (resizeCount)
						constantSubtract = (Dimension)
							(((int) (resizeAmount)) / resizeCount);

					while (resizeAmount > Dim0) {
						subtractAmount = Dim0;

						for (j = 1; j < depth; j++) {
							if (processList[i][j].ref->add)
								processList[i][j].loc -= (Position) subtractAmount;

							if (processList[i][j].ref->resizable && resizeAmount) {
								if (processList[i][j].leaf ||
									processList[i][j + 1].ref->add)
									resize = (Dimension)
										((int) (processList[i][j].size) - 1);
								else
									resize = Dim0;

								if (resize > Dim1) {
									if (constantSubtract < resize)
										resize = constantSubtract;
									subtractAmount += resize;
									processList[i][j].size -= resize;
									/*
									 * This is a fix for bug #1050774. This is because During this loop it may
									 * happen that the original amount of space that is to be distributed maybe
								  	 * less than the # of children, in which case subtractAmount becomes greater
									 * than resizeAmount and hence resizeAmount overflows(unsigned). So the
									 * comparison above while (resizeAmount > Dim0) succeeds always, making this
									 * an infinite loop
									 */
									resizeAmount -= resize;
								}
							}
							if (processList[i][j].leaf)
								break;
						}
						constantSubtract = Dim1;
					}
				}
			}
		}
	} /* end of i loop */



	/* Now each array line is processed such that its line is properly  */
	/* constrained to match the specified form size.  Since a single    */
	/* widget reference structure can be referenced in multiple array   */
	/* lines, the minumum constraint for each widget needs to be found. */
	/* When found, the width and height will be placed into the widgets */
	/* constraint structure.                                            */

	for (i = 1; i < depth; i++) {
		ref = NULL;

		for (j = 0; j < leaves + 1; j++) {	/* loop one too many -
							 * for exit */
			if(j < leaves)
				rptr = &processList[j][i];

			if (j == leaves || ref != rptr->ref) {
				if (j == leaves || ref != NULL) {
					if (ref != NULL) {
						constraint =
							(FormConstraintRec *) ref->c_this->core.constraints;
						parentConstraint =
							(FormConstraintRec *) ref->ref->core.constraints;

						if (orient == OL_HORIZONTAL)
							constraint->width = size;
						else
							constraint->height = size;

						/************************* CHECK THIS CODE  *********************/

						if (i > 1) {
							if (orient == OL_HORIZONTAL)

								/*
								 * POSSIBLE
								 * BUGGGGG:
								 * Don't
								 * understand
								 * why
								 */

								/*
								 * this is
								 * done, will
								 * overflow and
								 * this
								 */

								/*
								 * value is
								 * never used.
								 */
								constraint->x = parentConstraint->x + 
									(Position)separation;
							else

								/*
								 * POSSIBLE
								 * BUGGGGG:
								 * Don't
								 * understand
								 * why
								 */

								/*
								 * this is
								 * done, will
								 * overflow and
								 * this
								 */

								/*
								 * value is
								 * never used.
								 */
								constraint->y = parentConstraint->y + 
									(Position)separation;
						} else {
							if (orient == OL_HORIZONTAL)
								constraint->x = (Position)separation;
							else
								constraint->y = (Position)separation;
						}
						/************************ CHECK THIS CODE */
					}
					if (j == leaves)
						break;	/* exit out of the inner loop  */
				}
				ref = rptr->ref;
				separation = (Dimension) MAXSHORT;
				size = (Dimension) MAXSHORT;
			}
			if (ref != NULL) {
				if (size > rptr->size)
					size = rptr->size;

				if (i > 1) {
					if (separation > (Dimension)(rptr->loc - processList[j][i - 1].loc))
						separation = (Dimension)rptr->loc - processList[j][i - 1].loc;
				} else if (separation > (Dimension)rptr->loc)
					separation = (Dimension)rptr->loc;
			}
		} /* end j loop */
	} /* end i loop */
}

/* 
 *	When a child is destroyed, we need to remove references to it
 *  by other widgets thru their x_ref_widget/y_ref_widget fields.
 *  We do this by searching thru the width/height trees of the Form
 *  	Search each hierarchy in the tree. If we find a match at 
 *  any one level, we can stop after completing that level, since 
 *  we know that all widgets referencing the dead widget will be
 *  on the same level ...
 */
static Boolean
FixXRefWidget(FormRef *root, Widget dead_widget)
{
	Boolean found = False;
	int count = root->ref_to_count;
	Widget form = XtParent(dead_widget);
	FormConstraintRec *constraint;
	int i;

	if (form->core.being_destroyed)
	    return True;

	/* Step-1: Search the current level. Note that the
	 * x_ref_widget of every widget in a level need NOT be
	 * the same ... since the x_ref_widget of some widgets 
	 * could be unmanaged ...
	 */
	for (i = 0; i < count; i++) {
		constraint = root->ref_to[i]->c_this->core.constraints;
		if (constraint->x_ref_widget == dead_widget) {
			found = True;
			constraint->x_ref_widget = root->ref_to[i]->ref;
			XtFree( constraint->x_ref_name);
			constraint->x_ref_name = Strdup(XtName(root->ref_to[i]->ref));
		}
	}

	/* If we found a match, it implies that all widgets that referenced
	 * the dead widget are present in this level ... and  have been
	 * taken care of . So finis.
	 */
	if (found) 
		return True;
			
	/* Step-2: No match in this level, Recurse ... */
	for (i = 0; i < count; i++) {
		if (FixXRefWidget(root->ref_to[i], dead_widget))
			return True;
	}

	return False;
}

static Boolean
FixYRefWidget(FormRef *root, Widget dead_widget)
{
	Boolean found = False;
	int count = root->ref_to_count;
	Widget form = XtParent(dead_widget);
	FormConstraintRec *constraint;
	int i;

	if (form->core.being_destroyed)
	    return True;

	for (i = 0; i < count; i++) {
		constraint = root->ref_to[i]->c_this->core.constraints;
		if (constraint->y_ref_widget == dead_widget) {
			found = True;
			constraint->y_ref_widget = root->ref_to[i]->ref;
			XtFree( constraint->y_ref_name);
			constraint->y_ref_name = Strdup(XtName(root->ref_to[i]->ref));
		}
	}

	if (found) 
		return True;
			
	for (i = 0; i < count; i++) {
		if (FixYRefWidget(root->ref_to[i], dead_widget))
			return True;
	}

	return False;
}
