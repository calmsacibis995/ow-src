#pragma ident	"@(#)FlatState.c	302.3	92/04/29 lib/libXol SMI"	/* flat:FlatState.c 1.26	*/

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
 *	This file contains the routines that expand a flat container's
 *	sub-objects and routines that do Set and Get values on the 
 *	sub-objects.
 *
 ******************************file*header********************************
 */


#include <stdarg.h>
#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ConstrainP.h>

#include <Xol/Error.h>
#include <Xol/FlatP.h>
#include <Xol/OpenLookP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public Procedures 
 *
 **************************forward*declarations***************************
 */
					/* private procedures		*/

static void	BuildExtractorList (Widget, FlatPart *,
						FlatClassPart*);
static void	BuildRequiredList (Widget, FlatPart *,
						FlatClassPart*);
static FlatItem CacheManager(Widget w, Cardinal item_index, FlatItem item, int action);		/* manages cached items		*/
static Boolean	CallItemProcs (WidgetClass, int, Widget, FlatItem,
				FlatItem, FlatItem, ArgList, Cardinal *);
static void	CallItemsTouched (WidgetClass, Widget, Widget, Widget,
					ArgList, Cardinal *);
static void	ClassExpandItem (WidgetClass,Widget,Cardinal,char*);
static void	ExpandItem (Widget, FlatPart *, FlatClassPart *,
					Cardinal, FlatItem);
static void	GetItemState(Widget w, FlatClassPart *fcp, Cardinal item_index, ArgList args, Cardinal *num_args);		/* queries an item's field	*/
static void	InitializeReqResources (Widget, FlatPart *,
						FlatClassPart *);
static void	StoreItemValues (FlatPart *, FlatClassPart *,
						Cardinal, FlatItem);
static void	SetItemState(Widget w, FlatPart *fp, FlatClassPart *fcp, Cardinal item_index, ArgList args, Cardinal *num_args);		/* updates an item's field	*/
		

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */
				/* Declare some private Xt externs	*/

extern void	_XtCountVaList (va_list, int *, int *);
extern void	_XtVaToArgList (Widget, va_list, int,
						ArgList*, Cardinal*);

#define	LOOKUP			1
#define	ADD_REFERENCE		2
#define	DEC_REFERENCE		3

#define	CALL_ITEM_INITIALIZE	2
#define CALL_ANALYZE_ITEMS	3
#define	CALL_ITEM_SETVALUES	4
#define	CALL_ITEM_GETVALUES	5

#define IGNORE_FIELD		(~0)

#define RSC_INDEX(f, i)		((f)->required_resources[i].rsc_index)
#define	RSC_QNAME(f,i)		((f)->quarked_items[i])
#define	RSC_SIZE(f,i)		((f)->item_resources[i].resource_size)
#define	RSC_OFFSET(f,i)		((f)->item_resources[i].resource_offset)

#define FLATCLASS(wc)	(((FlatWidgetClass)(wc))->flat_class)
#define FPART(w)	(&((FlatWidget)(w))->flat)
#define FCPART(w)	(&FLATCLASS(XtClass(w)))

#define STACK_SIZE	512
#define FREE_STACK(ptr,stk_ptr)	\
	if ((XtPointer)ptr != (XtPointer)stk_ptr) XtFree((XtPointer)ptr)

#define W_CNAME(w)	(XtClass(w)->core_class.class_name)

			/* Define a structure to cache items	*/

typedef struct _ItemCache {
	struct _ItemCache *	next;		/* next node		*/
	Cardinal		ref_count;	/* reference count	*/
	Cardinal		item_index;	/* index of item	*/
	FlatItem		item;		/* cached item		*/
} ItemCache;

typedef struct _WidgetCache {
	struct _WidgetCache *	next;		/* next node		*/
	Widget			w;		/* widget owning items	*/
	ItemCache *		i_root;		/* root of cache list	*/
} WidgetCache;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************private*procedures***************************
 */

/*
 *************************************************************************
 * BuildExtractorList - the procedure builds an array of information that
 * will be used to expand the sub-object item.
 ****************************procedure*header*****************************
 */
static void
BuildExtractorList (Widget w, FlatPart *fp, FlatClassPart *fcp)
{
	Cardinal		count;	/* number of fields for this part*/
	register Cardinal	i;	/* loop counter			*/
	register Cardinal	n;	/* extractor field index	*/
	register String *	item_fields = fp->item_fields;
	register XrmQuark	quark;	/* temporary quark holder	*/
	XrmQuarkList		qlist;
	Cardinal *		xlist;	/* extractor list		*/

					/* Free the old extractor list	*/

	XtFree((char *) fp->resource_info.xlist);
	fp->resource_info.xlist		= (Cardinal *)NULL;
	fp->resource_info.num_xlist	= (Cardinal)0;

	if (fp->num_item_fields == (Cardinal)0 ||
	    fp->num_items == (Cardinal)0 ||
	    fp->items == (XtPointer)NULL)
	{
		return;
	}

			/* Allocate a large enough array to hold all
			 * application-specified fields			*/

	xlist = (Cardinal *) XtMalloc((Cardinal)
				(fp->num_item_fields * sizeof(Cardinal)));

					/* Build the Extractor list	*/

	for (n=0, count=0; n < fp->num_item_fields; ++n, ++item_fields)
	{
		quark = XrmStringToQuark(*item_fields);
		qlist = fcp->quarked_items;

		for (i=0; i < fcp->num_item_resources; ++i, ++qlist)
		{
		    if (quark == *qlist)
		    {
			xlist[count] = i;
			++count;
			break;
		    }
		}

		if (i == fcp->num_item_resources)
		{
			char *	resource = (item_fields != (String *)NULL &&
					    *item_fields != (String)NULL ?
					    *item_fields : "");

			OlVaDisplayWarningMsg(XtDisplay(w),
				OleNinvalidResource, OleTbadItemResource,
				OleCOlToolkitWarning,
				OleMinvalidResource_badItemResource,
				XrmQuarkToString(w->core.xrm_name),
				XtClass(w)->core_class.class_name,
				resource);

				/* Add a dummy resource to skip the
				 * bad field.				*/

			xlist[count] = (Cardinal) IGNORE_FIELD;
			++count;
		}
	}

		/* If this list contains less elements than we allocated,
		 * reallocate it					*/

	if (count < n)
	{
		if (count == (Cardinal)0)
		{
			XtFree((XtPointer)xlist);
			xlist = (Cardinal *)NULL;
		}
		else
		{
			xlist = (Cardinal *) XtRealloc((char *)xlist,
					(count * sizeof(Cardinal)));
		}
	}

	fp->resource_info.xlist		= xlist;
	fp->resource_info.num_xlist	= count;
} /* END OF BuildExtractorList() */

/*
 *************************************************************************
 * BuildRequiredList - this procedure builds an array of information that
 * contains indices of resources that a sub-object requires so that
 * it is initialized properly.   The maximum allowable set of resources
 * that can have indices in this list are those specified in this
 * subclass's required_resources list.
 ****************************procedure*header*****************************
 */
static void
BuildRequiredList (Widget w, FlatPart *fp, FlatClassPart *fcp)
{
	Cardinal		num_req = fcp->num_required_resources;
	OlFlatResourceInfo *	ri = &(fp->resource_info);
	OlFlatReqRsc *		req_rsc;
	Cardinal *		new_list;
	Cardinal		new_entries;
	Cardinal		record_size;
	Cardinal		req_rsc_index;
	Cardinal		xlist_index;

			/* Free existing data and re-initialize fields	*/

	XtFree(ri->rdata);
	XtFree((char *) ri->rlist);
	fp->resource_info.rlist		= (Cardinal *)NULL;
	fp->resource_info.num_rlist	= (Cardinal)0;
	fp->resource_info.rdata		= (char *)NULL;

	if (num_req == (Cardinal)0 ||
	    fp->num_items == (Cardinal)0)
	{
		return;
	}

			/* Loop over all the required resources for this
			 * widget to see which ones were not in the extractor
			 * list.  If required resource is not in the extractor
			 * list, then add it to the instance required_resource
			 * index list only if it has a NULL predicate
			 * procedure or the predicate procedure returns
			 * TRUE.					*/

	for (req_rsc_index = record_size = new_entries = (Cardinal)0,
	     req_rsc = fcp->required_resources,
	     new_list = (Cardinal *)XtMalloc((num_req+1) * sizeof(Cardinal));
	     req_rsc_index < num_req;
	     ++req_rsc_index, ++req_rsc)
	{
			/* See if the required resource is in the
			 * extractor list.				*/

		for (xlist_index=0; xlist_index < ri->num_xlist; ++xlist_index)
		{
			if (req_rsc->rsc_index == ri->xlist[xlist_index])
				break;
		}

			/* the required resource is not in the extractor
			 * list, so check the predicate procedure.	*/

		if (xlist_index == ri->num_xlist &&
		    (req_rsc->predicate == (OlFlatReqRscPredicateFunc)NULL ||
		     (*req_rsc->predicate)(w,
					RSC_OFFSET(fcp, req_rsc->rsc_index),
					req_rsc->name) == (Boolean)True))
		{
				/* cache the class req_rsc index in the
				 * cardinal array.			*/
			new_list[new_entries] = req_rsc_index;
			record_size += RSC_SIZE(fcp, req_rsc->rsc_index);
			++new_entries;
		}
	}

	if (new_entries == (Cardinal)0)
	{
		XtFree((char *)new_list);
	}
	else
	{
		if (new_entries < num_req)
		{
					/* realloc, leaving enough room
					 * for the record_size		*/

			new_list = (Cardinal *)XtRealloc((char *)new_list,
				(new_entries+1)*sizeof(Cardinal));
		}

				/* Store record size in the last slot.	*/

		new_list[new_entries] = record_size;

		ri->rlist	= new_list;
		ri->num_rlist	= new_entries;

				/* Allocate enough bytes to 'fp->num_items'
				 * of records and initialize them.	*/

		ri->rdata	= XtMalloc(record_size * fp->num_items);

		InitializeReqResources(w, fp, fcp);
	}
} /* END OF BuildRequiredList() */

/*
 *************************************************************************
 * CacheManager - this routine manages an item cache.  What the routine
 * actually does is dependent on the supplied action flag.  The routine
 * returns the address of the expanded item or NULL.
 ****************************procedure*header*****************************
 */
static FlatItem
CacheManager(Widget w, Cardinal item_index, FlatItem item, int action)
	      		  			/* flat widget id	*/
	        	           		/* requested item	*/
	        	     			/* item or NULL		*/
	   		       			/* What to do		*/
{
	static WidgetCache *	w_root = (WidgetCache *)NULL;

	WidgetCache *		w_null = (WidgetCache *)NULL;
	WidgetCache *		w_self = w_root;
	WidgetCache *		w_prev = w_null;
	ItemCache *		i_null = (ItemCache *)NULL;
	ItemCache *		i_self = i_null;
	ItemCache *		i_prev = i_null;
	FlatPart *		fp = FPART(w);
	FlatClassPart *		fcp = FCPART(w);

			/* Search for the widget id.  Once that is
			 * found, search for the item index.		*/

	while (w_self != w_null)
	{
		if (w_self->w == w)
		{
			i_self = w_self->i_root;

			while(i_self != i_null &&
			      i_self->item_index != item_index)
			{
				i_prev	= i_self;
				i_self	= i_self->next;
			}
			break;
		}
		else
		{
			w_prev	= w_self;
			w_self	= w_self->next;
		}
	}

				/* now that the widget and item cache
				 * pointers are initialized, so some
				 * action.				*/

	switch(action)
	{
	case ADD_REFERENCE:
		if (i_self == i_null)
		{
			i_self			= XtNew(ItemCache);
			i_self->next		= i_prev;
			i_self->item_index	= item_index;
			i_self->ref_count	= (Cardinal)0;
			i_self->item		= item;

			ExpandItem(w, fp, fcp, item_index, i_self->item);
		}

				/* Increment the reference count and
				 * set the return item address.		*/

		++i_self->ref_count;
		item = i_self->item;

		if (w_self == w_null)
		{
			w_self		= XtNew(WidgetCache);
			w_self->next	= w_root;
			w_root		= w_self;

			w_self->w	= w;
			w_self->i_root	= i_self;
		}
		else if (i_self->ref_count == 1)
		{
			i_self->next	= w_self->i_root;
			w_self->i_root	= i_self;
		}
		break;
	case DEC_REFERENCE:
		if (--i_self->ref_count == 0)
		{
			if (i_prev == i_null)
			{
				w_self->i_root = i_self->next;
			}
			else
			{
				i_prev->next = i_self->next;
			}

				/* Before freeing the structure, update
				 * the application's list with the
				 * information stored in the new item.	*/

			StoreItemValues(fp, fcp, item_index, i_self->item);

			XtFree((XtPointer)i_self);

			if (w_self->i_root == i_null)
			{
				if (w_prev == w_null)
				{
					w_root = w_self->next;
				}
				else
				{
					w_prev->next = w_self->next;
				}

				XtFree((XtPointer)w_self);
			}
		}
		break;
	default:					/* LOOKUP	*/
		if (i_self == i_null)
		{
			ExpandItem(w, fp, fcp, item_index, item);
		}
		else
		{
			(void)memcpy(item, i_self->item, (int)(fcp->rec_size));
		}
		break;
	}
	return(item);
} /* END OF CacheManager() */

/*
 *************************************************************************
 * CallItemProcs - this routine recursively calls the initialize item
 * procedure for each flat subclass.
 ****************************procedure*header*****************************
 */
static Boolean
CallItemProcs (WidgetClass wc, int opcode, Widget w, FlatItem current, FlatItem request, FlatItem new, ArgList args, Cardinal *num_args)
{
	Boolean		return_val = False;
	FlatClassPart *	fcp = &FLATCLASS(wc);

	if (wc != flatWidgetClass)
	{
		if (CallItemProcs(wc->core_class.superclass, opcode, w, 
			current, request, new, args, num_args) == True)
		{
			return_val = True;
		}
	}

					/* Call subclass's routine	*/

	switch(opcode)
	{
	case CALL_ITEM_GETVALUES:
		if (fcp->item_get_values != (OlFlatItemGetValuesProc)NULL)
		{
			(*fcp->item_get_values)(w, current, args, num_args);
		}
		break;
	case CALL_ITEM_INITIALIZE:
		if (fcp->item_initialize != (OlFlatItemInitializeProc)NULL)
		{
			(*fcp->item_initialize)(w, request, new,
							args, num_args);
		}
		break;
	case CALL_ITEM_SETVALUES:
		if (fcp->item_set_values != (OlFlatItemSetValuesFunc)NULL)
		{
			if ((*fcp->item_set_values)(w, current, request,
						new, args, num_args) == True)
			{
				return_val = True;
			}
		}
		break;
	default:				/* CALL_ANALYZE_ITEMS	*/
		if (fcp->analyze_items != (OlFlatAnalyzeItemsProc)NULL)
		{
			(*fcp->analyze_items)(w, args, num_args);
		}
		break;
	}
	return(return_val);
} /* END OF CallItemProcs() */

/*
 *************************************************************************
 * CallItemsTouched - this forward chains calls to each subclasses
 * items_touched routines
 ****************************procedure*header*****************************
 */
static void
CallItemsTouched (WidgetClass wc, Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args)
{
	FlatClassPart *	fcp = &FLATCLASS(wc);

	if (wc != flatWidgetClass)
	{
		CallItemsTouched(wc->core_class.superclass,
				current, request, new, args, num_args);
	}

	if (fcp->items_touched != (OlFlatItemsTouchedProc)NULL)
	{
		(*fcp->items_touched)(current, request, new, args, num_args);
	}
} /* END OF CallItemsTouched() */

/*
 *************************************************************************
 * ClassExpandItem - this routine recursively initializes an item record
 * by using the default item part found in the widget instance and then
 * by calling the widgetclass's expand item procedure.
 ****************************procedure*header*****************************
 */
static void
ClassExpandItem (WidgetClass wc, Widget w, Cardinal item_index, char *base)
{
	FlatClassPart *	fcp = &FLATCLASS(wc);

	if (wc != flatWidgetClass)
	{
		ClassExpandItem(wc->core_class.superclass, w, item_index, base);
	}

				/* If the subclass supplied an initialized
				 * item, load this into the destination	*/

	if (fcp->part_size != (Cardinal)0)
	{
		(void)memcpy((base + fcp->part_in_rec_offset),
				(char *) ((char *)w + fcp->part_offset),
				(int) fcp->part_size);
	}

				/* After copying the default part in,
				 * set the item index if this is the
				 * ultimate superclass.			*/

	if (wc == flatWidgetClass)
	{
		((FlatItem)base)->flat.item_index = item_index;
	}

			/* Now call the class's expand item procedure	*/

	if (FLATCLASS(wc).expand_item != (OlFlatExpandItemProc)NULL)
	{
		(*FLATCLASS(wc).expand_item)(w, (FlatItem)base);
	}

} /* END OF ClassExpandItem() */

/*
 *************************************************************************
 * ExpandItem - the procedure expands the subclasses contribution
 * to the item into the destination address.  The expansion process is
 * is a forward-chained process.  If the subclass has a part to expand,
 * it is expanded into the destination at the correct offset.
 *
 * Since a particular class does not know if a subclass has an additional
 * part contribution, the expansion must stop an the class specified
 * by the 'container' argument.  (If we don't stop here, we be mashing
 * memory since we will exceed the calling class's structure.)
 *
 * The procedure copies values (using their correct size) from the
 * application list into the supplied structure using the offsets defined
 * for each field.
 * This procedure expands only one sub-object element and assumes the
 * the src has fields of equal size.
 ****************************procedure*header*****************************
 */
static void
ExpandItem (Widget w, FlatPart *fp, FlatClassPart *fcp, Cardinal item_index, FlatItem item)
{
	char *			dest = (char *)item;
	const char *		src;	/* sub_object base address	*/
	register Cardinal *	list;	/* extractor list		*/
	Cardinal		num;

				/* Recursively initialize the item	*/

	ClassExpandItem(XtClass(w), w, item_index, dest);

	if ((num = fp->resource_info.num_xlist) != (Cardinal)0)
	{
		src = (char *)((char *)fp->items +
				(item_index * sizeof(XtArgVal) *
					fp->resource_info.num_xlist));

		for (list = fp->resource_info.xlist;
		     num != (Cardinal)0; --num, ++list)
		{
			if (*list != (Cardinal)IGNORE_FIELD)
			{
				_OlCopyFromXtArgVal(*((XtArgVal *)src), 
					(char *)(dest + RSC_OFFSET(fcp, *list)),
					RSC_SIZE(fcp, *list));
			}
			src += sizeof(XtArgVal);
		}
	}

			/* If there are required sub-object resources being
			 * managed internally, copy them into the
			 * expanded item.				*/

	if ((num = fp->resource_info.num_rlist) != (Cardinal)0)
	{
		Cardinal	record_size;
		XtResource *	rsc;
		
		list		= fp->resource_info.rlist;
		record_size	= list[num];
		src		= fp->resource_info.rdata +
					(item_index * record_size);

		for ( ; num != (Cardinal)0; --num, ++list)
		{
			rsc = fcp->item_resources + RSC_INDEX(fcp, *list);

			(void)memcpy((dest + rsc->resource_offset),
					src, (int)rsc->resource_size);
			src += rsc->resource_size;
		}
	}
} /* END OF ExpandItem() */

/*
 *************************************************************************
 * GetItemState - this routine queries the fields of an item
 * using the Args that are supplied.
 *
 * The source address of the item to be put back is determined by
 * taking the head of the item list as the base address.  Then the number
 * of items skipped over must be added (base + record_size * index).  Then,
 * we have to add the offset of the particular field, (num * part_size).
 ****************************procedure*header*****************************
 */
static void
GetItemState(Widget w, FlatClassPart *fcp, Cardinal item_index, ArgList args, Cardinal *num_args)
	      		  		/* Flat widget subclass		*/
	               	    		/* Flat Class Part address	*/
	        	           	/* item to be expanded		*/
	       		     		/* list of Args to get		*/
	          	         	/* number of Args		*/
{
	XrmQuarkList	qlist;		/* all subclass item resources	*/
	XrmQuark	quark;
	Cardinal	num;
	Cardinal	count;
	char 		stack[STACK_SIZE];
	FlatItem	new;
	FlatItem	save_new;

	save_new = (fcp->rec_size > sizeof(stack) ?
			(new	= (FlatItem)XtMalloc(fcp->rec_size)) :
			(new	= (FlatItem)stack));

					/* Expand the item with the 
					 * CacheManager.		*/

	new = CacheManager(w, item_index, new, ADD_REFERENCE);

			/* Call the subclasses to add information
			 * or possibly change some of it.		*/

	(void)CallItemProcs(XtClass(w), CALL_ITEM_GETVALUES, w, new,
			(FlatItem)NULL, (FlatItem)NULL, args, num_args);

						/* Loop over the args.	*/

	for (count = (Cardinal)0; count < *num_args; ++count, ++args)
	{
		qlist = fcp->quarked_items;
		quark = XrmStringToQuark(args->name);

		for (num=0; num < fcp->num_item_resources; ++qlist, ++num)
		{
			if (quark == *qlist)
			{
				/* Copy the value out of the expanded
				 * item and place it in the application's
				 * arg list.				*/

				_OlCopyToXtArgVal((char *) ((char *)new +
						RSC_OFFSET(fcp, num)),
						(XtArgVal *)&args->value,
						RSC_SIZE(fcp, num));
				break;
			}
		} /* looping over item resources for this subclass */
	} /* looping over num_args */

				/* Decrement the cached item's reference
				 * count.				*/

	(void)CacheManager(w, item_index, new, DEC_REFERENCE);

	FREE_STACK(save_new,stack);

} /* END OF GetItemState() */

/*
 *************************************************************************
 * InitializeReqResources - initializes required resources for an
 * flattened widget instance.  The default value in the item resource
 * list is used for the initializing.
 ****************************procedure*header*****************************
 */
static void
InitializeReqResources (Widget w, FlatPart *fp, FlatClassPart *fcp)
{
	OlFlatResourceInfo *	ri = &(fp->resource_info);
	XtResource *		rsc;
	char *			dest;
	Cardinal		i;
	Cardinal		size;

				/* Initialize the required_resources	*/

	for (size=i=0, dest=ri->rdata; i < ri->num_rlist; ++i, dest += size)
	{
		rsc	= fcp->item_resources + RSC_INDEX(fcp, ri->rlist[i]);
		size	= rsc->resource_size;

		if (!strcmp(rsc->default_type, XtRCallProc))
		{
			XrmValue		to_val;
			XtResourceDefaultProc	proc = (XtResourceDefaultProc)
							rsc->default_addr;
			to_val.size = (unsigned int)0;

			(*proc)(w, rsc->resource_offset, &to_val);

			if ((Cardinal)to_val.size == size)
			{
				(void)memcpy(dest, (const char *)to_val.addr,
						(int)size);
			}
			else
			{
				(void)memset(dest, 0, (int)size);
			}
		}
		else		/* default and resource types are same	*/
		{
			(void)memcpy(dest,
				(const char *)rsc->default_addr, (int)size);
		}
	}

		/* At this point, we've only initialized the first record
		 * for the internally-managed required resource list.
		 * So for now, copy the first record to all the remaining
		 * records.						*/

	size = ri->rlist[ri->num_rlist];	/* the record_size	*/

	for (i=1, dest=ri->rdata + size; i < fp->num_items; ++i, dest+=size)
	{
		(void)memcpy(dest, (const char *)ri->rdata, (int)size);
	}
} /* END OF InitializeReqResources() */

/*
 *************************************************************************
 * StoreItemValues - this routine copies values from an item and places
 * them into the application's list and for any required_resources, puts
 * the values into the required resource list.
 ****************************procedure*header*****************************
 */
static void
StoreItemValues (FlatPart *fp, FlatClassPart *fcp, Cardinal item_index, FlatItem item_rec)
{
	char *			src = (char *)item_rec;
	char *			dest;
	Cardinal *		list;
	Cardinal		num;

	if ((num = fp->resource_info.num_xlist) != (Cardinal)0)
	{
		list = fp->resource_info.xlist;
		dest = (char *)((char *)fp->items +
				(item_index * sizeof(XtArgVal) * num));

		for ( ; num; --num, ++list)
		{
			if (*list != (Cardinal)IGNORE_FIELD)
			{
				_OlConvertToXtArgVal((src +
					RSC_OFFSET(fcp, *list)),
					(XtArgVal *)dest,
					RSC_SIZE(fcp, *list));
			}
			dest += sizeof(XtArgVal);
		}
	}

			/* If there are required sub-object resources being
			 * managed internally, copy them from the expanded
			 * item back into the internal storage.		*/

	if ((num = fp->resource_info.num_rlist) != (Cardinal)0)
	{
		Cardinal	record_size;
		XtResource *	rsc;

		list		= fp->resource_info.rlist;
		record_size	= list[num];
		dest		= fp->resource_info.rdata +
					(item_index * record_size);

		for ( ; num != (Cardinal)0; --num, ++list)
		{
			rsc = fcp->item_resources + RSC_INDEX(fcp, *list);

			(void)memcpy(dest,
				(const char *)(src + rsc->resource_offset),
				(int)rsc->resource_size);
			dest += rsc->resource_size;
		}
	}
} /* END OF StoreItemValues() */

/*
 *************************************************************************
 * SetItemState - this routine updates the fields of an item
 * using the Args that are supplied.
 *
 * This routine also makes a geometry request if the changing of the
 * item's attributes causes a change in the layout and/or size of the
 * widget.  Note: the geometry request is not made if this routine is
 * called while the flat widget is checking items.
 *
 * The destination address of the item to be put back is determined by
 * taking the head of the item list as the base address.  Then the number
 * of items skipped over must be added (base + record_size * index).  Then,
 * we have to add the offset of the particular field, (num * part_size).
 ****************************procedure*header*****************************
 */
static void
SetItemState(Widget w, FlatPart *fp, FlatClassPart *fcp, Cardinal item_index, ArgList args, Cardinal *num_args)
	      		  		/* Flat widget subclass		*/
	          	   		/* Flat Part address		*/
	               	    		/* Flat Class Part address	*/
	        	           	/* item to be expanded		*/
	       		     		/* list of Args to touch	*/
	          	         	/* number of Args		*/
{
	Boolean			redisplay;
	Boolean			layout_changed;
	Boolean			managed;
	OlFlatGeometry 		old_geom;
	OlFlatGeometry *	new_geom = &fp->item_geom;
	OlFlatDrawInfo		old_di;
	OlFlatDrawInfo		new_di;
	Dimension		tmp_width;
	Dimension		tmp_height;
	ArgList			cached_args = args;
	XrmQuark		quark;
	Cardinal *		list;
	Cardinal		num;
	Cardinal		count;
	char			stack[3][STACK_SIZE];
	FlatItem		current;	/* The current item	*/
	FlatItem		request;	/* The request item	*/
	FlatItem		new;		/* The new item		*/
	FlatItem		save_new;	/* The new item		*/

						/* Set up the stacks	*/

	save_new = (fcp->rec_size > sizeof(stack[0]) ?
			(current	= (FlatItem)XtMalloc(fcp->rec_size),
			 request	= (FlatItem)XtMalloc(fcp->rec_size),
			 new		= (FlatItem)XtMalloc(fcp->rec_size)) :
			(current	= (FlatItem)stack[0],
			 request	= (FlatItem)stack[1],
			 new		= (FlatItem)stack[2]));

				/* Expand the new item before the
				 * changes are applied and copy them
				 * into current.			*/
	
	new = CacheManager(w, item_index, new, ADD_REFERENCE);

	(void)memcpy(current, new, (int)fcp->rec_size);

				/* Save certain characteristics about
				 * the item and the widget before the
				 * changes are made.  Only do this if
				 * we're not dealing with a new list.	*/

	managed		= ((FlatItem)current)->flat.managed;

	if (fp->items_touched == False)
	{
		tmp_width	= w->core.width;
		tmp_height	= w->core.height;
		old_geom	= fp->item_geom;

		_OlFlatGetDrawInfo(w, item_index, &old_di);
	}

			/* Loop over the args and copy the requested
			 * values into the new item.			*/

	for (count = (Cardinal)0; count < *num_args; ++count, ++args)
	{
		for (list = fp->resource_info.xlist,
		     quark = XrmStringToQuark(args->name),
		     num = fp->resource_info.num_xlist;
		     num != (Cardinal)0; --num, ++list)
		{
			if (*list != (Cardinal)IGNORE_FIELD &&
			    quark == RSC_QNAME(fcp, *list))
			{
				/* Copy the value out of the Arg list
				 * place it into the expanded item.	*/

				_OlCopyFromXtArgVal(args->value,
					(char *)((char *)new +
						RSC_OFFSET(fcp, *list)),
					RSC_SIZE(fcp, *list));
				break;
			}
		} /* looping over item resources for this subclass */

			/* If the arg was not in the extractor list,
			 * check to see if it's in the required resource
			 * list.					*/

		if (num == (Cardinal)0 &&
		   (num = fp->resource_info.num_rlist) != (Cardinal)0)
		{
			Cardinal	rsc_index;

			for (list = fp->resource_info.rlist;
			     num != (Cardinal)0; --num, ++list)
			{
				rsc_index = RSC_INDEX(fcp, *list);

				if (quark == RSC_QNAME(fcp, rsc_index))
				{
					XtResource *	rsc =
							fcp->item_resources +
							rsc_index;

					_OlCopyFromXtArgVal(args->value,
						(char *)((char *)new +
						rsc->resource_offset),
						(int)rsc->resource_size);
					break;
				}
			}
		}

	} /* looping over num_args */

				/* Make the request item be a copy of the
				 * new item before the widget gets to
				 * see the changes.			*/

	(void)memcpy(request, new, (int)fcp->rec_size);

			/* Call subclass's item_set_values routine	*/

	redisplay = CallItemProcs(XtClass(w), CALL_ITEM_SETVALUES, w,
			current, request, new, cached_args, num_args);

				/* see if the item's managed state has
				 * changed.  (Do this before freeing
				 * the stack.)				*/

	layout_changed = (managed != ((FlatItem)new)->flat.managed ?
				True : False);

				/* Decrement the cached item's reference
				 * count.				*/

	(void)CacheManager(w, item_index, new, DEC_REFERENCE);

			/* Free the temporary stacks if necessary	*/

	FREE_STACK(current,stack[0]);
	FREE_STACK(request,stack[1]);
	FREE_STACK(save_new,stack[2]);

	if (fp->items_touched == False)
	{
		Boolean		refresh_container;

			/* Get the new geometry characteristics.	*/

		_OlFlatLayout(w, &w->core.width, &w->core.height);

				/* If the item's width or height has
				 * changed, relayout the container	*/

#define DIFF_GEO(field)	(old_geom.field != new_geom->field)
#define DIFF_DI(field)	(old_di.field != new_di.field)

		if (layout_changed == True	||
		    DIFF_GEO(bounding_width)	||
		    DIFF_GEO(bounding_height)	||
		    DIFF_GEO(rows)		||
		    DIFF_GEO(cols))
		{
			layout_changed = True;
		}
		else
		{
				/* If the layout didn't change, reset
				 * the x and y offsets since they are
				 * based on the container size which
				 * really didn't change if the layout
				 * didn't.				*/

			layout_changed		= False;
			new_geom->x_offset	= old_geom.x_offset;
			new_geom->y_offset	= old_geom.y_offset;

				/* Reset the container's width and height
				 * since the layout procedure may have
				 * changed them even though the layout
				 * did not change.  (Due to XtNlayoutWidth
				 * or XtNlayoutHeight)			*/

			w->core.width	= tmp_width;
			w->core.height	= tmp_height;
		}

		_OlFlatGetDrawInfo(w, item_index, &new_di);

		refresh_container = (	layout_changed == True	||
					DIFF_DI(x)		||
					DIFF_DI(y)		||
					DIFF_DI(width)		||
					DIFF_DI(height) ? True : False);
#undef DIFF_GEO
#undef DIFF_DI

				/* If we need a resize, make a geometry
				 * request and continue looping until
				 * the request is resolved.		*/

		if (layout_changed == True &&
		    (tmp_width != w->core.width ||
			tmp_height != w->core.height))
		{
			Cardinal		size;
			Widget			oldw;
			XtGeometryResult	result;
			XtWidgetGeometry	request;
			XtWidgetGeometry	reply;
			XtWidgetProc		resize_proc = XtClass(w)->
							core_class.resize;
			XtAlmostProc		almost_proc = XtClass(w)->
						core_class.set_values_almost;

					/* Set the request and copy the
					 * original dimensions back into
					 * the widget.			*/

			request.request_mode = (XtGeometryMask)0;
			
			if (tmp_width != w->core.width)
			{
				request.request_mode	|= CWWidth;
				request.width		= w->core.width;
				w->core.width		= tmp_width;
			}

			if (tmp_height != w->core.height)
			{
				request.request_mode	|= CWHeight;
				request.height		= w->core.height;
				w->core.height		= tmp_height;
			}

				/* Create a current widget.  To do this
				 * safely, we must also copy the constraints
				 * after copying the instance data.	*/

			size = XtClass(w)->core_class.widget_size;

			oldw = (Widget) (size > STACK_SIZE ?
						XtMalloc(size) : stack[0]);

			(void) memcpy((char *)oldw, (const char *)w,
							(int)size);

			if (w->core.constraints != (XtPointer)NULL)
			{
				size = ((ConstraintWidgetClass)XtClass(
					XtParent(w)))->constraint_class.
							constraint_size;
				oldw->core.constraints = (XtPointer)
						(size > STACK_SIZE ?
						XtMalloc(size) : stack[1]);
				(void) memcpy((char *)oldw->core.constraints,
					(const char *)w->core.constraints,
							(int)size);
			}

			do
			{
				result = XtMakeGeometryRequest(w,
							&request, &reply);

				if (result == XtGeometryYes)
				{
					break;
				}

				/* Since XtMakeGeometryRequest never returns
				 * XtGeometryDone, we're here because of an
				 * XtGeometryAlmost or an XtGeometryNo
				 * return value.			*/

				if (almost_proc != (XtAlmostProc)NULL)
				{
					/*
					 * According to the Intrinsics
					 * documentation, the request_mode
					 * in the reply structure must be
					 * set to 0 if the result is
					 * XtGeometryNo.		*/

					if (result == XtGeometryNo)
					{
					   reply.request_mode = 0;
					}

					(*almost_proc)(oldw, w,
							&request, &reply);
				}
				else
				{
					OlVaDisplayErrorMsg(XtDisplay(w),
						OleNinvalidProcedure,
						OleTsetValuesAlmost,
						OleCOlToolkitError,
					   OleMinvalidProcedure_setValuesAlmost,
						W_CNAME(w));
				}
			} while (request.request_mode != (XtGeometryResult)0);

				/* If the result is Yes, then the window has
				 * been configured already (so we don't need
				 * to generate an exposure).  All we need to
				 * to is call the resize procedure if it
				 * exists.
				 * Else, just clear the container so it
				 * can redraw itself.			*/

			if (result == XtGeometryYes)
			{
			    if ((request.request_mode & (CWWidth|CWHeight)) &&
				resize_proc != (XtWidgetProc)NULL)
			    {
				(*resize_proc)(w); 
			    }
			}
			else
			{
				_OlClearWidget(w, (Bool)True);
			}

					/* Free the old widget and any 
					 * constraints.			*/

			if (oldw->core.constraints != NULL)
			{
				FREE_STACK(oldw->core.constraints,stack[1]);
			}
			FREE_STACK(oldw,stack[0]);
		}
		else if (refresh_container == True)
		{
			_OlClearWidget(w, (Bool)True);
		}
		else if (redisplay == True)
		{
			_OlFlatRefreshItem(w, item_index, True);
		}
	}
} /* END OF SetItemState() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlFlatExpandItem - this procedure initializes an application supplied
 * item.  The returned item is considered read-only since any changes
 * made to it will not be saved.  When initializing the item, the routine
 * checks if the requested item is currently cached.  If it is, it
 * returns a copy of the cached item.  If it is not, it does the expansion.
 ****************************procedure*header*****************************
 */
void
_OlFlatExpandItem (Widget w, Cardinal item_index, FlatItem item)
{
	if (item == (FlatItem)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemAddress,
			OleTflatState, OleCOlToolkitWarning,
			OleNbadItemAddress_flatState,
			XtName(w), W_CNAME(w), "_OlFlatExpandItem",
			(unsigned) item_index);
	}
	else
	{
		item = CacheManager(w, item_index, item, LOOKUP);
	}
} /* END OF _OlFlatExpandItem() */

/*
 *************************************************************************
 * _OlFlatInitializeItems - this routine is called whenever a flat widget
 * gets new item list or whenever the existing items get touched.
 * Normally, the internal state flag "items_touched" is set to True
 * prior to calling this routine and then is immediately set to False
 * once this routine returns.   (Usually, flat widget writers use the
 * convenience routine _OlFlatDoLayout() which calls this routine.)
 ****************************procedure*header*****************************
 */
void
_OlFlatInitializeItems (Widget c_w, Widget r_w, Widget w, ArgList args, Cardinal *num_args)
{
	FlatPart *		fp = FPART(w);
	FlatClassPart *		fcp = FCPART(w);
	Cardinal		i;		/* current item index	*/
	char			stack[2][STACK_SIZE];
	FlatItem		save_new;	/* points to new area	*/
	FlatItem		request;	/* The request item	*/
	FlatItem		new;		/* The new item		*/

	if (fp->num_item_fields != 0 && fp->item_fields == (String *)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w),
			OleNinvalidResource, OleTnullList, OleCOlToolkitWarning,
			OleMinvalidResource_nullList,
			XtName(w), W_CNAME(w), "XtNitemFields",
			"XtNnumItemFields");

					/* set the count to zero	*/
		fp->num_item_fields = 0;
	}

				/* build the extractor list and the
				 * required resource list		*/

	if (c_w == (Widget)NULL)
	{
		BuildExtractorList(w, fp, fcp);
		BuildRequiredList(w, fp, fcp);
	}
	else
	{
		FlatPart *	cfp = FPART(c_w);

		if (fp->num_items != cfp->num_items ||
		    fp->items != cfp->items ||
		    fp->item_fields != cfp->item_fields ||
		    fp->num_item_fields != cfp->num_item_fields)
		{
			BuildExtractorList(w, fp, fcp);
			BuildRequiredList(w, fp, fcp);
		}
	}

	CallItemsTouched(XtClass(w), c_w, r_w, w, args, num_args);

				/* Set Up the stack areas outside the
				 * loop.				*/

	save_new = (fcp->rec_size > sizeof(stack[0]) ?
			(request	= (FlatItem)XtMalloc(fcp->rec_size),
			 new		= (FlatItem)XtMalloc(fcp->rec_size)) :
			(request	= (FlatItem)stack[0],
			 new		= (FlatItem)stack[1]));

	for (i=0; i < fp->num_items; ++i)
	{
				/* Expand the item as it is before the
				 * update.				*/
	
		new = CacheManager(w, i, save_new, ADD_REFERENCE);

				/* Copy the new item into the request
				 * item so the flat widget can compare
				 * differences.				*/

		(void)memcpy(request, new, (int)fcp->rec_size);

			/* Call subclass's item_initialize routine	*/

		(void)CallItemProcs(XtClass(w), CALL_ITEM_INITIALIZE, w,
			(FlatItem)NULL, request, new, args, num_args);

				/* Decrement the cached item's reference
				 * count.				*/

		(void)CacheManager(w, i, new, DEC_REFERENCE);
	}

	FREE_STACK(request,stack[0]);
	FREE_STACK(save_new,stack[1]);

			/* Call subclass's analyze_items routine so
			 * that the flat widget can look at all items
			 * collectively.				*/

	(void)CallItemProcs(XtClass(w), CALL_ANALYZE_ITEMS, w, (FlatItem)NULL,
			(FlatItem)NULL, (FlatItem)NULL, args, num_args);

} /* END OF _OlFlatInitializeItems() */

/*
 *************************************************************************
 * OlFlatGetValues() - gets sub-object resources
 ****************************procedure*header*****************************
 */
void
OlFlatGetValues (Widget w, Cardinal item_index, ArgList args, Cardinal num_args)
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);
	static char *	proc_name = "OlFlatGetValues()";

	if (num_args == (Cardinal)0)
	{
		return;
	}
	else if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name);
		return;
	}
	else if (args == (ArgList)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNinvalidArgCount,
			OleTflatState, OleCOlToolkitWarning,
			OleMinvalidArgCount_flatState,
			(unsigned) num_args, proc_name,
			XtName(w), W_CNAME(w), (unsigned) item_index);
		return;
	}
	else if (item_index == (Cardinal)OL_NO_ITEM ||
		    item_index >= fp->num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), W_CNAME(w), proc_name,
			(unsigned) item_index);
		return;
	}
	else
	{
		GetItemState(w, fcp, item_index, args, &num_args);
	}
} /* END OF OlFlatGetValues() */

/*
 *************************************************************************
 * OlFlatSetValues() - sets sub-object resources.  If the width or height
 * of the sub-object changes during the setting of its resources,
 * the container's relayout procedure will be called.
 ****************************procedure*header*****************************
 */
void
OlFlatSetValues (Widget w, Cardinal item_index, ArgList args, Cardinal num_args)
{
	FlatPart *	fp = FPART(w);
	FlatClassPart *	fcp = FCPART(w);
	static char *	proc_name = "OlFlatSetValues()";

	if (num_args == (Cardinal)0)
	{
		return;
	}
	else if (w == (Widget)NULL)
	{
		OlVaDisplayWarningMsg((Display *)NULL, OleNnullWidget,
			OleTflatState, OleCOlToolkitWarning,
			OleMnullWidget_flatState, proc_name);
		return;
	}
	else if (args == (ArgList)NULL)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNinvalidArgCount,
			OleTflatState, OleCOlToolkitWarning,
			OleMinvalidArgCount_flatState,
			(unsigned) num_args, proc_name,
			XtName(w), W_CNAME(w), (unsigned) item_index);
		return;
	}
	else if (item_index == (Cardinal)OL_NO_ITEM ||
		    item_index >= fp->num_items)
	{
		OlVaDisplayWarningMsg(XtDisplay(w), OleNbadItemIndex,
			OleTflatState, OleCOlToolkitWarning,
			OleMbadItemIndex_flatState,
			XtName(w), W_CNAME(w), proc_name,
			(unsigned) item_index);
		return;
	}
	else
	{
		SetItemState(w, fp, fcp, item_index, args, &num_args);
	}
} /* END OF OlFlatSetValues() */

/*
 *************************************************************************
 * OlVaFlatGetValues() - variable argument interface to OlFlatGetValues.
 ****************************procedure*header*****************************
 */
void
OlVaFlatGetValues (Widget w, Cardinal item_index, ...)
{
	va_list		vargs;
	ArgList		args;
	Cardinal	num_args;
	int		total_count;
	int		typed_count;

	va_start(vargs, item_index);
	_XtCountVaList(vargs, &total_count, &typed_count);
	va_end(vargs);

	va_start(vargs, item_index);
	_XtVaToArgList(w, vargs, total_count, &args, &num_args);

	if (num_args != (Cardinal)0)
	{
		OlFlatGetValues(w, item_index, args, num_args);
		XtFree((char *)args);
	}
	va_end(vargs);
} /* END OF OlVaFlatGetValues() */

/*
 *************************************************************************
 * OlVaFlatSetValues() - variable argument interface to OlFlatSetValues.
 ****************************procedure*header*****************************
 */
void
OlVaFlatSetValues (Widget w, Cardinal item_index, ...)
{
	va_list		vargs;
	ArgList		args;
	Cardinal	num_args;
	int		total_count;
	int		typed_count;

	va_start(vargs, item_index);
	_XtCountVaList(vargs, &total_count, &typed_count);
	va_end(vargs);

	va_start(vargs, item_index);
	_XtVaToArgList(w, vargs, total_count, &args, &num_args);

	if (num_args != (Cardinal)0)
	{
		OlFlatSetValues(w, item_index, args, num_args);
		XtFree((char *)args);
	}
	va_end(vargs);
} /* END OF OlVaFlatSetValues() */

