#pragma ident	"@(#)FNonexclus.c	302.3	97/03/26 lib/libXol SMI"	/* flat:FNonexclus.c 1.12	*/

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


#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookP.h>
#include <Xol/FNonexcluP.h>


static XtResource
resources[] = {
	{ XtNexclusives, XtCExclusives, XtRBoolean, sizeof(Boolean),
	  XtOffsetOf(FlatNonexclusivesRec, exclusives.exclusive_settings),
	  XtRImmediate, (XtPointer) False },
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

FlatNonexclusivesClassRec
flatNonexclusivesClassRec = {
    {
	(WidgetClass)&flatExclusivesClassRec,	/* superclass		*/
	"FlatNonexclusives",			/* class_name		*/
	sizeof(FlatNonexclusivesRec),		/* widget_size		*/
	NULL,					/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	NULL,					/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	(Cardinal)0,				/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULL,					/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
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
	NULL,					/* callback_offsets	*/
	XtInheritTranslations,			/* tm_table		*/
	NULL,					/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
    }, /* End of Core Class Part Initialization */
    {
	NULL,					/* reserved1		*/
	XtInheritHighlightHandler,		/* highlight_handler	*/
	XtInheritTraversalHandler,		/* traversl_handler	*/
	NULL,					/* register_focus	*/
	XtInheritActivateFunc,			/* activate		*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	OlVersion,				/* version		*/
	(XtPointer)NULL,			/* extension		*/
	{
		(_OlDynResourceList)NULL,	/* resources		*/
		(Cardinal)0			/* num_resources	*/
	},					/* dyn_data		*/
	XtInheritTransparentProc,		/* transparent_proc	*/
	XtInheritSuperCaretQueryLocnProc,	/* query_sc_locn_proc	*/
    },	/* End of Primitive Class Part Initialization */
    {
	NULL,					/* item_resources	*/
	(Cardinal)0,				/* num_item_resources	*/
	NULL,					/* required_resources	*/
	(Cardinal)0,				/* num_required_resources*/
	NULL,					/* quarked_items	*/
	(Cardinal)0,				/* part_size		*/
	(Cardinal)0,				/* part_offset		*/
	(Cardinal)0,				/* part_in_rec_offset	*/
	sizeof(FlatExclusivesItemRec),		/* rec_size		*/
	True,					/* transparent_bg	*/
	(OlFlatAnalyzeItemsProc)NULL,		/* analyze_items	*/
	XtInheritFlatDrawItem,			/* draw_item		*/
	(OlFlatExpandItemProc)NULL,		/* expand_item		*/
	XtInheritFlatGetDrawInfo,		/* get_draw_info	*/
	XtInheritFlatGetIndex,			/* get_index		*/
	XtInheritFlatItemAcceptFocus,		/* item_accept_focus	*/
	XtInheritFlatItemActivate,		/* item_activate	*/
	XtInheritFlatItemDimensions,		/* item_dimensions	*/
	(OlFlatItemGetValuesProc)NULL,		/* item_get_values	*/
	XtInheritFlatItemHighlight,		/* item_highlight	*/
	(OlFlatItemInitializeProc)NULL,		/* item_initialize	*/
	(OlFlatItemSetValuesFunc)NULL,		/* item_set_values	*/
	(OlFlatItemsTouchedProc)NULL,		/* items_touched	*/
	XtInheritFlatLayoutClass,		/* layout_class		*/
	XtInheritFlatLayout,			/* layout		*/
	XtInheritFlatTraverseItems,		/* traverse_items	*/
	(XtPointer)NULL,			/* reserved2		*/
	(XtPointer)NULL,			/* reserved1		*/
	(XtPointer)NULL				/* extension		*/
    }, /* End of Flat Class Part Initialization */
    {
	True					/* allow_class_default	*/
    }, /* End of FlatExclusives Class Part Initialization */
    {
	NULL					/* no_class_fields	*/
    } /* End of FlatNonexclusives Class Part Initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass flatNonexclusivesWidgetClass =
			(WidgetClass) &flatNonexclusivesClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/* There are no private procedures */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/* There are no class procedures */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/* There are no action procedures */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/* There are no public procedures */
