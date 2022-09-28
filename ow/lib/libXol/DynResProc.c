#pragma ident	"@(#)DynResProc.c	302.3	92/08/26 lib/libXol SMI"	/* olmisc:DynResProc.c 1.9	*/

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


#include <ctype.h>
#include <libintl.h>
#include <stdio.h>
#include <string.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include <Xol/Converters.h>
#include <Xol/DynamicP.h>
#include <Xol/EventObjP.h>
#include <Xol/ManagerP.h>
#include <Xol/OlClients.h>
#include <Xol/OpenLookP.h>
#include <Xol/PrimitiveP.h>
#include <Xol/RootShell.h>	/* multiple display support */
#include <Xol/Util.h>
#include <Xol/VendorI.h>


/* MACROS */
#define RESOURCE_STACK_STEP_SIZE	10


/* GLOBAL DATA */
/* Dynamic-OL-color-processing-enable flag */
Boolean _OlDoDynResProcessing = True;

/* LOCAL DATA */
static XtArgVal *bot_stack_ptr1 = NULL;	/* holds current resource values */
static XtArgVal *bot_stack_ptr2 = NULL;	/* holds new resource values */
static String   *name_stack     = NULL;	/* holds UNCOMPILED resource names */
static XtResourceList res_list = NULL;	/* holds resource list to be checked */
static int stack_size = 0;		/* same size for both stacks */
static ArgList args = NULL;

/* LOCAL ROUTINES */
static void ProcessWidgetNode (
	Widget		w
);

/*
 * _OlDynResProc: This function processes dynamic changes of colors.
 */
void
_OlDynResProc (Screen *screen)
{
 	/*
	 * This is used to prevent recursion. Can't use DynResProcessing
	 * here, because it may be turned off temporarily before calling
	 * application callbacks.
	 */
	static Boolean door = False;

	Widget		ssw;
	Widget		*shptr;
	Widget		w;
	int		i;
	Cardinal	num;
	Arg		args[3];
	Boolean		doing_dynamic;

	/* check for recursion */
	if (door == True) {
		OlWarning(dgettext(OlMsgsDomain,
			"Recursive use of DynResProc()\n"));
		door = False;
		return;
	}

	door = True;

	ssw = _OlGetScreenShellOfScreen(screen);

	if (ssw == (Widget)NULL) return;

	XtSetArg(args[0], XtNshells, &shptr);
	XtSetArg(args[1], XtNnumShells, &num);
	XtSetArg(args[2], XtNdoingDynamicResProcessing, &doing_dynamic);

	XtGetValues(ssw, args, XtNumber(args));

	if (!doing_dynamic) return;

	/* loop through each shell on this screen */
	for (i=0; i < (int)num; i++) {
		ProcessWidgetNode(shptr[i]);
	} /* for each shell */

	door = False;
} /* DynResProc() */

static _OlDynData *
_OlGetDynResList (Widget w)
{
	WidgetClass wc_special = _OlClass(w);
	WidgetClass wc = XtClass(w);

	if (wc_special == primitiveWidgetClass) {
		return(&((PrimitiveWidgetClass)wc)->primitive_class.dyn_data);
	}
	else if (wc_special == managerWidgetClass) {
		return(&((ManagerWidgetClass)wc)->manager_class.dyn_data);
	}
	else if (wc_special == eventObjClass) {
		return(&((EventObjClass)wc)->event_class.dyn_data);
	}
	else if (wc_special == vendorShellWidgetClass) {
		OlVendorClassExtension ext = _OlGetVendorClassExtension(wc);
		return(ext ? &(ext->dyn_data) : NULL);
	}
	else
		return(NULL);
} /* _OlGetDynResList() */

OlTransparentProc
_OlGetTransProc (Widget w)
{
	WidgetClass wc_special = _OlClass(w);
	WidgetClass wc = XtClass(w);

	if (wc_special == primitiveWidgetClass) {
	return(((PrimitiveWidgetClass)wc)->primitive_class.transparent_proc);
	}
	else if (wc_special == managerWidgetClass) {
		return(((ManagerWidgetClass)wc)->manager_class.transparent_proc);
	}
	else if (wc_special == eventObjClass) {
		return(((EventObjClass)wc)->event_class.transparent_proc);
	}
	else if (wc_special == vendorShellWidgetClass) {
		OlVendorClassExtension ext = _OlGetVendorClassExtension(wc);
		return(ext ? (ext->transparent_proc) : NULL);
	}
	else
		return(NULL);
} /* _OlGetTransProc() */

static void
ProcessWidgetNode (Widget w)
{
	int i;
	int background_changed = 0;	/* for transparency processing */
	Pixel bg_pixel;
	OlTransparentProc proc;
{
/*
 * sub block: The purpose of this subblock is so that none of the following
 * 	automatic variables sit on the stack as we traverse down the tree.
 */

	int j;
	int num_res;		/* # of resources to be checked */
	_OlDynData *list;	/* ptr to the list of dynamic res. */
	XtArgVal *sp1;		/* stack ptr into stack 1 */
	XtArgVal *sp2;		/* stack ptr into stack 2 */
	XtResource *res;	/* resource ptr into res_list */
	_OlDynResource *rptr;	/* resource ptr */
	

	/* get list of dynamic resources */
	list = _OlGetDynResList(w);

	if (list && (list->num_resources > 0)) {
		/* get the # of resources to be checked */
		num_res = list->num_resources;

		if (num_res > 0) {
		/* check stack size */
		if (stack_size < num_res) {
			stack_size = MAX(num_res,
					 stack_size + RESOURCE_STACK_STEP_SIZE);
			bot_stack_ptr1 = (XtArgVal *)
			    XtRealloc((char *)bot_stack_ptr1,
					stack_size * sizeof(XtArgVal));
			bot_stack_ptr2 = (XtArgVal *)
			    XtRealloc((char *)bot_stack_ptr2,
					stack_size * sizeof(XtArgVal));
			res_list       = (XtResourceList)
			    XtRealloc((char *)res_list,
					stack_size * sizeof(XtResource));
			args = (ArgList)
			    XtRealloc((char *)args,stack_size*sizeof(Arg));
			if ((bot_stack_ptr1 == NULL) || (res_list == NULL) ||
			    (bot_stack_ptr2 == NULL) || (args == NULL))
				OlError(dgettext(OlMsgsDomain,
					"DynResProc: Not enough memory"));
		}

		/* clear stacks */
		memset((XtPointer)bot_stack_ptr1,
		       0,
		       stack_size * sizeof(XtArgVal));
		memset((XtPointer)bot_stack_ptr2,
		       0,
		       stack_size * sizeof(XtArgVal));

		/* copy resources into ArgList and res_list */
		for (i=0, rptr=list->resources, sp1=bot_stack_ptr1,
			 sp2=bot_stack_ptr2, j=0, res=res_list;
			 i < list->num_resources;
			 i++, rptr++) {
			if (!_OlGetDynBit(w, rptr)) {
				/* check dirty bit here */
				XtSetArg(args[j], rptr->res.resource_name, sp1);
				j++; sp1++;
				*res = rptr->res;
				res->resource_offset = (char *)sp2 -
							 (char *)bot_stack_ptr2;
				res++;
				sp2++;
			}
		} /* for */
		/* now j holds the # of resources to be checked */

		/* get the current resource values */
		XtGetValues(w, args, j);

		/* get new resource values */
		XtGetSubresources(w, bot_stack_ptr2, NULL, NULL, res_list,
				j, NULL, 0);
		num_res = j;

		/* compare new and old */
		/* Note: can probably do a quick check by memcmping the entire
		   stack, but then make sure the stacks were memset to 0 in
		   the beginning.
		 */
		for (i=0, sp1=bot_stack_ptr1, sp2=bot_stack_ptr2, res=res_list,
			j=0, rptr=list->resources;
			i < num_res; i++, sp1++, sp2++, res++, rptr++) {
			if (memcmp((XtPointer)sp1,
				   (XtPointer)sp2,
				   res->resource_size)) {
				/*
				 * If background changed, remember it. So
				 * we can do tranparency processing later.
				 */
				if (!background_changed &&
				   (!strcmp(args[i].name,
					 XtNbackground))) {
					background_changed = 1;
					bg_pixel = (Pixel)*sp2;
				}

				/* Note the (re)use of args[i].name */
				XtSetArg(args[j], 
					args[i].name, *sp2);
				j++;
			} /* if */
		} /* for */

		if (j > 0)
			XtSetValues(w, args, j);
		} /* num_res > 0 */

	} /* list && (list->num_resources > 0) */
	
} /* sub block */

	if (XtIsComposite(w)) {
		/* traverse down the tree */
		WidgetList child = ((CompositeWidget)w)->composite.children;

		for (i=((CompositeWidget)w)->composite.num_children; i > 0;
			 i--,child++) {
			/*
			 * ProcessWidgetNode() must be called first.
			 * Because TransparentProc may change the resource
			 * values. Thus, for example, checkbox manager will
			 * not get a setvalue call.
			 */
			ProcessWidgetNode(*child);
			if (background_changed &&
			    (proc = _OlGetTransProc(*child)))
				(*proc)(*child, bg_pixel, XtUnspecifiedPixmap);
		}
	}
} /* ProcessWidgetNode() */

int
_OlGetDynBit (Widget w, _OlDynResourceList res)
{
	if (res->bit_offset == 0)
		return(1);
	else {
		char *base;

		if (res->proc)
			base = (res->proc)(w, FALSE, res);
		else
			base = (char *)w;
		if (base == NULL)
			return(1);
		else
			return(*(base + res->offset) & res->bit_offset);
	}
}

void
_OlSetDynBit (Widget w, _OlDynResourceList res)
{
	if (res->bit_offset) {
		char *base;

		if (res->proc)
			base = (res->proc)(w, FALSE, res);
		else
			base = (char *)w;
		if (base == NULL)
			return;

		*(base + res->offset) |= res->bit_offset;
	}
}

void
_OlUnsetDynBit (Widget w, _OlDynResourceList res)
{
	if (res->bit_offset) {
		char *base;

		if (res->proc)
			base = (res->proc)(w, FALSE, res);
		else
			base = (char *)w;
		if (base == NULL)
			return;

		*(base + res->offset) &= ~(res->bit_offset);
	}
}

/*
 *************************************************************************
 * _OlInitDynResources() initializes the dirty bits of all the dynamic
 * resources, including subclasses. Note that is actually zero out the
 * entire byte (char), not just the specified bit. This routine must be
 * called before calling _OlGetDynBit(), _OlSetDynBit(), or _OlUnsetDynBit(),
 * because this routine calls the OlBaseProc with the init flag set to TRUE.
 * All the other routines have the init flag set to FALSE.
 ****************************procedure*header*****************************
 */
void
_OlInitDynResources (Widget w, _OlDynData *data)
{
	_OlDynResourceList dynres;
	int i;
	char *base;

	for (i=0, dynres=data->resources; i < data->num_resources;
		 i++, dynres++) {
		if (dynres->bit_offset) {
			if (dynres->proc)
				base = (dynres->proc)(w, TRUE, dynres);
			else
				base = (char *)w;
			if (base)
				*(base + dynres->offset) = 0;
		}
	}
}

/*
 *************************************************************************
 * _OlCheckDynResources() checks the arglist against the dyn resource
 * list. If there is a match, the set the dirty bit associated with the
 * dynamic resource.
 ****************************procedure*header*****************************
 */
void
_OlCheckDynResources (Widget w, _OlDynData *data, ArgList args, Cardinal num_args)
{
	_OlDynResourceList dynres;
	int i, j;

	if (_OlDynResProcessing == FALSE) {
	
	for (i=num_args; i > 0; i--, args++) {
		for (j=0, dynres=data->resources; j < data->num_resources;
			 j++, dynres++) {
			if (!_OlGetDynBit(w, dynres) &&
			    !strcmp(dynres->res.resource_name, args->name))
				_OlSetDynBit(w, dynres);
		}
	} /* for i */
	} /* if */
}

/*
 *************************************************************************
 * This function merges the "new" dynamic resource list into "old"
 * and puts the new list into "new".
 ****************************procedure*header*****************************
 */
void
_OlMergeDynResources (new, old)
_OlDynData *	 new;
_OlDynData *	 old;
{
	/* merge the two lists */
	int i,j;
	_OlDynResourceList o_rsc;
	_OlDynResourceList rsc;
	_OlDynResourceList new_rsc;
	_OlDynResourceList old_rsc;

	o_rsc = rsc = (_OlDynResourceList) XtMalloc(sizeof(_OlDynResource) *
			(new->num_resources + old->num_resources));
		
	/* start with the pc list */
	memcpy ((XtPointer)rsc, (XtPointer)(new->resources),
 		new->num_resources * sizeof(_OlDynResource));
	for (i=0, rsc += new->num_resources, old_rsc=old->resources;
		 i < old->num_resources; i++, old_rsc++ ) {
		for (j=0, new_rsc=new->resources; j < new->num_resources;
			 j++, new_rsc++ ) {
			if (!strcmp(new_rsc->res.resource_name,
				    old_rsc->res.resource_name))
				break;
		}

		if (j == new->num_resources) {
			/* a resource that is in sc, but not being
			   overridden by pc */
			*rsc++ = *old_rsc;
		}
	} /* for (i=0... */

	new->resources = o_rsc;
	new->num_resources =(int)(rsc - o_rsc);
}

void
_OlDefaultTransparentProc (Widget w, Pixel pixel, Pixmap pixmap)
{
	XSetWindowAttributes wattr;
	unsigned long valuemask;
	Display *display;
	Window window;
	

	if (!XtIsRealized(w))
		return;
	display = XtDisplayOfObject(w);
	window = XtWindowOfObject(w);

	if (pixmap != XtUnspecifiedPixmap) {
		wattr.background_pixmap = pixmap;
		valuemask = CWBackPixmap;
		w->core.background_pixmap = pixmap;
	}
	else {
		wattr.background_pixel = pixel;
		valuemask = CWBackPixel;
		w->core.background_pixel = pixel;
	}

	XChangeWindowAttributes(display, window, valuemask, &wattr);
	XClearArea(display, window, 0, 0, 0, 0, TRUE);
	
	if (XtIsSubclass(w, managerWidgetClass)) {
		int i;
		WidgetList child = ((CompositeWidget)w)->composite.children;
		OlTransparentProc proc;

		for (i=((CompositeWidget)w)->composite.num_children; i > 0;
		 	i--,child++)
			if (proc = _OlGetTransProc(*child))
				(*proc)(*child, pixel, 
				   (pixmap == XtUnspecifiedPixmap) ?
				      XtUnspecifiedPixmap : ParentRelative);
    	}
}


/* Convenience routine which Gets the real pixmap
 *  (ie not ParentRelative) of w's parent
 */

Pixmap 
_OlGetRealPixmap (Widget w)
{
	w = XtParent(w);

	while (w != NULL) {
		if (w->core.background_pixmap != ParentRelative)
			return (w->core.background_pixmap);
		else
			w = XtParent(w);
	}
	return XtUnspecifiedPixmap;
}

